// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/**
 * @file thread_logger.h
 * @brief Internal logging interface for the thread system.
 *
 */

#pragma once

#include <string>
#include <string_view>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <iostream>
#include <atomic>
#include <cstdlib>
#include <ctime>

namespace kcenon::thread {

/**
 * @enum log_level
 * @brief Logging severity levels (legacy).
 *
 * @deprecated Since v1.0.0. Use `kcenon::thread::log_level_v2` (from
 * `<kcenon/thread/core/log_level.h>`) or `common::interfaces::log_level`
 * (from common_system) instead. Will be removed in v2.0.0.
 * See Issue #261 for migration details.
 *
 * NOTE: `[[deprecated]]` is applied at the `thread_logger` member-function
 * level (log/set_level/log_error) rather than on this enum itself, because
 * this type is still referenced internally by the SDOF-safe shutdown
 * helpers. Using `log_level` via a deprecated API emits a warning; direct
 * use without the logger does not.
 */
enum class log_level {
    trace,
    debug,
    info,
    warning,
    error,
    critical
};

/**
 * @class thread_logger
 * @brief Structured logger for thread system
 *
 * Provides thread-safe, structured logging with timestamps,
 * thread IDs, and severity levels for better diagnostics.
 *
 * Thread Safety:
 * - All methods are thread-safe
 * - Uses mutex for synchronized output
 * - Lock-free in disabled state
 *
 * SDOF Prevention:
 * - The shutdown handler is registered early during program initialization
 *   via thread_logger_init.cpp, ensuring it runs after all user static
 *   object destructors complete (atexit handlers run in LIFO order)
 * - This guarantees is_shutting_down() returns true during static destruction
 *
 * @see thread_logger_init.cpp for early registration implementation
 */
class thread_logger {
public:
    /**
     * @brief Get singleton instance
     *
     * Uses intentional leak pattern to avoid static destruction order issues.
     * The logger may be accessed during other singletons' destruction,
     * so we intentionally leak to ensure it remains valid.
     *
     * @note The atexit handler for SDOF protection is registered early during
     *       program initialization by thread_logger_init.cpp, not here.
     *       This ensures the handler runs after all user static object
     *       destructors complete, guaranteeing is_shutting_down() returns
     *       true during the destruction phase.
     *
     * @see thread_logger_init.cpp
     */
    static thread_logger& instance() {
        // Intentionally leak to avoid static destruction order issues
        // Logger may be accessed during other singletons' destruction
        static thread_logger* logger = []() {
            auto* ptr = new thread_logger();
            // Note: atexit handler is registered early by thread_logger_init.cpp
            // We register here as well for safety (multiple calls to atexit with
            // the same function are safe per C++ standard - they just add multiple
            // entries, and prepare_shutdown() is idempotent)
            std::atexit(prepare_shutdown);
            return ptr;
        }();
        return *logger;
    }

    /**
     * @brief Prepare for process shutdown
     *
     * Call this before process termination to prevent log calls
     * during static destruction. Once called, all log operations
     * become no-ops.
     */
    static void prepare_shutdown() {
        is_shutting_down_.store(true, std::memory_order_release);
    }

    /**
     * @brief Check if shutdown is in progress
     */
    static bool is_shutting_down() {
        return is_shutting_down_.load(std::memory_order_acquire);
    }

    /**
     * @brief Enable/disable logging.
     *
     * @deprecated Since v1.0.0. Inject a `common::interfaces::ILogger`
     * via `thread_context` and control logging through its implementation
     * instead. Will be removed in v2.0.0.
     */
    [[deprecated(
        "Use 'common::interfaces::ILogger' via thread_context instead. "
        "Will be removed in v2.0.")]]
    void set_enabled(bool enabled) {
        enabled_ = enabled;
    }

    /**
     * @brief Check if logging is enabled.
     *
     * @deprecated Since v1.0.0. Check availability through
     * `thread_context::has_logger()` instead. Will be removed in v2.0.0.
     */
    [[deprecated(
        "Use 'thread_context::has_logger()' instead. "
        "Will be removed in v2.0.")]]
    bool is_enabled() const {
        return enabled_;
    }

    /**
     * @brief Set minimum log level.
     *
     * @deprecated Since v1.0.0. Configure log level on the injected
     * `common::interfaces::ILogger` implementation instead.
     * Will be removed in v2.0.0.
     */
    [[deprecated(
        "Configure log level on 'common::interfaces::ILogger' instead. "
        "Will be removed in v2.0.")]]
    void set_level(log_level level) {
        min_level_ = level;
    }

