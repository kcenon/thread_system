#pragma once

/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2024, DongCheol Shin
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file error_handling.h
 * @brief Error codes and utilities for the thread system
 *
 * This file provides thread-system-specific error codes and integration with
 * the common_system Result/VoidResult types for unified error handling.
 *
 * @note As of v3.0, all result types have been unified to use kcenon::common::Result<T>
 *       and kcenon::common::VoidResult. The legacy thread::result<T>, thread::result_void,
 *       and thread::error types have been removed. See docs/ERROR_SYSTEM_MIGRATION_GUIDE.md
 *       for migration instructions.
 */

#include <string>
#include <string_view>
#include <system_error>

// Include common_system Result types
#include <kcenon/common/patterns/result.h>

namespace kcenon::thread {

/**
 * @enum error_code
 * @brief Strongly typed error codes for thread system operations
 *
 * These error codes are specific to thread_system operations and can be
 * converted to common::error_info for use with common::Result<T> and
 * common::VoidResult.
 */
enum class error_code {
    // General errors
    success = 0,
    unknown_error,
    operation_canceled,
    operation_timeout,
    not_implemented,
    invalid_argument,

    // Thread errors
    thread_already_running = 100,
    thread_not_running,
    thread_start_failure,
    thread_join_failure,

    // Queue errors
    queue_full = 200,
    queue_empty,
    queue_stopped,
    queue_busy,  // Queue is temporarily busy with concurrent operations

    // Job errors
    job_creation_failed = 300,
    job_execution_failed,
    job_invalid,

    // Resource errors
    resource_allocation_failed = 400,
    resource_limit_reached,

    // Synchronization errors
    mutex_error = 500,
    deadlock_detected,
    condition_variable_error,

    // IO errors
    io_error = 600,
    file_not_found
};

/**
 * @brief Converts an error_code to a string representation
 * @param code The error code to convert
 * @return A human-readable string describing the error
 */
inline std::string error_code_to_string(error_code code) {
    using map_entry = std::pair<error_code, std::string_view>;
    static constexpr map_entry kMap[] = {
        {error_code::success, "Success"},
        {error_code::unknown_error, "Unknown error"},
        {error_code::operation_canceled, "Operation canceled"},
        {error_code::operation_timeout, "Operation timed out"},
        {error_code::not_implemented, "Not implemented"},
        {error_code::invalid_argument, "Invalid argument"},
        {error_code::thread_already_running, "Thread is already running"},
        {error_code::thread_not_running, "Thread is not running"},
        {error_code::thread_start_failure, "Failed to start thread"},
        {error_code::thread_join_failure, "Failed to join thread"},
        {error_code::queue_full, "Queue is full"},
        {error_code::queue_empty, "Queue is empty"},
        {error_code::queue_stopped, "Queue is stopped"},
        {error_code::queue_busy, "Queue is busy"},
        {error_code::job_creation_failed, "Failed to create job"},
        {error_code::job_execution_failed, "Failed to execute job"},
        {error_code::job_invalid, "Invalid job"},
        {error_code::resource_allocation_failed, "Failed to allocate resource"},
        {error_code::resource_limit_reached, "Resource limit reached"},
        {error_code::mutex_error, "Mutex error"},
        {error_code::deadlock_detected, "Deadlock detected"},
        {error_code::condition_variable_error, "Condition variable error"},
        {error_code::io_error, "I/O error"},
        {error_code::file_not_found, "File not found"},
    };
    for (const auto& [k, v] : kMap) {
        if (k == code) return std::string(v);
    }
    return std::string("Unknown error code");
}

// ============================================================================
// common::Result Integration Utilities
// ============================================================================

/**
 * @brief Convert a thread::error_code to common::error_info
 * @param code The thread-specific error code
 * @param message Optional additional message (defaults to error_code_to_string)
 * @return A common::error_info structure
 *
 * Example:
 * @code
 * auto info = to_error_info(error_code::queue_full, "Custom message");
 * return common::VoidResult(info);
 * @endcode
 */
inline common::error_info to_error_info(error_code code, const std::string& message = "") {
    return common::error_info{
        static_cast<int>(code),
        message.empty() ? error_code_to_string(code) : message,
        "thread_system"
    };
}

/**
 * @brief Create a common::VoidResult error from a thread::error_code
 * @param code The thread-specific error code
 * @param message Optional additional message
 * @return A common::VoidResult in error state
 *
 * Example:
 * @code
 * if (queue_is_full) {
 *     return make_error_result(error_code::queue_full);
 * }
 * return common::ok();
 * @endcode
 */
inline common::VoidResult make_error_result(error_code code, const std::string& message = "") {
    return common::VoidResult(to_error_info(code, message));
}

/**
 * @brief Create a common::Result<T> error from a thread::error_code
 * @tparam T The value type of the result
 * @param code The thread-specific error code
 * @param message Optional additional message
 * @return A common::Result<T> in error state
 *
 * Example:
 * @code
 * common::Result<int> get_value() {
 *     if (not_ready) {
 *         return make_error_result<int>(error_code::not_implemented);
 *     }
 *     return common::Result<int>::ok(42);
 * }
 * @endcode
 */
template<typename T>
common::Result<T> make_error_result(error_code code, const std::string& message = "") {
    return common::Result<T>(to_error_info(code, message));
}

/**
 * @brief Extract thread::error_code from a common::error_info
 * @param info The error_info from a common::Result or common::VoidResult
 * @return The corresponding thread::error_code
 *
 * Example:
 * @code
 * auto result = some_operation();
 * if (result.is_err()) {
 *     auto code = get_error_code(result.error());
 *     if (code == error_code::queue_full) {
 *         // handle queue full case
 *     }
 * }
 * @endcode
 */
inline error_code get_error_code(const common::error_info& info) {
    return static_cast<error_code>(info.code);
}

// ============================================================================
// std::error_code Integration
// ============================================================================

/**
 * @class thread_error_category
 * @brief std::error_category implementation for thread_system errors
 *
 * This allows thread_system error_code to be used with std::error_code,
 * enabling seamless integration with standard library error handling.
 */
class thread_error_category : public std::error_category {
public:
    /**
     * @brief Returns the name of the error category
     */
    [[nodiscard]] const char* name() const noexcept override {
        return "thread_system";
    }

