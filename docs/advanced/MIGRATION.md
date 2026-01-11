# Thread System Migration Guide

> **Language:** **English** | [한국어](MIGRATION_KO.md)

## Table of Contents

- [v3.0.0 Migration (common_system)](#v300-migration-common_system)
- [Overview](#overview)
- [Migration Status](#migration-status)
  - [Phase 1: Interface Extraction and Cleanup ✅ COMPLETE](#phase-1-interface-extraction-and-cleanup-complete)
  - [Phase 2: Create New Repository Structure ✅ COMPLETE](#phase-2-create-new-repository-structure-complete)
  - [Phase 3: Component Migration ✅ COMPLETE](#phase-3-component-migration-complete)
  - [Phase 4: Integration Testing ✅ COMPLETE](#phase-4-integration-testing-complete)
  - [2025-09 Updates (Phase 2–3)](#2025-09-updates-phase-23)
  - [Phase 5: Gradual Deployment 🔄 PENDING](#phase-5-gradual-deployment-pending)
- [Breaking Changes](#breaking-changes)
  - [API Changes](#api-changes)
  - [Build System Changes](#build-system-changes)
- [Migration Instructions for Users](#migration-instructions-for-users)
  - [Current Users (Phase 1)](#current-users-phase-1)
  - [Future Migration (Phase 2-5)](#future-migration-phase-2-5)
- [Timeline](#timeline)
  - [Current Status (2025-09-13)](#current-status-2025-09-13)
  - [Detailed Status Log](#detailed-status-log)

## v3.0.0 Migration (common_system)

**Release Date:** 2025-12-19

v3.0.0 completes the migration to **common_system-only** public contracts. This is a **breaking change** release.

### Removed Types

| Legacy Type | Replacement |
|-------------|-------------|
| `kcenon::thread::result<T>` | `kcenon::common::Result<T>` |
| `kcenon::thread::result_void` | `kcenon::common::VoidResult` |
| `kcenon::thread::error` | `kcenon::common::error_info` |
| `kcenon::thread::logger_interface` | `kcenon::common::interfaces::ILogger` |
| `kcenon::thread::log_level` | `kcenon::common::log_level` |
| `kcenon::thread::monitoring_interface` | `kcenon::common::interfaces::IMonitor` |
| `kcenon::thread::monitorable_interface` | `kcenon::common::interfaces::IMonitorable` |
| `kcenon::shared::*` | `kcenon::common::interfaces::IExecutor` |

### Quick Migration

```cpp
// Error handling
// Before:
#include <kcenon/thread/core/error_handling.h>
kcenon::thread::result<int> foo();

// After:
#include <kcenon/common/result.h>
kcenon::common::Result<int> foo();

// API changes:
// .has_error() → .is_err()
// .get_error() → .error()
// .value() → .value() (unchanged)
```

### Detailed Migration Guides

- [Error System Migration Guide](ERROR_SYSTEM_MIGRATION_GUIDE.md)
- [Logger Interface Migration Guide](../guides/LOGGER_INTERFACE_MIGRATION_GUIDE.md)

---

## Overview

This document tracks the migration of the thread_system from a monolithic architecture to a modular ecosystem.

## Migration Status

### Phase 1: Interface Extraction and Cleanup ✅ COMPLETE

**Completed Tasks:**
- Verified existing interfaces (`logger_interface.h`, `monitoring_interface.h`) are properly isolated
- Updated `thread_context.h` to support multi-pool monitoring with overloaded methods
- Fixed initialization order warnings in `thread_pool.cpp` and `thread_worker.cpp`
- Updated sample code to use correct API signatures
- Fixed namespace conflicts in `multi_process_monitoring_integration` sample
- All tests passing successfully

**Key Changes:**
1. Added overloaded `update_thread_pool_metrics` method in `thread_context.h`:
   ```cpp
   void update_thread_pool_metrics(const std::string& pool_name,
                                  std::uint32_t pool_instance_id,
                                  const monitoring_interface::thread_pool_metrics& metrics)
   ```

2. Fixed constructor initialization order in:
   - `thread_pool.cpp`: Reordered to match member declaration order
   - `thread_worker.cpp`: Reordered to match member declaration order

3. Updated sample code:
   - Fixed `callback_job` constructor parameter order (callback first, then name)
   - Updated to use new `thread_pool::start()` API (no worker count parameter)
   - Fixed namespace resolution for monitoring interface types

### Phase 2: Create New Repository Structure ✅ COMPLETE

**Completed Tasks:**
- Created modular directory structure under `modular_structure/`
- Set up core module CMakeLists.txt with proper export configuration
- Created integration templates for logger and monitoring modules
- Prepared CMake package configuration for find_package support
- Documented integration patterns for optional modules

**New Structure:**
```
modular_structure/
├── core/                    # Core thread_system module
│   ├── CMakeLists.txt      # Main build configuration
│   ├── cmake/              # CMake config templates
│   ├── include/            # Public headers
│   └── src/                # Implementation files
└── optional/               # Integration templates
    ├── logger_integration/
    └── monitoring_integration/
```

**Key Features:**
1. Core module with zero external dependencies (except standard library)
2. Clean CMake export configuration for easy integration
3. Comprehensive integration guides for logger and monitoring
4. Backward compatibility support via target aliases

### Phase 3: Component Migration ✅ COMPLETE

**Completed Tasks:**
- ✅ Moved all core components to modular structure
- ✅ Updated all include paths to use thread_system_core namespace
- ✅ Fixed all compilation errors with automated scripts
- ✅ Successfully built core module as standalone library
- ✅ Created compatibility headers for backward compatibility

**Key Changes:**
1. Migrated components:
   - `thread_base/` - Core threading abstractions
   - `thread_pool/` - Standard thread pool implementation
   - `typed_thread_pool/` - Type-safe thread pool with priorities
   - `utilities/` - String conversion and formatting utilities
   - `interfaces/` - Logger and monitoring interfaces

2. Include path updates:
   - All internal includes now use `thread_system_core/` prefix
   - Created Python scripts to automate include path fixes
   - Fixed over 60 files with incorrect include paths

3. Build system improvements:
   - Core module builds with C++20 standard
   - Added platform-specific support (iconv for macOS)
   - C++20 std::format used exclusively (fmt library removed)
   - Clean CMake export configuration

4. Compatibility:
   - Created `.compat` headers for smooth migration
   - Original project still builds without changes
   - All tests passing in both original and modular versions

### Phase 4: Integration Testing ✅ COMPLETE

**Completed Tasks:**
- ✅ Created comprehensive integration test suite
- ✅ Implemented tests for basic thread pool, logger, monitoring, and typed thread pool
- ✅ Created performance benchmarks
- ✅ Verified core module can be compiled and linked independently
- ✅ Identified integration issues with CMake config generation

**Key Findings:**
1. Core module builds successfully as standalone library
2. Job queue and job execution work correctly in isolation
3. CMake config file generation has issues (EOF in config file)
4. Thread pool worker initialization may need adjustment
5. API signatures have evolved (callback_job requires result types)

**Test Files Created:**
- `test_basic_thread_pool.cpp` - Basic thread pool functionality
- `test_logger_integration.cpp` - Custom logger implementation tests
- `test_monitoring_integration.cpp` - Custom monitoring implementation tests
- `test_typed_thread_pool.cpp` - Priority-based thread pool tests
- `benchmark_thread_system.cpp` - Performance benchmarks
- `simple_test.cpp` - Minimal integration test
- `minimal_test.cpp` - Direct job queue test

---

### 2025-09 Updates (Phase 2–3)

The project completed a structural migration and documentation pass:

- New source layout under core/, implementations/, interfaces/, utilities/
- CMake updated with per-module targets and an optional `docs` target (Doxygen)
- Added public interfaces: executor_interface, scheduler_interface, monitorable_interface
- job_queue implements scheduler_interface; thread_pool and typed_thread_pool implement executor_interface
- Documentation added:
  - docs/API_REFERENCE.md (complete API documentation with interfaces)
  - docs/USER_GUIDE.md (build, usage, docs generation)
  - Module READMEs in core/, implementations/, interfaces/

Action items for downstream integrations:
- Update include paths to the new module headers
- Link to the new library targets (thread_base, thread_pool, typed_thread_pool, lockfree, interfaces, utilities)
- Generate Doxygen docs via `cmake --build build --target docs` (requires Doxygen)

**Integration Patterns Verified:**
- Custom logger implementation works with thread_context
- Custom monitoring implementation captures metrics correctly
- Job queue enqueue/dequeue operations function properly
- Module uses C++20 std::format exclusively (no external format library dependency)

### Phase 5: Gradual Deployment 🔄 PENDING

**Planned Tasks:**
- Create migration guide for users
- Release alpha/beta versions
- Gather feedback and iterate
- Final release with deprecation notices

## Breaking Changes

### API Changes
1. `thread_pool::start()` no longer accepts worker count parameter
2. `callback_job` constructor now takes callback first, then optional name
3. Namespace `monitoring_interface` contains both namespace and class of same name
4. API consistency: `thread_pool` methods now return `result_void` instead of `std::optional<std::string>`
   - Updated signatures:
     - `auto start() -> result_void`
     - `auto stop(bool immediately = false) -> result_void`
     - `auto enqueue(std::unique_ptr<job>&&) -> result_void`
     - `auto enqueue_batch(std::vector<std::unique_ptr<job>>&&) -> result_void`
   - Check errors via `has_error()` and inspect with `get_error().to_string()`

### Build System Changes
- Will require separate module dependencies in future phases
- Include paths will change from internal to external modules

### Phase 1.3.3: thread_pool policy_queue Support (2025-01)

**New Feature:** `thread_pool` now supports `policy_queue` through the adapter pattern.

#### New Constructors

```cpp
#include <kcenon/thread/adapters/job_queue_adapter.h>
#include <kcenon/thread/adapters/policy_queue_adapter.h>

// Using job_queue_adapter (wraps existing job_queue)
auto adapter = std::make_unique<job_queue_adapter>();
auto pool = std::make_shared<thread_pool>("my_pool", std::move(adapter));

// Using make_standard_queue_adapter() helper
auto pool2 = std::make_shared<thread_pool>(
    "pool2",
    make_standard_queue_adapter());

// Using make_lockfree_queue_adapter() helper
auto pool3 = std::make_shared<thread_pool>(
    "pool3",
    make_lockfree_queue_adapter());
```

#### Backward Compatibility

All existing code continues to work without changes:

```cpp
// Still works - default constructor
auto pool = std::make_shared<thread_pool>();

// Still works - custom job_queue
auto queue = std::make_shared<job_queue>();
auto pool = std::make_shared<thread_pool>("my_pool", queue);
```

#### Adapter Interface

The `pool_queue_adapter_interface` provides a unified API for both queue types:

```cpp
class pool_queue_adapter_interface {
public:
    virtual auto enqueue(std::unique_ptr<job>&&) -> common::VoidResult = 0;
    virtual auto enqueue_batch(std::vector<std::unique_ptr<job>>&&) -> common::VoidResult = 0;
    virtual auto dequeue() -> common::Result<std::unique_ptr<job>> = 0;
    virtual auto try_dequeue() -> common::Result<std::unique_ptr<job>> = 0;
    virtual auto empty() const -> bool = 0;
    virtual auto size() const -> std::size_t = 0;
    virtual auto clear() -> void = 0;
    virtual auto stop() -> void = 0;
    virtual auto is_stopped() const -> bool = 0;
    virtual auto get_capabilities() const -> queue_capabilities = 0;
    virtual auto to_string() const -> std::string = 0;
    virtual auto get_job_queue() const -> std::shared_ptr<job_queue> = 0;
    virtual auto get_scheduler() -> scheduler_interface& = 0;
};
```

#### Current Limitations

- **Workers with policy_queue:** When using `policy_queue_adapter` directly (not wrapping a `job_queue`), workers currently require a `job_queue` backend. This limitation may be lifted in future versions when `thread_worker` is updated to use `scheduler_interface`.

### Phase 1.3.4: Integration Tests for policy_queue (2025-01)

**New Test Files:** Comprehensive integration tests for policy_queue have been added.

#### Test Files

| File | Description |
|------|-------------|
| `policy_queue_integration_test.cpp` | Tests standard_queue, policy_lockfree_queue, bounded queues |
| `queue_performance_comparison_test.cpp` | Performance benchmarks comparing legacy job_queue vs policy_queue |

#### Test Coverage

- Basic queue operations (enqueue, dequeue, clear, stop)
- FIFO ordering verification
- Concurrent enqueue/dequeue with multiple threads
- Bounded queue overflow policies (reject, drop_oldest)
- Queue capabilities and scheduler interface compliance
- Single-threaded and multi-threaded throughput benchmarks
- Dequeue latency measurements

#### Running Tests

```bash
# Run policy_queue tests only
./bin/integration_tests --gtest_filter="PolicyQueue*"

# Run performance comparison tests
./bin/integration_tests --gtest_filter="QueuePerformance*"

# Run all integration tests
./bin/integration_tests
```

#### Known Disabled Tests

| Test | Reason |
|------|--------|
| `ThreadPoolWithStandardQueueAdapter` | policy_queue adapter requires job_queue backend for workers |
| `ThreadPoolWithLockfreeQueueAdapter` | Same limitation as above |
| `LockfreeQueueConcurrentOperations` | Potential issues in lockfree_sync_policy under high contention |

These limitations will be addressed in future updates when `thread_worker` is updated to use `scheduler_interface` directly.

## Migration Instructions for Users

### Current Users (Phase 1)
No action required. All changes are backward compatible.

### Future Migration (Phase 2-5)
1. Update CMake to use find_package for separate modules
2. Update include paths for logger and monitoring
3. Link against separate libraries instead of monolithic thread_system

## Timeline

- Phase 1: ✅ Complete (2025-01-27)
- Phase 2: ✅ Complete (2025-01-27)
- Phase 3: ✅ Complete (2025-01-27)
- Phase 4: ✅ Complete (2025-01-27)
- Phase 5: In Progress - Estimated 6 weeks

Total estimated completion: Q1 2025
### Current Status (2025-09-13)

The migration is complete with the modular structure in place and interfaces integrated across pools and queues. All documentation has been updated to reflect the current architecture. See details below.
### Detailed Status Log

The previously separate status document (MIGRATION_STATUS.md) has been merged into this section to keep migration guidance and current state together.

---

*Last Updated: 2025-01-11*
