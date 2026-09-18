#!/usr/bin/env python3
"""Validated, correlated coherence dispatch. A 204 response is not test evidence."""
from __future__ import annotations
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time
import zipfile
from concurrent.futures import ThreadPoolExecutor, as_completed

REPOS = {f"{name}_system" for name in ("common", "thread", "container", "logger", "network", "monitoring", "database", "pacs")}
CENTRAL = "common_system"
REQUEST = "coherence-check-v1"
CHANGE = "coherence-change-v1"


def sha(value):
    if not isinstance(value, str) or not re.fullmatch(r"[0-9a-f]{40}", value):
        raise ValueError("dispatch pins must be full lowercase commit SHAs")
    return value


def api(endpoint, data=None, binary=False):
    command = ["gh", "api", endpoint]
    if data is not None:
        command += ["--method", "POST", "--input", "-"]
    result = subprocess.run(command, input=json.dumps(data).encode() if data is not None else None,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=90)
    if result.returncode:
        raise ValueError(f"GitHub API failed for {endpoint}: {result.stderr.decode(errors='replace').strip()}")
    if binary:
        return result.stdout
    return json.loads(result.stdout) if result.stdout else None


def correlation(repository, commit, lock_revision):
    return hashlib.sha256(f"{repository}:{commit}:{lock_revision}".encode()).hexdigest()[:32]


def payload(repository, commit, lock_revision, route, receiver_sha=None):
    value = {"schema_version": 1, "origin": f"kcenon/{CENTRAL if route == REQUEST else repository}",
             "candidate_repository": repository, "candidate_sha": sha(commit), "lock_revision": sha(lock_revision),
             "correlation": correlation(repository, commit, lock_revision)}
    if route == REQUEST:
        value["receiver_sha"] = sha(receiver_sha)
    return validate_payload(route, value)


def validate_payload(route, value):
    if route not in (REQUEST, CHANGE) or not isinstance(value, dict) or value.get("schema_version") != 1:
        raise ValueError("unsupported dispatch event/schema")
    repo = value.get("candidate_repository")
    if repo not in REPOS:
        raise ValueError("candidate repository is outside the ecosystem")
    sha(value.get("candidate_sha"))
    sha(value.get("lock_revision"))
    expected_origin = CENTRAL if route == REQUEST else repo
    if value.get("origin") != f"kcenon/{expected_origin}":
        raise ValueError("invalid dispatch origin/route (results must never notify again)")
    expected_id = correlation(repo, value["candidate_sha"], value["lock_revision"])
    if value.get("correlation") != expected_id:
        raise ValueError("correlation does not identify the candidate and lock")
    if route == REQUEST:
        sha(value.get("receiver_sha"))
    elif "receiver_sha" in value:
        raise ValueError("change notifications cannot masquerade as receiver requests")
    return value


def locked_sources(revision):
    import base64
    content = api(f"repos/kcenon/common_system/contents/ci/ecosystem-lock.json?ref={sha(revision)}")
    lock = json.loads(base64.b64decode(content["content"]))
    if lock.get("schema_version") != 1 or lock.get("status") != "accepted" or set(lock.get("repositories", {})) != REPOS:
        raise ValueError("dispatch requires an accepted eight-repository lock at the supplied revision")
    for commit in lock["repositories"].values():
        sha(commit)
    validation = lock.get("validation", {})
    if validation.get("repositories") != lock["repositories"] or validation.get("profile") != lock.get("profile"):
        raise ValueError("lock lacks matching validation evidence")
    for repo, commit in lock["repositories"].items():
        result = validation.get("results", {}).get(repo, {})
        if result.get("sha") != commit or result.get("worktree_changes") is not False:
            raise ValueError(f"lock has no clean source proof for {repo}")
        for check in ("conformance", "version_drift", "configure", "build", "consumer"):
            code = result.get("checks", {}).get(check)
            if type(code) is not int or code != 0:
                raise ValueError(f"lock has no raw passing {repo}/{check}")
    return lock["repositories"]


