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
 * @brief Modern error handling for the thread system
 *
 * This file provides typed error handling capabilities using a result type pattern
 * that is similar to std::expected from C++23, but can be used with C++20.
 */

#include <string>
#include <optional>
#include <type_traits>
#include <string_view>
#include <ostream>
#include <functional>
#include <stdexcept>
#include <system_error>

// THREAD_HAS_COMMON_RESULT is defined by CMake when common_system is available
// This is more reliable than __has_include alone, which may not respect build configuration
#if defined(THREAD_HAS_COMMON_RESULT) || (defined(BUILD_WITH_COMMON_SYSTEM) && __has_include(<kcenon/common/patterns/result.h>))
#ifndef THREAD_HAS_COMMON_RESULT
#define THREAD_HAS_COMMON_RESULT 1
#endif
#include <kcenon/common/patterns/result.h>
#endif

namespace kcenon::thread {

/**
 * @enum error_code
 * @brief Strongly typed error codes for thread system operations
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

/**
 * @class error
 * @brief Represents an error in the thread system
 * 
 * This class encapsulates an error code and an optional message.
 */
class error {
public:
    /**
     * @brief Constructs an error with a code and optional message
     * @param code The error code
     * @param message Optional detailed message about the error
     */
    explicit error(error_code code, std::string message = {})
        : code_(code), message_(std::move(message)) {}
    
    /**
     * @brief Gets the error code
     * @return The error code
     */
    [[nodiscard]] error_code code() const noexcept { return code_; }
    
    /**
     * @brief Gets the error message
     * @return The error message
     */
    [[nodiscard]] const std::string& message() const noexcept { return message_; }
    
    /**
     * @brief Converts the error to a string representation
     * @return A string describing the error
     */
    [[nodiscard]] std::string to_string() const {
        if (message_.empty()) {
            return error_code_to_string(code_);
        }
        return error_code_to_string(code_) + ": " + message_;
    }
    
    /**
     * @brief Implicit conversion to string
     */
    operator std::string() const {
        return to_string();
    }
    
private:
    error_code code_;
    std::string message_;
};

// Forward declaration of result template
template <typename T>
class result;

/**
 * @brief Wrapper for void result
 * 
 * This class represents a result that doesn't return a value (void),
 * but can indicate success or failure.
 */
class result_void {
public:
    /**
     * @brief Constructs a successful result
     */
    result_void() = default;
    
    /**
     * @brief Constructs a result with an error
     * @param err The error
     */
    result_void(const error& err) : has_error_(true), error_(err) {}
    
    /**
     * @brief Checks if the result contains an error
     * @return true if the result contains an error, false otherwise
     */
    [[nodiscard]] bool has_error() const noexcept { return has_error_; }

    /**
     * @brief Checks if the result is successful (has a value)
     * @return true if successful, false if it contains an error
     * @note Added for API compatibility with common::Result and result<T>
     */
    [[nodiscard]] bool has_value() const noexcept { return !has_error_; }

    /**
     * @brief Checks if the result is successful
     * @return true if successful, false if it contains an error
     * @note Added for API compatibility with common::Result
     */
    [[nodiscard]] bool is_ok() const noexcept { return !has_error_; }

    /**
     * @brief Checks if the result contains an error
     * @return true if the result contains an error, false otherwise
     * @note Added for API compatibility with common::Result
     */
    [[nodiscard]] bool is_error() const noexcept { return has_error_; }

    /**
     * @brief Gets the error
     * @return A reference to the contained error
     */
    [[nodiscard]] const error& get_error() const { return error_; }

    /**
     * @brief Converts to bool for condition checking
     * @return true if successful (no error), false otherwise
     */
    explicit operator bool() const noexcept { return !has_error_; }
    
private:
    bool has_error_ = false;
    error error_{error_code::success, ""};
};

/**
 * @class result
 * @brief A template class representing either a value or an error
 *
 * This class is similar to std::expected from C++23, but can be used with C++20.
 * It holds either a value of type T or an error.
 *
 * @note Phase 1 Migration: When THREAD_HAS_COMMON_RESULT is defined, this class
 *       uses common::Result<T> internally for unified error handling.
 *
 * @deprecated This class will be replaced by kcenon::common::Result<T> in the next
 *             major version. New code should use kcenon::common::Result<T> directly.
 *             See docs/ERROR_SYSTEM_MIGRATION_GUIDE.md for migration instructions.
 */
template <typename T>
class result {
public:
    using value_type = T;
    using error_type = error;

