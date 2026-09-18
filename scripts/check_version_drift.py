#!/usr/bin/env python3
"""Check local FetchContent pins against vcpkg overrides (stdlib only).
Exit 0: applicable comparisons agree; 1: drift/unresolved pinned input;
2: invalid input or no comparisons without a documented reason.
Shared source: kcenon/common_system, issue #701.
"""
from __future__ import annotations
import argparse
import json
import re
import sys
from pathlib import Path

ALIASES = {"googletest": "gtest", "googlebenchmark": "benchmark"}
VERSION_FIELDS = ("version", "version-semver", "version-string", "version-date")


def normalize_version(tag):
    tag = tag.strip().strip('"')
    if re.fullmatch(r"asio-\d+(?:-\d+)+", tag):
        return tag[5:].replace("-", ".")
    return re.sub(r"^(?:release-|v)", "", tag)


def read_json(path):
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        raise ValueError(f"{path}: {exc}") from exc
    if not isinstance(value, dict):
        raise ValueError(f"{path}: expected a JSON object")
    return value


def parse_vcpkg_overrides(path):
    data = read_json(path)
    entries = data.get("overrides", [])
    if not isinstance(entries, list):
        raise ValueError(f"{path}: overrides must be an array")
    result = {}
    for entry in entries:
        if not isinstance(entry, dict) or not isinstance(entry.get("name"), str):
            raise ValueError(f"{path}: invalid override {entry!r}")
        fields = [key for key in VERSION_FIELDS if key in entry]
        if len(fields) != 1 or not isinstance(entry[fields[0]], str):
            raise ValueError(f"{path}: override {entry['name']} needs one version string")
        name, version = entry["name"].lower(), entry[fields[0]]
        if not version or name in result:
            raise ValueError(f"{path}: empty or duplicate override {name}")
        result[name] = version
    return result


def commands(text):
    """Read CMake commands while respecting quoted strings and comments."""
    text = re.sub(r"#\[(=*)\[.*?\]\1\]", "", text, flags=re.S)
    text = re.sub(r'"(?:\\.|[^"\\])*"|#[^\n]*',
                  lambda m: m[0] if m[0].startswith('"') else "", text)
    pattern = re.compile(r"\b(\w+)\s*\(")
    pos = 0
    while match := pattern.search(text, pos):
        start = pos = match.end()
        depth, quoted, escaped = 1, False, False
        while pos < len(text) and depth:
            char = text[pos]
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quoted = not quoted
            elif not quoted:
                depth += (char == "(") - (char == ")")
            pos += 1
        if depth:
            raise ValueError(f"unterminated CMake command {match[1]}")
        tokens = re.findall(r'"(?:\\.|[^"\\])*"|[^\s()]+', text[start:pos-1])
        yield match[1].lower(), [t[1:-1] if t.startswith('"') else t for t in tokens]


def find_cmake_files(root):
    result = []
    for path in root.rglob("*"):
        if path.name != "CMakeLists.txt" and path.suffix != ".cmake":
            continue
        parts = path.relative_to(root).parts[:-1]
        if any(p.startswith(("build", "_build", ".")) or p in
               ("_deps", "vendor", "third_party", "fixtures") for p in parts):
            continue
        if path.is_file():
            result.append(path)
    return sorted(result)


def resolve(value, variables):
    for _ in range(10):
        expanded = re.sub(r"\$\{(\w+)\}", lambda m: variables.get(m[1], m[0]), value)
        if expanded == value:
            break
        value = expanded
    return value


def parse_fetchcontent_tags(files):
    parsed, assignments = [], {}
    for path in files:
        for name, args in commands(path.read_text(encoding="utf-8")):
            parsed.append((path, name, args))
            if name == "set" and len(args) >= 2:
                if len(args) == 2 or args[2] in ("CACHE", "PARENT_SCOPE"):
                    assignments.setdefault(args[0], set()).add(args[1])
    variables = {k: next(iter(v)) for k, v in assignments.items() if len(v) == 1}
    result = []
    for path, name, args in parsed:
        if name != "fetchcontent_declare" or not args:
            continue
        upper = [a.upper() for a in args]
        field = "GIT_TAG" if "GIT_TAG" in upper else "URL" if "URL" in upper else None
        if not field:
            continue
        index = upper.index(field)
        if index + 1 == len(args):
            raise ValueError(f"{path}: {field} requires a value")
        raw = resolve(args[index+1], variables)
        if field == "URL":
            match = re.search(r"/(?:refs/tags/)?([^/]+?)(?:\.tar\.gz|\.zip|\.tgz)$", raw)
            raw = match[1] if match else raw
        dep = resolve(args[0], variables).lower()
        result.append({"dep": ALIASES.get(dep, dep), "tag": raw, "file": str(path)})
    return result


