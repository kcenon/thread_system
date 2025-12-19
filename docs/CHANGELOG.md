# thread_system Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed
- **Issue #303 - Phase 2a**: Replace internal result<T> with common::Result<T> in core headers
  - Updated all core library headers and implementation files to use `kcenon::common::Result<T>` internally
  - Replaced `result_void` return types with `kcenon::common::VoidResult`
  - Updated error creation to use `common::error_info{code, message, module}` pattern
  - All unit tests, integration tests, and examples updated to use new API
  - API migration pattern: `.has_error()` → `.is_err()`, `.get_error()` → `.error()`
  - Files updated include: thread_pool, thread_worker, job_queue, callback_job, and all queue implementations
  - 14 example files updated to demonstrate new API usage

### Added
- **Issue #289**: Unify Result type API across common_system and thread_system
  - Added `has_value()`, `is_ok()`, `is_error()` methods to `result_void` class
  - Added `is_ok()` and `is_error()` methods to `result<T>` and `result<void>` classes
  - Updated both `include/kcenon/thread/core/error_handling.h` and `core/sync/include/error_handling.h`
  - API now consistent across `result_void`, `result<T>`, `result<void>`, and `common::Result<T>`
  - Added unit tests for new compatibility methods
  - Updated ERROR_SYSTEM_MIGRATION_GUIDE.md with new API mapping
  - 100% backward compatible - all existing code continues to work
- **Issue #271**: Apply updated common_system with C++20 Concepts
  - New `include/kcenon/thread/concepts/thread_concepts.h` header unifying all C++20 Concepts
  - Concepts for callable validation: `Callable`, `VoidCallable`, `ReturningCallable`, `CallableWith`
  - Concepts for job type constraints: `JobType`, `JobCallable`, `PoolJob`
  - Type detection concepts: `Duration`, `FutureLike`
  - Type traits: `is_duration_v`, `is_future_like_v`, `callable_return_type_t`, `is_valid_job_type_v`
  - Full C++17 fallback support using `constexpr bool` when concepts unavailable
  - Re-exported to `detail` namespace for backward compatibility
- **Issue #276**: Add CMake configuration for C++20 Concepts feature detection
  - New `check_common_concepts_support()` function in `ThreadSystemFeatures.cmake`
  - Detects `common_system` C++20 concepts header availability
  - Verifies compiler version requirements (GCC 10+, Clang 10+, Apple Clang 12+, MSVC 19.23+)
  - Defines `THREAD_HAS_COMMON_CONCEPTS` macro when concepts are available
  - Displays available concept categories during CMake configuration
  - Part of parent issue #271 (Apply updated common_system with C++20 Concepts)