    /**
     * @brief Constructs a result with a value
     * @param value The value to store
     */
    result(T value)
#ifdef THREAD_HAS_COMMON_RESULT
        : impl_(common::Result<T>::ok(std::move(value))) {}
#else
        : has_value_(true), value_(std::move(value)), error_(error_code::success, "") {}
#endif

    /**
     * @brief Constructs a result with an error
     * @param err The error to store
     */
    result(error err)
#ifdef THREAD_HAS_COMMON_RESULT
        : impl_(common::error_info{static_cast<int>(err.code()), err.message(), "thread_system"}) {}
#else
        : has_value_(false), error_(std::move(err)) {}
#endif

    /**
     * @brief Checks if the result contains a value
     * @return true if the result contains a value, false if it contains an error
     */
    [[nodiscard]] explicit operator bool() const noexcept {
#ifdef THREAD_HAS_COMMON_RESULT
        return impl_.is_ok();
#else
        return has_value_;
#endif
    }

    /**
     * @brief Checks if the result contains a value
     * @return true if the result contains a value, false if it contains an error
     */
    [[nodiscard]] bool has_value() const noexcept {
#ifdef THREAD_HAS_COMMON_RESULT
        return impl_.is_ok();
#else
        return has_value_;
#endif
    }

    /**
     * @brief Checks if the result is successful
     * @return true if the result contains a value, false if it contains an error
     * @note Added for API compatibility with common::Result
     */
    [[nodiscard]] bool is_ok() const noexcept {
#ifdef THREAD_HAS_COMMON_RESULT
        return impl_.is_ok();
#else
        return has_value_;
#endif
    }

    /**
     * @brief Checks if the result contains an error
     * @return true if the result contains an error, false if it contains a value
     * @note Added for API compatibility with common::Result
     */
    [[nodiscard]] bool is_error() const noexcept {
#ifdef THREAD_HAS_COMMON_RESULT
        return impl_.is_err();
#else
        return !has_value_;
#endif
    }

    /**
     * @brief Gets the value
     * @return A reference to the contained value
     * @throws std::runtime_error if the result contains an error
     */
    [[nodiscard]] T& value() & {
#ifdef THREAD_HAS_COMMON_RESULT
        if (!impl_.is_ok()) {
            throw std::runtime_error("Cannot access value of an error result");
        }
        return const_cast<T&>(impl_.value());
#else
        if (!has_value_) {
            throw std::runtime_error("Cannot access value of an error result");
        }
        return value_;
#endif
    }

    /**
     * @brief Gets the value
     * @return A const reference to the contained value
     * @throws std::runtime_error if the result contains an error
     */
    [[nodiscard]] const T& value() const & {
#ifdef THREAD_HAS_COMMON_RESULT
        if (!impl_.is_ok()) {
            throw std::runtime_error("Cannot access value of an error result");
        }
        return impl_.value();
#else
        if (!has_value_) {
            throw std::runtime_error("Cannot access value of an error result");
        }
        return value_;
#endif
    }

    /**
     * @brief Gets the value
     * @return An rvalue reference to the contained value
     * @throws std::runtime_error if the result contains an error
     */
    [[nodiscard]] T&& value() && {
#ifdef THREAD_HAS_COMMON_RESULT
        if (!impl_.is_ok()) {
            throw std::runtime_error("Cannot access value of an error result");
        }
        return std::move(const_cast<T&>(impl_.value()));
#else
        if (!has_value_) {
            throw std::runtime_error("Cannot access value of an error result");
        }
        return std::move(value_);
#endif
    }
    
    /**
     * @brief Gets the error
     * @return A reference to the contained error
     * @throws std::runtime_error if the result contains a value
     */
    [[nodiscard]] error& get_error() & {
#ifdef THREAD_HAS_COMMON_RESULT
        if (impl_.is_ok()) {
            throw std::runtime_error("Cannot get error from successful result");
        }
        // Convert common::error_info to thread::error on demand
        cached_error_ = error{
            static_cast<error_code>(impl_.error().code),
            impl_.error().message
        };
        return cached_error_;
#else
        if (has_value_) {
            throw std::runtime_error("Cannot get error from successful result");
        }
        return error_;
#endif
    }

