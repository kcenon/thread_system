# Log Level Migration Guide

> **Language:** **English** | [한국어](LOG_LEVEL_MIGRATION_GUIDE_KO.md)

**Version:** 0.1.0
**Date:** 2025-11-26
**Applies to:** thread_system v2.x and later

## Overview

This guide helps you migrate from the legacy `log_level` enum to the new `log_level_v2` enum introduced in thread_system. The new enum uses **ascending order** (trace=0 to critical=5), which aligns with industry standards (syslog, log4j, spdlog) and the rest of the unified_system ecosystem.

## Why Migrate?

### Benefits of `log_level_v2`

1. **Natural Comparison Semantics**: `if (level >= log_level_v2::warn)` works intuitively
2. **Ecosystem Alignment**: Matches logger_system and common_system
3. **Industry Standard**: Follows syslog RFC 5424, log4j, and spdlog conventions
4. **Simplified Adapters**: No conversion needed between systems

### Comparison of Orderings

| Level | Legacy `log_level` | New `log_level_v2` |
|-------|-------------------|-------------------|
| trace | 0 | 0 |
| debug | 1 | 1 |
| info | 2 | 2 |
| warning/warn | 3 | 3 |
| error | 4 | 4 |
| critical | 5 | 5 |
| off | N/A | 6 |

> **Note:** The legacy `log_level` in thread_system already uses ascending order internally. This migration primarily provides explicit documentation, the `off` level, and better interoperability APIs.

## Migration Steps

### Step 1: Include the New Header

```cpp
// Add this include
#include <kcenon/thread/core/log_level.h>
```

### Step 2: Update Type Declarations

**Before:**
```cpp
using kcenon::thread::log_level;

void set_log_level(log_level level);
log_level current_level = log_level::info;
```

**After:**
```cpp
using kcenon::thread::log_level_v2;

void set_log_level(log_level_v2 level);
log_level_v2 current_level = log_level_v2::info;
```

### Step 3: Update Level Comparisons

The new API provides a `should_log()` helper for clearer semantics:

**Before:**
```cpp
if (static_cast<int>(level) >= static_cast<int>(min_level)) {
    // Log the message
}
```

**After:**
```cpp
if (should_log(level, min_level)) {
    // Log the message
}

// Or using natural comparison
if (level >= min_level) {
    // Log the message
}
```

### Step 4: Update String Parsing

**Before:**
```cpp
// Custom parsing logic
log_level parse_level(const std::string& str) {
    if (str == "debug") return log_level::debug;
    // ...
}
```

**After:**
```cpp
// Use built-in parser
auto level = kcenon::thread::parse_log_level("debug");
```

### Step 5: Update String Conversion

**Before:**
```cpp
std::string level_to_string(log_level level) {
    switch (level) {
        case log_level::debug: return "DEBUG";
        // ...
    }
}
```

**After:**
```cpp
// Use built-in converter
auto str = kcenon::thread::to_string(log_level_v2::debug);  // Returns "DEBUG"
```

## Conversion Helpers

For gradual migration, conversion helpers are provided:

```cpp
#include <kcenon/thread/core/log_level.h>

using namespace kcenon::thread;

// Convert legacy to new
log_level old_level = log_level::error;
log_level_v2 new_level = to_v2(old_level);

// Convert new to legacy
log_level_v2 new_level = log_level_v2::warn;
log_level old_level = from_v2(new_level);
```

## Using the `off` Level

The new `log_level_v2::off` level allows complete logging suppression:

```cpp
void configure_logging(bool enabled) {
    if (enabled) {
        set_min_level(log_level_v2::trace);  // Log everything
    } else {
        set_min_level(log_level_v2::off);    // Log nothing
    }
}

// Check with should_log
if (should_log(log_level_v2::critical, log_level_v2::off)) {
    // This will NOT log - off disables everything
}
```

## Complete Example

### Before Migration

```cpp
#include <kcenon/thread/thread_logger.h>

using namespace kcenon::thread;

class MyLogger {
public:
    void log(log_level level, const std::string& message) {
        if (static_cast<int>(level) >= static_cast<int>(min_level_)) {
            // Log the message
            std::cout << "[" << level_to_string(level) << "] " << message << "\n";
        }
    }

    void set_min_level(log_level level) {
        min_level_ = level;
    }

private:
    log_level min_level_ = log_level::info;

    std::string level_to_string(log_level level) {
        switch (level) {
            case log_level::trace: return "TRACE";
            case log_level::debug: return "DEBUG";
            case log_level::info: return "INFO";
            case log_level::warning: return "WARN";
            case log_level::error: return "ERROR";
            case log_level::critical: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }
};
```

### After Migration

```cpp
#include <kcenon/thread/core/log_level.h>

using namespace kcenon::thread;

class MyLogger {
public:
    void log(log_level_v2 level, const std::string& message) {
        if (should_log(level, min_level_)) {
            std::cout << "[" << to_string(level) << "] " << message << "\n";
        }
    }

    void set_min_level(log_level_v2 level) {
        min_level_ = level;
    }

    void disable_logging() {
        min_level_ = log_level_v2::off;
    }

private:
    log_level_v2 min_level_ = log_level_v2::info;
};
```

## Interoperability with Other Systems

### With logger_system

```cpp
#include <kcenon/thread/core/log_level.h>
#include <kcenon/logger/log_level.h>

// Direct cast works because both use ascending order
auto thread_level = kcenon::thread::log_level_v2::error;
auto logger_level = static_cast<kcenon::logger::log_level>(
    static_cast<int>(thread_level)
);
```

### With common_system

```cpp
#include <kcenon/thread/core/log_level.h>
#include <kcenon/common/log_level.h>

// Compatible orderings allow direct comparison
assert(static_cast<int>(thread::log_level_v2::error) ==
       static_cast<int>(common::log_level::error));
```

## FAQ

### Q: Is the legacy `log_level` deprecated?

A: Currently, both enums are available. The legacy enum will be deprecated in a future minor version and removed in the next major version (v3.0).

### Q: Do I need to migrate immediately?

A: No. The legacy enum continues to work. However, we recommend migrating for:
- New code
- Code that interacts with logger_system or common_system
- Code where you want clearer comparison semantics

### Q: What about `warning` vs `warn`?

A: `log_level_v2` uses `warn` (matching spdlog/log4j conventions). The parser accepts both `warning` and `warn` for input.

### Q: How do I handle the `off` level with legacy code?

A: When converting `log_level_v2::off` to legacy, it maps to `critical`. Use the new API if you need the `off` functionality.

## Timeline

| Phase | Version | Status |
|-------|---------|--------|
| Phase 1: Add `log_level_v2` | v2.x (current) | Complete |
| Phase 2: Documentation & Examples | v2.x | In Progress |
| Phase 3: Deprecate legacy | v2.x+1 | Planned |
| Phase 4: Remove legacy | v3.0 | Planned |

## Related Documents

- [LOG_LEVEL_UNIFICATION_PLAN.md](../advanced/LOG_LEVEL_UNIFICATION_PLAN.md)
- [ERROR_SYSTEM_MIGRATION_GUIDE.md](../advanced/ERROR_SYSTEM_MIGRATION_GUIDE.md)
- [logger_system LOG_LEVEL_SEMANTIC_STANDARD.md](../../../logger_system/docs/advanced/LOG_LEVEL_SEMANTIC_STANDARD.md)

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-11-26 | Initial release |
