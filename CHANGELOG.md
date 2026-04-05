# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Documentation

- Modernize Doxygen documentation with doxygen-awesome-css theme and refactored mainpage ([#652](https://github.com/kcenon/thread_system/issues/652))

### Performance

- Replace mutex with atomic counter for `job_queue` read-only query APIs ([#607](https://github.com/kcenon/thread_system/issues/607))
- Add exponential backoff to `lockfree_job_queue` CAS retry loops to reduce CPU spin waste ([#608](https://github.com/kcenon/thread_system/issues/608))

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
