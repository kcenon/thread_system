#!/usr/bin/env python3
"""Cross-system conformance linter for the kcenon ecosystem.

This is the Wave 0 ecosystem SSOT (single source of truth) gate. It runs against
the repository it lives in and enforces a set of structural and metadata
conventions that every kcenon system repository is expected to follow. It is
intended to be copied verbatim into the other ecosystem repositories so that a
single, identical gate governs all of them.

Checks performed (each can be toggled off via a flag):
  1. version       : Doxyfile PROJECT_NUMBER == root VERSION == vcpkg.json version.
                     This is a lightweight cross-system assertion; the in-repo
                     deep drift checker (scripts/check_version_drift.py) remains
                     the canonical tool for version remediation. A Doxyfile that
                     uses the CMake placeholder @PROJECT_VERSION@ is considered
                     correctly wired to the SSOT and therefore conformant.
  2. examples      : exactly one examples directory exists and no stray samples/ dir.
  3. fuzz          : a fuzz/ directory exists.
  4. layout        : include/kcenon/<sys>/ canonical layout is present.
  5. artifacts     : no committed build artifacts matching cmake_test_discovery_*.json.
  6. readme-parity : README.kr.md is >= THRESHOLD of README.md by byte size.

Only the Python 3 standard library is used so the script runs anywhere python3
is available (no build toolchain required).

Exit code is 0 when every enabled check passes, 1 when any check fails, and 2 on
a usage / configuration error.
"""

from __future__ import annotations

import argparse
import fnmatch
import json
import os
import re
import sys
from dataclasses import dataclass, field


# --------------------------------------------------------------------------- #
# Result model
# --------------------------------------------------------------------------- #
@dataclass
class CheckResult:
    name: str
    ok: bool
    detail: str
    skipped: bool = False


@dataclass
class Report:
    results: list = field(default_factory=list)

    def add(self, result: CheckResult) -> None:
        self.results.append(result)

    @property
    def failures(self):
        return [r for r in self.results if not r.ok and not r.skipped]

    def render(self) -> str:
        lines = ["conformance lint report", "=" * 23]
        for r in self.results:
            if r.skipped:
                mark = "SKIP"
            elif r.ok:
                mark = "PASS"
            else:
                mark = "FAIL"
            lines.append("[{:>4}] {:<14} {}".format(mark, r.name, r.detail))
        lines.append("-" * 23)
        if self.failures:
            lines.append("RESULT: FAIL ({} violation(s))".format(len(self.failures)))
        else:
            lines.append("RESULT: PASS")
        return "\n".join(lines)


# --------------------------------------------------------------------------- #
# Helpers
# --------------------------------------------------------------------------- #
def _read_text(path: str):
    try:
        with open(path, "r", encoding="utf-8") as fh:
            return fh.read()
    except OSError:
        return None


def find_doxyfile(root: str):
    """Return the first Doxyfile or Doxyfile template found, or None.

    Looks for a root Doxyfile first, then a checked-in template, then any
    Doxyfile* anywhere in the tree (excluding .git).
    """
    candidates = [
        os.path.join(root, "Doxyfile"),
        os.path.join(root, "cmake", "templates", "Doxyfile.in"),
        os.path.join(root, "cmake", "template", "Doxyfile.in"),
        os.path.join(root, "Doxyfile.in"),
    ]
    for c in candidates:
        if os.path.isfile(c):
            return c
    for dirpath, dirnames, filenames in os.walk(root):
        if ".git" in dirnames:
            dirnames.remove(".git")
        for fn in filenames:
            if fn.lower().startswith("doxyfile"):
                return os.path.join(dirpath, fn)
    return None


def read_doxygen_project_number(path: str):
    """Extract PROJECT_NUMBER value from a Doxyfile / template."""
    text = _read_text(path)
    if text is None:
        return None
    for line in text.splitlines():
        m = re.match(r"\s*PROJECT_NUMBER\s*=\s*(.*?)\s*$", line)
        if m:
            return m.group(1).strip().strip('"')
    return None


