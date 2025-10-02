/*
 * BSD 3-Clause License
 * Copyright (c) 2025, kcenon
 */

#pragma once

#include <memory>
#include <string>

// Check if common_system is available
#ifdef BUILD_WITH_COMMON_SYSTEM
#include <kcenon/common/interfaces/logger_interface.h>
#include <kcenon/common/patterns/result.h>
#endif

#include "../interfaces/logger_interface.h"

namespace kcenon::thread::adapters {

#ifdef BUILD_WITH_COMMON_SYSTEM

/**
 * @brief Adapter to expose thread_system logger as common::interfaces::ILogger
 *
 * This adapter allows thread_system's logger to be used
 * through the standard common_system logger interface.
 */
class common_system_logger_adapter : public ::common::interfaces::ILogger {
public:
    /**
     * @brief Construct adapter with thread_system logger
     * @param logger Shared pointer to thread_system logger
     */
    explicit common_system_logger_adapter(
        std::shared_ptr<logger_interface> logger)
        : logger_(logger) {}

    ~common_system_logger_adapter() override = default;

    /**
     * @brief Log a message with specified level
     */
    ::common::VoidResult log(
        ::common::interfaces::log_level level,
        const std::string& message) override {
        if (!logger_) {
            return ::common::VoidResult(
                ::common::error_info(1, "Logger not initialized", "thread_system"));
        }

        // Convert common log level to thread_system log level
        auto thread_level = convert_level_to_thread(level);
        logger_->log(thread_level, message);
        return ::common::VoidResult(std::monostate{});
    }

    /**
     * @brief Log a message with source location information
     */
    ::common::VoidResult log(
        ::common::interfaces::log_level level,
        const std::string& message,
        const std::string& file,
        int line,
        const std::string& function) override {
        if (!logger_) {
            return ::common::VoidResult(
                ::common::error_info(1, "Logger not initialized", "thread_system"));
        }

        auto thread_level = convert_level_to_thread(level);
        logger_->log(thread_level, message, file, line, function);
        return ::common::VoidResult(std::monostate{});
    }

    /**
     * @brief Log a structured entry
     */
    ::common::VoidResult log(
        const ::common::interfaces::log_entry& entry) override {
        if (!logger_) {
            return ::common::VoidResult(
                ::common::error_info(1, "Logger not initialized", "thread_system"));
        }

        auto thread_level = convert_level_to_thread(entry.level);
        logger_->log(thread_level, entry.message,
                    entry.file, entry.line, entry.function);
        return ::common::VoidResult(std::monostate{});
    }

    /**
     * @brief Check if logging is enabled for the specified level
     */
    bool is_enabled(::common::interfaces::log_level level) const override {
        if (!logger_) {
            return false;
        }
        auto thread_level = convert_level_to_thread(level);
        return logger_->is_enabled(thread_level);
    }

    /**
     * @brief Set the minimum log level
     */
    ::common::VoidResult set_level(
        ::common::interfaces::log_level level) override {
        // Thread_system logger doesn't have set_level in interface
        // Store it locally for filtering
        min_level_ = level;
        return ::common::VoidResult(std::monostate{});
    }

    /**
     * @brief Get the current minimum log level
     */
    ::common::interfaces::log_level get_level() const override {
        return min_level_;
    }

    /**
     * @brief Flush any buffered log messages
     */
    ::common::VoidResult flush() override {
        if (!logger_) {
            return ::common::VoidResult(
                ::common::error_info(1, "Logger not initialized", "thread_system"));
        }
        logger_->flush();
        return ::common::VoidResult(std::monostate{});
    }

private:
    std::shared_ptr<logger_interface> logger_;
    ::common::interfaces::log_level min_level_ = ::common::interfaces::log_level::info;

    /**
     * @brief Convert common log level to thread_system log level
     */
    static log_level convert_level_to_thread(::common::interfaces::log_level level) {
        switch(level) {
            case ::common::interfaces::log_level::trace:
                return log_level::trace;
            case ::common::interfaces::log_level::debug:
                return log_level::debug;
            case ::common::interfaces::log_level::info:
                return log_level::info;
            case ::common::interfaces::log_level::warning:
                return log_level::warning;
            case ::common::interfaces::log_level::error:
                return log_level::error;
            case ::common::interfaces::log_level::critical:
                return log_level::critical;
            case ::common::interfaces::log_level::off:
            default:
                return log_level::info;
        }
    }
};

/**
 * @brief Adapter to expose common::interfaces::ILogger as thread_system logger
 * @deprecated This reverse adapter will be removed in Phase 3
 *
 * Phase 2 Note: Bidirectional adapters create circular conversion risk.
 * This adapter allows a common_system logger to be used
 * through the thread_system logger interface.
 *
 * MIGRATION: Use common::interfaces::ILogger directly instead of wrapping it.
 * Reverse adapters (Common→System) are being phased out in favor of
 * native common interface adoption.
 *
 * Will be removed in Phase 3 (Adapter Optimization)
 */
class [[deprecated("Will be removed in Phase 3. Use common::interfaces::ILogger directly")]] logger_from_common_adapter : public logger_interface {
public:
    /**
     * @brief Construct adapter with common logger
     * @param logger Shared pointer to common logger
     */
    explicit logger_from_common_adapter(
        std::shared_ptr<::common::interfaces::ILogger> logger)
        : logger_(logger) {}

    ~logger_from_common_adapter() override = default;

    /**
     * @brief Log a message with specified level
     */
    void log(log_level level, const std::string& message) override {
        if (!logger_) {
            return;
        }

        auto common_level = convert_level_to_common(level);
        logger_->log(common_level, message);
    }

    /**
     * @brief Log a message with source location information
     */
    void log(log_level level,
             const std::string& message,
             const std::string& file,
             int line,
             const std::string& function) override {
        if (!logger_) {
            return;
        }

        auto common_level = convert_level_to_common(level);
        logger_->log(common_level, message, file, line, function);
    }

    /**
     * @brief Check if logging is enabled for the specified level
     */
    bool is_enabled(log_level level) const override {
        if (!logger_) {
            return false;
        }
        auto common_level = convert_level_to_common(level);
        return logger_->is_enabled(common_level);
    }

    /**
     * @brief Flush any buffered log messages
     */
    void flush() override {
        if (logger_) {
            logger_->flush();
        }
    }

private:
    std::shared_ptr<::common::interfaces::ILogger> logger_;

    /**
     * @brief Convert thread_system log level to common log level
     */
    static ::common::interfaces::log_level convert_level_to_common(log_level level) {
        switch(level) {
            case log_level::trace:
                return ::common::interfaces::log_level::trace;
            case log_level::debug:
                return ::common::interfaces::log_level::debug;
            case log_level::info:
                return ::common::interfaces::log_level::info;
            case log_level::warning:
                return ::common::interfaces::log_level::warning;
            case log_level::error:
                return ::common::interfaces::log_level::error;
            case log_level::critical:
                return ::common::interfaces::log_level::critical;
            default:
                return ::common::interfaces::log_level::info;
        }
    }
};

#endif // BUILD_WITH_COMMON_SYSTEM

} // namespace kcenon::thread::adapters