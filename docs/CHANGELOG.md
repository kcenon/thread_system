# thread_system Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **Issue #387**: Phase 1.3.2 - Thread Dump Functionality Enhancement
  - Enhanced `dump_thread_states()` to return actual worker information:
    - Real thread IDs via `thread_base::get_thread_id()`
    - Unique worker IDs from `thread_worker::get_worker_id()`
    - Current job information for active workers
  - Worker statistics tracking in `thread_worker`:
    - `get_jobs_completed()`: Count of successfully completed jobs
    - `get_jobs_failed()`: Count of failed jobs
    - `get_total_busy_time()`: Accumulated time executing jobs
    - `get_total_idle_time()`: Accumulated time waiting for jobs
    - `get_state_since()`: Timestamp of last state transition
    - `get_current_job_info()`: Information about currently executing job
  - Added `collect_worker_diagnostics()` to `thread_pool` for thread-safe worker info collection
  - Accurate utilization calculation based on actual busy/idle time
  - Thread-safe state collection with proper synchronization
- **Issue #376**: Phase 1.3 - Diagnostics API with Thread Dump and Health Checks
  - New `thread_pool_diagnostics` class providing comprehensive observability features
  - Thread dump functionality:
    - `dump_thread_states()`: Get detailed info for all worker threads
    - `format_thread_dump()`: Human-readable thread dump output (Java-style)
    - `thread_info` struct with worker state, utilization, current job info
    - `worker_state` enum: `idle`, `active`, `stopping`, `stopped`
  - Job inspection:
    - `get_active_jobs()`: List currently executing jobs
    - `get_pending_jobs()`: List jobs waiting in queue
    - `get_recent_jobs()`: Circular buffer of recently completed jobs
    - `job_info` struct with timing, status, and optional stack trace
    - `job_status` enum: `pending`, `running`, `completed`, `failed`, `cancelled`, `timed_out`
  - Bottleneck detection:
    - `detect_bottlenecks()`: Analyze pool for performance issues
    - `bottleneck_report` struct with saturation levels and recommendations
    - `bottleneck_type` enum: `none`, `queue_full`, `slow_consumer`, `worker_starvation`, etc.
    - Automatic severity calculation (0-3 scale)
  - Health check (HTTP integration ready):
    - `health_check()`: Get pool operational status
    - `is_healthy()`: Quick boolean check
    - `health_status` struct with component-level health
    - `health_state` enum: `healthy`, `degraded`, `unhealthy`, `unknown`
    - `is_operational()` helper for liveness probes
  - Event tracing:
    - `enable_tracing()`: Enable/disable event history
    - `add_listener()` / `remove_listener()`: Observer pattern support
    - `execution_event_listener` interface for custom event handling
    - `job_execution_event` struct with event type, timestamps, worker info
    - `event_type` enum: `enqueued`, `dequeued`, `started`, `completed`, `failed`, etc.
  - Export/serialization:
    - `to_json()`: Full diagnostics state in JSON format
    - `to_prometheus()`: Prometheus/OpenMetrics format export
    - Configurable via `diagnostics_config` struct
  - Integration:
    - Lazy initialization via `thread_pool::diagnostics()` method
    - Zero overhead when diagnostics not accessed
    - Forward declarations in `forward.h`
    - Umbrella header `<kcenon/thread/diagnostics.h>`
  - Comprehensive integration tests (18 test cases)
