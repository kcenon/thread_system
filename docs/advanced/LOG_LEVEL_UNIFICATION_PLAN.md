# Log Level Enum Unification Plan

**Version:** 1.0
**Date:** 2025-11-09
**Status:** RFC (Request for Comments)

## Problem Statement

Currently, `thread_system` and `logger_system`/`common_system` use **incompatible log level enum orderings**:

### Current State

```cpp
// thread_system (DESCENDING order)
enum class log_level {
    critical = 0,  // Highest priority
    error = 1,
    warning = 2,
    info = 3,
    debug = 4,
    trace = 5      // Lowest priority
};

// logger_system / common_system (ASCENDING order - STANDARD)
enum class log_level {
    trace = 0,     // Lowest priority
    debug = 1,
    info = 2,
    warn = 3,
    error = 4,
    fatal = 5      // Highest priority
};
```

### Impact

1. **Comparison Logic Errors**:
   ```cpp
   // WRONG: Comparison fails with thread_system ordering
   if (msg_level >= min_level) { ... }
   ```

2. **Adapter Complexity**:
   ```cpp
   // Required conversion in every adapter
   auto to_common_level(thread::log_level level) {
       return static_cast<common::log_level>(5 - static_cast<int>(level));
   }
   ```

3. **User Confusion**: Same semantic level has different numeric values
4. **Integration Overhead**: Template metaprogramming needed for dual ordering support

## Proposed Solution

### Option A: Adopt Standard Ascending Order (RECOMMENDED)

**Rationale:**
- Industry standard (syslog, log4j, spdlog all use ascending)
- Matches common_system and logger_system
- Natural comparison semantics (`level >= threshold`)
- Eliminates adapter complexity

**Change:**
```cpp
// thread_system (NEW - matches ecosystem standard)
enum class log_level {
    trace = 0,
    debug = 1,
    info = 2,
    warn = 3,
    error = 4,
    critical = 5,  // Note: renamed from fatal for consistency
    off = 6        // Special value to disable logging
};
```

### Option B: Keep Descending with Explicit Comparison

**Alternative (NOT RECOMMENDED):**
- Keep current ordering
- Provide explicit comparison operators
- More complex, non-standard

## Migration Plan

### Phase 1: Add Compatibility Layer (Next Minor Version)

```cpp
namespace kcenon::thread {
    // New standard ordering
    enum class log_level_v2 {
        trace = 0,
        debug = 1,
        info = 2,
        warn = 3,
        error = 4,
        critical = 5
    };

    // Deprecate old enum
    enum class [[deprecated("Use log_level_v2. Will be removed in next major version")]] log_level {
        critical = 0,
        error = 1,
        warning = 2,
        info = 3,
        debug = 4,
        trace = 5
    };

    // Conversion helpers
    constexpr log_level_v2 to_v2(log_level old) {
        return static_cast<log_level_v2>(5 - static_cast<int>(old));
    }

    constexpr log_level to_v1(log_level_v2 new_level) {
        return static_cast<log_level>(5 - static_cast<int>(new_level));
    }
}
```

### Phase 2: Deprecation Period (6 months)

- Both enums available
- Compiler warnings guide users
- Automatic conversion provided
- Update all examples and documentation

### Phase 3: Complete Migration (Next Major Version)

```cpp
namespace kcenon::thread {
    // Only standard ordering remains
    enum class log_level {
        trace = 0,
        debug = 1,
        info = 2,
        warn = 3,
        error = 4,
        critical = 5,
        off = 6
    };
}
```

## Breaking Changes

### For Users

**Before:**
```cpp
using kcenon::thread::log_level;

void set_min_level(log_level level) {
    // Descending comparison
    if (static_cast<int>(level) <= static_cast<int>(log_level::warning)) {
        // ...
    }
}
```

**After:**
```cpp
using kcenon::thread::log_level;

void set_min_level(log_level level) {
    // Standard ascending comparison
    if (level >= log_level::warn) {  // Natural comparison!
        // ...
    }
}
```