### Changed
- **Issue #271**: Refactor pool_traits.h and type_traits.h
  - Removed duplicated concept definitions from `pool_traits.h` (#273)
  - Removed duplicated concept definitions from `type_traits.h` (#274)
  - Both files now import from centralized `thread_concepts.h`
  - Use `requires` clauses for C++20 concepts where applicable
  - ~30% reduction in code duplication
  - Improved template error messages with clearer concept constraints
- **Issue #275**: Refactor atomic_wait.h to use C++20 concepts
  - Replace `std::enable_if<std::is_integral<U>::value>` SFINAE patterns with `requires std::integral<T>` clauses
  - Add `<concepts>` header include when `USE_STD_CONCEPTS` is defined
  - Maintain C++17 fallback using original SFINAE pattern within `#else` block
  - Cleaner template declarations with improved compile-time error messages

### Deprecated
- **Issue #299**: Add [[deprecated]] attribute to result types (Phase 2 of Result<T> unification)
  - `thread::result<T>` marked deprecated - use `common::Result<T>` instead
  - `thread::result_void` marked deprecated - use `common::VoidResult` instead
  - `thread::result<void>` marked deprecated - use `common::VoidResult` instead
  - Deprecation only active when `THREAD_HAS_COMMON_RESULT` is defined
  - Internal compatibility layer functions have warnings suppressed
  - See docs/advanced/ERROR_SYSTEM_MIGRATION_GUIDE.md for migration instructions
  - Created sub-issues #300, #301 for remaining migration work
- **Issue #263**: Mark thread-local logger_interface as deprecated
  - Added `[[deprecated]]` attribute to `log_level` enum in `logger_interface.h`
  - `logger_interface` and `logger_registry` classes already had deprecation attributes
  - Compiler warnings now generated when using deprecated types
  - Created comprehensive migration guide: `docs/guides/LOGGER_INTERFACE_MIGRATION_GUIDE.md`
  - Migration path: Use `common::interfaces::ILogger` from common_system instead
  - **Timeline**: Deprecated in v1.x, to be removed in v2.0

### Fixed
- **Issue #316**: Replace unsafe hazard_pointer with safe_hazard_pointer in lockfree_job_queue
  - TICKET-002 follow-up: Fixed memory ordering issues on weak memory model architectures (ARM64)
  - Removed `HAZARD_POINTER_FORCE_ENABLE` usage from production code
  - Migrated to `safe_hazard_pointer.h` with explicit memory ordering guarantees
  - Uses `safe_hazard_guard` for RAII-style hazard pointer management
  - Uses `safe_retire_hazard<T>()` for safe memory reclamation
  - Added weak memory model verification tests for ARM64/Apple Silicon
  - Resolves CVSS 8.5 security issue on weak memory model architectures
- **Issue #297**: Improve atexit handler registration timing for SDOF prevention
  - Added `thread_logger_init.cpp` for early atexit handler registration
  - Uses platform-specific initialization (GCC/Clang `__attribute__((constructor(101)))`, MSVC CRT section)
  - Ensures `is_shutting_down()` returns true during static destruction phase
  - Added comprehensive SDOF prevention tests
  - Cross-platform support: Linux, macOS, Windows
  - Related to #295, #296 (initial SDOF prevention), network_system#301
- **Issue #295**: Prevent SDOF in thread_pool destructor and stop() method
  - Added `stop_unsafe()` private method for logging-free shutdown during static destruction
  - Modified destructor to check `thread_logger::is_shutting_down()` before calling `stop()`
  - Added shutdown checks to all `thread_context::log()` method overloads
  - Prevents `free(): invalid pointer` error when thread_pool is destroyed during static destruction
  - Related to #293 (thread_logger Intentional Leak pattern)
- **Issue #293**: Prevent static destruction order issues in thread_logger
  - Changed `instance()` to use intentional leak pattern (allocate with `new`, never delete)
  - Added `is_shutting_down_` atomic flag to skip logging during process termination
  - Added `prepare_shutdown()` method for explicit shutdown signaling
  - Fixes `free(): invalid pointer` error in network_system CI tests on Ubuntu
  - No API breaking changes - purely internal implementation fix

### Added
- **Issue #262**: Add LoggerSystemAdapter for runtime binding
  - New `logger_system_adapter` class bridging logger_system to common_system's ILogger interface
  - Implements all ILogger methods with proper log level conversion
  - Supports C++20 source_location and legacy file/line/function parameters
  - Uses typed_adapter base for type safety and wrapper depth tracking (max depth: 2)
  - Thread-safe operation verified with concurrent writer tests
  - Conditional compilation with BUILD_WITH_COMMON_SYSTEM and BUILD_WITH_LOGGER_SYSTEM flags
  - Comprehensive unit test suite covering all methods and error scenarios
- **Issue #251**: Add error handling and edge case tests for adaptive_job_queue
  - 22 comprehensive test cases covering error handling and edge cases
  - Mode switch error handling tests (5 tests): non-manual policy rejection, concurrent mode switching
  - Accuracy guard edge case tests (3 tests): deep nesting, concurrent release, manual policy
  - Data integrity stress tests (2 tests): mode switching during operations, multi-producer/consumer scenarios
  - Empty queue operation tests (4 tests): empty dequeue, mode switch, size/empty checks, clear
  - Null/invalid job handling tests (2 tests): rejection in both mutex and lock-free modes
  - Statistics accuracy tests (2 tests): mode switch counting, time tracking
  - Stop/shutdown edge case tests (4 tests): operations after stop, mode switch after stop
- **Issue #246**: Re-enable adaptive_queue_sample example
  - Removed logger dependency from adaptive_queue_sample.cpp
  - Replaced write_information/write_error with std::cout/std::cerr
  - Updated to use new kcenon/thread header structure and policy-based API
  - Demonstrates 5 comprehensive examples: policy comparison, adaptive behavior, different policies, performance monitoring, and web server simulation

### Changed
- **Issue #227**: Move typed_pool implementation headers to public include path
  - Relocated 14 header files from `src/impl/typed_pool/` to `include/kcenon/thread/impl/typed_pool/`
  - Updated forwarding headers to use stable `<kcenon/thread/impl/typed_pool/...>` includes
  - Eliminated brittle `../../../../src/impl/` relative path dependencies
  - Headers are now properly installed alongside other public headers
  - Improved IDE/tooling compatibility and static analyzer support

### Added
- **Issue #234**: Phase 5 - Queue Factory & Integration
  - New `queue_factory` utility class for convenient queue creation
  - Convenience factory methods: `create_standard_queue()`, `create_lockfree_queue()`, `create_adaptive_queue()`
  - Requirements-based queue creation: `create_for_requirements()` with `requirements` struct
  - Environment-optimized queue selection: `create_optimal()` (considers CPU architecture and core count)
  - Compile-time type selection templates: `queue_type_selector<>` and `queue_t<>`
  - Pre-defined type aliases: `accurate_queue_t`, `fast_queue_t`, `balanced_queue_t`
  - Comprehensive unit tests (21 test cases) including backward compatibility verification
  - 100% backward compatible - purely additive utility, existing code unchanged
- **Issue #233**: Phase 4 - Adaptive Queue Implementation
  - New `adaptive_job_queue` class wrapping both mutex-based and lock-free queues
  - Support for multiple selection policies: `accuracy_first`, `performance_first`, `balanced`, `manual`
  - RAII `accuracy_guard` for temporary accuracy mode switching
  - Thread-safe mode switching with automatic data migration between queues
  - Statistics tracking for mode switches and time spent in each mode
  - Full `scheduler_interface` and `queue_capabilities_interface` support
  - Capabilities reflect current mode (exact_size in mutex, lock_free in performance mode)
  - 100% backward compatible - new class, existing queues unchanged
  - Comprehensive unit tests (22 tests) including concurrent access tests
- **Issue #232**: Phase 3 - Lock-free Queue Improvements
  - Extended `lockfree_job_queue` to implement `scheduler_interface`
  - Extended `lockfree_job_queue` to implement `queue_capabilities_interface`
  - Added `schedule()` and `get_next_job()` methods (delegate to enqueue/dequeue)
  - Added `get_capabilities()` returning lock-free queue characteristics
  - Fixed destructor race condition using hazard pointer for safe reclamation
  - Increased max hazard pointers per thread from 4 to 8
  - 100% backward compatible - all existing tests pass unchanged
  - Comprehensive unit tests for new interfaces
- **Issue #231**: Phase 2 - Mutex-based Queue Implementation
  - Extended `job_queue` to inherit from `queue_capabilities_interface`
  - Implemented `get_capabilities()` override returning mutex-based capabilities
  - Added convenience methods: `has_exact_size()`, `is_lock_free()`, etc.
  - 100% backward compatible - all existing tests pass unchanged
  - Unit tests for job_queue capability queries
- **Issue #230**: Phase 1 - Queue Capabilities Infrastructure
  - `queue_capabilities` struct for runtime capability description
  - `queue_capabilities_interface` mixin for capability introspection
  - Non-breaking additive interface for gradual adoption
  - Unit tests for all capability queries
- Documentation standardization compliance
- README.md, ARCHITECTURE.md, CHANGELOG.md
- **ARM64 compatibility tests**: Comprehensive tests for macOS Apple Silicon (#223)
  - Manual worker batch enqueue validation
  - Concurrent job submission with multiple workers
  - Static assertions for memory alignment verification
  - Individual vs batch worker enqueue comparison

### Fixed
- **Issue #291**: pthread.h not found error on Windows MSVC builds
  - Added `gtest_disable_pthreads ON` option to `find_or_fetch_gtest()` function
  - Fixes build failure when using thread_system as a subdirectory on Windows MSVC
  - Windows MSVC does not provide pthread.h by default, so GTest's pthread support must be disabled
- **Issue #225**: EXC_BAD_ACCESS on macOS ARM64 with batch worker enqueue (follow-up to #223)
  - Root cause: Data race between `on_stop_requested()` and job destruction in `do_work()`
  - The race occurred when `on_stop_requested()` accessed a job's virtual method while
    `do_work()` was simultaneously destroying the job object
  - Solution: Added mutex synchronization to protect job access during destruction
  - `on_stop_requested()` now acquires `queue_mutex_` before accessing current job
  - `do_work()` now destroys job while holding `queue_mutex_`
  - Verified with ThreadSanitizer and AddressSanitizer (all 28 tests pass)

---

## [2.0.0] - 2025-11-15

### Added
- **typed_thread_pool**: Type-safe thread pool implementation
  - Compile-time type safety
  - Custom process functions
  - Auto type deduction
- **adaptive_queue**: Dynamic resizing queue
  - Automatic load-based scaling
  - Configurable thresholds
  - Memory-efficient
- **hazard_pointer**: Safe memory reclamation for lock-free structures
  - ABA problem mitigation
  - Automatic garbage collection
- **Service Infrastructure**: Service lifecycle management
  - service_registry for dependency injection
  - service_base abstract class
  - Automatic cleanup

### Changed
- **thread_pool**: Major performance improvements
  - Work-stealing algorithm implementation
  - 4.5x throughput improvement (1.2M ops/sec)
  - Reduced latency to 0.8 μs
  - Near-linear scaling up to 16 cores
- **mpmc_queue**: Lock-free optimization
  - 5.2x performance improvement (2.1M ops/sec)
  - Better cache locality
  - Reduced false sharing
- **thread_base**: Enhanced lifecycle management
  - C++20 jthread support
  - Improved error handling
  - Better monitoring capabilities

### Fixed
- **Issue #45**: Race condition in thread_pool shutdown
  - Added proper synchronization
  - Ensured all tasks complete before shutdown
- **Issue #38**: Memory leak in mpmc_queue
  - Implemented hazard pointer
  - Fixed node cleanup logic
- **Issue #29**: Deadlock in service_registry
  - Removed circular dependencies
  - Added deadlock detection

### Performance
- **thread_pool**: 4.5x improvement
  - Before: 267K ops/sec
  - After: 1.2M ops/sec
  - Latency: 3.6 μs → 0.8 μs
- **mpmc_queue**: 5.2x improvement
  - Before: 404K ops/sec
  - After: 2.1M ops/sec
  - Latency: 2.5 μs → 0.5 μs
- **typed_thread_pool**: 3.8x improvement over basic implementation
  - 980K ops/sec
  - Type safety with zero runtime cost

---

## [1.5.0] - 2025-10-22

### Added
- **spsc_queue**: Single-producer single-consumer queue
  - Lock-free circular buffer
  - 3.5M ops/sec throughput
- **Read-Write Lock**: Optimized for read-heavy workloads
  - Writer starvation prevention
  - Configurable fairness

### Changed
- **thread_pool**: Priority-based task execution
  - 3-level priority system
  - Fair scheduling algorithm
- **thread_base**: Enhanced thread naming
  - Automatic ID generation
  - Custom name support

### Fixed
- **Issue #22**: Spurious wakeups in condition variables
- **Issue #18**: Exception safety in task execution

---

## [1.0.0] - 2025-09-15

### Added
- Initial release of thread_system
- **thread_base**: Foundation thread abstraction
  - Start/stop lifecycle
  - Condition monitoring
  - State management
- **thread_pool**: Basic thread pool implementation
  - Fixed-size worker pool
  - Task queue
  - Future/Promise pattern
- **mpmc_queue**: Basic MPMC queue
  - Mutex-based implementation
  - Thread-safe operations
- **Synchronization primitives**:
  - spinlock
  - Basic locking mechanisms

### Performance
- thread_pool: 267K ops/sec
- mpmc_queue: 404K ops/sec
- Basic functionality verified

---

## 버전 규칙

### Major Version (X.0.0)
- API 호환성이 깨지는 변경
- 아키텍처 대규모 변경
- 필수 의존성 주요 업데이트

### Minor Version (0.X.0)
- 새로운 기능 추가 (하위 호환성 유지)
- 성능 개선
- 내부 리팩토링

### Patch Version (0.0.X)
- 버그 수정
- 문서 업데이트
- 마이너한 개선

---

## 참조

- [프로젝트 이슈](https://github.com/kcenon/thread_system/issues)
- [마일스톤](https://github.com/kcenon/thread_system/milestones)

---

[Unreleased]: https://github.com/kcenon/thread_system/compare/v2.0.0...HEAD
[2.0.0]: https://github.com/kcenon/thread_system/compare/v1.5.0...v2.0.0
[1.5.0]: https://github.com/kcenon/thread_system/compare/v1.0.0...v1.5.0
[1.0.0]: https://github.com/kcenon/thread_system/releases/tag/v1.0.0