- **Issue #375**: Phase 1.2 - Backpressure Mechanisms with Rate Limiting and Adaptive Control
  - New `token_bucket` class for lock-free rate limiting
    - Continuous refill algorithm with fixed-point arithmetic for sub-token precision
    - Lock-free implementation using atomic CAS operations
    - Methods: `try_acquire()`, `try_acquire_for()`, `available_tokens()`, `set_rate()`
  - New `backpressure_config.h` header with configuration and types
    - `backpressure_policy` enum: `block`, `drop_oldest`, `drop_newest`, `callback`, `adaptive`
    - `backpressure_decision` enum for callback-based decisions
    - `pressure_level` enum: `none`, `low`, `high`, `critical`
    - `backpressure_config` struct with watermarks, rate limiting settings, callbacks
    - `backpressure_stats` for tracking accepted/rejected/dropped jobs and rate limit waits
  - New `backpressure_job_queue` class extending `job_queue`
    - Supports all 5 backpressure policies with policy-specific handlers
    - Pressure level detection based on configurable watermarks (low/high)
    - Optional rate limiting integration via token bucket
    - Pressure callback invocation when crossing thresholds
    - Statistics tracking with `get_backpressure_stats()` and `reset_stats()`
  - Extended `thread_pool` with custom queue constructor
    - New constructor: `thread_pool(name, custom_queue, context)` for queue injection
    - Enables using `backpressure_job_queue` with thread pools
  - Comprehensive integration tests (11 test cases)
    - Token bucket basic acquisition and timeout tests
    - Pressure level detection tests
    - Drop newest/oldest policy tests
    - Rate limiting integration tests
    - Statistics tracking tests
    - Thread pool with backpressure queue integration tests
- **Issue #374**: Enhanced Metrics System with Histogram and Percentile Support
  - `LatencyHistogram`: HDR-style histogram with logarithmic buckets for latency distribution
    - Provides accurate percentile calculations (P50/P90/P99/P99.9)
    - Lock-free atomic operations with < 100ns overhead per record
    - Memory efficient: < 1KB per histogram
  - `SlidingWindowCounter`: Time-based counter for throughput measurement
    - Configurable window sizes (1s, 60s, etc.)
    - Lock-free circular buffer implementation
  - `EnhancedThreadPoolMetrics`: Comprehensive metrics aggregating:
    - Enqueue latency histogram
    - Execution latency histogram
    - Wait time (queue time) histogram
    - Throughput counters (1s and 1min windows)
    - Per-worker utilization tracking
    - Queue depth monitoring
  - Thread pool integration:
    - `set_enhanced_metrics_enabled(bool)`: Enable/disable enhanced metrics
    - `is_enhanced_metrics_enabled()`: Check if enhanced metrics is enabled
    - `enhanced_metrics()`: Access enhanced metrics (throws if not enabled)
    - `enhanced_metrics_snapshot()`: Get snapshot of all metrics
  - Export formats:
    - `to_json()`: JSON serialization of metrics
    - `to_prometheus(prefix)`: Prometheus/OpenMetrics format export

### Changed
- **Issue #359**: Fix misleading lockfree_queue naming (Kent Beck "Reveals Intention" principle)
  - Create `concurrent/` directory for fine-grained locking queue implementations
  - Move `concurrent_queue<T>` from `lockfree/` to `concurrent/concurrent_queue.h`
  - Convert `lockfree/lockfree_queue.h` to backward compatibility header
  - Improve deprecation messages to include "MISLEADING NAME" warning
  - Update documentation with new header paths and namespace references

### Added
- **Issue #358 / #362**: Queue consolidation - Phase 3
  - Add template `enqueue<T>()` method to `job_queue` for type-safe job submission
  - Add template `enqueue<T>()` method to `adaptive_job_queue` for type-safe job submission
  - Enables submitting job subclasses without explicit casting

### Documentation
- **Issue #358 / #363**: Queue consolidation - Phase 4 (Documentation)
  - Update `queue_factory.h` documentation to recommend `adaptive_job_queue` as default choice
  - Update README.md with simplified queue API (8 → 2 public types)
  - Update QUEUE_SELECTION_GUIDE.md with new decision tree and recommendations
  - Update queue_factory_sample.cpp to use `adaptive_job_queue` instead of deprecated `lockfree_job_queue`
  - Add Kent Beck Simple Design principle references throughout documentation