    /**
     * @brief Gets the error
     * @return A const reference to the contained error
     * @throws std::runtime_error if the result contains a value
     */
    [[nodiscard]] const error& get_error() const & {
#ifdef THREAD_HAS_COMMON_RESULT
        if (impl_.is_ok()) {
            throw std::runtime_error("Cannot get error from successful result");
        }
        // Convert common::error_info to thread::error on demand
        cached_error_ = error{
            static_cast<error_code>(impl_.error().code),
            impl_.error().message
        };
        return cached_error_;
#else
        if (has_value_) {
            throw std::runtime_error("Cannot get error from successful result");
        }
        return error_;
#endif
    }

    /**
     * @brief Gets the error
     * @return An rvalue reference to the contained error
     * @throws std::runtime_error if the result contains a value
     */
    [[nodiscard]] error&& get_error() && {
#ifdef THREAD_HAS_COMMON_RESULT
        if (impl_.is_ok()) {
            throw std::runtime_error("Cannot get error from successful result");
        }
        cached_error_ = error{
            static_cast<error_code>(impl_.error().code),
            impl_.error().message
        };
        return std::move(cached_error_);
#else
        if (has_value_) {
            throw std::runtime_error("Cannot get error from successful result");
        }
        return std::move(error_);
#endif
    }
    
    /**
     * @brief Gets the value or a default
     * @param default_value The value to return if the result contains an error
     * @return The contained value or the default
     */
    template <typename U>
    [[nodiscard]] T value_or(U&& default_value) const & {
#ifdef THREAD_HAS_COMMON_RESULT
        return impl_.value_or(static_cast<T>(std::forward<U>(default_value)));
#else
        if (has_value_) {
            return value_;
        }
        return static_cast<T>(std::forward<U>(default_value));
#endif
    }

    /**
     * @brief Gets the value or a default
     * @param default_value The value to return if the result contains an error
     * @return The contained value or the default
     */
    template <typename U>
    [[nodiscard]] T value_or(U&& default_value) && {
#ifdef THREAD_HAS_COMMON_RESULT
        return impl_.value_or(static_cast<T>(std::forward<U>(default_value)));
#else
        if (has_value_) {
            return std::move(value_);
        }
        return static_cast<T>(std::forward<U>(default_value));
#endif
    }

    /**
     * @brief Gets the value or throws an exception
     * @return The contained value
     * @throws std::runtime_error with the error message if the result contains an error
     */
    [[nodiscard]] T value_or_throw() const & {
#ifdef THREAD_HAS_COMMON_RESULT
        if (impl_.is_err()) {
            throw std::runtime_error(impl_.error().message);
        }
        return impl_.value();
#else
        if (has_value_) {
            return value_;
        }
        throw std::runtime_error(error_.to_string());
#endif
    }

    /**
     * @brief Gets the value or throws an exception
     * @return The contained value
     * @throws std::runtime_error with the error message if the result contains an error
     */
    [[nodiscard]] T value_or_throw() && {
#ifdef THREAD_HAS_COMMON_RESULT
        if (impl_.is_err()) {
            throw std::runtime_error(impl_.error().message);
        }
        return std::move(const_cast<T&>(impl_.value()));
#else
        if (has_value_) {
            return std::move(value_);
        }
        throw std::runtime_error(error_.to_string());
#endif
    }
    
    /**
     * @brief Maps the result to another type using a function
     * @param fn The function to apply to the value
     * @return A new result with the mapped value or the original error
     */
    template <typename Fn>
    [[nodiscard]] auto map(Fn&& fn) const -> result<std::invoke_result_t<Fn, const T&>> {
        using ResultType = result<std::invoke_result_t<Fn, const T&>>;

#ifdef THREAD_HAS_COMMON_RESULT
        if (impl_.is_ok()) {
            return ResultType(std::invoke(std::forward<Fn>(fn), impl_.value()));
        }
        // Convert common::error_info to thread::error
        return ResultType(error{
            static_cast<error_code>(impl_.error().code),
            impl_.error().message
        });
#else
        if (has_value_) {
            return ResultType(std::invoke(std::forward<Fn>(fn), value_));
        }
        return ResultType(error_);
#endif
    }
    
