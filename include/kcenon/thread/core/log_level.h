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
// Legacy log_level compatibility
// ============================================================================
// Note: Legacy log_level from thread_logger.h is deprecated.
// Use log_level_v2 or common::interfaces::log_level instead.
// See Issue #261 for migration details.

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