### Changed
- **Issue #358**: Queue consolidation - Phase 1 & 2
  - Move `concurrent_queue<T>` and `lockfree_job_queue` to `detail::` namespace (internal implementation)
  - `queue_factory::create_lockfree_queue()` now returns `adaptive_job_queue` with `performance_first` policy
  - `queue_type_selector<false, true>` now returns `adaptive_job_queue` instead of `lockfree_job_queue`
  - Add optional `max_size` parameter to `job_queue` constructor for bounded queue functionality
  - Add `is_bounded()`, `get_max_size()`, `set_max_size()`, `is_full()` methods to `job_queue`

### Deprecated
- **Issue #358**: Queue consolidation deprecations
  - `lockfree_job_queue` - Use `adaptive_job_queue` with `policy::performance_first` instead
  - `concurrent_queue<T>` - Use `adaptive_job_queue` or `job_queue` instead
  - `bounded_job_queue` - Use `job_queue` with `max_size` parameter instead
  - `queue_factory::create_lockfree_queue()` - Use `create_adaptive_queue(policy::performance_first)` instead
- **Issue #358 / #362**: Typed queue deprecations
  - `typed_job_queue_t<T>` - Use `job_queue` or `adaptive_job_queue` with template `enqueue<T>()` instead
  - `typed_lockfree_job_queue_t<T>` - Use `adaptive_job_queue` with `policy::performance_first` instead
  - `adaptive_typed_job_queue_t<T>` - Use `adaptive_job_queue` with template `enqueue<T>()` instead

### Changed
- **Issue #340**: Rename `lockfree_queue<T>` to `concurrent_queue<T>`
  - The class name was misleading as it uses fine-grained locking, not lock-free algorithms
  - Old name `lockfree_queue<T>` is now a deprecated alias for backward compatibility
  - Update any existing code to use `concurrent_queue<T>` to avoid deprecation warnings

- **Issue #338**: Migrate error_code enum to negative range for central registry compliance
  - Moved all error_code values from positive to negative range (-100 to -199)
  - Error code ranges are now organized as:
    - General errors: -100 to -109
    - Thread errors: -110 to -119
    - Queue errors: -120 to -129
    - Job errors: -130 to -139
    - Resource errors: -140 to -149
    - Synchronization errors: -150 to -159
    - IO errors: -160 to -169
  - Added `queue_busy` error code to sync/error_handling.h for consistency
  - Added compile-time range validation via static_assert
  - **BREAKING CHANGE**: Any code checking specific error_code integer values will need to be updated

### Deprecated
- **Issue #336**: Deprecate BUILD_WITH_LOGGER_SYSTEM and logger_system_adapter
  - Mark `BUILD_WITH_LOGGER_SYSTEM` CMake option as deprecated (will be removed in v0.5.0.0)
  - Add `[[deprecated]]` attribute to `logger_system_adapter` class
  - Add CMake deprecation warning when BUILD_WITH_LOGGER_SYSTEM is enabled
  - This resolves bidirectional dependency risk between thread_system and logger_system

### Added
- **Issue #336**: Add ILogger DI registration functions
  - `register_logger_instance()`: Register existing ILogger with ServiceContainer
  - `register_logger_factory()`: Register logger factory function (singleton/transient)
  - `is_logger_registered()`: Check if ILogger is registered
  - `unregister_logger()`: Remove ILogger from container
  - Integration tests for ILogger DI registration
  - This provides the preferred way to integrate logging without direct logger_system dependency

### Fixed
- **Issue #358**: Fix queue_factory_integration_test for deprecated lockfree_job_queue
  - Update `RequirementsSatisfaction_LockFreeUnderLoad` test to use `adaptive_job_queue`
  - Remove dead code in `OptimalSelection_FunctionalUnderLoad` that cast to `lockfree_job_queue`
  - Add deprecation warning suppression for backward compatibility test

- **Issue #333**: Remove deprecated 5-parameter log() method from example logger implementations
  - Updated `composition_example.cpp` console_logger to use `log(const log_entry&)` directly
  - Updated `mock_logger.h` to use `log(const log_entry&)` directly
  - Fixes build failure due to common_system ILogger interface removing the deprecated method

