# THR-003: Complete ERROR_SYSTEM_MIGRATION Plan

**ID**: THR-003
**Category**: CORE
**Priority**: HIGH
**Status**: DONE
**Estimated Duration**: 4-5 days
**Dependencies**: None
**Completed**: 2025-11-23

---

## Summary

Standardize error handling across thread_system by completing the ERROR_SYSTEM_MIGRATION plan. Align with common_system error conventions for ecosystem consistency.

---

## Background

- **Current State**: Mixed error handling patterns
- **Target**: Unified error_code/error_category pattern
- **Reference**: `include/kcenon/thread/core/error_handling.h`

---

## Current Error Handling Analysis

### Existing Patterns
```cpp
// Pattern 1: Boolean returns
bool try_push(const T& item);

// Pattern 2: Exception-based
void push(const T& item);  // throws on full

// Pattern 3: Optional returns
std::optional<T> try_pop();

// Pattern 4: error_code (target)
std::error_code push(const T& item);
```

---

## Acceptance Criteria

- [ ] `thread_error_category` defined with all error codes
- [ ] All public APIs return `std::error_code` or `expected<T, error_code>`
- [ ] Legacy APIs marked deprecated with migration path
- [ ] Error codes documented with recovery suggestions
- [ ] Integration tests verify error propagation
- [ ] Example code updated with new error handling

---

## Error Code Design

```cpp
namespace kcenon::thread {

enum class thread_errc {
    success = 0,

    // Queue errors (100-199)
    queue_full = 100,
    queue_empty = 101,
    queue_closed = 102,

    // Thread pool errors (200-299)
    pool_shutdown = 200,
    pool_overload = 201,
    worker_failed = 202,

    // Task errors (300-399)
    task_cancelled = 300,
    task_timeout = 301,
    task_exception = 302,

    // Resource errors (400-499)
    resource_exhausted = 400,
    invalid_argument = 401,
    operation_not_permitted = 402
};

class thread_error_category : public std::error_category {
public:
    const char* name() const noexcept override;
    std::string message(int ev) const override;
};

const std::error_category& thread_category() noexcept;

std::error_code make_error_code(thread_errc e) noexcept;

} // namespace kcenon::thread
```

---

## Migration Tasks

### Phase 1: Add Error Infrastructure
- [ ] Define `thread_errc` enum
- [ ] Implement `thread_error_category`
- [ ] Add `make_error_code` helper

### Phase 2: Update Core APIs
- [ ] `job_queue` - Add error_code overloads
- [ ] `thread_pool` - Add error_code submit variants
- [ ] `typed_thread_pool` - Migrate dispatch methods

### Phase 3: Deprecate Old APIs
- [ ] Mark boolean-returning methods deprecated
- [ ] Provide inline migration helpers
- [ ] Update documentation

### Phase 4: Test & Document
- [ ] Unit tests for all error codes
- [ ] Integration tests for error propagation
- [ ] Migration guide with before/after examples

---

## Files to Modify

- `include/kcenon/thread/core/error_handling.h` - Expand
- `include/kcenon/thread/interfaces/error_handler.h` - Update
- `src/impl/thread_pool/` - Add error variants
- `docs/guides/ERROR_MIGRATION_GUIDE.md` - Create

---

## API Migration Example

### Before
```cpp
if (!pool.try_submit(task)) {
    // Handle failure - reason unknown
    retry_later();
}
```

### After
```cpp
if (auto ec = pool.submit(task); ec) {
    if (ec == thread_errc::pool_shutdown) {
        log_error("Pool is shutting down");
    } else if (ec == thread_errc::queue_full) {
        apply_backpressure();
    }
}
```

---

## Success Metrics

| Metric | Target |
|--------|--------|
| APIs with error_code | 100% |
| Error code coverage | All failure modes |
| Deprecation warnings | All legacy APIs |
| Documentation | Complete migration guide |
