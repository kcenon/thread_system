# Phase 3: Error Handling Preparation - thread_system

**Version**: 1.0
**Date**: 2025-10-09
**Status**: Ready for Implementation

---

## Overview

This document outlines the migration path for thread_system to adopt the centralized error handling from common_system Phase 3.

---

## Current State

### Error Handling Status

**Current Approach**:
- Mix of exceptions and boolean returns
- Some void functions that may fail
- Inconsistent error reporting

**Example**:
```cpp
// Current: throws exceptions
void thread_pool::start(size_t num_threads) {
    if (started_) {
        throw std::runtime_error("Pool already started");
    }
    // ...
}
```

---

## Migration Plan

### Phase 3.1: Import Error Codes

**Action**: Add common_system error code dependency

```cpp
#include <kcenon/common/error/error_codes.h>
#include <kcenon/common/patterns/result.h>

using namespace common;
using namespace common::error;
```

### Phase 3.2: Key API Migrations

#### Priority 1: Pool Lifecycle (High Impact)

```cpp
// Before
void start(size_t num_threads);
void stop();

// After
Result<void> start(size_t num_threads);
Result<void> stop();
```

**Error Codes**:
- `codes::thread_system::pool_already_running`
- `codes::thread_system::pool_not_started`
- `codes::thread_system::invalid_pool_size`

**Example Implementation**:
```cpp
Result<void> thread_pool::start(size_t num_threads) {
    if (started_) {
        return error<std::monostate>(
            codes::thread_system::pool_already_running,
            "Thread pool already started",
            "thread_pool"
        );
    }

    if (num_threads == 0) {
        return error<std::monostate>(
            codes::thread_system::invalid_pool_size,
            "Thread pool size must be > 0",
            "thread_pool"
        );
    }

    try {
        // Start workers...
        started_ = true;
        return ok();
    } catch (const std::exception& e) {
        return error<std::monostate>(
            codes::common::internal_error,
            "Failed to start thread pool",
            "thread_pool",
            e.what()
        );
    }
}
```

#### Priority 2: Job Submission

```cpp
// Before
template<typename F>
auto submit(F&& func) -> std::future<decltype(func())>;

// After
template<typename F>
auto submit(F&& func) -> Result<std::future<decltype(func())>>;
```

**Error Codes**:
- `codes::thread_system::pool_full`
- `codes::thread_system::pool_shutdown`
- `codes::thread_system::job_rejected`

**Example Implementation**:
```cpp
template<typename F>
auto thread_pool::submit(F&& func)
    -> Result<std::future<decltype(func())>>
{
    if (!started_ || shutdown_requested_) {
        return error<std::future<decltype(func())>>(
            codes::thread_system::pool_shutdown,
            "Thread pool not accepting jobs",
            "thread_pool"
        );
    }

    try {
        auto future = queue_.push(std::forward<F>(func));
        return ok(std::move(future));
    } catch (const queue_full_exception& e) {
        return error<std::future<decltype(func())>>(
            codes::thread_system::queue_full,
            "Job queue is full",
            "thread_pool",
            std::to_string(queue_.size())
        );
    }
}
```

#### Priority 3: Worker Management

```cpp
// Internal functions can remain exception-based
// Only public API needs Result<T>

// Public API:
Result<size_t> get_active_workers() const;
Result<void> resize(size_t new_size);
```

---

## Migration Checklist

### Code Changes

- [ ] Add common_system error code includes
- [ ] Migrate `thread_pool::start()` to Result<void>
- [ ] Migrate `thread_pool::stop()` to Result<void>
- [ ] Migrate `thread_pool::submit()` to Result<future<T>>
- [ ] Migrate `thread_pool::resize()` to Result<void>
- [ ] Update internal error handling to use error codes
- [ ] Add error context (pool state, queue size, etc.)

### Test Updates

- [ ] Update unit tests for Result<T> APIs
- [ ] Add error case tests for each error code
- [ ] Test error propagation patterns
- [ ] Verify error message quality
- [ ] Test monadic operation chains

### Documentation

- [ ] Update API reference with Result<T> signatures
- [ ] Document error codes for each function
- [ ] Add migration examples
- [ ] Update integration examples
- [ ] Create error handling guide

---

## Example Migrations

### Example 1: Basic Lifetime Management

```cpp
// Usage before
try {
    pool->start(4);
    pool->submit(my_task);
    pool->stop();
} catch (const std::exception& e) {
    log_error(e.what());
}

// Usage after
auto start_result = pool->start(4);
if (start_result.is_err()) {
    log_error(start_result.error().message);
    return;
}

auto submit_result = pool->submit(my_task);
if (submit_result.is_err()) {
    log_error(submit_result.error().message);
    return;
}

pool->stop();  // Returns Result<void>, check if needed
```

