/*
 * BSD 3-Clause License
 * Copyright (c) 2025, kcenon
 */

#pragma once

#include <memory>
#include <string>
#include <string_view>

// Check if both common_system and logger_system are available
#if defined(BUILD_WITH_COMMON_SYSTEM) && defined(BUILD_WITH_LOGGER_SYSTEM)

#include <kcenon/common/interfaces/logger_interface.h>
#include <kcenon/common/patterns/result.h>
#include <kcenon/common/adapters/typed_adapter.h>
#include <kcenon/common/utils/source_location.h>
#include <kcenon/logger/core/logger.h>
#include <kcenon/logger/interfaces/logger_types.h>

namespace kcenon::thread::adapters {

/**
 * @brief Adapter that bridges logger_system to common_system's ILogger
 *
 * This adapter implements the ILogger interface and forwards calls
 * to logger_system's concrete implementation. It enables runtime binding
 * of logger_system's logger to the common ILogger interface.
 *
 * Features:
 * - Type safety via typed_adapter base class
 * - Wrapper depth tracking (max depth: 2)
 * - Log level conversion between common_system and logger_system
 * - Source location information preservation
 * - Thread-safe operation
 *
 * @note This adapter requires both BUILD_WITH_COMMON_SYSTEM and
 *       BUILD_WITH_LOGGER_SYSTEM to be defined.
 *
 * @example
 * @code
 * // Create logger_system logger
 * auto logger = std::make_shared<kcenon::logger::logger>(true, 8192);
 * logger->start();
 *
 * // Create adapter
 * auto adapter = std::make_shared<logger_system_adapter>(logger);
 *
 * // Register globally (if using GlobalLoggerRegistry)
 * // GlobalLoggerRegistry::instance().set(adapter);
 *
 * // Use through ILogger interface
 * adapter->log(common::interfaces::log_level::info, "Application started");
 * @endcode
 */
class logger_system_adapter
    : public ::common::adapters::typed_adapter<
          ::common::interfaces::ILogger,
          ::kcenon::logger::logger> {
    using base_type = ::common::adapters::typed_adapter<
        ::common::interfaces::ILogger,
        ::kcenon::logger::logger>;

public:
    /**
     * @brief Construct adapter with logger_system's logger instance
     * @param logger Shared pointer to logger_system's logger
     */
    explicit logger_system_adapter(
        std::shared_ptr<::kcenon::logger::logger> logger)
        : base_type(std::move(logger)) {}

    ~logger_system_adapter() override = default;

    // Prevent copying
    logger_system_adapter(const logger_system_adapter&) = delete;
    logger_system_adapter& operator=(const logger_system_adapter&) = delete;

    // Allow moving
    logger_system_adapter(logger_system_adapter&&) = default;
    logger_system_adapter& operator=(logger_system_adapter&&) = default;

    /**
     * @brief Log a message with specified level
     * @param level Log level from common_system
     * @param message The message to log
     * @return VoidResult indicating success or error
     */
    ::common::VoidResult log(
        ::common::interfaces::log_level level,
        const std::string& message) override {
        if (!this->impl_) {
            return ::common::VoidResult(
                ::common::error_info(1, "Logger not initialized", "logger_system_adapter"));
        }

        auto logger_level = to_logger_level(level);
        this->impl_->log(logger_level, message);
        return ::common::VoidResult(std::monostate{});
    }

    /**
     * @brief Log a message with C++20 source location
     * @param level Log level from common_system
     * @param message The message to log
     * @param loc Source location (automatically captured at call site)
     * @return VoidResult indicating success or error
     */
    ::common::VoidResult log(
        ::common::interfaces::log_level level,
        std::string_view message,
        const ::common::source_location& loc = ::common::source_location::current()) override {
        if (!this->impl_) {
            return ::common::VoidResult(
                ::common::error_info(1, "Logger not initialized", "logger_system_adapter"));
        }

        auto logger_level = to_logger_level(level);
        this->impl_->log(logger_level, std::string(message),
                         std::string(loc.file_name()),
                         loc.line(),
                         std::string(loc.function_name()));
        return ::common::VoidResult(std::monostate{});
    }

    /**
     * @brief Log a message with legacy source location parameters
     * @param level Log level from common_system
     * @param message The message to log
     * @param file Source file name
     * @param line Source line number
     * @param function Function name
     * @return VoidResult indicating success or error
     *
     * @deprecated This method implements the deprecated ILogger interface method.
     *             Will be removed when common_system v3.0.0 removes the base method.
     *             Use log(level, message, source_location) instead.
     */
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
    ::common::VoidResult log(
        ::common::interfaces::log_level level,
        const std::string& message,
        const std::string& file,
        int line,
        const std::string& function) override {
        if (!this->impl_) {
            return ::common::VoidResult(
                ::common::error_info(1, "Logger not initialized", "logger_system_adapter"));
        }

        auto logger_level = to_logger_level(level);
        this->impl_->log(logger_level, message, file, line, function);
        return ::common::VoidResult(std::monostate{});
    }
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

    /**
     * @brief Log a structured entry
     * @param entry Log entry containing all information
     * @return VoidResult indicating success or error
     */
    ::common::VoidResult log(
        const ::common::interfaces::log_entry& entry) override {
        if (!this->impl_) {
            return ::common::VoidResult(
                ::common::error_info(1, "Logger not initialized", "logger_system_adapter"));
        }

        auto logger_level = to_logger_level(entry.level);
        this->impl_->log(logger_level, entry.message,
                         entry.file, entry.line, entry.function);
        return ::common::VoidResult(std::monostate{});
    }

    /**
     * @brief Check if logging is enabled for the specified level
     * @param level Log level to check
     * @return true if logging is enabled for this level
     */
    bool is_enabled(::common::interfaces::log_level level) const override {
        if (!this->impl_) {
            return false;
        }
        auto logger_level = to_logger_level(level);
        return this->impl_->is_enabled(logger_level);
    }

    /**
     * @brief Set the minimum log level
     * @param level Minimum level for messages to be logged
     * @return VoidResult indicating success or error
     */
    ::common::VoidResult set_level(
        ::common::interfaces::log_level level) override {
        if (!this->impl_) {
            return ::common::VoidResult(
                ::common::error_info(1, "Logger not initialized", "logger_system_adapter"));
        }

        auto logger_level = to_logger_level(level);
        this->impl_->set_min_level(logger_level);
        min_level_ = level;
        return ::common::VoidResult(std::monostate{});
    }

    /**
     * @brief Get the current minimum log level
     * @return Current minimum log level
     */
    ::common::interfaces::log_level get_level() const override {
        if (!this->impl_) {
            return min_level_;
        }
        return from_logger_level(this->impl_->get_min_level());
    }

    /**
     * @brief Flush any buffered log messages
     * @return VoidResult indicating success or error
     */
    ::common::VoidResult flush() override {
        if (!this->impl_) {
            return ::common::VoidResult(
                ::common::error_info(1, "Logger not initialized", "logger_system_adapter"));
        }
        this->impl_->flush();
        return ::common::VoidResult(std::monostate{});
    }

    /**
     * @brief Get the underlying logger_system logger
     * @return Shared pointer to the wrapped logger
     *
     * Convenience method for direct access to logger_system features
     * not exposed through ILogger interface.
     */
    std::shared_ptr<::kcenon::logger::logger> get_logger() const {
        return this->unwrap();
    }

private:
    ::common::interfaces::log_level min_level_ = ::common::interfaces::log_level::info;

    /**
     * @brief Convert common_system log level to logger_system log level
     * @param level common_system log level
     * @return Corresponding logger_system log level
     */
    static ::kcenon::logger::log_level to_logger_level(
        ::common::interfaces::log_level level) {
        switch (level) {
            case ::common::interfaces::log_level::trace:
                return ::kcenon::logger::log_level::trace;
            case ::common::interfaces::log_level::debug:
                return ::kcenon::logger::log_level::debug;
            case ::common::interfaces::log_level::info:
                return ::kcenon::logger::log_level::info;
            case ::common::interfaces::log_level::warning:
                return ::kcenon::logger::log_level::warning;
            case ::common::interfaces::log_level::error:
                return ::kcenon::logger::log_level::error;
            case ::common::interfaces::log_level::critical:
                return ::kcenon::logger::log_level::critical;
            case ::common::interfaces::log_level::off:
                return ::kcenon::logger::log_level::off;
            default:
                return ::kcenon::logger::log_level::info;
        }
    }

    /**
     * @brief Convert logger_system log level to common_system log level
     * @param level logger_system log level
     * @return Corresponding common_system log level
     */
    static ::common::interfaces::log_level from_logger_level(
        ::kcenon::logger::log_level level) {
        switch (level) {
            case ::kcenon::logger::log_level::trace:
                return ::common::interfaces::log_level::trace;
            case ::kcenon::logger::log_level::debug:
                return ::common::interfaces::log_level::debug;
            case ::kcenon::logger::log_level::info:
                return ::common::interfaces::log_level::info;
            case ::kcenon::logger::log_level::warning:
                return ::common::interfaces::log_level::warning;
            case ::kcenon::logger::log_level::error:
                return ::common::interfaces::log_level::error;
            case ::kcenon::logger::log_level::critical:
            case ::kcenon::logger::log_level::fatal:
                return ::common::interfaces::log_level::critical;
            case ::kcenon::logger::log_level::off:
                return ::common::interfaces::log_level::off;
            default:
                return ::common::interfaces::log_level::info;
        }
    }
};

} // namespace kcenon::thread::adapters

#endif // BUILD_WITH_COMMON_SYSTEM && BUILD_WITH_LOGGER_SYSTEM