def validate_receiver(event, repository):
    value = validate_payload(event.get("action"), event.get("client_payload"))
    if event["action"] != REQUEST or repository not in REPOS - {CENTRAL}:
        raise ValueError("request is not for a downstream receiver")
    locked = locked_sources(value["lock_revision"])
    expected = value["candidate_sha"] if repository == value["candidate_repository"] else locked[repository]
    if value["receiver_sha"] != expected:
        raise ValueError("receiver SHA is not the intended candidate/locked commit")
    return value


def matching_runs(repository, workflow, request):
    data = api(f"repos/kcenon/{repository}/actions/workflows/{workflow}/runs?event=repository_dispatch&per_page=100")
    return [run for run in data.get("workflow_runs", [])
            if run.get("display_title") == "coherence/" + request["correlation"]
            and run.get("event") == "repository_dispatch"
            and run.get("path", "").split("@")[0] == ".github/workflows/" + workflow]


def artifact_result(repository, run):
    artifacts = api(f"repos/kcenon/{repository}/actions/runs/{run['id']}/artifacts")["artifacts"]
    found = [item for item in artifacts if item["name"] == "coherence-result" and not item.get("expired")]
    if len(found) != 1:
        raise ValueError("receiver has no unique, unexpired raw result artifact")
    archive = api(f"repos/kcenon/{repository}/actions/artifacts/{found[0]['id']}/zip", binary=True)
    if len(archive) > 4 * 1024 * 1024:
        raise ValueError("unexpectedly large result artifact")
    with zipfile.ZipFile(io.BytesIO(archive)) as bundle:
        files = [item for item in bundle.infolist() if item.filename == "dispatch-result.json"]
        if len(files) != 1 or files[0].file_size > 1024 * 1024:
            raise ValueError("result artifact is malformed")
        return json.loads(bundle.read(files[0]))


def check_result(result, request, repository):
    for field in ("correlation", "candidate_repository", "candidate_sha", "lock_revision"):
        if result.get(field) != request[field]:
            raise ValueError(f"receiver result has wrong {field}")
    if result.get("repository") != repository or type(result.get("raw_exit_code")) is not int or result["raw_exit_code"] != 0:
        raise ValueError("receiver raw checks failed or were skipped")
    if "receiver_sha" in request and result.get("source_sha") != request["receiver_sha"]:
        raise ValueError("receiver checked the wrong commit")


