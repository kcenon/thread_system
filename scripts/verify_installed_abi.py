#!/usr/bin/env python3
"""Install Thread and run external CMake/pkg-config consumers, including negative controls."""
import argparse
import ast
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys


def run(command, *, expected=0, env=None):
    print("+ " + shlex.join(map(str, command)), flush=True)
    result = subprocess.run(list(map(str, command)), env=env, timeout=1200)
    if result.returncode != expected:
        raise RuntimeError(f"Expected exit {expected}, got {result.returncode}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--common", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--jthread", choices=["ON", "OFF"], required=True)
    args = parser.parse_args()
    source, common, work = args.source.resolve(), args.common.resolve(), args.work.resolve()
    if work.is_relative_to(source):
        raise RuntimeError("Consumers must live outside the source tree")
    work.mkdir(parents=True, exist_ok=False)
    prefix = work / "prefix"
    options = ["-G", "Ninja", "-DCMAKE_BUILD_TYPE=RelWithDebInfo", "-DCMAKE_INSTALL_LIBDIR=lib",
               f"-DCMAKE_INSTALL_PREFIX={prefix}", "-DBUILD_TESTING=OFF"]
    run(["cmake", "-S", common, "-B", work / "common-build", *options,
         "-DCOMMON_BUILD_TESTS=OFF", "-DCOMMON_BUILD_EXAMPLES=OFF", "-DCOMMON_BUILD_BENCHMARKS=OFF",
         "-DCOMMON_BUILD_INTEGRATION_TESTS=OFF"])
    run(["cmake", "--build", work / "common-build", "--parallel", "3"])
    run(["cmake", "--install", work / "common-build"])
    run(["cmake", "-S", source, "-B", work / "library-build", *options,
         f"-DCMAKE_PREFIX_PATH={prefix}", "-DBUILD_SAMPLES=OFF", "-DBUILD_DOCUMENTATION=OFF",
         "-DTHREAD_BUILD_INTEGRATION_TESTS=OFF", "-DTHREAD_BUILD_ABI_TESTS=ON",
         "-DCMAKE_DISABLE_FIND_PACKAGE_simdutf=ON",
         *[f"-DSET_STD_{feature}={args.jthread}" for feature in
           ["JTHREAD", "CONCEPTS", "ATOMIC_WAIT", "LATCH", "SPAN"]],
         f"-DTHREAD_ENABLE_WORK_STEALING={args.jthread}"])
    run(["cmake", "--build", work / "library-build", "--parallel", "3"])
    run(["cmake", "--install", work / "library-build"])

    # Reuse the scenario maintained in common_system#751 without running its
    # eight-repository orchestration or maintaining another worker lifecycle test.
    module = ast.parse((common / "scripts/ecosystem_build.py").read_text())
    consumers = next(ast.literal_eval(node.value) for node in module.body
                     if isinstance(node, ast.Assign)
                     and any(isinstance(t, ast.Name) and t.id == "CONSUMERS" for t in node.targets))
    consumer = work / "consumer"
    shutil.copytree(source / "tests/installed_consumer", consumer)
    (consumer / "main.cpp").write_text((consumer / "main.cpp.in").read_text().replace(
        "@WORKER_SMOKE_BODY@", consumers["thread_system"]))
    expected_jthread = "1" if args.jthread == "ON" else "0"
    base = ["cmake", "-S", consumer, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=RelWithDebInfo",
            f"-DCMAKE_PREFIX_PATH={prefix}", f"-DEXPECT_JTHREAD={expected_jthread}"]
    run([*base, "-B", work / "cmake-consumer"])
    run(["cmake", "--build", work / "cmake-consumer", "--parallel", "3"])
    run(["ctest", "--test-dir", work / "cmake-consumer", "--output-on-failure", "--no-tests=error"])
    run([*base, "-B", work / "cmake-negative", "-DABI_NEGATIVE=ON"])
    run(["cmake", "--build", work / "cmake-negative", "--parallel", "3"])
    suffix = ".exe" if os.name == "nt" else ""
    run([work / "cmake-negative" / ("consumer" + suffix)], expected=2)

    env = dict(os.environ, PKG_CONFIG_PATH=str(prefix / "lib/pkgconfig"),
               PKG_CONFIG_LIBDIR=str(prefix / "lib/pkgconfig"))
    pkg_command = ["pkg-config", "--static", "--cflags", "--libs", "thread_system"]
    if os.name == "nt":
        pkg_command.insert(1, "--msvc-syntax")
    pkg_output = subprocess.check_output(pkg_command, env=env, text=True)
    print("pkg-config: " + pkg_output, flush=True)
    # pkg-config emits shell-quoted paths; even on Windows they use forward slashes.
    flags = shlex.split(pkg_output)
    compiler = shlex.split(os.environ.get("CXX", "cl" if os.name == "nt" else "c++"))
    cxxflags = shlex.split(os.environ.get("CXXFLAGS", ""))
    ldflags = shlex.split(os.environ.get("LDFLAGS", ""))
    results = {"jthread": args.jthread, "cmake": "passed", "cmake_negative": "rejected"}
    for negative in (False, True):
        output = work / (("pkg-negative" if negative else "pkg-consumer") + suffix)
        mutation = []
        if negative:
            mutation = [("/" if os.name == "nt" else "-") +
                        ("UUSE_STD_JTHREAD" if args.jthread == "ON" else "DUSE_STD_JTHREAD")]
        if os.name == "nt":
            # /link options emitted by pkg-config must follow the source and /Fe.
            command = [*compiler, "/nologo", "/std:c++20", "/EHsc", "/MD", *cxxflags,
                       f"/DEXPECT_JTHREAD={expected_jthread}", str(consumer / "main.cpp"),
                       f"/Fe:{output}"]
            split = flags.index("/link") if "/link" in flags else len(flags)
            command += [*flags[:split], *mutation, *flags[split:], *ldflags]
        else:
            command = [*compiler, "-std=c++20", *cxxflags, f"-DEXPECT_JTHREAD={expected_jthread}",
                       consumer / "main.cpp", "-o", output, *flags, *mutation, *ldflags]
        run(command, env=env)
        run([output], expected=2 if negative else 0, env=env)
        results["pkg_negative" if negative else "pkg_config"] = "rejected" if negative else "passed"
    (work / "result.json").write_text(json.dumps(results, indent=2) + "\n")


if __name__ == "__main__":
    main()
