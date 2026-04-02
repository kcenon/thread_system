// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#pragma once

/**
 * @file future_job.h
 * @brief Job wrapper that provides std::future for async result returns
 *
 * This file provides the future_job template class that wraps callables with
 * std::promise to enable async result retrieval through std::future.
 */

#include <kcenon/thread/core/job.h>
#include <kcenon/thread/core/error_handling.h>

#include <functional>
#include <future>
#include <memory>
#include <type_traits>

namespace kcenon::thread {

/**
 * @class future_job
 * @brief A job that wraps a callable and provides a future for its result.
 *
 * @tparam R The return type of the callable
 *
 * Used internally by thread_pool::submit() to provide std::future-based
 * async result retrieval. Prefer using thread_pool::submit() directly
 * rather than creating future_job instances manually.
 *
 * ### Thread Safety
 * - The promise is stored in a shared_ptr to ensure safe access from
 *   both the job execution context and the caller context.
 * - The future can be retrieved before or after job execution.
 *
 * ### Exception Handling
 * - Any exception thrown by the callable is captured and stored in the
 *   promise, to be rethrown when get() is called on the future.
 */
template<typename R>
class future_job : public job {
public:
    /**
     * @brief Constructs a future_job from a callable
     *
     * @tparam F Callable type (function, lambda, functor, etc.)
     * @param callable The function to execute
     * @param name Optional name for the job (default: "future_job")
     *
     * @note The callable must be invocable with no arguments and return R.
     */
    template<typename F,
             typename = std::enable_if_t<std::is_invocable_r_v<R, F>>>
    explicit future_job(F&& callable, const std::string& name = "future_job")
        : job(name)
        , callable_(std::forward<F>(callable))
        , promise_(std::make_shared<std::promise<R>>())
    {}

    /**
     * @brief Get the future associated with this job
     *
     * @return A std::future that will hold the result of the callable
     *
     * @note This method should be called before the job is executed.
     *       The future remains valid after the job is moved.
     */
    [[nodiscard]] auto get_future() -> std::future<R> {
        return promise_->get_future();
    }

protected:
    /**
     * @brief Executes the callable and sets the promise value
     *
     * @return common::VoidResult indicating success or failure
     *
     * This method is called by the worker thread. It:
     * 1. Checks if cancellation was requested
     * 2. Invokes the callable
     * 3. Sets the promise value (or exception if one was thrown)
     */
    [[nodiscard]] auto do_work() -> common::VoidResult override {
        // Check for cancellation before starting
        if (cancellation_token_.is_cancelled()) {
            promise_->set_exception(
                std::make_exception_ptr(
                    std::runtime_error("Job cancelled before execution")
                )
            );
            return make_error_result(error_code::operation_canceled);
        }

        try {
            if constexpr (std::is_void_v<R>) {
                callable_();
                promise_->set_value();
            } else {
                promise_->set_value(callable_());
            }
        } catch (...) {
            promise_->set_exception(std::current_exception());
            return make_error_result(
                error_code::job_execution_failed,
                "Exception thrown during job execution"
            );
        }

        return common::ok();
    }

private:
    std::function<R()> callable_;
    std::shared_ptr<std::promise<R>> promise_;
};

} // namespace kcenon::thread
