# ADR-001: v3.0 API Surface and Compatibility Policy

**Status:** Proposed
**Date:** 2025-12-19
**Authors:** thread_system maintainers
**Related Issues:** #308, #309, #310, #311, #312, #313, #314, #315

## Context

thread_system has evolved through multiple phases to integrate with common_system:

- **Phase 1**: Initial integration with common_system as a dependency
- **Phase 2**: Migration to `common::Result<T>` internally with deprecation warnings
- **Phase 3** (Current): Complete removal of legacy types from the public API

The v2.x series maintains backward compatibility through deprecated aliases and conditional compilation (`THREAD_HAS_COMMON_RESULT`, `BUILD_WITH_COMMON_SYSTEM`). For v3.0, we need to:

1. Remove all deprecated thread-specific types from the installed headers
2. Define a clean, stable public API surface
3. Establish clear contracts for cross-module integration

This ADR documents the final decisions for the v3.0 public API.

---

## Decisions

### Decision 1: Result/Error Contract

**Decision ID:** `ADR-001-D1`

#### What Gets Removed

The following types will be removed from the installed public headers:

| Removed Type | Replacement |
|-------------|-------------|
| `kcenon::thread::result<T>` | `kcenon::common::Result<T>` |
| `kcenon::thread::result_void` | `kcenon::common::VoidResult` |
| `kcenon::thread::error` | `kcenon::common::error_info` |
| `kcenon::thread::error_code` (enum) | Internal use only |

#### What Remains

- `kcenon::thread::error_code` enum MAY remain as an **internal** type for thread-specific error codes
- The `thread_error_category` and `make_error_code()` functions for `std::error_code` integration will be preserved for internal use
- Error codes will be mapped to `common::error_info::code` field with values offset by `common::error_codes::THREAD_ERROR_BASE`

#### Public API Contract

```cpp
// All public APIs return common::Result types
namespace kcenon::thread {
    // Example: thread_pool::enqueue
    auto enqueue(std::unique_ptr<job> j) -> common::VoidResult;

    // Example: thread_pool::stop
    auto stop(bool force = false) -> common::VoidResult;
}
```

#### Migration Path

Users should replace:
```cpp
// Old (v2.x)
kcenon::thread::result<int> res = some_function();
if (res.has_value()) { /* ... */ }

// New (v3.0)
kcenon::common::Result<int> res = some_function();
if (res.is_ok()) { /* ... */ }
```

---

### Decision 2: Executor/Job Contract

**Decision ID:** `ADR-001-D2`

#### Integration Path

The canonical integration path for thread_system with common_system is:

1. **IExecutor**: `kcenon::common::interfaces::IExecutor` is the standard executor interface
2. **IJob**: `kcenon::common::interfaces::IJob` is the standard job interface
3. **Adapter**: `kcenon::thread::adapters::thread_pool_executor_adapter` bridges thread_pool to IExecutor

#### kcenon::thread::job Status

**Decision:** `kcenon::thread::job` **remains public** but is now a concrete implementation detail.

- `kcenon::thread::job` is the internal job type for `thread_pool`
- Its `do_work()` method returns `common::VoidResult`
- Users who want cross-module compatibility should use `common::interfaces::IJob` and submit via `IExecutor`

#### Public API Contract

```cpp
namespace kcenon::thread {
    // thread_pool accepts its native job type
    class thread_pool {
    public:
        auto enqueue(std::unique_ptr<job> j) -> common::VoidResult;
    };

    // job returns common::VoidResult
    class job {
    public:
        virtual auto do_work() -> common::VoidResult;
    };
}

namespace kcenon::thread::adapters {
    // Adapter bridges to common::interfaces::IExecutor
    class thread_pool_executor_adapter : public common::interfaces::IExecutor {
    public:
        auto execute(std::unique_ptr<common::interfaces::IJob>&& job)
            -> common::Result<std::future<void>> override;
    };
}
```

---

### Decision 3: Monitoring/Logging Contract

**Decision ID:** `ADR-001-D3`

#### Logger Contract

**Decision:** Only `kcenon::common::interfaces::ILogger` is supported in v3.0.

| Removed | Replacement |
|---------|-------------|
| `kcenon::thread::logger_interface` | `kcenon::common::interfaces::ILogger` |
| `kcenon::thread::logger_registry` | `kcenon::common::interfaces::ILoggerRegistry` |
| `kcenon::thread::log_level` | `kcenon::common::interfaces::log_level` |
| `THREAD_LOG_*` macros | Remove or migrate to common logging macros |

**Note:** The legacy `log_level` enum had inverted ordering (critical=0, trace=5). The unified interface uses standard ordering (trace=0, critical=5).

#### Monitoring Contract

**Decision:** Only `kcenon::common::interfaces::IMonitor` and `IMonitorable` are supported in v3.0.

| Removed | Replacement |
|---------|-------------|
| `monitoring_interface::monitoring_interface` | `common::interfaces::IMonitor` |
| `monitoring_interface::thread_pool_metrics` | `common::interfaces::thread_pool_metrics` |
| `monitoring_interface::worker_metrics` | `common::interfaces::worker_metrics` |
| `monitoring_interface::system_metrics` | `common::interfaces::system_metrics` |

#### Metrics Mapping Strategy

