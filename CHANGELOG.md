# Changelog

All notable changes to the Thread System project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased] - 2025-06-14

### Added
- Enhanced README.md with comprehensive project overview and benefits
- Detailed architecture documentation with component relationships
- Performance tuning guide with benchmarking examples
- Migration guide for existing threading code
- Troubleshooting guide for concurrency issues
- FAQ documentation covering common use cases
- Lock-free MPMC queue implementation with significant performance improvements
- Lock-free implementation for typed_job_queue with per-type MPMC queues

### Changed
- **BREAKING**: Renamed `priority_thread_pool` to `typed_thread_pool` to better reflect the job type-based scheduling paradigm
- **BREAKING**: Changed `job_priorities` enum to `job_types` with values: RealTime, Batch, Background
- All `priority_*` classes renamed to `typed_*` for consistency
- Updated documentation to reflect the type-based approach instead of priority-based
- Improved error handling with result_void pattern
- Enhanced typed queue implementation with better performance characteristics
- Updated samples with more realistic use cases
- **MPMC Queue**: Completely removed thread-local storage for improved stability
- **MPMC Queue**: Added retry limits (MAX_TOTAL_RETRIES = 1000) to prevent infinite loops
- **MPMC Queue**: All stress tests now enabled and passing reliably
- **Type Thread Pool**: Migrated from mutex-based to lock-free MPMC queues for each job type

### Fixed
- Thread safety improvements in typed job queue
- Memory leak fixes in worker thread destruction
- Platform-specific compilation issues
- MPMC queue infinite loop issues under high contention
- Thread-local storage cleanup segmentation faults
- All disabled tests now enabled and passing

## [1.0.0] - 2024-01-15

### Added
- Initial release of Thread System framework
- Core thread_base class with C++20 std::jthread support
- Priority-based thread pool system
- Asynchronous logging framework with multiple output targets
- Comprehensive sample applications
- Cross-platform build system with vcpkg integration
- Extensive unit test suite with Google Test

### Core Components
- **Thread Base Module**: Foundation for all threading operations
- **Logging System**: High-performance asynchronous logging
- **Thread Pool System**: Efficient worker thread management
- **Priority Thread Pool System**: Advanced priority-based scheduling (renamed to Typed Thread Pool in later versions)

### Supported Platforms
- Windows (MSVC 2019+, MinGW, MSYS2)
- Linux (GCC 9+, Clang 10+)
- macOS (Clang 10+)

### Performance Characteristics
- Thread creation overhead: ~24.5 microseconds (measured on Apple M1)
- Job scheduling latency: ~77 nanoseconds (10x improvement)
- Lock-free queue operations: 431% faster than mutex-based
- Memory efficiency: <1MB baseline usage
- Throughput: 1.16M+ jobs/second with basic pool, 1.24M+ with typed pool

---

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of conduct and the process for submitting pull requests.

## License

This project is licensed under the BSD 3-Clause License - see the [LICENSE](LICENSE) file for details.