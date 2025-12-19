# Error System Migration Guide

**Version:** 1.0.0
**Date:** 2025-12-19
**Status:** Phase 3 Complete

## Overview

The thread_system has completed migration from its custom `thread::result<T>` error handling system to the unified `kcenon::common::Result<T>` system from common_system. As of v3.0, all public APIs exclusively use the unified types.

## Migration Phases Summary

### Phase 1: Internal Unification ✅ Complete (v0.1.0)

When `THREAD_HAS_COMMON_RESULT` was defined, `thread::result<T>` used `common::Result<T>` internally while maintaining the legacy API.

### Phase 2: Deprecation Period ✅ Complete (v0.2.0)

Legacy types were marked with `[[deprecated]]` attribute, generating compiler warnings while remaining functional.

### Phase 3: Complete Removal ✅ Complete (v3.0.0)

**Breaking Changes:**
- `thread::result<T>` class removed from public headers
- `thread::result_void` class removed from public headers
- `thread::error` class removed from public headers
- Only `kcenon::common::Result<T>` and `kcenon::common::VoidResult` available

**Preserved:**
- `thread::error_code` enum for thread-system-specific error codes
- `std::error_code` integration via `thread_error_category`
- Helper functions: `to_error_info()`, `make_error_result()`, `get_error_code()`

## Quick Migration Reference

### Include Changes

```cpp
// Before (v2.x)
#include <kcenon/thread/core/error_handling.h>

// After (v3.x) - same header, new types
#include <kcenon/thread/core/error_handling.h>
// OR directly include common pattern
#include <kcenon/common/patterns/result.h>
```

### Type Aliases

```cpp
// Before (v2.x)
using kcenon::thread::result;
using kcenon::thread::result_void;
using kcenon::thread::error;

// After (v3.x)
using kcenon::common::Result;
using kcenon::common::VoidResult;
using kcenon::common::error_info;
// Thread-specific helpers
using kcenon::thread::error_code;
using kcenon::thread::to_error_info;
using kcenon::thread::make_error_result;
using kcenon::thread::get_error_code;
```

### Creating Results

```cpp
// Before (v2.x)
result<int> compute() {
    if (error_condition) {
        return result<int>(error{error_code::invalid_argument, "Bad input"});
    }
    return result<int>(42);
}

// After (v3.x)
common::Result<int> compute() {
    if (error_condition) {
        return make_error_result<int>(error_code::invalid_argument, "Bad input");
    }
    return common::Result<int>::ok(42);
}
```

### Creating VoidResult

```cpp
// Before (v2.x)
result_void do_work() {
    if (error_condition) {
        return result_void(error{error_code::operation_failed, "Failed"});
    }
    return result_void{};
}

// After (v3.x)
common::VoidResult do_work() {
    if (error_condition) {
        return make_error_result(error_code::operation_failed, "Failed");
    }
    return common::ok();
}
```

### Checking Results

```cpp
// Before (v2.x)
if (result.has_value()) {
    auto value = result.value();
}
if (result.has_error()) {
    auto& err = result.get_error();
    auto code = err.code();
}

// After (v3.x)
if (result.is_ok()) {
    auto value = result.value();
}
if (result.is_err()) {
    auto& err = result.error();
    auto code = get_error_code(err);  // Helper to extract thread::error_code
}
```

## API Mapping Table

| Old (v2.x) | New (v3.x) | Notes |
|------------|-----------|-------|
| `result<T>` | `common::Result<T>` | Full replacement |
| `result_void` | `common::VoidResult` | Full replacement |
| `error{code, msg}` | `to_error_info(code, msg)` | Use helper function |
| `.has_value()` | `.is_ok()` | |
| `.has_error()` | `.is_err()` | |
| `.value()` | `.value()` | Unchanged |
| `.get_error()` | `.error()` | |
| `.get_error().code()` | `get_error_code(.error())` | Helper function |
| `.value_or(default)` | `.value_or(default)` | Unchanged |
| `result_void{}` | `common::ok()` | Success factory |

## Thread-Specific Error Codes

The `thread::error_code` enum is preserved and can be used with the new helper functions:

```cpp
using kcenon::thread::error_code;
using kcenon::thread::to_error_info;
using kcenon::thread::make_error_result;
using kcenon::thread::get_error_code;

// Create error_info from thread::error_code
auto info = to_error_info(error_code::queue_full, "Custom message");

// Create Result<T> error
auto result = make_error_result<int>(error_code::invalid_argument);

// Create VoidResult error
auto void_result = make_error_result(error_code::thread_start_failure);

// Extract error_code from error_info
if (result.is_err()) {
    error_code code = get_error_code(result.error());
    if (code == error_code::queue_full) {
        // Handle specific error
    }
}
```

## std::error_code Integration

Thread-system error codes remain compatible with `std::error_code`:

```cpp
#include <kcenon/thread/core/error_handling.h>

// Implicit conversion
std::error_code ec = kcenon::thread::error_code::queue_full;

// Explicit creation
std::error_code ec = kcenon::thread::make_error_code(
    kcenon::thread::error_code::operation_timeout
);

// Category check
if (ec.category().name() == std::string("thread_system")) {
    // Thread system error
}

// Condition equivalence
if (ec == std::errc::timed_out) {
    // Operation timeout
}
```

## Common Patterns

### Callback Jobs

```cpp
auto job = std::make_unique<callback_job>(
    []() -> common::VoidResult {
        // Do work
        if (failed) {
            return make_error_result(error_code::job_execution_failed, "Reason");
        }
        return common::ok();
    }
);
```

### Thread Pool Operations

```cpp
auto pool = std::make_shared<thread_pool>("MyPool");
auto start_result = pool->start();
if (start_result.is_err()) {
    auto code = get_error_code(start_result.error());
    std::cerr << "Failed to start: " << start_result.error().message << "\n";
    return;
}

// Enqueue job
auto enqueue_result = pool->enqueue(std::move(job));
if (enqueue_result.is_err()) {
    // Handle error
}
```

### Error Propagation

```cpp
common::VoidResult process() {
    auto result = step1();
    if (result.is_err()) {
        return result;  // Propagate error
    }

    auto result2 = step2();
    if (result2.is_err()) {
        return common::VoidResult(result2.error());  // Convert from Result<T>
    }

    return common::ok();
}
```

## Benefits of Migration

1. **Unified Error Handling**: Consistent patterns across kcenon::common and kcenon::thread
2. **Smaller API Surface**: Single set of types to learn and use
3. **Better Type Safety**: `common::Result<T>` has stronger compile-time guarantees
4. **Rust-Inspired API**: Familiar patterns for developers from Rust background
5. **Future-Proof**: Aligned with ecosystem direction

## Timeline Summary

| Phase | Version | Date | Status |
|-------|---------|------|--------|
| Phase 1 | 0.1.0 | 2025-11-09 | ✅ Complete |
| Phase 2 | 0.2.0 | 2025-12-16 | ✅ Complete |
| Phase 3 | 3.0.0 | 2025-12-19 | ✅ Complete |

## Support

For questions or issues during migration:
1. Check this guide
2. See `common_system/include/kcenon/common/patterns/result.h` for API reference
3. Consult `error_handling.h` for thread-specific helper functions
4. Open an issue at https://github.com/kcenon/thread_system/issues
