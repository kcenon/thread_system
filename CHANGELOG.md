# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2026-04-15

### Added

- vcpkg CMake preset for streamlined builds ([#632](https://github.com/kcenon/thread_system/issues/632))
- Tutorials, FAQ, and troubleshooting guide ([#661](https://github.com/kcenon/thread_system/issues/661))
- Getting Started guide with categorized examples ([#655](https://github.com/kcenon/thread_system/issues/655))
- Architecture Decision Records (ADR) ([#648](https://github.com/kcenon/thread_system/issues/648))
- SSOT documentation registry ([#646](https://github.com/kcenon/thread_system/issues/646))
- Issue/PR templates, CONTRIBUTING, CODE_OF_CONDUCT, SECURITY ([#628](https://github.com/kcenon/thread_system/issues/628))
- CHANGELOG.md and CONTRIBUTING.md ([#596](https://github.com/kcenon/thread_system/issues/596))
- Documentation audit CI workflow for PRs ([#650](https://github.com/kcenon/thread_system/issues/650))
- Automated vcpkg registry sync on release ([#629](https://github.com/kcenon/thread_system/issues/629))

### Changed

- **BREAKING**: Rename CMake project from `ThreadSystem` to `thread_system` ([#642](https://github.com/kcenon/thread_system/issues/642))
- Replace `throw` statements with `Result<T>` in all public API headers ([#675](https://github.com/kcenon/thread_system/issues/675))
- Add deprecation markers for v1.0 API freeze ([#676](https://github.com/kcenon/thread_system/issues/676))
- Remove deprecated job classes, resilience headers, and policy methods ([#611](https://github.com/kcenon/thread_system/issues/611))
- Standardize license headers to short BSD-3-Clause format ([#641](https://github.com/kcenon/thread_system/issues/641))
- Standardize CMake export naming to `thread_system-targets` ([#588](https://github.com/kcenon/thread_system/issues/588))
- Standardize minimum CMake version to 3.20 ([#621](https://github.com/kcenon/thread_system/issues/621))
- Migrate vcpkg version field to `version-semver` ([#624](https://github.com/kcenon/thread_system/issues/624))

### Fixed

- Thread-safe diagnostics initialization with `std::call_once` ([#669](https://github.com/kcenon/thread_system/issues/669))
- vcpkg usage target name mismatch and stale fmt dependency ([#664](https://github.com/kcenon/thread_system/issues/664))
- `hazard_pointer` memory ordering issues for lock-free correctness
- pkg-config template for unified library ([#634](https://github.com/kcenon/thread_system/issues/634))
- Missing port-version in vcpkg overlay manifest ([#636](https://github.com/kcenon/thread_system/issues/636))

### Performance

- Replace mutex with atomic counter for `job_queue` read-only query APIs ([#609](https://github.com/kcenon/thread_system/issues/609))
- Add exponential backoff to `lockfree_job_queue` CAS retry loops ([#610](https://github.com/kcenon/thread_system/issues/610))
- Lock-free freelist pool for queue node allocation
- Cap affinity matrix growth and add diagnostics sampling
- Replace 10ms polling sleep with `condition_variable` blocking in thread pool

### Documentation

- Modernize Doxygen with doxygen-awesome-css theme ([#654](https://github.com/kcenon/thread_system/issues/654))
- Split oversized documents into focused sub-documents ([#666](https://github.com/kcenon/thread_system/issues/666))
- Add ecosystem navigation and dependency map ([#657](https://github.com/kcenon/thread_system/issues/657))
- Add `@file` Doxygen blocks to public API headers ([#647](https://github.com/kcenon/thread_system/issues/647))
- Add YAML frontmatter to all docs/ markdown files ([#644](https://github.com/kcenon/thread_system/issues/644))
- Standardize README badge set and Table of Contents ([#593](https://github.com/kcenon/thread_system/issues/593), [#595](https://github.com/kcenon/thread_system/issues/595))

### Infrastructure

- Upgrade dependencies: spdlog 1.15.3, gtest 1.17.0, benchmark 1.9.5
- Adopt setup-vcpkg composite action for CI standardization ([#622](https://github.com/kcenon/thread_system/issues/622))
- Bump codecov/codecov-action to v6 ([#637](https://github.com/kcenon/thread_system/issues/637))

## [0.3.1] - 2026-03-14

### Fixed
- CMake: lower MINIMUM_SUPPORTED_VERSION and add missing vcpkg dependency (#577)
- CI: pin osv-scanner-action to v2.3.3 and grant contents:write for docs (#573)

### Documentation
- Replace GIT_TAG main with v0.3.0 in FetchContent examples (#578)
- Improve Doxyfile configuration and fix comment gaps (#575)

## [0.3.0] - 2026-03-11

### Added
- Queue consolidation: 10 implementations reduced to 2 public types (adaptive_job_queue, job_queue)
- Dependabot and OSV-Scanner weekly vulnerability monitoring (#561)
- IEC 62304 SOUP register (#556)
- SBOM generation and CVE scanning workflows

### Changed
- Replaced libiconv (LGPL-2.1) with simdutf (MIT/Apache-2.0) for license compliance (#560)
- Synchronized vcpkg baseline with ecosystem standard (#555)
- Graduated sanitizer jobs from Phase 0 to Phase 1 (#554)

### Fixed
- Updated stale examples and benchmarks to current kcenon::thread API (#553)
- Build: link kcenon::common_system target for vcpkg find_package path (#571)

### Documentation
- SOUP list for IEC 62304 compliance (#549)
- Ecosystem-aware CVE scanning with license check (#552)

### Infrastructure
- Reusable Doxygen workflow from common_system (#544)
- Cross-platform CI with GCC, Clang, MSVC
- ThreadSanitizer, AddressSanitizer, UBSan integration