### Changed
- **Issue #333**: Adopt unified KCENON_* feature flags
  - Replaced `THREAD_HAS_COMMON_EXECUTOR`, `THREAD_HAS_COMMON_RESULT`, `THREAD_HAS_COMMON_CONCEPTS` with `KCENON_HAS_*` equivalents
  - Added `feature_flags.h` include from common_system (guarded)
  - Removed local `__has_include` based macro definitions in thread_pool headers
  - Legacy `THREAD_HAS_*` macros are still defined as aliases for backward compatibility (deprecated, will be removed in v1.0.0)
  - CMake now defines both `KCENON_HAS_*` (primary) and `THREAD_HAS_*` (legacy alias) compile definitions

### Removed
- **Issue #331**: Remove deprecated THREAD_LOG_* macros from thread_logger.h
  - Removed unused THREAD_LOG_TRACE, THREAD_LOG_DEBUG, THREAD_LOG_INFO, THREAD_LOG_WARN, THREAD_LOG_ERROR macros
  - These were defined but never used, and conflicted with the standard LOG_* macros

### Changed (Continued)
- **Issue #331**: Migrate from deprecated common_system APIs
  - Added deprecation warning suppression for legacy ILogger::log() method in logger_system_adapter
  - This method remains implemented as it overrides a pure virtual function in ILogger interface
  - Will be removed when common_system v3.0.0 removes the deprecated base method

- **Issue #329**: Enable deprecated declaration warnings in compiler flags
  - Changed `-Wno-deprecated-declarations` to `-Wdeprecated-declarations` for GCC/Clang
  - Removed `/wd4996` flag for MSVC to enable deprecated warnings
  - This ensures early detection of deprecated API usage before common_system v3.0.0

## [3.0.0] - 2025-12-19

### BREAKING CHANGES

This release completes the migration to **common_system-only** public contracts. The following legacy types and interfaces have been removed from the public API:

**Error Handling**
- `kcenon::thread::result<T>` → Use `kcenon::common::Result<T>`
- `kcenon::thread::result_void` → Use `kcenon::common::VoidResult`
- `kcenon::thread::error` → Use `kcenon::common::error_info`

**Logging**
- `kcenon::thread::logger_interface` → Use `kcenon::common::interfaces::ILogger`
- `kcenon::thread::log_level` → Use `kcenon::common::log_level`
- `kcenon::thread::logger_registry` → Use common_system's logger registration

**Monitoring**
- `kcenon::thread::monitoring_interface` → Use `kcenon::common::interfaces::IMonitor`
- `kcenon::thread::monitorable_interface` → Use `kcenon::common::interfaces::IMonitorable`

**Executor/Shared Interfaces**
- `kcenon::shared::*` contracts → Use `kcenon::common::interfaces::IExecutor`
- `shared_interfaces.h` header removed
- Legacy adapters consolidated to `thread_pool_executor_adapter`

### Migration Guide

See the following migration guides for detailed instructions:
- [Error System Migration Guide](docs/advanced/ERROR_SYSTEM_MIGRATION_GUIDE.md)
- [Logger Interface Migration Guide](docs/guides/LOGGER_INTERFACE_MIGRATION_GUIDE.md)

**Quick Migration Summary:**

```cpp
// Before (v2.x)
#include <kcenon/thread/core/error_handling.h>
kcenon::thread::result<int> foo();

// After (v3.0)
#include <kcenon/common/result.h>
kcenon::common::Result<int> foo();
```

```cpp
// Before (v2.x)
class MyLogger : public kcenon::thread::logger_interface { ... };

// After (v3.0)
class MyLogger : public kcenon::common::interfaces::ILogger { ... };
```