    /**
     * @brief Maps the result to another type using a function that returns a result
     * @param fn The function to apply to the value
     * @return A new result with the mapped value or the original error
     */
    template <typename Fn>
    [[nodiscard]] auto and_then(Fn&& fn) const -> std::invoke_result_t<Fn, const T&> {
        using ResultType = std::invoke_result_t<Fn, const T&>;
        static_assert(std::is_same_v<typename ResultType::error_type, error>, "Function must return a result with the same error type");

#ifdef THREAD_HAS_COMMON_RESULT
        if (impl_.is_ok()) {
            return std::invoke(std::forward<Fn>(fn), impl_.value());
        }
        // Convert common::error_info to thread::error
        return ResultType(error{
            static_cast<error_code>(impl_.error().code),
            impl_.error().message
        });
#else
        if (has_value_) {
            return std::invoke(std::forward<Fn>(fn), value_);
        }
        return ResultType(error_);
#endif
    }

private:
#ifdef THREAD_HAS_COMMON_RESULT
    common::Result<T> impl_;
    mutable error cached_error_{error_code::success, ""};  // For get_error() conversion
#else
    bool has_value_ = false;
    T value_{};
    error error_{error_code::success, ""};
#endif
};

/**
 * @brief Specialization of result for void
 */
template <>
class result<void> {
public:
    using value_type = void;
    using error_type = error;
    
    /**
     * @brief Constructs a successful void result
     */
    result() : success_(true) {}
    
    /**
     * @brief Constructs a void result from a result_void
     */
    result(const result_void& r) : success_(!r.has_error()), error_(r.has_error() ? r.get_error() : error{error_code::success, ""}) {}
    
    /**
     * @brief Constructs a result with an error
     * @param err The error to store
     */
    result(error err) : success_(false), error_(std::move(err)) {}
    
    /**
     * @brief Checks if the result is successful
     * @return true if the result is successful, false if it contains an error
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return success_;
    }
    
    /**
     * @brief Checks if the result is successful
     * @return true if the result is successful, false if it contains an error
     */
    [[nodiscard]] bool has_value() const noexcept {
        return success_;
    }

    /**
     * @brief Checks if the result is successful
     * @return true if the result is successful, false if it contains an error
     * @note Added for API compatibility with common::Result
     */
    [[nodiscard]] bool is_ok() const noexcept {
        return success_;
    }

    /**
     * @brief Checks if the result contains an error
     * @return true if the result contains an error, false if successful
     * @note Added for API compatibility with common::Result
     */
    [[nodiscard]] bool is_error() const noexcept {
        return !success_;
    }

    /**
     * @brief Gets the error
     * @return A reference to the contained error
     * @throws std::runtime_error if the result is successful
     */
    [[nodiscard]] error& get_error() & {
        if (success_) {
            throw std::runtime_error("Cannot get error from successful result");
        }
        return error_;
    }

    /**
     * @brief Gets the error
     * @return A const reference to the contained error
     * @throws std::runtime_error if the result is successful
     */
    [[nodiscard]] const error& get_error() const & {
        if (success_) {
            throw std::runtime_error("Cannot get error from successful result");
        }
        return error_;
    }
    
    /**
     * @brief Gets the error
     * @return An rvalue reference to the contained error
     * @throws std::runtime_error if the result is successful
     */
    [[nodiscard]] error&& get_error() && {
        if (success_) {
            throw std::runtime_error("Cannot get error from successful result");
        }
        return std::move(error_);
    }
    
    /**
     * @brief Throws an exception if the result contains an error
     * @throws std::runtime_error with the error message if the result contains an error
     */
    void value_or_throw() const {
        if (!success_) {
            throw std::runtime_error(error_.to_string());
        }
    }
    
    /**
     * @brief Maps the result to another type using a function
     * @param fn The function to apply if successful
     * @return A new result with the mapped value or the original error
     */
    template <typename Fn>
    [[nodiscard]] auto map(Fn&& fn) const -> result<std::invoke_result_t<Fn>> {
        using ResultType = result<std::invoke_result_t<Fn>>;
        
        if (success_) {
            return ResultType(std::invoke(std::forward<Fn>(fn)));
        }
        return ResultType(error_);
    }
    
