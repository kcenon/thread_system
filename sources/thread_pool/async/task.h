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
 * @file task.h
 * @brief Implementation of a coroutine-based task system
 * 
 * This file provides the implementation of a C++20 coroutine-based asynchronous task system.
 * It includes the necessary promise types, awaitable objects, and task classes to enable
 * writing asynchronous code with the co_await syntax. The system supports error handling
 * through the result<T> and result_void types.
 * 
 * @author Thread System Team
 * @date 2024-05-14
 * @copyright BSD-3-Clause
 */

#include "../../thread_base/sync/error_handling.h"

#include <exception>
#include <future>
#include <memory>
#include <type_traits>
#include <utility>
#include <atomic>
#include <variant>
#include <chrono>

// Handle different coroutine headers based on compiler and environment
#if defined(_MSC_VER)
  // MSVC has std::coroutine support with C++20
  #include <coroutine>
#elif defined(__clang__)
  #if __has_include(<coroutine>) && __cplusplus >= 202002L
    #include <coroutine>
  #elif __has_include(<experimental/coroutine>)
    #include <experimental/coroutine>
    namespace std {
      using namespace std::experimental;
    }
  #else
    #error "No coroutine support detected in Clang"
  #endif
#elif defined(__GNUC__) || defined(__GNUG__)
  // GCC 10+ with -fcoroutines flag
  #if __GNUC__ >= 10 && __cplusplus >= 202002L
    #include <coroutine>
  #elif __has_include(<coroutine>)
    #include <coroutine>
  #else
    #error "No coroutine support detected in GCC"
  #endif
#else
  #if __has_include(<coroutine>)
    #include <coroutine>
  #elif __has_include(<experimental/coroutine>)
    #include <experimental/coroutine>
    namespace std {
      using namespace std::experimental;
    }
  #else
    #error "No coroutine support detected"
  #endif
#endif

// Platform detection macros for internal use
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
  #define THREAD_SYSTEM_WINDOWS
#endif

#if defined(__MINGW32__) || defined(__MINGW64__)
  #define THREAD_SYSTEM_MINGW
#endif

#if defined(__MSYS__)
  #define THREAD_SYSTEM_MSYS
#endif

namespace thread_pool_module {

using namespace thread_module;

/**
 * @namespace detail
 * @brief Contains internal implementation details for the task system
 * 
 * This namespace contains type traits and helper utilities that are used internally
 * by the task implementation but are not intended to be part of the public API.
 */
namespace detail {
    /**
     * @brief Type trait to detect if a type is a result<T>
     * 
     * Primary template (base case) that identifies non-result types
     * 
     * @tparam T The type to check
     */
    template <typename T>
    struct is_result : std::false_type {};
    
    /**
     * @brief Type trait specialization for result<T> types
     * 
     * Specialization that identifies result<T> types
     * 
     * @tparam T The type parameter of the result
     */
    template <typename T>
    struct is_result<result<T>> : std::true_type {};
    
    /**
     * @brief Helper variable template for easy checking if a type is a result<T>
     * 
     * @tparam T The type to check
     */
    template <typename T>
    inline constexpr bool is_result_v = is_result<T>::value;
}

// Forward declaration
/**
 * @class task
 * @brief A class representing an asynchronous coroutine task
 * 
 * This template class represents an asynchronous task that can be awaited using
 * the co_await operator. It automatically manages the coroutine state and provides
 * proper synchronization for waiting on the task's result.
 * 
 * @tparam T The type of the value that the task will produce
 */
template <typename T>
class task;

/**
 * @class task_promise
 * @brief Promise type for coroutine task implementation
 * 
 * This class implements the promise type requirement for C++20 coroutines
 * using the task<T> return type. It provides the necessary methods to
 * create and manage coroutines, handle return values, and propagate exceptions.
 * 
 * @tparam T The type of value that will be produced by the coroutine
 */
template <typename T>
class task_promise {
public:
    /**
     * @brief Default constructor
     * 
     * Constructs a new task_promise object with default state.
     */
    task_promise() = default;
    