    /**
     * @brief Log a message with context.
     *
     * @param level Severity level
     * @param thread_name Thread identifier
     * @param message Log message
     * @param context Additional context (optional)
     *
     * @deprecated Since v1.0.0. Use `thread_context::log()` backed by
     * `common::interfaces::ILogger` instead. Will be removed in v2.0.0.
     */
    [[deprecated(
        "Use 'thread_context::log()' with 'common::interfaces::ILogger' "
        "instead. Will be removed in v2.0.")]]
    void log(log_level level, std::string_view thread_name,
             std::string_view message, std::string_view context = "") {
        // Early return during shutdown to avoid accessing potentially destroyed resources
        if (is_shutting_down_.load(std::memory_order_acquire)) {
            return;
        }

        if (!enabled_ || level < min_level_) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        auto now = std::chrono::system_clock::now();
        auto time_t_val = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        // Use thread-safe localtime variant
        std::tm tm_buf{};
#if defined(_MSC_VER) || defined(_WIN32)
        localtime_s(&tm_buf, &time_t_val);
#else
        localtime_r(&time_t_val, &tm_buf);
#endif

        std::cerr << "["
                  << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
                  << "." << std::setfill('0') << std::setw(3) << ms.count()
                  << "] "
                  << "[" << level_to_string(level) << "] "
                  << "[Thread:" << thread_name << "] "
                  << "[TID:" << std::this_thread::get_id() << "] "
                  << message;

        if (!context.empty()) {
            std::cerr << " | Context: " << context;
        }

        std::cerr << std::endl;
    }

    /**
     * @brief Log error with error code.
     *
     * @deprecated Since v1.0.0. Use `thread_context::log()` with a
     * `common::interfaces::ILogger` implementation instead. Will be
     * removed in v2.0.0.
     */
    template<typename ErrorType>
    [[deprecated(
        "Use 'thread_context::log()' with 'common::interfaces::ILogger' "
        "instead. Will be removed in v2.0.")]]
    void log_error(std::string_view thread_name, const ErrorType& error) {
        // Early return during shutdown
        if (is_shutting_down_.load(std::memory_order_acquire) || !enabled_) {
            return;
        }

        std::ostringstream oss;
        oss << "Error code: " << error.code()
            << ", Message: " << error.message();

        log(log_level::error, thread_name, oss.str());
    }

private:
    // Static shutdown flag - checked before any logging operation
    // Uses inline to allow definition in header (C++17)
    static inline std::atomic<bool> is_shutting_down_{false};

    thread_logger() = default;
    ~thread_logger() = default;
    thread_logger(const thread_logger&) = delete;
    thread_logger& operator=(const thread_logger&) = delete;

    static const char* level_to_string(log_level level) {
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

    // Default to warning level to minimize overhead in production
    // Use set_level(log_level::info) or set_level(log_level::debug) for verbose logging
    bool enabled_ = true;
    log_level min_level_ = log_level::warning;
    std::mutex mutex_;

    // Lightweight mode: disable structured logging entirely
    // When enabled, logging becomes a no-op with minimal overhead
    bool lightweight_mode_ = false;

public:
    /**
     * @brief Enable lightweight mode (disables all logging for maximum performance).
     *
     * In lightweight mode, all log calls become no-ops with minimal overhead.
     * Useful for performance-critical production deployments where diagnostics
     * are handled externally.
     *
     * @param enabled true to enable lightweight mode, false to use normal logging
     *
     * @deprecated Since v1.0.0. With `common::interfaces::ILogger`, a
     * null logger in `thread_context` achieves the same effect without
     * this toggle. Will be removed in v2.0.0.
     */
    [[deprecated(
        "Use a null 'common::interfaces::ILogger' in thread_context to "
        "disable logging. Will be removed in v2.0.")]]
    void set_lightweight_mode(bool enabled) {
        lightweight_mode_ = enabled;
        // Disable logging when in lightweight mode
        if (enabled) {
            enabled_ = false;
        }
    }

    /**
     * @brief Check if in lightweight mode.
     *
     * @deprecated Since v1.0.0. See `set_lightweight_mode` deprecation.
     * Will be removed in v2.0.0.
     */
    [[deprecated(
        "Superseded by 'common::interfaces::ILogger' injection. "
        "Will be removed in v2.0.")]]
    bool is_lightweight_mode() const {
        return lightweight_mode_;
    }
};

} // namespace kcenon::thread