    /**
     * @brief Returns a message for the given error code
     * @param ev The error value
     * @return Human-readable error message
     */
    [[nodiscard]] std::string message(int ev) const override {
        return error_code_to_string(static_cast<error_code>(ev));
    }

    /**
     * @brief Returns the equivalent std::errc for certain error codes
     * @param ev The error value
     * @param condition The std::error_condition to compare against
     * @return true if the error is equivalent to the condition
     */
    [[nodiscard]] bool equivalent(int ev, const std::error_condition& condition) const noexcept override {
        switch (static_cast<error_code>(ev)) {
            case error_code::invalid_argument:
                return condition == std::errc::invalid_argument;
            case error_code::resource_allocation_failed:
            case error_code::resource_limit_reached:
                return condition == std::errc::not_enough_memory;
            case error_code::operation_timeout:
                return condition == std::errc::timed_out;
            case error_code::deadlock_detected:
                return condition == std::errc::resource_deadlock_would_occur;
            default:
                return false;
        }
    }
};

/**
 * @brief Gets the singleton instance of thread_error_category
 * @return Reference to the thread_error_category singleton
 */
inline const std::error_category& thread_category() noexcept {
    static thread_error_category instance;
    return instance;
}

/**
 * @brief Creates a std::error_code from a thread_system error_code
 * @param e The thread_system error_code
 * @return std::error_code representing the error
 */
inline std::error_code make_error_code(error_code e) noexcept {
    return std::error_code(static_cast<int>(e), thread_category());
}

/**
 * @brief Creates a std::error_condition from a thread_system error_code
 * @param e The thread_system error_code
 * @return std::error_condition representing the error
 */
inline std::error_condition make_error_condition(error_code e) noexcept {
    return std::error_condition(static_cast<int>(e), thread_category());
}

} // namespace kcenon::thread

// ============================================================================
// std::error_code specialization (required to be in global namespace)
// ============================================================================

namespace std {
/**
 * @brief Specialization to enable implicit conversion of thread_system::error_code
 *        to std::error_code
 */
template <>
struct is_error_code_enum<kcenon::thread::error_code> : true_type {};
} // namespace std