    /**
     * @brief Creates the task object associated with this promise
     * 
     * Called by the coroutine machinery to create the return object that
     * represents the coroutine in the caller's context.
     * 
     * @return task<T> A new task object associated with this promise
     */
    task<T> get_return_object() noexcept;
    
    /**
     * @brief Determines whether the coroutine should suspend after being created
     * 
     * This implementation returns std::suspend_never, meaning the coroutine
     * will begin execution immediately after creation.
     * 
     * @return std::suspend_never Indicates that the coroutine should not suspend initially
     */
    constexpr std::suspend_never initial_suspend() const noexcept { return {}; }
    
    /**
     * @brief Awaiter object that is used when the coroutine reaches its end
     * 
     * This struct is used to notify any waiters when the coroutine completes execution.
     */
    struct final_awaiter {
        /**
         * @brief Determines if the awaiter is ready
         * 
         * @return bool Always returns false to ensure the coroutine is suspended
         */
        bool await_ready() const noexcept { return false; }
        
        /**
         * @brief Called when the coroutine is suspended at the final point
         * 
         * Notifies any tasks that are waiting for this coroutine's completion.
         * 
         * @tparam Promise The type of the promise
         * @param coroutine The coroutine handle
         */
        template <typename Promise>
        void await_suspend(std::coroutine_handle<Promise> coroutine) const noexcept {
            // Notify any waiters when the coroutine completes
            auto& promise = coroutine.promise();
            promise.notify_waiters();
        }
        
        /**
         * @brief Called when the awaiter is resumed
         * 
         * This is a no-op since the coroutine is never resumed after final suspension.
         */
        void await_resume() const noexcept {}
    };
    
    /**
     * @brief Determines what happens when the coroutine finishes
     * 
     * Creates and returns a final_awaiter that will notify waiters.
     * 
     * @return final_awaiter An awaiter that handles final suspension
     */
    constexpr auto final_suspend() const noexcept {
        return final_awaiter{};
    }
    
    /**
     * @brief Handles normal return values from the coroutine
     * 
     * This function is called when the coroutine returns a value with co_return.
     * It stores the value in the result variant.
     * 
     * @tparam U The type of the returned value 
     * @param value The value being returned from the coroutine
     * @return void
     * @requires std::convertible_to<U, T> The value must be convertible to the task's result type
     */
    template <typename U>
    requires std::convertible_to<U, T>
    void return_value(U&& value) noexcept(std::is_nothrow_constructible_v<T, U&&>) {
        result_.template emplace<T>(std::forward<U>(value));
    }
    
    /**
     * @brief Handles unhandled exceptions in the coroutine
     * 
     * This function is called when an exception is thrown and not caught within
     * the coroutine. It captures the exception pointer in the result variant.
     * 
     * @return void
     * @note This method is noexcept as it must not throw during exception handling
     */
    void unhandled_exception() noexcept {
        result_.template emplace<std::exception_ptr>(std::current_exception());
    }
    
    /**
     * @brief Marks the task as completed with an error
     * 
     * This function is used to explicitly set an error result for the task,
     * typically used when a task fails without throwing an exception.
     * 
     * @param err The error object to store as the result
     * @return void
     * @note This method is noexcept to ensure error handling doesn't throw
     */
    void set_error(error err) noexcept {
        result_.template emplace<error>(std::move(err));
    }
    
    /**
     * @brief Retrieves the result value or propagates exceptions
     * 
     * This function is called by the awaiter to get the final result of the coroutine.
     * If the coroutine completed with an exception or error, this function will throw.
     * 
     * @return T The value produced by the coroutine
     * @throws std::exception The original exception if the coroutine threw an exception
     * @throws std::runtime_error If the coroutine completed with an error
     */
    T get_result() {
        if (std::holds_alternative<std::exception_ptr>(result_)) {
            std::rethrow_exception(std::get<std::exception_ptr>(result_));
        } else if (std::holds_alternative<error>(result_)) {
            throw std::runtime_error(std::get<error>(result_).to_string());
        }
        
        return std::move(std::get<T>(result_));
    }
    