### Example 2: Monadic Chaining

```cpp
// Chain operations
auto result = pool->start(4)
    .and_then([&](auto) {
        return pool->submit(task1);
    })
    .and_then([&](auto future1) {
        return pool->submit(task2);
    })
    .map([](auto future2) {
        return future2.get();
    })
    .or_else([](const error_info& err) {
        log_error("Pool operation failed: {}", err.message);
        return ok(default_value());
    });
```

### Example 3: Error Recovery

```cpp
// Attempt to start pool, fallback to single-threaded
auto pool_result = create_thread_pool(num_threads)
    .or_else([](const error_info& err) {
        log_warn("Failed to create pool: {}", err.message);
        return create_thread_pool(1);  // Single thread fallback
    })
    .or_else([](const error_info& err) {
        log_error("Cannot create any pool: {}", err.message);
        return ok(std::make_shared<single_thread_executor>());
    });
```

---

## Error Code Mapping

### Thread System Error Codes (-100 to -199)

```cpp
namespace common::error::codes::thread_system {
    // Pool errors
    constexpr int pool_full = -100;
    constexpr int pool_shutdown = -101;
    constexpr int pool_not_started = -102;
    constexpr int invalid_pool_size = -103;

    // Worker errors
    constexpr int worker_failed = -120;
    constexpr int worker_not_found = -121;
    constexpr int worker_busy = -122;

    // Job errors
    constexpr int job_rejected = -140;
    constexpr int job_timeout = -141;
    constexpr int job_cancelled = -142;
    constexpr int invalid_job = -143;

    // Queue errors
    constexpr int queue_full = -160;
    constexpr int queue_empty = -161;
    constexpr int queue_stopped = -162;
}
```

### Error Messages

| Code | Message | When to Use |
|------|---------|-------------|
| pool_full | "Thread pool queue is full" | Job submission rejected due to queue capacity |
| pool_shutdown | "Thread pool is shutting down" | Operations during shutdown |
| pool_not_started | "Thread pool not started" | Operations before start() |
| invalid_pool_size | "Invalid pool size" | Size parameter validation |
| worker_failed | "Worker thread failed" | Worker exception or crash |
| job_rejected | "Job rejected" | Job validation failure |
| job_timeout | "Job execution timeout" | Timed job exceeded limit |
| queue_full | "Job queue full" | Queue capacity reached |

---

## Testing Strategy

### Unit Tests

```cpp
TEST(ThreadPoolPhase3, StartWithInvalidSize) {
    thread_pool pool;

    auto result = pool.start(0);

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(
        codes::thread_system::invalid_pool_size,
        result.error().code
    );
    EXPECT_EQ("thread_pool", result.error().module);
}

TEST(ThreadPoolPhase3, SubmitAfterShutdown) {
    thread_pool pool;
    pool.start(2);
    pool.stop();

    auto result = pool.submit([]{ return 42; });

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(
        codes::thread_system::pool_shutdown,
        result.error().code
    );
}

TEST(ThreadPoolPhase3, ErrorPropagation) {
    auto result = create_pool()
        .and_then([](auto pool) { return pool->start(4); })
        .and_then([](auto pool) { return pool->submit(task); });

    if (result.is_ok()) {
        auto future = result.value();
        // Use future...
    } else {
        log_error(result.error().message);
    }
}
```

---

## Performance Impact

### Expected Overhead

- **Result<T> size**: +24 bytes per return value (variant overhead)
- **Error creation**: ~2-3ns (string construction)
- **Success path**: ~0-1ns (inline optimization)

### Mitigation

- Move semantics prevent copies
- RVO/NRVO eliminate temporaries
- Inline optimization removes overhead
- Error paths already slow (exceptions were worse)

---

## Implementation Timeline

### Week 1: Foundation
- Day 1-2: Migrate pool lifecycle APIs
- Day 3: Update tests
- Day 4-5: Documentation

### Week 2: Job Submission
- Day 1-2: Migrate submit() APIs
- Day 3: Update integration tests
- Day 4-5: Code review and refinement

### Week 3: Finalization
- Day 1-2: Migrate remaining public APIs
- Day 3: Performance validation
- Day 4-5: Documentation and examples

---

## References

- [common_system Error Codes](../../common_system/include/kcenon/common/error/error_codes.h)
- [Error Handling Guidelines](../../common_system/docs/ERROR_HANDLING.md)
- [Result<T> Implementation](../../common_system/include/kcenon/common/patterns/result.h)

---

**Document Status**: Phase 3 Preparation Complete
**Next Action**: Begin implementation or await approval
**Maintainer**: thread_system team
