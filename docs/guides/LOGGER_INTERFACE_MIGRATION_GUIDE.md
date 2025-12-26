# Logger Interface Migration Guide

> **Language:** **English** | [한국어](LOGGER_INTERFACE_MIGRATION_GUIDE_KO.md)

**Version:** 0.1.0
**Date:** 2025-12-06
**Applies to:** thread_system v0.1.x and later

## Overview

This guide helps you migrate from the deprecated `kcenon::thread::logger_interface` to the unified `kcenon::common::interfaces::ILogger` from common_system.

> **⚠️ Breaking Change (v3.0.0):** The thread_system's local logger interface has been **removed** as of v3.0.0 (Issue #311). If you are using the old interface, you must migrate to `common::interfaces::ILogger` immediately.

## Why Migrate?

### Problems with thread_system's logger_interface

1. **Code Duplication**: Duplicates the logging interface already provided by common_system
2. **Maintenance Burden**: Two interfaces to maintain with diverging features
3. **Incompatible Types**: Cannot pass thread_system loggers to common_system without adapters
4. **Inverted Log Levels**: Uses critical=0...trace=5 (opposite of industry standards)

### Benefits of common_system's ILogger

1. **Unified Interface**: Single interface across all systems (thread_system, logger_system, etc.)
2. **Result-based Error Handling**: Returns `VoidResult` for consistent error handling
3. **Standard Log Level Ordering**: Uses trace=0...critical=5 (matches spdlog, log4j, syslog)
4. **Extended Features**: Named loggers, ILoggerRegistry, structured logging support
5. **Better Integration**: Seamless interoperability between systems

## Deprecation Timeline

