# Error System Migration Guide

**Version:** 0.2.0
**Date:** 2025-12-16
**Status:** Phase 2 In Progress

## Overview

The thread_system is migrating from its custom `thread::result<T>` error handling system to the unified `kcenon::common::Result<T>` system from common_system. This migration is being done in phases to ensure backward compatibility.

## Migration Phases

### Phase 1: Internal Unification (COMPLETE)

When `THREAD_HAS_COMMON_RESULT` is defined (i.e., when common_system is available), `thread::result<T>` now uses `common::Result<T>` internally. The public API remains unchanged, ensuring full backward compatibility.

**Status:** ✅ Complete
- `result<T>` uses `common::Result<T>` internally
- All existing code continues to work without changes
- No performance regression (89/89 tests pass)

### Phase 2: Deprecation Period (CURRENT)

**Status:** 🔄 In Progress

The following types are now marked with `[[deprecated]]` attribute when `THREAD_HAS_COMMON_RESULT` is defined:
- `thread::result<T>` - Use `common::Result<T>` instead
- `thread::result_void` - Use `common::VoidResult` instead
- `thread::result<void>` - Use `common::VoidResult` instead

Compiler warnings will guide users to migrate. Both APIs remain fully functional.

### Phase 3: Complete Migration (NEXT MAJOR VERSION)

In the next major version:
- `thread::result<T>` will be removed
- Only `kcenon::common::Result<T>` will be available
- `thread::error` will be replaced by `common::error_info`

## Migration Instructions

### For New Code

**Do this:**
```cpp
#include <kcenon/common/patterns/result.h>

using kcenon::common::Result;
using kcenon::common::VoidResult;

Result<int> compute() {
    if (some_error) {
        return Result<int>(common::error_info{1, "Error occurred", "thread_system"});
    }
    return Result<int>::ok(42);
}
```

**Instead of this:**
```cpp
#include <kcenon/thread/core/error_handling.h>

using kcenon::thread::result;

result<int> compute() {
    if (some_error) {
        return result<int>(thread::error{thread::error_code::operation_failed, "Error occurred"});
    }
    return result<int>(42);
}
```

### For Existing Code

Your existing code will continue to work without changes during Phase 1 and Phase 2. However, we recommend migrating proactively:

#### Step 1: Update includes

```cpp
// Before
#include <kcenon/thread/core/error_handling.h>

// After
#include <kcenon/common/patterns/result.h>
```

#### Step 2: Update type aliases

```cpp
// Before
using kcenon::thread::result;
using kcenon::thread::error;
using kcenon::thread::error_code;

// After
using kcenon::common::Result;
using kcenon::common::VoidResult;
using kcenon::common::error_info;
```

#### Step 3: Update error construction

```cpp
// Before
return result<int>(error{error_code::invalid_argument, "Invalid input"});

// After
return Result<int>(error_info{
    static_cast<int>(error_code::invalid_argument),
    "Invalid input",
    "thread_system"
});
```

#### Step 4: Update error checking

```cpp
// Before
if (result.has_value()) { ... }

// After
if (result.is_ok()) { ... }  // Preferred
// or
if (result.has_value()) { ... }  // Also works
```

## API Mapping

| thread::result<T> | common::Result<T> | Notes |
|-------------------|-------------------|-------|
| `has_value()` | `is_ok()` or `has_value()` | Both supported |
| `is_ok()` | `is_ok()` | ✅ Now unified |
| `is_error()` | `is_err()` | ✅ Now unified |
| `value()` | `value()` | |
| `get_error()` | `error()` | |
| `value_or(default)` | `value_or(default)` or `unwrap_or(default)` | |
| `value_or_throw()` | `value()` (throws automatically) | |
| `map(fn)` | `map(fn)` | |
| `and_then(fn)` | `and_then(fn)` | |

### result_void API Mapping

| thread::result_void | common::VoidResult | Notes |
|--------------------|-------------------|-------|
| `has_error()` | `is_err()` | Original API |
| `has_value()` | `is_ok()` | ✅ Added for compatibility |
| `is_ok()` | `is_ok()` | ✅ Added for compatibility |
| `is_error()` | `is_err()` | ✅ Added for compatibility |
| `get_error()` | `error()` | |
| `operator bool()` | `operator bool()` | Returns true if success |

## Error Code Mapping

thread_system error codes map to common_system error codes as follows:

| thread::error_code | common::error_code |
|--------------------|---------------------|
| `success` | (no error) |
| `unknown_error` | `-1` |
| `operation_canceled` | `-2` |
| `operation_timeout` | `-3` |
| `not_implemented` | `-4` |
| `invalid_argument` | `-5` |
| (thread-specific codes) | (use custom codes) |

## Performance Considerations

Phase 1 implementation shows:
- ✅ No performance regression
- ✅ Zero-cost abstraction when `THREAD_HAS_COMMON_RESULT` is not defined
- ✅ Minimal overhead (error conversion only) when defined

## Benefits of Migration

1. **Unified Error Handling**: Consistent error handling across all kcenon systems
2. **Better Type Safety**: `common::Result<T>` has stronger compile-time guarantees
3. **Richer API**: Additional methods like `is_err()`, `unwrap_or()`, etc.
4. **Better Documentation**: Comprehensive Rust-style API documentation
5. **Future-Proof**: Aligned with ecosystem direction

## Timeline

| Phase | Version | Date | Status |
|-------|---------|------|--------|
| Phase 1 | 0.1.0 | 2025-11-09 | ✅ Complete |
| Phase 2 | Current | 2025-12-16 | 🔄 In Progress |
| Phase 3 | Next major | TBD | Planned |

## Support

For questions or issues during migration:
1. Check this guide
2. See `common_system/include/kcenon/common/patterns/result.h` for API reference
3. Consult IMPROVEMENT_PLAN.md for implementation details

## Breaking Changes Summary

### Phase 1 (Complete) - No Breaking Changes
- Fully backward compatible
- Existing code works without modification

### Phase 2 (Current) - Deprecation Warnings Only
- Compiler warnings for `thread::result<T>` usage when `THREAD_HAS_COMMON_RESULT` is defined
- Still fully functional
- Internal compatibility layer functions suppress warnings to avoid noise

### Phase 3 (Next Major) - Breaking Changes
- `thread::result<T>` removed
- `thread::error` removed
- Must use `common::Result<T>` and `common::error_info`