    /**
     * @brief Checks if the task has completed
     * 
     * @return bool True if the task has a result (value, error, or exception)
     * @note This method is noexcept and can be safely called from any context
     */
    bool is_ready() const noexcept {
        return !std::holds_alternative<std::monostate>(result_);
    }
    
    /**
     * @brief Notifies waiters that the task has completed
     * 
     * This method resumes the continuation coroutine if one is set.
     * It's typically called by the final_awaiter when the coroutine completes.
     * 
     * @return void
     * @note This method is noexcept to ensure notification doesn't throw
     */
    void notify_waiters() noexcept {
        if (continuation_) {
            continuation_.resume();
        }
    }
    
    /**
     * @brief Sets a continuation coroutine to resume when this task completes
     * 
     * This is used to implement the awaiter's await_suspend method, allowing
     * the calling coroutine to automatically resume when this task completes.
     * 
     * @param continuation The coroutine handle to resume upon completion
     * @return void
     * @note This method is noexcept to ensure task chaining doesn't throw
     */
    void set_continuation(std::coroutine_handle<> continuation) noexcept {
        continuation_ = continuation;
    }
    
private:
    /**
     * @brief Storage for the task's result
     * 
     * This variant can hold:
     * - std::monostate: Initial state, no result yet
     * - T: The successful result value
     * - std::exception_ptr: An exception that occurred during execution
     * - error: An error that occurred during execution
     */
    std::variant<std::monostate, T, std::exception_ptr, error> result_;
    
    /**
     * @brief Handle to the coroutine that is waiting for this task
     * 
     * When this task completes, this coroutine will be resumed.
     * This enables the implementation of the co_await operator.
     */
    std::coroutine_handle<> continuation_;
};

/**
 * @class task_promise<void>
 * @brief Specialization of task_promise for void-returning coroutines
 * 
 * This specialization handles the case where a coroutine doesn't return a value.
 * It provides the same interface as the generic task_promise but with adaptations
 * for void return values.
 */
template <>
class task_promise<void> {
public:
    task<void> get_return_object() noexcept;
    
    constexpr std::suspend_never initial_suspend() const noexcept { return {}; }
    
    // Final awaiter for void promise
    struct final_awaiter {
        bool await_ready() const noexcept { return false; }
        
        template <typename Promise>
        void await_suspend(std::coroutine_handle<Promise> coroutine) const noexcept {
            auto& promise = coroutine.promise();
            promise.notify_waiters();
        }
        
        void await_resume() const noexcept {}
    };
    
    constexpr auto final_suspend() const noexcept {
        return final_awaiter{};
    }
    
    // Handle void returns
    void return_void() noexcept {
        result_ = result_state::value;
    }
    
    // Handle exceptions
    void unhandled_exception() noexcept {
        exception_ = std::current_exception();
        result_ = result_state::exception;
    }
    
    // Mark as completed with an error
    void set_error(error err) noexcept {
        error_ = std::move(err);
        result_ = result_state::error;
    }
    
    // Get the result (for awaiter)
    void get_result() {
        if (result_ == result_state::exception) {
            std::rethrow_exception(exception_);
        } else if (result_ == result_state::error) {
            throw std::runtime_error(error_.to_string());
        }
    }
    
    // Check if the task is ready
    bool is_ready() const noexcept {
        return result_ != result_state::empty;
    }
    
    // Notify waiters that the task is complete
    void notify_waiters() noexcept {
        if (continuation_) {
            continuation_.resume();
        }
    }
    
    // Set a continuation to execute when this task completes
    void set_continuation(std::coroutine_handle<> continuation) noexcept {
        continuation_ = continuation;
    }
    
private:
    enum class result_state { empty, value, exception, error };
    result_state result_ = result_state::empty;
    std::exception_ptr exception_;
    error error_{error_code::success, ""};
    std::coroutine_handle<> continuation_;
};

/**
 * @class task
 * @brief A coroutine-based task that can be awaited
 * 
 * Represents an asynchronous operation that can be co_awaited in a coroutine.
 */
template <typename T = void>
class task {
public:
    using promise_type = task_promise<T>;
    using value_type = T;

