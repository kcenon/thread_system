#!/usr/bin/env python3
"""Validate tag identity independently, then the actual port archive SHA512.

The expected merge commit comes from a reviewed provenance record, an explicit
release-process input, or a merged release PR. It is never inferred from the
tag itself or today's moving branch tip. Exit 1 is a failed check, 2 bad input.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request

REPOS = {f"kcenon/{name}_system" for name in
         ("common", "thread", "container", "logger", "monitoring", "database", "network", "pacs")}


def git(root, *args):
    return subprocess.check_output(["git", "-C", str(root), *args], text=True,
                                   stderr=subprocess.PIPE).strip()


def commit(value):
    if not isinstance(value, str) or not re.fullmatch(r"[0-9a-f]{40}", value):
        raise ValueError("expected a full lowercase 40-character commit SHA")
    return value


def tag_commit(root, tag):
    if not re.fullmatch(r"v?\d+\.\d+\.\d+(?:[-+][A-Za-z0-9.-]+)?", tag):
        raise ValueError(f"invalid release tag: {tag}")
    try:
        return commit(git(root, "rev-parse", "--verify", f"refs/tags/{tag}^{{commit}}"))
    except subprocess.CalledProcessError as exc:
        raise ValueError(f"tag does not resolve to a commit: {tag}") from exc


def release_provenance(root, repo, tag, resolved, expected=None):
    if expected:
        return commit(expected), "explicit release-process input"
    record_file = root / "ci/release-provenance.json"
    if record_file.exists():
        records = json.loads(record_file.read_text())
        if tag in records:
            record = records[tag]
            if not record.get("source_url", "").startswith(f"https://github.com/{repo}/pull/"):
                raise ValueError("release provenance requires a source release PR URL")
            return commit(record["merge_commit"]), record["source_url"]
    try:
        raw = subprocess.check_output(["gh", "api", f"repos/{repo}/commits/{resolved}/pulls?per_page=100"],
                                      text=True, stderr=subprocess.PIPE, timeout=60)
        prs = json.loads(raw)
    except (subprocess.SubprocessError, OSError, ValueError) as exc:
        raise ValueError(f"release provenance unavailable: {exc}") from exc
    matches = [pr for pr in prs if pr.get("merged_at") and pr.get("merge_commit_sha") == resolved
               and pr.get("base", {}).get("ref") == "main"
               and (pr.get("head", {}).get("ref") == "develop"
                    or pr.get("head", {}).get("ref", "").startswith("release/"))]
    if len(matches) != 1:
        raise ValueError("missing or ambiguous independent release-merge provenance")
    return commit(matches[0]["merge_commit_sha"]), matches[0]["html_url"]


def port_metadata(port_dir, repo, tag):
    manifest = json.loads((port_dir / "vcpkg.json").read_text())
    versions = [manifest[k] for k in ("version", "version-semver", "version-string") if k in manifest]
    if len(versions) != 1 or "v" + versions[0] != "v" + tag.removeprefix("v"):
        raise ValueError("port manifest version does not match the release tag")
    content = (port_dir / "portfile.cmake").read_text()
    blocks = re.findall(r"vcpkg_from_github\s*\((.*?)\)", content, re.S | re.I)
    if len(blocks) != 1:
        raise ValueError("expected one vcpkg_from_github source declaration")
    values = {}
    for name in ("REPO", "REF", "SHA512"):
        matches = re.findall(r"\b" + name + r'\s+("[^"\n]*"|[^\s)]+)', blocks[0])
        if len(matches) != 1:
            raise ValueError(f"expected one {name} in port source")
        values[name] = matches[0].strip('"')
    ref = values["REF"].replace("${VERSION}", versions[0])
    if values["REPO"] != repo or ref != tag:
        raise ValueError("port repository/ref does not identify the requested release")
    if not re.fullmatch(r"[0-9a-fA-F]{128}", values["SHA512"]):
        raise ValueError("port SHA512 must be an actual 128-digit digest")
    return f"https://github.com/{repo}/archive/refs/tags/{tag}.tar.gz", values["SHA512"].lower()


def archive_hash(url, destination):
    request = urllib.request.Request(url, headers={"User-Agent": "kcenon-tag-reality"})
    with urllib.request.urlopen(request, timeout=60) as response, destination.open("wb") as stream:
        if response.status != 200:
            raise ValueError(f"archive download returned HTTP {response.status}")
        total = 0
        digest = hashlib.sha512()
        while block := response.read(1024 * 1024):
            total += len(block)
            if total > 512 * 1024 * 1024:
                raise ValueError("archive exceeds 512 MiB validation limit")
            stream.write(block)
            digest.update(block)
    with destination.open("rb") as stream:
        if total == 0 or stream.read(2) != b"\x1f\x8b":
            raise ValueError("download is not a gzip source archive")
    return digest.hexdigest()


def verify(root, repo, tag, expected=None, port_dir=None, identity_only=False):
    if repo not in REPOS:
        raise ValueError("repository is outside the ecosystem allowlist")
    resolved = tag_commit(root, tag)
    expected, provenance = release_provenance(root, repo, tag, resolved, expected)
    if resolved != expected:
        raise ValueError(f"tag {tag} resolves to {resolved}, expected release merge {expected}")
    result = {"repository": repo, "tag": tag, "tag_commit": resolved,
              "release_merge_commit": expected, "provenance": provenance,
              "stage": "identity" if identity_only else "identity-and-port"}
    if not identity_only:
        port_dir = port_dir or root / "vcpkg-ports" / ("kcenon-" + repo.split("/")[1].replace("_", "-"))
        url, stored = port_metadata(port_dir, repo, tag)
        with tempfile.TemporaryDirectory() as temp:
            actual = archive_hash(url, Path(temp) / "source.tar.gz")
        if stored != actual:
            raise ValueError(f"port SHA512 mismatch: stored={stored}, downloaded={actual}")
        # A tag moved while downloading must not pass with mixed evidence.
        remote = subprocess.check_output(["git", "ls-remote", f"https://github.com/{repo}.git",
                                          f"refs/tags/{tag}", f"refs/tags/{tag}^{{}}"], text=True, timeout=60)
        refs = dict((line.split()[1], line.split()[0]) for line in remote.splitlines())
        if refs.get(f"refs/tags/{tag}^{{}}", refs.get(f"refs/tags/{tag}")) != resolved:
            raise ValueError("remote release tag changed during archive validation")
        result.update(archive_url=url, sha512=actual, port_directory=str(port_dir))
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--repo", required=True, choices=sorted(REPOS))
    parser.add_argument("--tag", required=True)
    parser.add_argument("--release-commit")
    parser.add_argument("--port-dir", type=Path)
    parser.add_argument("--identity-only", action="store_true")
    parser.add_argument("--json-output", type=Path)
    args = parser.parse_args(argv)
    try:
        port = args.port_dir or args.root / "vcpkg-ports" / ("kcenon-" + args.repo.split("/")[1].replace("_", "-"))
        result = verify(args.root, args.repo, args.tag, args.release_commit, port, args.identity_only)
        print(json.dumps(result, indent=2))
        if args.json_output:
            args.json_output.write_text(json.dumps(result, indent=2) + "\n")
        return 0
    except (ValueError, KeyError, OSError, subprocess.SubprocessError, urllib.error.URLError) as exc:
        print(f"TAG REALITY FAILED: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