    /**
     * @brief Maps the result to another type using a function that returns a result
     * @param fn The function to apply if successful
     * @return A new result with the mapped value or the original error
     */
    template <typename Fn>
    [[nodiscard]] auto and_then(Fn&& fn) const -> std::invoke_result_t<Fn> {
        using ResultType = std::invoke_result_t<Fn>;
        static_assert(std::is_same_v<typename ResultType::error_type, error>, "Function must return a result with the same error type");
        
        if (success_) {
            return std::invoke(std::forward<Fn>(fn));
        }
        return ResultType(error_);
    }
    
private:
    bool success_ = false;
    error error_{error_code::success, ""};
};

// Type aliases for common result types
template <typename T>
using result_t = result<T>;

// Helper to convert std::optional<std::string> to result<T>
template <typename T>
result<T> optional_error_to_result(const std::optional<std::string>& error, T&& value) {
    if (error) {
        return result<T>(kcenon::thread::error{error_code::unknown_error, *error});
    }
    return result<T>(std::forward<T>(value));
}

// Helper to convert std::optional<std::string> to result<void>
inline result<void> optional_error_to_result(const std::optional<std::string>& error) {
    if (error) {
        return result<void>(kcenon::thread::error{error_code::unknown_error, *error});
    }
    return result<void>();
}

// Compatibility layer for existing code
inline std::optional<std::string> result_to_optional_error(const result<void>& res) {
    if (res) {
        return std::nullopt;
    }
    return res.get_error().to_string();
}

template <typename T>
std::pair<std::optional<T>, std::optional<std::string>> result_to_pair(const result<T>& res) {
    if (res) {
        return {res.value(), std::nullopt};
    }
    return {std::nullopt, res.get_error().to_string()};
}

// ============================================================================
// Migration to common::Result - Phase 1: Type Mapping and Conversion
// ============================================================================

#ifdef THREAD_HAS_COMMON_RESULT
namespace detail {

// Explicit using declarations for common_system types
// This avoids namespace pollution while maintaining compatibility
using kcenon::common::error_info;
using kcenon::common::Result;
using kcenon::common::VoidResult;
using kcenon::common::ok;

/**
 * @brief Convert thread::error to common::error_info
 *
 * This function facilitates the migration from thread-specific error types
 * to the unified common::error_info type.
 */
inline error_info to_common_error(const error& err) {
    return error_info{
        static_cast<int>(err.code()),
        err.message(),
        "thread_system"  // module name
    };
}

/**
 * @brief Convert thread::error_code to common::error_info
 */
inline error_info to_common_error(error_code code, const std::string& message = "") {
    return error_info{
        static_cast<int>(code),
        message.empty() ? error_code_to_string(code) : message,
        "thread_system"
    };
}

/**
 * @brief Convert common::error_info to thread::error
 *
 * Used for backward compatibility during migration.
 */
inline error from_common_error(const error_info& info) {
    return error{
        static_cast<error_code>(info.code),
        info.message
    };
}

/**
 * @brief Convert thread::result_void to common::VoidResult
 */
inline VoidResult to_common_result(const result_void& res) {
    if (res.has_error()) {
        return VoidResult(to_common_error(res.get_error()));
    }
    return ok();
}

/**
 * @brief Convert thread::result<T> to common::Result<T>
 */
template<typename T>
Result<T> to_common_result(const result<T>& res) {
    if (!res) {
        return Result<T>(to_common_error(res.get_error()));
    }
    return Result<T>(res.value());
}

/**
 * @brief Convert common::VoidResult to thread::result_void
 */
inline result_void from_common_result(const VoidResult& res) {
    if (res.is_err()) {
        return result_void(from_common_error(res.error()));
    }
    return result_void{};
}

/**
 * @brief Convert common::Result<T> to thread::result<T>
 */
template<typename T>
result<T> from_common_result(const Result<T>& res) {
    if (res.is_err()) {
        return result<T>(from_common_error(res.error()));
    }
    return result<T>(res.value());
}

} // namespace detail

// ============================================================================
// Migration Phase 2: Unified Type Aliases (Planned)
// ============================================================================
//
// Once all code is migrated, these aliases will replace the thread-specific
// result types. Currently commented out to maintain existing code.
//
// WARNING: These are transition typedefs. The long-term plan is to:
// 1. Migrate all code to use common::Result directly
// 2. Remove thread-specific result/result_void classes
// 3. Remove these typedefs
//
// For new code, prefer using kcenon::common::Result<T> and
// kcenon::common::VoidResult directly.

// Uncomment these when ready for phase 2 migration:
// using ResultVoid = kcenon::common::VoidResult;
// template<typename T>
// using Result = kcenon::common::Result<T>;

#endif // THREAD_HAS_COMMON_RESULT

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
