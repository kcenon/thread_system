#!/usr/bin/env python3
"""Apply advisory/enforcing policy without losing the underlying gate result."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

CHECKS = {"conformance": "conformance_lint.py", "version-drift": "check_version_drift.py",
          "tag-reality": "check_tag_reality.py"}


def run_check(root, name, output, extra=(), strict=False):
    config = json.loads((root / "ci/coherence.json").read_text())
    if config.get("schema_version") != 1:
        raise ValueError("unsupported coherence schema")
    mode = config["modes"][name]
    if mode not in ("advisory", "enforcing"):
        raise ValueError(f"invalid gate mode: {mode}")
    if strict:
        mode = "enforcing"
    command = [sys.executable, str(root / "scripts" / CHECKS[name]), *extra]
    result = subprocess.run(command, cwd=root, text=True, capture_output=True)
    sha = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip()
    dirty = bool(subprocess.check_output(["git", "status", "--porcelain", "--untracked-files=all"],
                                        cwd=root, text=True).strip())
    report = {"schema_version": 1, "check": name, "mode": mode, "source_sha": sha,
              "worktree_changes": dirty, "command": command, "exit_code": result.returncode,
              "passed": result.returncode == 0, "stdout": result.stdout, "stderr": result.stderr}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + "\n")
    print(result.stdout, end="")
    print(result.stderr, end="", file=sys.stderr)
    if result.returncode:
        severity = "warning" if mode == "advisory" else "error"
        print(f"::{severity}::{name}: raw exit {result.returncode}; mode={mode}; report={output}")
    return (1 if result.returncode else 0) if mode == "enforcing" else 0


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("check", choices=CHECKS)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path)
    parser.add_argument("--strict", action="store_true")
    args, extra = parser.parse_known_args(argv)
    if extra[:1] == ["--"]:
        extra = extra[1:]
    try:
        root = args.root.resolve()
        return run_check(root, args.check, args.output or root / f"coherence-results/{args.check}.json",
                         extra, args.strict)
    except (OSError, ValueError, KeyError, subprocess.SubprocessError) as exc:
        print(f"ERROR: gate configuration/execution failed: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