def read_vcpkg_version(root: str):
    path = os.path.join(root, "vcpkg.json")
    text = _read_text(path)
    if text is None:
        return None, "vcpkg.json not found"
    try:
        data = json.loads(text)
    except json.JSONDecodeError as exc:
        return None, "vcpkg.json is not valid JSON: {}".format(exc)
    for key in ("version", "version-semver", "version-string", "version-date"):
        if key in data:
            return str(data[key]), None
    return None, "no version field in vcpkg.json"


def read_root_version(root: str):
    """Resolve the root project version (the SSOT).

    Preference order:
      1. a plain VERSION file at the repo root,
      2. the version declared in vcpkg.json.
    Returns (version, source_label) or (None, None).
    """
    vfile = os.path.join(root, "VERSION")
    if os.path.isfile(vfile):
        text = _read_text(vfile)
        if text and text.strip():
            return text.strip(), "VERSION file"
    version, _ = read_vcpkg_version(root)
    if version is not None:
        return version, "vcpkg.json"
    return None, None


# Placeholders that mean "wired to the CMake-resolved project version", i.e.
# the value is filled in at configure time from the SSOT and is conformant.
_VERSION_PLACEHOLDERS = {"@PROJECT_VERSION@", "${PROJECT_VERSION}", "@VERSION@"}


# --------------------------------------------------------------------------- #
# Checks
# --------------------------------------------------------------------------- #
def check_version(root: str) -> CheckResult:
    vcpkg_version, vcpkg_err = read_vcpkg_version(root)
    if vcpkg_version is None:
        return CheckResult("version", False, vcpkg_err or "vcpkg version unreadable")

    root_version, root_src = read_root_version(root)
    if root_version is None:
        return CheckResult("version", False, "could not resolve root version")

    if root_version != vcpkg_version:
        return CheckResult(
            "version",
            False,
            "root version ({}) from {} != vcpkg.json version ({})".format(
                root_version, root_src, vcpkg_version
            ),
        )

    doxyfile = find_doxyfile(root)
    if doxyfile is None:
        return CheckResult("version", False, "no Doxyfile / Doxyfile template found")

    project_number = read_doxygen_project_number(doxyfile)
    rel_doxy = os.path.relpath(doxyfile, root)
    if project_number is None:
        return CheckResult(
            "version", False, "PROJECT_NUMBER not found in {}".format(rel_doxy)
        )

    if project_number in _VERSION_PLACEHOLDERS:
        return CheckResult(
            "version",
            True,
            "vcpkg=={ver}; {dox} PROJECT_NUMBER={ph} (wired to SSOT)".format(
                ver=vcpkg_version, dox=rel_doxy, ph=project_number
            ),
        )

    if project_number != vcpkg_version:
        return CheckResult(
            "version",
            False,
            "{dox} PROJECT_NUMBER ({pn}) != root VERSION / vcpkg.json ({v}) "
            "[remediate with scripts/check_version_drift.py]".format(
                dox=rel_doxy, pn=project_number, v=vcpkg_version
            ),
        )

    return CheckResult(
        "version",
        True,
        "3-way match: PROJECT_NUMBER == root VERSION == vcpkg.json == {}".format(
            vcpkg_version
        ),
    )


def check_examples(root: str) -> CheckResult:
    examples = os.path.join(root, "examples")
    samples = os.path.join(root, "samples")
    has_examples = os.path.isdir(examples)
    has_samples = os.path.isdir(samples)

    problems = []
    if not has_examples:
        problems.append("missing examples/ directory")
    if has_samples:
        problems.append("stray samples/ directory present (use examples/)")

    if problems:
        return CheckResult("examples", False, "; ".join(problems))
    return CheckResult("examples", True, "exactly one examples/ dir, no samples/")


def check_fuzz(root: str) -> CheckResult:
    fuzz = os.path.join(root, "fuzz")
    if os.path.isdir(fuzz):
        return CheckResult("fuzz", True, "fuzz/ directory present")
    return CheckResult("fuzz", False, "missing fuzz/ directory")


def check_layout(root: str) -> CheckResult:
    base = os.path.join(root, "include", "kcenon")
    if not os.path.isdir(base):
        return CheckResult(
            "layout", False, "missing include/kcenon/ canonical layout"
        )
    systems = sorted(
        d for d in os.listdir(base) if os.path.isdir(os.path.join(base, d))
    )
    if not systems:
        return CheckResult(
            "layout", False, "include/kcenon/ has no <sys>/ subdirectory"
        )
    return CheckResult(
        "layout",
        True,
        "include/kcenon/<sys>/ present: {}".format(", ".join(systems)),
    )


