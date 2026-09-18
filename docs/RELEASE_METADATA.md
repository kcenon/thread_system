# Release metadata

The latest stable release is [v1.0.0](https://github.com/kcenon/thread_system/releases/tag/v1.0.0).
Its root `vcpkg.json` manifest and CMake project both declare `1.0.0`.
Release tags use `vMAJOR.MINOR.PATCH`.

The table below records the published releases as inspected on 2026-09-18.
Each version links to the file at the commit referenced by that release's tag.
The root package manifest and the source-local vcpkg port are separate files.

| Release | Root package manifest | CMake project | Source-local vcpkg port |
| --- | --- | --- | --- |
| [v1.0.0](https://github.com/kcenon/thread_system/releases/tag/v1.0.0) | [1.0.0](https://github.com/kcenon/thread_system/blob/6dd5c9e8c224fcd177560623cd7eccad6415f3e4/vcpkg.json) | [1.0.0](https://github.com/kcenon/thread_system/blob/6dd5c9e8c224fcd177560623cd7eccad6415f3e4/CMakeLists.txt) | [0.3.2](https://github.com/kcenon/thread_system/blob/6dd5c9e8c224fcd177560623cd7eccad6415f3e4/vcpkg-ports/kcenon-thread-system/vcpkg.json) |
| [v0.3.2](https://github.com/kcenon/thread_system/releases/tag/v0.3.2) | [0.3.1](https://github.com/kcenon/thread_system/blob/a9e1b8f01dab995ef2a916df7c3a18ef81b24b71/vcpkg.json) | [0.3.1](https://github.com/kcenon/thread_system/blob/a9e1b8f01dab995ef2a916df7c3a18ef81b24b71/CMakeLists.txt) | [0.3.1](https://github.com/kcenon/thread_system/blob/a9e1b8f01dab995ef2a916df7c3a18ef81b24b71/vcpkg-ports/kcenon-thread-system/vcpkg.json) |
| [v0.3.1](https://github.com/kcenon/thread_system/releases/tag/v0.3.1) | [0.3.0](https://github.com/kcenon/thread_system/blob/73903b6fc98a9a0e3555c435e9d17c322e8ba2ba/vcpkg.json) | [0.3.1](https://github.com/kcenon/thread_system/blob/73903b6fc98a9a0e3555c435e9d17c322e8ba2ba/CMakeLists.txt) | Absent |
| [v0.3.0](https://github.com/kcenon/thread_system/releases/tag/v0.3.0) | [0.3.0](https://github.com/kcenon/thread_system/blob/b5c314dbd33df97d0d0eee440ae434aaa929a70a/vcpkg.json) | [0.3.0.0](https://github.com/kcenon/thread_system/blob/b5c314dbd33df97d0d0eee440ae434aaa929a70a/CMakeLists.txt) | Absent |

The source-local port is `vcpkg-ports/kcenon-thread-system/vcpkg.json`.
In the v1.0.0 source archive it still declares `0.3.2`, despite the root manifest
and CMake project declaring `1.0.0`. This table does not describe the current
version of a separately maintained vcpkg registry.

For v0.3.2, the root manifest and CMake project still declare `0.3.1`.
For v0.3.1, the root manifest still declares `0.3.0`.
For v0.3.0, CMake declares the four-component version `0.3.0.0`.
These are historical metadata discrepancies. The existing tags and published
source archives are preserved; documenting the discrepancies does not change
their contents or make all historical versions agree.

Before publishing a future release, verify that the intended three-component
tag version, without the `v` prefix, equals the root manifest version and the
CMake project version at the tagged commit. Check the source-local port
separately and make any required source corrections before creating the tag.

[English README](../README.md) | [한국어 README](../README.kr.md)
