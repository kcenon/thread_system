# THR-004: Execute LOG_LEVEL Unification Plan

**ID**: THR-004
**Category**: CORE
**Priority**: HIGH
**Status**: TODO
**Estimated Duration**: 3-4 days
**Dependencies**: None

---

## Summary

Execute the LOG_LEVEL unification plan to align thread_system with ecosystem standard (ascending order). Resolve incompatibility with common_system/logger_system.

---

## Background

- **Reference Document**: `docs/advanced/LOG_LEVEL_UNIFICATION_PLAN.md` (RFC Status)
- **Current Issue**: Descending order (critical=0, trace=5) vs standard ascending order
- **Impact**: Comparison logic errors, adapter complexity, user confusion

---

## Current vs Target State

### Current (DESCENDING - Non-standard)
```cpp
enum class log_level {
    critical = 0,  // Highest priority
    error = 1,
    warning = 2,
    info = 3,
    debug = 4,
    trace = 5      // Lowest priority
};
```

### Target (ASCENDING - Standard)
```cpp
enum class log_level {
    trace = 0,     // Lowest priority
    debug = 1,
    info = 2,
    warn = 3,      // renamed from warning
    error = 4,
    critical = 5,
    off = 6
};
```

---

## Acceptance Criteria

- [ ] RFC approval obtained
- [ ] Phase 1: `log_level_v2` enum added
- [ ] Phase 1: Old `log_level` marked deprecated
- [ ] Phase 1: Conversion helpers implemented
- [ ] All internal code migrated to v2
- [ ] Examples updated
- [ ] Migration script created
- [ ] Unit tests for both orderings
- [ ] Documentation updated

---

## Implementation Tasks

### Phase 1: Add Compatibility Layer (This ticket)

#### Task 1: Define New Enum
```cpp
// include/kcenon/thread/core/log_level.h
namespace kcenon::thread {

enum class log_level_v2 : uint8_t {
    trace = 0,
    debug = 1,
    info = 2,
    warn = 3,
    error = 4,
    critical = 5,
    off = 6
};

// Deprecate old enum
enum class [[deprecated("Use log_level_v2")]] log_level : uint8_t {
    critical = 0,
    error = 1,
    warning = 2,
    info = 3,
    debug = 4,
    trace = 5
};

} // namespace
```

#### Task 2: Conversion Helpers
```cpp
constexpr log_level_v2 to_v2(log_level old) noexcept {
    constexpr log_level_v2 mapping[] = {
        log_level_v2::critical, // critical=0
        log_level_v2::error,    // error=1
        log_level_v2::warn,     // warning=2
        log_level_v2::info,     // info=3
        log_level_v2::debug,    // debug=4
        log_level_v2::trace     // trace=5
    };
    return mapping[static_cast<size_t>(old)];
}

constexpr log_level to_v1(log_level_v2 new_level) noexcept {
    constexpr log_level mapping[] = {
        log_level::trace,    // trace=0
        log_level::debug,    // debug=1
        log_level::info,     // info=2
        log_level::warning,  // warn=3
        log_level::error,    // error=4
        log_level::critical  // critical=5
    };
    return mapping[static_cast<size_t>(new_level)];
}
```

#### Task 3: Update Logger Interface
```cpp
// thread_logger.h
class logger_interface {
public:
    virtual void log(log_level_v2 level, std::string_view message) = 0;

    // Deprecated compatibility
    [[deprecated]]
    void log(log_level level, std::string_view message) {
        log(to_v2(level), message);
    }
};
```

#### Task 4: Migration Script
```python
# tools/migrate_log_levels.py
# - Find all log_level usage
# - Replace with log_level_v2
# - Reverse comparison operators if needed
```

---

## Files to Modify

- `include/kcenon/thread/core/thread_logger.h` - Add v2 enum
- `include/kcenon/thread/interfaces/logger_interface.h` - Update interface
- `include/kcenon/thread/adapters/common_system_logger_adapter.h` - Simplify
- `src/impl/thread_pool/thread_pool.cpp` - Migrate usage
- `src/core/thread_base.cpp` - Migrate usage
- `examples/` - Update all examples
- `tools/migrate_log_levels.py` - Create script

---

## Test Coverage

```cpp
TEST(LogLevelMigration, ConversionRoundTrip) {
    for (int i = 0; i <= 5; ++i) {
        auto old = static_cast<log_level>(i);
        auto v2 = to_v2(old);
        auto back = to_v1(v2);
        EXPECT_EQ(old, back);
    }
}

TEST(LogLevelMigration, CommonSystemAlignment) {
    EXPECT_EQ(static_cast<int>(log_level_v2::error),
              static_cast<int>(common::log_level::error));
}
```

---

## Success Metrics

| Metric | Target |
|--------|--------|
| API compatibility | 100% backward compatible |
| Deprecation warnings | All old usages flagged |
| Common system alignment | Direct cast works |
| Migration guide | Complete with examples |