    // Constructs an empty task
    task() noexcept : handle_(nullptr) {}
    
    // Takes ownership of a coroutine handle
    explicit task(std::coroutine_handle<promise_type> handle) noexcept
        : handle_(handle) {}
        
    // Move constructor
    task(task&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }
    
    // Move assignment
    task& operator=(task&& other) noexcept {
        if (this != &other) {
            if (handle_) {
                handle_.destroy();
            }
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }
    
    // Destructor
    ~task() {
        if (handle_) {
            handle_.destroy();
        }
    }
    
    // No copying
    task(const task&) = delete;
    task& operator=(const task&) = delete;
    
    // Check if the task has a valid coroutine
    bool valid() const noexcept {
        return handle_ != nullptr;
    }
    
    // Check if the task is done
    bool is_ready() const noexcept {
        return !handle_ || handle_.promise().is_ready();
    }
    
    // Class to make task awaitable in coroutines
    class awaiter {
    public:
        explicit awaiter(std::coroutine_handle<promise_type> handle) noexcept
            : handle_(handle) {}
            
        bool await_ready() const noexcept {
            return !handle_ || handle_.promise().is_ready();
        }
        
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> continuation) noexcept {
            handle_.promise().set_continuation(continuation);
            return handle_;
        }
        
        T await_resume() {
            if (!handle_) {
                throw std::runtime_error("Awaiting an empty task");
            }
            return handle_.promise().get_result();
        }
        
    private:
        std::coroutine_handle<promise_type> handle_;
    };
    
    // Make this task awaitable
    auto operator co_await() const noexcept {
        return awaiter{handle_};
    }
    
    // Wait synchronously for the task to complete (blocking)
    result_t<T> wait() const {
        if (!valid()) {
            return error{error_code::invalid_argument, "Cannot wait on an invalid task"};
        }
        
        // If the task is already completed, get the result immediately
        if (handle_.promise().is_ready()) {
            try {
                if constexpr (std::is_void_v<T>) {
                    handle_.promise().get_result();
                    return result_void{};
                } else {
                    return handle_.promise().get_result();
                }
            } catch (const std::exception& e) {
                return error{error_code::job_execution_failed, e.what()};
            }
        }
        
        // Structure for safe continuation & cleanup (especially important for Windows)
        struct continuation_context {
            std::promise<void> barrier;
            
            static void invoke_and_destroy(void* ptr) {
                auto* ctx = static_cast<continuation_context*>(ptr);
                try {
                    ctx->barrier.set_value();
                } catch (...) {
                    // Ignore any exceptions if the promise is already satisfied
                }
                delete ctx;
            }
        };
        
        // Create a new continuation context with our barrier
        auto* ctx = new continuation_context;
        auto future = ctx->barrier.get_future();
        
        // Set up the continuation
        handle_.promise().set_continuation(
            std::coroutine_handle<>::from_address(ctx)
        );
        
        // Wait for completion with a timeout
        auto status = future.wait_for(std::chrono::seconds(5));
        
        if (status == std::future_status::timeout) {
            // Clean up our context on timeout to avoid memory leaks
            delete ctx;
            return error{error_code::operation_timeout, "Task wait timed out after 5 seconds"};
        }
        
        try {
            if constexpr (std::is_void_v<T>) {
                handle_.promise().get_result();
                return result_void{};
            } else {
                return handle_.promise().get_result();
            }
        } catch (const std::exception& e) {
            return error{error_code::job_execution_failed, e.what()};
        }
    }
    
    // Create a task that completes with a value
    template <typename U = T>
    static typename std::enable_if<!std::is_void<U>::value, task<U>>::type
    from_result(U value) {
        co_return value;
    }
    
    // Specialization for void return type
    template <typename U = T>
    static typename std::enable_if<std::is_void<U>::value, task<U>>::type
    from_result() {
        co_return;
    }
    