def dispatch_and_wait(repository, route, request, workflows, timeout=900, sleep=time.sleep, now=time.monotonic):
    validate_payload(route, request)
    # Retried notifications reuse the same identity and existing runs. Checking
    # an identical tuple twice is harmless; receivers have no notification path.
    existing = {workflow: matching_runs(repository, workflow, request) for workflow in workflows}
    if not any(existing.values()):
        api(f"repos/kcenon/{repository}/dispatches", {"event_type": route, "client_payload": request})
    deadline = now() + timeout
    completed = {}
    while now() < deadline:
        for workflow in workflows:
            if workflow in completed:
                continue
            runs = matching_runs(repository, workflow, request)
            if not runs:
                continue
            run = max(runs, key=lambda item: item["id"])
            if run.get("status") != "completed":
                continue
            if run.get("conclusion") != "success":
                raise ValueError(f"receiver {run.get('html_url')} concluded {run.get('conclusion')}")
            check_result(artifact_result(repository, run), request, repository)
            completed[workflow] = run["html_url"]
        if len(completed) == len(workflows):
            return completed
        sleep(min(10, max(0, deadline - now())))
    raise ValueError(f"dispatch timed out; no passing raw result for {sorted(set(workflows) - set(completed))}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    receiver = sub.add_parser("receive")
    receiver.add_argument("--event", type=Path, required=True)
    receiver.add_argument("--repository", choices=sorted(REPOS), required=True)
    receipt = sub.add_parser("result")
    receipt.add_argument("--event", type=Path, required=True)
    receipt.add_argument("--repository", choices=sorted(REPOS), required=True)
    receipt.add_argument("--source-root", type=Path, required=True)
    receipt.add_argument("--output", type=Path, default=Path("dispatch-result.json"))
    sender = sub.add_parser("notify")
    sender.add_argument("--repository", choices=sorted(REPOS), required=True)
    sender.add_argument("--sha", required=True)
    sender.add_argument("--lock-revision")
    fanout = sub.add_parser("fanout")
    fanout.add_argument("--repository", choices=sorted(REPOS), required=True)
    fanout.add_argument("--sha", required=True)
    fanout.add_argument("--lock-revision", required=True)
    for command in (sender, fanout):
        command.add_argument("--output", type=Path, required=True)
        command.add_argument("--timeout", type=int, default=1800)
    args = parser.parse_args()
    try:
        if args.command in ("receive", "result"):
            result = validate_receiver(json.loads(args.event.read_text()), args.repository)
            if args.command == "result":
                result = dict(result, repository=args.repository, raw_exit_code=0)
                result["source_sha"] = subprocess.check_output(["git", "-C", str(args.source_root), "rev-parse", "HEAD"], text=True).strip()
                for name in ("conformance", "version-drift"):
                    path = args.source_root / "coherence-results" / f"{name}.json"
                    gate = json.loads(path.read_text()) if path.exists() else {}
                    if (type(gate.get("exit_code")) is not int or gate["exit_code"] != 0
                            or gate.get("source_sha") != result["receiver_sha"] or gate.get("worktree_changes") is not False):
                        result["raw_exit_code"] = 1
                if result["source_sha"] != result["receiver_sha"]:
                    result["raw_exit_code"] = 1
                args.output.write_text(json.dumps(result, indent=2) + "\n")
                return result["raw_exit_code"]
            if os.environ.get("GITHUB_OUTPUT"):
                with open(os.environ["GITHUB_OUTPUT"], "a") as out:
                    out.write(f"sha={result['receiver_sha']}\n")
            print(json.dumps(result, indent=2))
            return 0
        revision = args.lock_revision or api("repos/kcenon/common_system/commits/main")["sha"]
        locked = locked_sources(revision)
        results = {"candidate_repository": args.repository, "candidate_sha": sha(args.sha),
                   "lock_revision": revision, "runs": {}}
        args.output.parent.mkdir(parents=True, exist_ok=True)
        try:
            if args.command == "notify":
                value = payload(args.repository, args.sha, revision, CHANGE)
                results["runs"][CENTRAL] = dispatch_and_wait(CENTRAL, CHANGE, value,
                    ["ecosystem-cross-build.yml", "port-sync-check.yml"], args.timeout)
            else:
                errors = []
                with ThreadPoolExecutor(max_workers=7) as pool:
                    tasks = {}
                    for repository in sorted(REPOS - {CENTRAL}):
                        selected = args.sha if repository == args.repository else locked[repository]
                        value = payload(args.repository, args.sha, revision, REQUEST, selected)
                        tasks[pool.submit(dispatch_and_wait, repository, REQUEST, value,
                                          ["coherence-receiver.yml"], args.timeout)] = repository
                    for task in as_completed(tasks):
                        repository = tasks[task]
                        try:
                            results["runs"][repository] = task.result()
                        except (ValueError, subprocess.SubprocessError) as exc:
                            results["runs"][repository] = {"error": str(exc)}
                            errors.append(repository)
                if errors:
                    raise ValueError(f"receivers failed or timed out: {', '.join(errors)}")
        except (ValueError, subprocess.SubprocessError) as exc:
            results["error"] = str(exc)
            raise
        finally:
            args.output.write_text(json.dumps(results, indent=2) + "\n")
        print(json.dumps(results, indent=2))
        return 0
    except (OSError, ValueError, KeyError, subprocess.SubprocessError) as exc:
        print(f"COHERENCE DISPATCH FAILED: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
