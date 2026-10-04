# Versioning Policy

This document is the single source of truth (SSOT) for how `thread_system` is
versioned and released.

## Overview

`thread_system` follows [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0/).

## Version Format

Given a version number `MAJOR.MINOR.PATCH`, increment the:

- **MAJOR** version for incompatible (breaking) public API changes
- **MINOR** version for backward-compatible new functionality
- **PATCH** version for backward-compatible bug fixes

## Where the Version Lives

The canonical version is declared in the top-level
[`CMakeLists.txt`](CMakeLists.txt) via the CMake `project(... VERSION X.Y.Z)`
command. This value is exposed at configure time as `PROJECT_VERSION` and is the
authoritative version for the build, packaging, and install steps.

The same version must be reflected in:

- [`CHANGELOG.md`](CHANGELOG.md) — a dated section per released version
- The git release tag (e.g., `v1.0.0`)
- The vcpkg manifest (`version-semver` field)

When bumping the version, update the `project(... VERSION ...)` line first; all
other locations follow from it.

## Release Process

1. Update the `VERSION` in `CMakeLists.txt` (`project(thread_system VERSION X.Y.Z ...)`).
2. Move the relevant entries in [`CHANGELOG.md`](CHANGELOG.md) from
   `[Unreleased]` into a new dated `[X.Y.Z]` section.
3. Update the vcpkg manifest `version-semver` field to match.
4. Create and push a git tag matching the version (`vX.Y.Z`) to trigger the
   release workflow.

## Pre-Release Versions

Pre-release builds use a SemVer suffix:

- `X.Y.Z-alpha.N` — alpha release
- `X.Y.Z-beta.N` — beta release
- `X.Y.Z-rc.N` — release candidate

## Compatibility Guarantees

- Within the same MAJOR version, the public API stays backward-compatible.
- Deprecated APIs are marked with `[[deprecated]]` and documented in
  [`CHANGELOG.md`](CHANGELOG.md) before removal in a subsequent MAJOR release.
- Internal implementation details (anything under a `detail::` namespace) are
  not part of the public API and may change in any release.

## Related Documents

- [`CMakeLists.txt`](CMakeLists.txt) — authoritative `PROJECT_VERSION` (SSOT)
- [`CHANGELOG.md`](CHANGELOG.md) — record of all notable changes, following
  [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
