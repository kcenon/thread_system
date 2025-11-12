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
#include <future>
#include <functional>
#include <memory>
#include <any>
#include <chrono>

#ifdef BUILD_WITH_COMMON_SYSTEM
#include <kcenon/common/patterns/result.h>
#endif

namespace kcenon::shared {

#ifdef BUILD_WITH_COMMON_SYSTEM
// Use common::Result types when available
using VoidResult = common::VoidResult;
template<typename T>
using Result = common::Result<T>;
#endif

/**
 * @brief Log level enumeration
 */
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

/**
 * @brief Common logging interface
 */
class ILogger {
public:
    virtual ~ILogger() = default;

    /**
     * @brief Log a message with specified level
     * @param level Log level
     * @param message Message to log
     * @return VoidResult indicating success or error
     *
     * @note Can fail due to I/O errors, disk space issues, or queue overflow
     */
#ifdef BUILD_WITH_COMMON_SYSTEM
    virtual VoidResult log(LogLevel level, std::string_view message) = 0;
#else
    virtual void log(LogLevel level, std::string_view message) = 0;
#endif

    /**
     * @brief Log a formatted message
     * @param level Log level
     * @param format Format string
     * @param args Arguments for formatting
     * @return VoidResult indicating success or error
     */
#ifdef BUILD_WITH_COMMON_SYSTEM
    template<typename... Args>
    VoidResult log_formatted(LogLevel level, std::string_view format, Args&&... args) {
        // Default implementation - derived classes can override
        return log(level, format); // Simplified for now
    }

    /**
     * @brief Flush pending log entries
     * @return VoidResult indicating success or error
     *
     * @note Can fail due to I/O errors during flush operation
     */
    virtual VoidResult flush() { return common::ok(); }
#else
    template<typename... Args>
    void log_formatted(LogLevel level, std::string_view format, Args&&... args) {
        // Default implementation - derived classes can override
        log(level, format); // Simplified for now
    }

    /**
     * @brief Flush pending log entries
     */
    virtual void flush() {}
#endif
};

/**
 * @brief Metrics snapshot structure
 */
struct MetricsSnapshot {
    std::chrono::steady_clock::time_point timestamp;
    std::size_t active_threads{0};
    std::size_t pending_tasks{0};
    double cpu_usage{0.0};
    std::size_t memory_usage_mb{0};
    std::size_t logs_per_second{0};
    double average_task_duration_ms{0.0};
};

/**
 * @brief Common monitoring interface
 */
class IMonitorable {
public:
    virtual ~IMonitorable() = default;

    /**
     * @brief Get current metrics snapshot
     * @return Current metrics
     */
    virtual MetricsSnapshot get_metrics() const = 0;

    /**
     * @brief Enable or disable metrics collection
     * @param enabled True to enable, false to disable
     * @return VoidResult indicating success or error
     *
     * @note Can fail if metrics system cannot be initialized or shutdown
     */
#ifdef BUILD_WITH_COMMON_SYSTEM
    virtual VoidResult set_metrics_enabled(bool enabled) = 0;
#else
    virtual void set_metrics_enabled(bool enabled) = 0;
#endif
};

/**
 * @brief Common task executor interface
 */
class IExecutor {
public:
    virtual ~IExecutor() = default;

    /**
     * @brief Execute a task asynchronously
     * @param task Task to execute
     * @return Result<std::future<void>> indicating success or error
     *
     * @note Can fail if:
     *       - Task queue is full
     *       - Executor is shutting down
     *       - System resources are exhausted
     */
#ifdef BUILD_WITH_COMMON_SYSTEM
    virtual Result<std::future<void>> execute(std::function<void()> task) = 0;
#else
    virtual std::future<void> execute(std::function<void()> task) = 0;
#endif

    /**
     * @brief Execute a task with result
     * @tparam T Result type
     * @param task Task to execute
     * @return Result<std::future<T>> or std::future<T> depending on BUILD_WITH_COMMON_SYSTEM
     */
#ifdef BUILD_WITH_COMMON_SYSTEM
    template<typename T>
    Result<std::future<T>> execute_with_result(std::function<T()> task) {
        // Default implementation using std::async
        try {
            return common::ok(std::async(std::launch::async, std::move(task)));
        } catch (const std::exception& e) {
            return common::make_error<std::future<T>>(
                common::error_codes::THREAD_ERROR_BASE - 1,
                "Failed to launch async task",
                "IExecutor",
                e.what()
            );
        }
    }
#else
    template<typename T>
    std::future<T> execute_with_result(std::function<T()> task) {
        // Default implementation using std::async
        return std::async(std::launch::async, std::move(task));
    }
#endif

    /**
     * @brief Get executor capacity
     * @return Maximum number of concurrent tasks
     */
    virtual std::size_t capacity() const = 0;

    /**
     * @brief Get current load
     * @return Number of active tasks
     */
    virtual std::size_t active_tasks() const = 0;
};

/**
 * @brief Service lifecycle interface
 */
class IService {
public:
    virtual ~IService() = default;

    /**
     * @brief Initialize the service
     * @return VoidResult indicating success or failure with details
     *
     * @note Provides detailed error information about initialization failures
     */
#ifdef BUILD_WITH_COMMON_SYSTEM
    virtual VoidResult initialize() = 0;
#else
    [[deprecated("Use VoidResult version when BUILD_WITH_COMMON_SYSTEM is enabled")]]
    virtual bool initialize() = 0;
#endif

    /**
     * @brief Shutdown the service
     * @return VoidResult indicating success or error
     *
     * @note Can fail during resource cleanup
     */
#ifdef BUILD_WITH_COMMON_SYSTEM
    virtual VoidResult shutdown() = 0;
#else
    virtual void shutdown() = 0;
#endif

    /**
     * @brief Check if service is running
     * @return True if running
     */
    virtual bool is_running() const = 0;

    /**
     * @brief Get service name
     * @return Service name
     */
    virtual std::string name() const = 0;
};

/**
 * @brief Configuration interface
 */
class IConfigurable {
public:
    virtual ~IConfigurable() = default;

    /**
     * @brief Apply configuration
     * @param config Configuration object (any type)
     * @return VoidResult indicating success or error
     *
     * @note Can fail if configuration is invalid or cannot be applied
     */
#ifdef BUILD_WITH_COMMON_SYSTEM
    virtual VoidResult configure(const std::any& config) = 0;
#else
    virtual void configure(const std::any& config) = 0;
#endif

    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    virtual std::any get_configuration() const = 0;

    /**
     * @brief Validate configuration
     * @param config Configuration to validate
     * @return VoidResult with error details if invalid
     *
     * @note Provides specific information about what makes the configuration invalid
     */
#ifdef BUILD_WITH_COMMON_SYSTEM
    virtual VoidResult validate_configuration(const std::any& config) const = 0;
#else
    [[deprecated("Use VoidResult version when BUILD_WITH_COMMON_SYSTEM is enabled")]]
    virtual bool validate_configuration(const std::any& config) const = 0;
#endif
};

} // namespace kcenon::shared