| Phase | Version | Status | Description |
|-------|---------|--------|-------------|
| Phase 1 | v1.x | Completed | Deprecation warnings added |
| Phase 2 | v1.x | Completed | Migration documentation |
| Phase 3 | v3.0 | **Completed** | File removed entirely (Issue #311) |

## Migration Steps

### Step 1: Update Include Statements

**Before:**
```cpp
#include <kcenon/thread/interfaces/logger_interface.h>
```

**After:**
```cpp
#include <kcenon/common/interfaces/logger_interface.h>
```

### Step 2: Update Namespace and Type References

**Before:**
```cpp
using kcenon::thread::logger_interface;
using kcenon::thread::logger_registry;
using kcenon::thread::log_level;

class MyLogger : public logger_interface {
    void log(log_level level, const std::string& message) override;
    void log(log_level level, const std::string& message,
             const std::string& file, int line, const std::string& function) override;
    bool is_enabled(log_level level) const override;
    void flush() override;
};
```

**After:**
```cpp
using kcenon::common::interfaces::ILogger;
using kcenon::common::interfaces::ILoggerRegistry;
using kcenon::common::interfaces::log_level;

class MyLogger : public ILogger {
    void log(log_level level, const std::string& message) override;
    void log(log_level level, const std::string& message,
             const std::string& file, int line, const std::string& function) override;
    bool is_enabled(log_level level) const override;
    void flush() override;
};
```

### Step 3: Update Log Level Values

The log level ordering is **inverted** between the two interfaces:

| Level | thread::log_level | common::log_level |
|-------|-------------------|-------------------|
| trace | 5 | 0 |
| debug | 4 | 1 |
| info | 3 | 2 |
| warning | 2 | 3 |
| error | 1 | 4 |
| critical | 0 | 5 |

**Before (thread_system - inverted ordering):**
```cpp
// Higher severity = lower value (non-standard)
if (static_cast<int>(level) <= static_cast<int>(min_level)) {
    // Log the message
}
```

**After (common_system - standard ordering):**
```cpp
// Higher severity = higher value (standard)
if (level >= min_level) {
    // Log the message
}
```

### Step 4: Update Comparisons

**Before:**
```cpp
// thread_system: critical=0, trace=5
if (level <= log_level::warning) {
    // This logs warning, error, and critical
}
```

**After:**
```cpp
// common_system: trace=0, critical=5
if (level >= log_level::warning) {
    // This logs warning, error, and critical
}
```

### Step 5: Update Registry Usage

**Before:**
```cpp
// Set global logger
kcenon::thread::logger_registry::set_logger(my_logger);

// Get global logger
auto logger = kcenon::thread::logger_registry::get_logger();

// Clear logger
kcenon::thread::logger_registry::clear_logger();
```

**After:**
```cpp
// Use ILoggerRegistry for named loggers
auto& registry = kcenon::common::interfaces::ILoggerRegistry::instance();

// Register with a name
registry.register_logger("my_app", my_logger);

// Get by name
auto logger = registry.get_logger("my_app");

// Get or create default
auto default_logger = registry.get_default_logger();
```

### Step 6: Update Macros (if used)

The `THREAD_LOG_*` macros are deprecated. Replace with direct logger calls or your own macros.

**Before:**
```cpp
THREAD_LOG_ERROR("Something went wrong");
THREAD_LOG_INFO("Operation completed");
```

**After:**
```cpp
// Option 1: Direct logger calls
if (auto logger = get_logger()) {
    logger->log(log_level::error, "Something went wrong", __FILE__, __LINE__, __FUNCTION__);
}

// Option 2: Define your own macros using ILogger
#define APP_LOG(level, msg) \
    do { \
        if (auto logger = MyApp::get_logger()) { \
            if (logger->is_enabled(level)) { \
                logger->log(level, msg, __FILE__, __LINE__, __FUNCTION__); \
            } \
        } \
    } while(0)
```

## Complete Migration Example

### Before Migration

```cpp
#include <kcenon/thread/interfaces/logger_interface.h>

namespace myapp {

class ConsoleLogger : public kcenon::thread::logger_interface {
public:
    void log(kcenon::thread::log_level level, const std::string& message) override {
        if (static_cast<int>(level) <= static_cast<int>(min_level_)) {
            std::cout << "[" << level_to_string(level) << "] " << message << "\n";
        }
    }

    void log(kcenon::thread::log_level level, const std::string& message,
             const std::string& file, int line, const std::string& function) override {
        if (static_cast<int>(level) <= static_cast<int>(min_level_)) {
            std::cout << "[" << level_to_string(level) << "] "
                      << file << ":" << line << " " << message << "\n";
        }
    }

    bool is_enabled(kcenon::thread::log_level level) const override {
        return static_cast<int>(level) <= static_cast<int>(min_level_);
    }

    void flush() override {
        std::cout.flush();
    }

private:
    kcenon::thread::log_level min_level_ = kcenon::thread::log_level::info;

    std::string level_to_string(kcenon::thread::log_level level) {
        switch (level) {
            case kcenon::thread::log_level::critical: return "CRIT";
            case kcenon::thread::log_level::error: return "ERROR";
            case kcenon::thread::log_level::warning: return "WARN";
            case kcenon::thread::log_level::info: return "INFO";
            case kcenon::thread::log_level::debug: return "DEBUG";
            case kcenon::thread::log_level::trace: return "TRACE";
            default: return "???";
        }
    }
};

void setup() {
    auto logger = std::make_shared<ConsoleLogger>();
    kcenon::thread::logger_registry::set_logger(logger);
}

} // namespace myapp
```

### After Migration

```cpp
#include <kcenon/common/interfaces/logger_interface.h>

namespace myapp {

class ConsoleLogger : public kcenon::common::interfaces::ILogger {
public:
    void log(kcenon::common::interfaces::log_level level,
             const std::string& message) override {
        if (level >= min_level_) {
            std::cout << "[" << level_to_string(level) << "] " << message << "\n";
        }
    }

    void log(kcenon::common::interfaces::log_level level,
             const std::string& message,
             const std::string& file, int line, const std::string& function) override {
        if (level >= min_level_) {
            std::cout << "[" << level_to_string(level) << "] "
                      << file << ":" << line << " " << message << "\n";
        }
    }

    bool is_enabled(kcenon::common::interfaces::log_level level) const override {
        return level >= min_level_;
    }

    void flush() override {
        std::cout.flush();
    }

private:
    kcenon::common::interfaces::log_level min_level_
        = kcenon::common::interfaces::log_level::info;

    std::string level_to_string(kcenon::common::interfaces::log_level level) {
        switch (level) {
            case kcenon::common::interfaces::log_level::trace: return "TRACE";
            case kcenon::common::interfaces::log_level::debug: return "DEBUG";
            case kcenon::common::interfaces::log_level::info: return "INFO";
            case kcenon::common::interfaces::log_level::warning: return "WARN";
            case kcenon::common::interfaces::log_level::error: return "ERROR";
            case kcenon::common::interfaces::log_level::critical: return "CRIT";
            default: return "???";
        }
    }
};

void setup() {
    auto logger = std::make_shared<ConsoleLogger>();
    auto& registry = kcenon::common::interfaces::ILoggerRegistry::instance();
    registry.set_default_logger(logger);
}

} // namespace myapp
```

## FAQ

### Q: What if I don't have common_system?

A: If your project doesn't use common_system, you have two options:
1. Add common_system as a dependency (recommended)
2. Continue using the deprecated interface until you can migrate

### Q: Will my code still compile after the deprecation?

A: Yes, for v1.x releases. You'll see compiler warnings but the code will work. In v2.0, the deprecated interface will be removed and compilation will fail.

### Q: How do I silence deprecation warnings temporarily?

A: Use compiler-specific pragmas:
```cpp
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

// Your code using deprecated interface

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
```

### Q: Is there an adapter for gradual migration?

A: The `logger_system_adapter` is now **deprecated** (Issue #336). Instead, use the DI-based approach with `ServiceContainer`. See the section below.

## Migrating from logger_system_adapter (Issue #336)

> **⚠️ Deprecation Notice:** `logger_system_adapter` is deprecated as of v0.4.0.0 and will be removed in v0.5.0.0. Use the ServiceContainer approach instead.

### Why Deprecate logger_system_adapter?

The `logger_system_adapter` creates a bidirectional dependency risk:
- thread_system optionally depends on logger_system (`BUILD_WITH_LOGGER_SYSTEM`)
- logger_system optionally depends on thread_system (`LOGGER_HAS_THREAD_SYSTEM`)
- Both enabled creates circular dependency risk

### Migration: From logger_system_adapter to ServiceContainer

**Before (Direct dependency - deprecated):**
```cpp
#ifdef BUILD_WITH_LOGGER_SYSTEM
#include <kcenon/thread/adapters/logger_system_adapter.h>
#include <kcenon/logger/core/logger.h>

// Create logger_system logger
auto logger = std::make_shared<kcenon::logger::logger>(true, 8192);
logger->start();

// Wrap with adapter
auto adapter = std::make_shared<kcenon::thread::adapters::logger_system_adapter>(logger);

// Use adapter
adapter->log(kcenon::common::interfaces::log_level::info, "Message");
#endif
```

**After (DI-based - recommended):**
```cpp
#include <kcenon/common/di/service_container.h>
#include <kcenon/common/interfaces/logger_interface.h>
#include <kcenon/thread/di/service_registration.h>

// Get service container
auto& container = kcenon::common::di::service_container::global();

// Option 1: Register existing ILogger instance
auto logger = create_my_logger();  // Any ILogger implementation
kcenon::thread::di::register_logger_instance(container, logger);

// Option 2: Register factory (lazy creation)
kcenon::thread::di::register_logger_factory(container, []() {
    return create_my_logger();
});

// Resolve and use anywhere
auto resolved = container.resolve<kcenon::common::interfaces::ILogger>();
if (resolved.is_ok()) {
    resolved.value()->log(kcenon::common::interfaces::log_level::info, "Message");
}
```

### CMake Migration

**Before:**
```cmake
option(BUILD_WITH_LOGGER_SYSTEM "Enable logger_system integration" ON)
```

**After:**
```cmake
# BUILD_WITH_LOGGER_SYSTEM is deprecated and OFF by default
# logger_system can provide ILogger implementation via ServiceContainer
# No direct dependency required
```

### Deprecation Timeline

| Version | Action |
|---------|--------|
| v0.4.0.0 | Add deprecation warnings, document migration |
| v0.4.x | Monitor usage, assist migrations |
| v0.5.0.0 | Remove BUILD_WITH_LOGGER_SYSTEM and logger_system_adapter |

## Related Documents

- [LOG_LEVEL_MIGRATION_GUIDE.md](LOG_LEVEL_MIGRATION_GUIDE.md) - Log level enum migration
- [Issue #263](https://github.com/kcenon/thread_system/issues/263) - Deprecation tracking issue
- [Issue #336](https://github.com/kcenon/thread_system/issues/336) - logger_system_adapter deprecation

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.1 | 2025-12-26 | Added Issue #336 migration guide for logger_system_adapter deprecation |
| 1.0 | 2025-12-06 | Initial release |