### Removed
- **Issue #313 - Phase 3**: Remove shared_interfaces.h and consolidate executor adapters
  - Removed `kcenon::shared::*` contracts from public headers
  - Removed `include/kcenon/thread/interfaces/shared_interfaces.h` header file
  - Removed `include/kcenon/thread/adapters/thread_pool_executor.h` legacy adapter
  - Removed `include/kcenon/thread/adapters/common_system_executor_adapter.h` duplicate adapter
  - Consolidated to single canonical adapter: `thread_pool_executor_adapter` in `common_executor_adapter.h`
  - Updated `service_registration.h` to use canonical `thread_pool_executor_adapter`
  - All integrations now use `kcenon::common::interfaces::IExecutor` exclusively
- **Issue #312 - Phase 3**: Migrate monitoring to common::interfaces::IMonitor/IMonitorable
  - Removed `kcenon::thread::monitoring_interface` namespace from public headers
  - Removed `include/kcenon/thread/interfaces/monitoring_interface.h` header file
  - Removed `include/kcenon/thread/interfaces/monitorable_interface.h` header file
  - Removed `include/kcenon/thread/adapters/common_system_monitoring_adapter.h` adapter
  - `thread_context` now uses `common::interfaces::IMonitor` for metrics recording
  - Metrics are now recorded via `IMonitor::record_metric()` with tags for component identification
  - All code should now use `kcenon::common::interfaces::IMonitor` from common_system
  - Updated examples to demonstrate new IMonitor API usage
- **Issue #311 - Phase 3**: Remove deprecated thread_system logger_interface
  - Removed `kcenon::thread::logger_interface` class from public headers
  - Removed `kcenon::thread::log_level` enum from public headers
  - Removed `kcenon::thread::logger_registry` class from public headers
  - Removed `include/kcenon/thread/interfaces/logger_interface.h` header file
  - Removed `interfaces/logger_interface.cpp` implementation file
  - Removed `include/kcenon/thread/adapters/common_system_logger_adapter.h` adapter
  - All code should now use `kcenon::common::interfaces::ILogger` from common_system
  - See docs/guides/LOGGER_INTERFACE_MIGRATION_GUIDE.md for migration instructions
- **Issue #310 - Phase 3**: Remove legacy error types from public API
  - Removed `kcenon::thread::result<T>` class from public headers
  - Removed `kcenon::thread::result_void` class from public headers
  - Removed `kcenon::thread::error` class from public headers
  - All public APIs now exclusively use `kcenon::common::Result<T>` and `kcenon::common::VoidResult`
  - Added helper functions: `to_error_info()`, `make_error_result()`, `get_error_code()`
  - `error_code` enum and `std::error_code` integration preserved
  - Added CI guard to prevent re-introduction of legacy types
  - Updated all tests to use unified common::Result types
  - See docs/advanced/ERROR_SYSTEM_MIGRATION_GUIDE.md for migration instructions

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
- **PR #319**: Fix CI failures in safe_hazard_pointer integration (follow-up to #316)
  - Fixed deadlock in `retire()` by moving `collect()` call outside the lock
  - Added duplicate address handling to prevent double-free in memory reuse scenarios
  - Clear hazard pointers in `acquire()` when reusing records to avoid stale pointers
  - Check ALL records in `collect_internal()` to handle race during record reuse
  - Added retry limits to `enqueue()` and `dequeue()` to prevent hangs during contention
  - Added `queue_busy` error code for operations that exceed retry limits
  - Added hazard pointer protection to `empty()` to prevent UAF
  - Fixed infinite drain loop in `adaptive_job_queue::migrate_to_mode()` by updating mode before draining
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

[Unreleased]: https://github.com/kcenon/thread_system/compare/v3.0.0...HEAD
[3.0.0]: https://github.com/kcenon/thread_system/compare/v2.0.0...v3.0.0
[2.0.0]: https://github.com/kcenon/thread_system/compare/v1.5.0...v2.0.0
[1.5.0]: https://github.com/kcenon/thread_system/compare/v1.0.0...v1.5.0
[1.0.0]: https://github.com/kcenon/thread_system/releases/tag/v1.0.0