### Migration Script

Automated migration tool provided:

```cpp
// tools/migrate_log_levels.py
# Converts old log_level usage to log_level_v2
# - Finds all enum usage
# - Reverses comparison operators
# - Updates enum values
```

## Benefits

1. **Eliminates Adapter Overhead**: No more conversion needed
2. **Natural Comparisons**: Standard `>=` semantics work correctly
3. **Ecosystem Alignment**: Matches logger_system, common_system
4. **Industry Standard**: Follows syslog, log4j, spdlog conventions
5. **Reduced Complexity**: Simpler template code, fewer #ifdefs

## Risks and Mitigation

| Risk | Impact | Mitigation |
|------|--------|------------|
| Existing code breaks | High | 6-month deprecation + migration tools |
| Comparison logic inverted | Medium | Compiler warnings + thorough testing |
| Integration confusion | Low | Clear documentation + examples |

## Timeline

| Phase | Version | Duration | Status |
|-------|---------|----------|--------|
| Phase 1 | Next minor (2.x) | Week 1-2 | Planned |
| Phase 2 | Deprecation period | 6 months | Planned |
| Phase 3 | Next major (3.0) | Week 1 | Planned |

## Implementation Checklist

### Phase 1
- [ ] Define `log_level_v2` enum
- [ ] Mark old `log_level` deprecated
- [ ] Add conversion helpers
- [ ] Update internal thread_system code to use v2
- [ ] Provide backward compatibility shims
- [ ] Add comprehensive tests

### Phase 2
- [ ] Update all examples to use v2
- [ ] Update documentation
- [ ] Create migration guide
- [ ] Provide automated migration script
- [ ] Monitor deprecation warnings in ecosystem

### Phase 3
- [ ] Remove old `log_level` enum
- [ ] Rename `log_level_v2` to `log_level`
- [ ] Update all documentation
- [ ] Release major version

## API Mapping

| Old (Descending) | New (Ascending) | Common/Logger |
|------------------|-----------------|---------------|
| `critical = 0` | `critical = 5` | `fatal = 5` |
| `error = 1` | `error = 4` | `error = 4` |
| `warning = 2` | `warn = 3` | `warn = 3` |
| `info = 3` | `info = 2` | `info = 2` |
| `debug = 4` | `debug = 1` | `debug = 1` |
| `trace = 5` | `trace = 0` | `trace = 0` |

## Testing Strategy

### Conversion Tests
```cpp
TEST(LogLevelMigration, OldToNewConversion) {
    EXPECT_EQ(to_v2(log_level::critical), log_level_v2::critical);
    EXPECT_EQ(to_v2(log_level::trace), log_level_v2::trace);
}
```

### Comparison Tests
```cpp
TEST(LogLevelMigration, NaturalComparison) {
    // New standard ordering
    EXPECT_TRUE(log_level_v2::error >= log_level_v2::warn);
    EXPECT_TRUE(log_level_v2::trace < log_level_v2::debug);
}
```

### Integration Tests
```cpp
TEST(LogLevelMigration, CommonSystemInterop) {
    auto thread_level = log_level_v2::error;
    auto common_level = common::log_level::error;
    EXPECT_EQ(static_cast<int>(thread_level), static_cast<int>(common_level));
}
```

## Decision

**Status**: Awaiting approval

**Recommendation**: Adopt Option A (Standard Ascending Order) following the 3-phase migration plan.

**Approvers**:
- [ ] Lead Architect
- [ ] thread_system Maintainer
- [ ] logger_system Maintainer

## References

1. **Industry Standards**:
   - syslog RFC 5424
   - log4j severity levels
   - spdlog level ordering

2. **Related Documents**:
   - ERROR_SYSTEM_MIGRATION_GUIDE.md
   - IMPROVEMENT_PLAN.md (Sprint 3)
   - logger_system/docs/LOG_LEVEL_SEMANTIC_STANDARD.md

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-11-09 | Initial RFC |