def check_matrix_consistency(versions, path):
    column, findings, rows = None, [], 0
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.startswith("|"):
            if column is not None:
                break
            continue
        cells = [v.strip(" *") for v in line.strip("|").split("|")]
        if "Dependency" in cells and "Ecosystem Standard" in cells:
            column = cells.index("Ecosystem Standard")
            continue
        if column is None or len(cells) <= column:
            continue
        name = cells[0].lower()
        name = {"otel c++": "opentelemetry-cpp"}.get(name, name)
        if name not in versions:
            continue
        match = re.search(r"\d+\.\d+(?:\.\d+)?", cells[column])
        if not match:
            raise ValueError(f"{path}: no ecosystem version for {name}")
        rows += 1
        if normalize_version(versions[name]) != match[0]:
            findings.append(f"MATRIX {name}: override={versions[name]} standard={match[0]}")
    if column is None or not rows:
        raise ValueError(f"{path}: no applicable ecosystem matrix rows")
    return findings, rows


def check(root, manifest, matrix=None, no_comparisons_reason=""):
    versions = parse_vcpkg_overrides(manifest)
    declarations = parse_fetchcontent_tags(find_cmake_files(root))
    report = {"compared": [], "unmatched": [], "unresolved": [], "findings": [],
              "matrix_compared": 0, "no_comparisons_reason": no_comparisons_reason}
    for item in declarations:
        dep, tag = item["dep"], item["tag"]
        item["file"] = str(Path(item["file"]).relative_to(root))
        if dep not in versions:
            report["unmatched"].append(item)
            continue
        expected, actual = normalize_version(versions[dep]), normalize_version(tag)
        if not re.fullmatch(r"\d+\.\d+(?:\.\d+)*(?:[-+][\w.-]+)?", actual):
            report["unresolved"].append(item)
            report["findings"].append(f"UNRESOLVED {dep}: {tag} ({item['file']})")
            continue
        report["compared"].append(dict(item, expected=expected))
        if expected != actual:
            report["findings"].append(f"DRIFT {dep}: override={expected} tag={tag} ({item['file']})")
    if matrix:
        findings, count = check_matrix_consistency(versions, matrix)
        report["findings"].extend(findings)
        report["matrix_compared"] = count
    if not report["compared"] and not report["unresolved"] and not no_comparisons_reason.strip():
        raise ValueError("no comparable FetchContent pins; configure a reviewed no_comparisons_reason if intentional")
    return report


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--vcpkg-json", type=Path)
    parser.add_argument("--matrix", type=Path)
    parser.add_argument("--no-matrix", action="store_true")
    parser.add_argument("--no-color", action="store_true", help="Output is always plain text")
    parser.add_argument("--verbose", action="store_true")
    parser.add_argument("--json-output", type=Path)
    args = parser.parse_args(argv)
    try:
        root = args.project_root.resolve()
        if not root.is_dir():
            raise ValueError(f"{root}: project root not found")
        config_path = root / "ci/coherence.json"
        config = read_json(config_path) if config_path.exists() else {}
        matrix = None if args.no_matrix else args.matrix
        if matrix is None and not args.no_matrix and config.get("matrix"):
            matrix = root / config["matrix"]
        reason = config.get("no_comparisons_reason", "")
        if not isinstance(reason, str):
            raise ValueError("no_comparisons_reason must be a string")
        report = check(root, args.vcpkg_json or root / "vcpkg.json", matrix, reason)
        for finding in report["findings"]:
            print(finding)
        print(f"Compared {len(report['compared'])} FetchContent pin(s), "
              f"{report['matrix_compared']} matrix row(s); "
              f"{len(report['unmatched'])} declarations without matching overrides")
        if not report["compared"]:
            print("Local comparisons: N/A — " + (reason or "unresolved pins (failure)"))
        if args.verbose:
            print(json.dumps(report, indent=2))
        if args.json_output:
            args.json_output.write_text(json.dumps(report, indent=2) + "\n")
        return 1 if report["findings"] else 0
    except (OSError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
