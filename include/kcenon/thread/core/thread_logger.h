#pragma once

#include <string>
#include <string_view>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <iostream>

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
     */
    static thread_logger& instance() {
        static thread_logger logger;
        return logger;
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
        if (!enabled_) return;

        std::ostringstream oss;
        oss << "Error code: " << error.code()
            << ", Message: " << error.message();

        log(log_level::error, thread_name, oss.str());
    }

private:
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

    bool enabled_ = true;
    log_level min_level_ = log_level::info;
    std::mutex mutex_;
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