    // Create a task that completes with an error
    static task<T> from_error(error err) {
        auto handle = std::coroutine_handle<promise_type>::from_promise(
            *new promise_type());
        handle.promise().set_error(std::move(err));
        return task<T>{handle};
    }
    
private:
    std::coroutine_handle<promise_type> handle_;
};

// Implement get_return_object for task_promise
template <typename T>
task<T> task_promise<T>::get_return_object() noexcept {
    return task<T>{std::coroutine_handle<task_promise>::from_promise(*this)};
}

inline task<void> task_promise<void>::get_return_object() noexcept {
    return task<void>{std::coroutine_handle<task_promise>::from_promise(*this)};
}

// Helper function to create a task from a callable
template <typename Func>
auto make_task(Func&& func) -> task<std::invoke_result_t<Func>> {
    using result_type = std::invoke_result_t<Func>;
    
#if defined(_MSC_VER)
    // MSVC-specific implementation
    // MSVC requires different approach for handling errors from coroutines
    // We need to handle error conditions by wrapping the entire function
    try {
        if constexpr (std::is_void_v<result_type>) {
            std::forward<Func>(func)();
            co_return;
        } else {
            co_return std::forward<Func>(func)();
        }
    } catch (const error& e) {
        // Special case for our error type - we already have the error information
        if constexpr (std::is_void_v<result_type>) {
            co_return;
        } else if constexpr (detail::is_result_v<result_type>) {
            co_return result_type(e);
        } else {
            co_return static_cast<result_type>(0);
        }
    } catch (const std::exception& e) {
        // For standard exceptions, convert to our error type
        if constexpr (std::is_void_v<result_type>) {
            co_return;
        } else if constexpr (std::is_same_v<result_type, result_t<void>> ||
                           std::is_same_v<result_type, result_void>) {
            co_return result_type(error{error_code::job_execution_failed, e.what()});
        } else if constexpr (detail::is_result_v<result_type>) {
            co_return result_type(error{error_code::job_execution_failed, e.what()});
        } else {
            co_return static_cast<result_type>(0);
        }
    }
#elif defined(THREAD_SYSTEM_MINGW) || defined(THREAD_SYSTEM_MSYS)
    // MinGW/MSYS-specific implementation
    // These environments need special handling similar to MSVC
    try {
        if constexpr (std::is_void_v<result_type>) {
            std::forward<Func>(func)();
            co_return;
        } else {
            co_return std::forward<Func>(func)();
        }
    } catch (const error& e) {
        // Special case for our error type
        if constexpr (std::is_void_v<result_type>) {
            co_return;
        } else if constexpr (detail::is_result_v<result_type>) {
            co_return result_type(e);
        } else {
            co_return static_cast<result_type>(0);
        }
    } catch (const std::exception& e) {
        // For standard exceptions
        if constexpr (std::is_void_v<result_type>) {
            co_return;
        } else if constexpr (std::is_same_v<result_type, result_t<void>> ||
                           std::is_same_v<result_type, result_void>) {
            co_return result_type(error{error_code::job_execution_failed, e.what()});
        } else if constexpr (detail::is_result_v<result_type>) {
            co_return result_type(error{error_code::job_execution_failed, e.what()});
        } else {
            co_return static_cast<result_type>(0);
        }
    }
#else
    // Implementation for Clang, GCC, etc.
    try {
        if constexpr (std::is_void_v<result_type>) {
            std::forward<Func>(func)();
            co_return;
        } else {
            co_return std::forward<Func>(func)();
        }
    } catch (const error& e) {
        // Direct handling of our error type
        #if defined(__clang__) && defined(__has_builtin) && __has_builtin(__builtin_coro_frame)
            // Clang-specific implementation using __builtin_coro_frame if available
            auto handle = std::coroutine_handle<task_promise<result_type>>::from_promise(
                *reinterpret_cast<task_promise<result_type>*>(
                    std::coroutine_handle<>::from_address(
                        __builtin_coro_frame()
                    ).address()
                )
            );
            handle.promise().set_error(e);
        #else
            // For other compilers, rethrow and handle in the catch block below
            throw e;
        #endif
        
        // Fallback return values
        if constexpr (std::is_void_v<result_type>) {
            co_return;
        } else if constexpr (detail::is_result_v<result_type>) {
            co_return result_type(e);
        } else {
            co_return static_cast<result_type>(0);
        }
    } catch (const std::exception& e) {
        // Can't use co_await in catch block, so manually handle the error
        // We need a safe, cross-compiler way to access the promise
        #if defined(__clang__) && defined(__has_builtin) && __has_builtin(__builtin_coro_frame)
            // Clang-specific implementation using __builtin_coro_frame
            auto handle = std::coroutine_handle<task_promise<result_type>>::from_promise(
                *reinterpret_cast<task_promise<result_type>*>(
                    std::coroutine_handle<>::from_address(
                        __builtin_coro_frame()
                    ).address()
                )
            );
            handle.promise().set_error(error{error_code::job_execution_failed, e.what()});
        #elif defined(__GNUC__) || defined(__GNUG__)
            // GCC implementation
            // Convert to our error type then throw - the promise will catch it
            throw error{error_code::job_execution_failed, e.what()};
        #else
            // Generic fallback for other compilers
            throw error{error_code::job_execution_failed, e.what()};
        #endif
        
        // We need to provide a return value for non-void tasks
        if constexpr (std::is_void_v<result_type>) {
            co_return;
        } else if constexpr (std::is_same_v<result_type, result_t<void>> ||
                           std::is_same_v<result_type, result_void>) {
            // For result-based return types
            co_return result_type(error{error_code::operation_canceled, "Task was cancelled"});
        } else if constexpr (detail::is_result_v<result_type>) {
            // For any other result<T> types
            co_return result_type(error{error_code::operation_canceled, "Task was cancelled"});
        } else {
            // For regular return types - return default value of T
            co_return static_cast<result_type>(0);
        }
    }
#endif
}

// Simple helper function to sleep and resume the coroutine
inline void sleep_and_resume(std::chrono::milliseconds duration, std::coroutine_handle<> handle) {
    // Create a shared state to ensure we properly manage thread lifetime
    struct shared_state {
        std::thread timer_thread;
        std::atomic<bool> completed{false};
        std::atomic<bool> destroyed{false};
        
        ~shared_state() {
            destroyed.store(true);
            if (timer_thread.joinable()) {
                timer_thread.join();
            }
        }
    };
    
    // Using shared_ptr for automatic cleanup when all references are gone
    auto state = std::make_shared<shared_state>();
    
    // Create the timer thread that will sleep and then resume the coroutine
    state->timer_thread = std::thread([state, duration, handle]() {
        std::this_thread::sleep_for(duration);
        
        // Only resume if we haven't been destroyed yet
        if (!state->destroyed.load()) {
            handle.resume();
        }
        
        state->completed.store(true);
    });
    
#if defined(THREAD_SYSTEM_WINDOWS)
    // On Windows (including MSVC, MinGW, MSYS), we use a dedicated cleanup approach
    std::thread cleanup_thread([state]() {
        // This thread's only job is to hold a reference to the state 
        // until the timer thread completes to ensure proper cleanup
        while (!state->completed.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    cleanup_thread.detach();
#else
    // On non-Windows platforms, we can safely detach the thread
    // The shared_ptr will keep the state alive until all references are gone
    state->timer_thread.detach();
    
    // Create a second detached thread as a safety net that will
    // ensure the state lives long enough for the timer thread to finish
    std::thread([state]() {
        while (!state->completed.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }).detach();
#endif
}

// Helper to make a task that completes after a delay
inline task<void> delay(std::chrono::milliseconds duration) {
    struct delay_awaiter {
        std::chrono::milliseconds duration;
        
        bool await_ready() const noexcept { 
            return duration.count() <= 0; 
        }
        
        void await_suspend(std::coroutine_handle<> handle) noexcept {
            // Use the helper function to sleep and resume
            sleep_and_resume(duration, handle);
        }
        
        void await_resume() noexcept {}
    };
    
    co_await delay_awaiter{duration};
}

} // namespace thread_pool_module