`thread_pool` metrics will be reported via `IMonitor::record_metric()` with the following tag schema:

```cpp
// Example: Recording thread pool metrics
monitor->record_metric("thread_pool.jobs_completed", value, {
    {"pool_name", pool_name},
    {"pool_instance_id", std::to_string(instance_id)}
});

monitor->record_metric("thread_pool.pending_jobs", value, {
    {"pool_name", pool_name}
});

monitor->record_metric("thread_pool.worker_count", value, {
    {"pool_name", pool_name},
    {"state", "active"}  // or "idle"
});
```

---

### Decision 4: Header/Include Policy

**Decision ID:** `ADR-001-D4`

#### Stable Umbrella Headers

v3.0 introduces stable umbrella headers under `<kcenon/thread/*.h>`:

| Header | Contents |
|--------|----------|
| `<kcenon/thread/thread_pool.h>` | `thread_pool`, `thread_worker` |
| `<kcenon/thread/job.h>` | `job`, `callback_job`, `cancellable_job` |
| `<kcenon/thread/queue.h>` | `job_queue`, `adaptive_job_queue`, `lockfree_job_queue` |
| `<kcenon/thread/typed_pool.h>` | `typed_thread_pool`, typed job types |
| `<kcenon/thread/adapters.h>` | All adapter classes |
| `<kcenon/thread/sync.h>` | `cancellation_token`, sync primitives |

#### Legacy Header Removal

The following headers will be **removed** in v3.0:

| Header | Action |
|--------|--------|
| `<kcenon/thread/interfaces/logger_interface.h>` | Remove |
| `<kcenon/thread/interfaces/monitoring_interface.h>` | Remove |
| `<kcenon/thread/interfaces/shared_interfaces.h>` | Remove |
| `<kcenon/thread/forward.h>` | Remove (contains stale forward declarations) |

#### forward.h Analysis

The current `forward.h` contains forward declarations that don't match actual types:
- Declares `ThreadPool` (PascalCase) but actual type is `thread_pool` (snake_case)
- Declares non-existent types: `Mutex`, `Semaphore`, `ConditionVariable`, `SpinLock`, etc.
- Declares `Future<T>`, `Promise<T>`, `SharedFuture<T>` which don't exist

**Decision:** Remove `forward.h` entirely. If forward declarations are needed, they should be generated or maintained alongside the actual type definitions.

---

### Decision 5: Compatibility Strategy

**Decision ID:** `ADR-001-D5`

#### No Compatibility Header

**Decision:** v3.0 will **not** ship a `thread_system_compat.h` header.

Rationale:
1. Phase 2 provided ample deprecation warnings with migration guidance
2. A compatibility layer adds maintenance burden and potential confusion
3. Clean break enables simpler API and documentation

#### Breaking Changes Summary

v3.0 is a **breaking release**. Users upgrading from v2.x must:

1. Replace `kcenon::thread::result<T>` with `kcenon::common::Result<T>`
2. Replace `kcenon::thread::result_void` with `kcenon::common::VoidResult`
3. Replace `kcenon::thread::error` with `kcenon::common::error_info`
4. Replace `kcenon::thread::logger_interface` with `kcenon::common::interfaces::ILogger`
5. Replace `monitoring_interface::*` types with `kcenon::common::interfaces::*`
6. Update include paths to use umbrella headers

#### Minimum common_system Version

v3.0 requires `common_system >= 2.0.0` which provides all unified interfaces.

---

## Consequences

### Positive

1. **Simplified API**: Single set of types across the ecosystem
2. **Better Error Handling**: Unified `Result<T>` with rich error information
3. **Consistent Monitoring**: Standard metrics interface for all systems
4. **Reduced Maintenance**: No duplicate type definitions or compatibility layers
5. **Clear Documentation**: One API to document and support

### Negative

1. **Breaking Changes**: Existing v2.x users must update their code
2. **Dependency Requirement**: common_system becomes a hard dependency
3. **Migration Effort**: Downstream projects need updates

### Neutral

1. **No Opt-in Legacy Mode**: Users cannot gradually migrate; must update fully
2. **Version Requirement**: Requires coordinated release across ecosystem

---

## Implementation Checklist

- [ ] **#310**: Remove `thread::result` / `thread::error` from installed API
- [ ] **#311**: Remove deprecated `thread_system logger_interface`
- [ ] **#312**: Migrate monitoring to `common::interfaces::{IMonitor, IMonitorable}`
- [ ] **#313**: Remove `shared_interfaces.h` and consolidate adapters
- [ ] **#314**: Add stable umbrella headers + align API docs
- [ ] **#315**: Update tests/examples to common_system-only API
- [ ] **CI**: Add check to verify no legacy types are exported

---

## References

- [common_system Result<T> Documentation](https://github.com/kcenon/common_system/blob/main/docs/patterns/result.md)
- [common_system Interface Documentation](https://github.com/kcenon/common_system/blob/main/docs/interfaces/)
- [Phase 2 Migration Guide](../advanced/ERROR_SYSTEM_MIGRATION_GUIDE.md)
- [Logger Interface Migration Guide](../guides/LOGGER_INTERFACE_MIGRATION_GUIDE.md)

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2025-12-19 | thread_system maintainers | Initial draft |
