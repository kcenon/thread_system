# Installed Thread ABI verification

`installed-abi.yml` runs on main/develop pushes and PRs and on manual dispatch.
Linux GCC, macOS LLVM and Windows MSVC each install jthread ON and OFF builds,
then build and run both a CMake `find_package` and a pkg-config consumer outside
the source tree. AddressSanitizer instruments the archive and both consumers.

Feature probes register their selected definitions in
`THREAD_SYSTEM_PUBLIC_FEATURES`. The library exports these as PUBLIC definitions;
pkg-config takes its Cflags from the target's INTERFACE_COMPILE_DEFINITIONS.
This covers USE_STD_JTHREAD, USE_STD_CONCEPTS, HAS_STD_ATOMIC_WAIT, HAS_STD_LATCH,
USE_STD_SPAN and the other selected standard-library features. Work stealing's
public config macro is exported too. Platform/compiler detection macros remain
header-derived; common executor and integration definitions retain their public
contract. No sanitizer suppression or test exclusion is added.

The opt-in `THREAD_BUILD_ABI_TESTS` adds test-only probe functions to the actual
archive. They measure the selected feature bits and sizes of lifecycle_controller,
thread_base and thread_worker using the archive's compile settings. Consumers
compare those values with their own compilation and require the requested jthread
mode. The ON/OFF matrix also toggles the other four optional header features and
work stealing, checking disabled definitions are absent after installation.
The worker creation/enqueue/start/stop body is read from the immutable
common_system#751 commit `4aa24d2650c1d3d8446f7e9f681824bec12b5112`; there is no
second maintained lifecycle implementation here.

Each consumption method also deliberately flips USE_STD_JTHREAD and must return
ABI-mismatch exit code 2. The comparison happens before allocation, so a broken
export fails deterministically rather than depending solely on an ASan crash.
Raw logs, target exports, pkg-config flags, compile commands and result JSON are
uploaded for each of the six jobs. Existing required checks/rulesets are unchanged.
The six `Installed ABI / <os> / jthread-<ON|OFF>` contexts are candidates for
additional required checks after validation.

To reproduce locally (CMake, Ninja, pkg-config and a C++20 compiler required):

```sh
CXX=clang++ CXXFLAGS='-fsanitize=address -fno-omit-frame-pointer' \
  python3 scripts/verify_installed_abi.py --common /path/to/common_system \
  --work /tmp/thread-abi-on --jthread ON
```

Use a fresh work directory per invocation. Repeat with OFF. The fixture uses
Thread's pinned simdutf FetchContent dependency and installs it alongside Thread;
its consumer cannot accidentally link an unrelated system simdutf archive.

## Delivery boundary (#739)

The source fixes in main from #749/#750 do not alter the existing v1.0.0 archive.
Registry main at `670f090e1b3a238e4826a0a6efb658f079cdbcd6` still selects Thread
0.3.2#2; registry develop at `14296b9` separately carries the v1.0.0 port.
Neither is evidence that this PR's public feature contract has been deployed.
Issue #739 must remain open until a separately reviewed package delivery carries
the corrected sources/exports and real external registry consumers pass the same
ABI and worker checks. This PR changes no release tag or package version.

The existing v1.0.0 port was also installed without binary cache on arm64-osx via
Git registry baseline and reference
`14296b92eb8accac35e9c6c24aa4c1f3c2f3faef`, using vcpkg tool commit
`2200159cd465157f20133e363e4604ac4d68181b` and builtin baseline
`b02e341c927f16d991edbd915d8ea43eac52096c`. The verified archive corresponds to
Thread source `6dd5c9e8c224fcd177560623cd7eccad6415f3e4`. Installation succeeds,
but its archive is compiled with all five header feature macros while the
installed CMake target exports none of them and pkg-config Cflags contain only
include paths. External CMake and pkg-config acceptance consumers both fail the
five macro checks before worker allocation. This confirms the package delivery
condition remains unmet, independently of the Windows MSYS2 tooling repair.