def check_artifacts(root: str, pattern: str) -> CheckResult:
    matches = []
    for dirpath, dirnames, filenames in os.walk(root):
        if ".git" in dirnames:
            dirnames.remove(".git")
        for fn in filenames:
            if fnmatch.fnmatch(fn, pattern):
                matches.append(os.path.relpath(os.path.join(dirpath, fn), root))
    if matches:
        return CheckResult(
            "artifacts",
            False,
            "committed build artifacts found: {}".format(", ".join(sorted(matches))),
        )
    return CheckResult(
        "artifacts", True, "no committed artifacts matching '{}'".format(pattern)
    )


def check_readme_parity(root: str, threshold: float) -> CheckResult:
    en = os.path.join(root, "README.md")
    kr = os.path.join(root, "README.kr.md")
    if not os.path.isfile(en):
        return CheckResult("readme-parity", False, "README.md not found")
    if not os.path.isfile(kr):
        return CheckResult("readme-parity", False, "README.kr.md not found")
    en_size = os.path.getsize(en)
    kr_size = os.path.getsize(kr)
    if en_size == 0:
        return CheckResult("readme-parity", False, "README.md is empty")
    ratio = kr_size / en_size
    if ratio < threshold:
        return CheckResult(
            "readme-parity",
            False,
            "README.kr.md is {:.1%} of README.md (< {:.0%}); kr={}B en={}B".format(
                ratio, threshold, kr_size, en_size
            ),
        )
    return CheckResult(
        "readme-parity",
        True,
        "README.kr.md is {:.1%} of README.md (>= {:.0%})".format(ratio, threshold),
    )


# --------------------------------------------------------------------------- #
# CLI
# --------------------------------------------------------------------------- #
CHECK_NAMES = [
    "version",
    "examples",
    "fuzz",
    "layout",
    "artifacts",
    "readme-parity",
]


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="kcenon ecosystem cross-system conformance linter (Wave 0 SSOT gate).",
    )
    parser.add_argument(
        "--root",
        default=".",
        help="repository root to lint (default: current directory).",
    )
    parser.add_argument(
        "--readme-threshold",
        type=float,
        default=0.70,
        help="minimum README.kr.md / README.md byte ratio (default: 0.70).",
    )
    parser.add_argument(
        "--artifact-glob",
        default="cmake_test_discovery_*.json",
        help="filename glob for forbidden committed build artifacts.",
    )
    parser.add_argument(
        "--skip",
        action="append",
        default=[],
        choices=CHECK_NAMES,
        metavar="CHECK",
        help="disable a check (repeatable). One of: " + ", ".join(CHECK_NAMES),
    )
    parser.add_argument(
        "--only",
        action="append",
        default=[],
        choices=CHECK_NAMES,
        metavar="CHECK",
        help="run only these checks (repeatable).",
    )
    return parser


def main(argv=None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    root = os.path.abspath(args.root)
    if not os.path.isdir(root):
        sys.stderr.write("error: root '{}' is not a directory\n".format(root))
        return 2

    if not (0.0 <= args.readme_threshold <= 1.0):
        sys.stderr.write("error: --readme-threshold must be between 0.0 and 1.0\n")
        return 2

    if args.only:
        enabled = set(args.only)
    else:
        enabled = set(CHECK_NAMES) - set(args.skip)

    report = Report()

    def run(name, fn):
        if name in enabled:
            report.add(fn())
        else:
            report.add(CheckResult(name, True, "disabled", skipped=True))

    run("version", lambda: check_version(root))
    run("examples", lambda: check_examples(root))
    run("fuzz", lambda: check_fuzz(root))
    run("layout", lambda: check_layout(root))
    run("artifacts", lambda: check_artifacts(root, args.artifact_glob))
    run("readme-parity", lambda: check_readme_parity(root, args.readme_threshold))

    print("linting repository: {}".format(root))
    print(report.render())

    return 1 if report.failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
