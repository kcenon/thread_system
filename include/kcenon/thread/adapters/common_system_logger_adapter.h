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
#include <kcenon/common/adapters/typed_adapter.h>
#endif

#include "../interfaces/logger_interface.h"

namespace kcenon::thread::adapters {

#ifdef BUILD_WITH_COMMON_SYSTEM

/**
 * @brief Adapter to expose thread_system logger as common::interfaces::ILogger
 *
 * This adapter allows thread_system's logger to be used
 * through the standard common_system logger interface.
 *
 * Now inherits from typed_adapter for:
 * - Type safety and wrapper depth tracking
 * - Automatic prevention of infinite adapter chains (max depth: 2)
 * - Unwrap support to access underlying logger_interface
 */
class common_system_logger_adapter
    : public ::common::adapters::typed_adapter<::common::interfaces::ILogger, logger_interface> {
    using base_type = ::common::adapters::typed_adapter<::common::interfaces::ILogger, logger_interface>;
public:
    /**
     * @brief Construct adapter with thread_system logger
     * @param logger Shared pointer to thread_system logger
     */
    explicit common_system_logger_adapter(
        std::shared_ptr<logger_interface> logger)
        : base_type(logger) {}

    ~common_system_logger_adapter() override = default;

    /**
     * @brief Log a message with specified level
     */
    ::common::VoidResult log(
        ::common::interfaces::log_level level,
        const std::string& message) override {
        if (!this->impl_) {
            return ::common::VoidResult(
                ::common::error_info(1, "Logger not initialized", "thread_system"));
        }

        // Convert common log level to thread_system log level
        auto thread_level = convert_level_to_thread(level);
        this->impl_->log(thread_level, message);
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
        if (!this->impl_) {
            return ::common::VoidResult(
                ::common::error_info(1, "Logger not initialized", "thread_system"));
        }

        auto thread_level = convert_level_to_thread(level);
        this->impl_->log(thread_level, message, file, line, function);
        return ::common::VoidResult(std::monostate{});
    }

    /**
     * @brief Log a structured entry
     */
    ::common::VoidResult log(
        const ::common::interfaces::log_entry& entry) override {
        if (!this->impl_) {
            return ::common::VoidResult(
                ::common::error_info(1, "Logger not initialized", "thread_system"));
        }

        auto thread_level = convert_level_to_thread(entry.level);
        this->impl_->log(thread_level, entry.message,
                    entry.file, entry.line, entry.function);
        return ::common::VoidResult(std::monostate{});
    }

    /**
     * @brief Check if logging is enabled for the specified level
     */
    bool is_enabled(::common::interfaces::log_level level) const override {
        if (!this->impl_) {
            return false;
        }
        auto thread_level = convert_level_to_thread(level);
        return this->impl_->is_enabled(thread_level);
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
        if (!this->impl_) {
            return ::common::VoidResult(
                ::common::error_info(1, "Logger not initialized", "thread_system"));
        }
        this->impl_->flush();
        return ::common::VoidResult(std::monostate{});
    }

private:
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

#endif // BUILD_WITH_COMMON_SYSTEM

} // namespace kcenon::thread::adapters