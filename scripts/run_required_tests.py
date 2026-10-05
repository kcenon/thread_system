#!/usr/bin/env python3
"""Run CTest only after proving the configured, selected test set is present."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("--config", default="Debug")
    parser.add_argument("--timeout", default="300")
    parser.add_argument("--include", default=".*")
    parser.add_argument("--gtest", action="store_true", help="Require nonempty GoogleTest XML for every executable")
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
    env = dict(os.environ)
    if args.gtest:
        report_dir = Path(tempfile.mkdtemp(prefix="required-gtest-", dir=args.build.resolve()))
        env["GTEST_OUTPUT"] = "xml:" + report_dir.as_posix() + "/"
    result = subprocess.run(command + ["--output-on-failure", "--no-tests=error",
                                       "--timeout", args.timeout], env=env)
    if result.returncode:
        return result.returncode
    if args.gtest:
        for test in tests:
            executable = Path(test["command"][0]).stem
            report = report_dir / (executable + ".xml")
            suite = ET.parse(report).getroot()
            if int(suite.attrib.get("tests", "0")) == 0:
                raise RuntimeError(f"GoogleTest ran zero tests: {test['name']}")
        print(f"Verified nonempty GoogleTest execution for all {len(tests)} suites", flush=True)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError, ValueError, ET.ParseError, subprocess.CalledProcessError) as error:
        print(f"CI test gate: {error}", file=sys.stderr)
        sys.exit(1)
