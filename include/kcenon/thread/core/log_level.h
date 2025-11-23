// BSD 3-Clause License
// Copyright (c) 2024, kcenon

#pragma once

#include <cstdint>
#include <string_view>

namespace kcenon::thread {

/**
 * @enum log_level_v2
 * @brief Logging severity levels with explicit ascending values
 *
 * Standard ascending order (matches syslog, log4j, spdlog conventions):
 * - Lower values = less severe (trace)
 * - Higher values = more severe (critical)
 *
 * This enables natural comparison: if (level >= log_level_v2::warn) { ... }
 */
enum class log_level_v2 : uint8_t {
    trace = 0,      ///< Finest-grained informational events
    debug = 1,      ///< Fine-grained informational events for debugging
    info = 2,       ///< Informational messages highlighting progress
    warn = 3,       ///< Potentially harmful situations
    error = 4,      ///< Error events that might still allow continuation
    critical = 5,   ///< Severe error events that lead to termination
    off = 6         ///< Special level to disable logging
};

/**
 * @brief Convert log_level_v2 to string representation
 */
constexpr std::string_view to_string(log_level_v2 level) noexcept {
    switch (level) {
        case log_level_v2::trace:    return "TRACE";
        case log_level_v2::debug:    return "DEBUG";
        case log_level_v2::info:     return "INFO";
        case log_level_v2::warn:     return "WARN";
        case log_level_v2::error:    return "ERROR";
        case log_level_v2::critical: return "CRITICAL";
        case log_level_v2::off:      return "OFF";
        default:                     return "UNKNOWN";
    }
}

// ============================================================================
// Legacy log_level compatibility (from thread_logger.h)
// ============================================================================

// Forward declaration of legacy log_level (defined in thread_logger.h)
enum class log_level;

/**
 * @brief Convert legacy log_level to log_level_v2
 *
 * Legacy log_level uses implicit enum values (trace=0, debug=1, etc.)
 * which happens to match log_level_v2, so conversion is straightforward.
 */
constexpr log_level_v2 to_v2(log_level old_level) noexcept {
    // Legacy: trace=0, debug=1, info=2, warning=3, error=4, critical=5
    // New:    trace=0, debug=1, info=2, warn=3,    error=4, critical=5
    return static_cast<log_level_v2>(static_cast<uint8_t>(old_level));
}

/**
 * @brief Convert log_level_v2 to legacy log_level
 */
constexpr log_level from_v2(log_level_v2 new_level) noexcept {
    // off level doesn't exist in legacy, map to critical
    if (new_level == log_level_v2::off) {
        return static_cast<log_level>(5); // critical
    }
    return static_cast<log_level>(static_cast<uint8_t>(new_level));
}

/**
 * @brief Check if a log level should be logged given a minimum level
 *
 * With ascending order, we log if message_level >= min_level
 */
constexpr bool should_log(log_level_v2 message_level, log_level_v2 min_level) noexcept {
    if (min_level == log_level_v2::off) {
        return false;
    }
    return static_cast<uint8_t>(message_level) >= static_cast<uint8_t>(min_level);
}

/**
 * @brief Parse string to log_level_v2
 */
inline log_level_v2 parse_log_level(std::string_view str) noexcept {
    if (str == "trace" || str == "TRACE") return log_level_v2::trace;
    if (str == "debug" || str == "DEBUG") return log_level_v2::debug;
    if (str == "info"  || str == "INFO")  return log_level_v2::info;
    if (str == "warn"  || str == "WARN" || str == "warning" || str == "WARNING")
        return log_level_v2::warn;
    if (str == "error" || str == "ERROR") return log_level_v2::error;
    if (str == "critical" || str == "CRITICAL" || str == "fatal" || str == "FATAL")
        return log_level_v2::critical;
    if (str == "off" || str == "OFF") return log_level_v2::off;
    return log_level_v2::info; // default
}

} // namespace kcenon::thread
