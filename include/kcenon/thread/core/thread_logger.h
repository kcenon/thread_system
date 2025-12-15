// BSD 3-Clause License
//
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

namespace kcenon::thread {

/**
 * @enum log_level
 * @brief Logging severity levels
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
 */
class thread_logger {
public:
    /**
     * @brief Get singleton instance
     *
     * Uses intentional leak pattern to avoid static destruction order issues.
     * The logger may be accessed during other singletons' destruction,
     * so we intentionally leak to ensure it remains valid.
     */
    static thread_logger& instance() {
        // Intentionally leak to avoid static destruction order issues
        // Logger may be accessed during other singletons' destruction
        static thread_logger* logger = new thread_logger();
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
     * @brief Enable/disable logging
     */
    void set_enabled(bool enabled) {
        enabled_ = enabled;
    }

    /**
     * @brief Check if logging is enabled
     */
    bool is_enabled() const {
        return enabled_;
    }

    /**
     * @brief Set minimum log level
     */
    void set_level(log_level level) {
        min_level_ = level;
    }

    /**
     * @brief Log a message with context
     *
     * @param level Severity level
     * @param thread_name Thread identifier
     * @param message Log message
     * @param context Additional context (optional)
     */
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
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::cerr << "["
                  << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
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
     * @brief Log error with error code
     */
    template<typename ErrorType>
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
     * @brief Enable lightweight mode (disables all logging for maximum performance)
     *
     * In lightweight mode, all log calls become no-ops with minimal overhead.
     * Useful for performance-critical production deployments where diagnostics
     * are handled externally.
     *
     * @param enabled true to enable lightweight mode, false to use normal logging
     */
    void set_lightweight_mode(bool enabled) {
        lightweight_mode_ = enabled;
        // Disable logging when in lightweight mode
        if (enabled) {
            enabled_ = false;
        }
    }

    /**
     * @brief Check if in lightweight mode
     */
    bool is_lightweight_mode() const {
        return lightweight_mode_;
    }
};

// Convenience macros
#define THREAD_LOG_TRACE(thread, msg, ctx) \
    kcenon::thread::thread_logger::instance().log(\
        kcenon::thread::log_level::trace, thread, msg, ctx)

#define THREAD_LOG_DEBUG(thread, msg, ctx) \
    kcenon::thread::thread_logger::instance().log(\
        kcenon::thread::log_level::debug, thread, msg, ctx)

#define THREAD_LOG_INFO(thread, msg, ctx) \
    kcenon::thread::thread_logger::instance().log(\
        kcenon::thread::log_level::info, thread, msg, ctx)

#define THREAD_LOG_WARN(thread, msg, ctx) \
    kcenon::thread::thread_logger::instance().log(\
        kcenon::thread::log_level::warning, thread, msg, ctx)

#define THREAD_LOG_ERROR(thread, msg, ctx) \
    kcenon::thread::thread_logger::instance().log(\
        kcenon::thread::log_level::error, thread, msg, ctx)

} // namespace kcenon::thread
