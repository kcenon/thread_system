#!/usr/bin/env python3
"""Run CTest only after proving the configured, selected test set is present."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("--config", default="Debug")
    parser.add_argument("--timeout", default="300")
    parser.add_argument("--include", default=".*")
    parser.add_argument("--require", action="append", required=True)
    args = parser.parse_args()
    cache = (args.build / "CMakeCache.txt").read_text()
    if not re.search(r"^BUILD_TESTING:[^=]+=(ON|TRUE|1)$", cache, re.MULTILINE):
        raise RuntimeError("BUILD_TESTING must be explicitly ON")
    command = ["ctest", "--test-dir", str(args.build), "-C", args.config, "-R", args.include]
    discovery = subprocess.run(command + ["--show-only=json-v1"], check=True,
                               text=True, capture_output=True)
    tests = json.loads(discovery.stdout)["tests"]
    names = {test["name"] for test in tests}
    if not names:
        raise RuntimeError("No selected tests were discovered")
    missing = set(args.require) - names
    if missing:
        raise RuntimeError(f"Required tests were not discovered: {sorted(missing)}")
    print(f"Discovered {len(names)} selected tests: {', '.join(sorted(names))}", flush=True)
    return subprocess.run(command + ["--output-on-failure", "--no-tests=error",
                                     "--timeout", args.timeout]).returncode


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as error:
        print(f"CI test gate: {error}", file=sys.stderr)
        sys.exit(1)
