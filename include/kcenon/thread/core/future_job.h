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
 * @brief A job that wraps a callable and provides a future for its result
 *
 * @deprecated Since v3.0.0. Use composition pattern with job_builder and
 * std::promise directly instead:
 * @code
 * // Old way (deprecated):
 * auto [job_ptr, future] = make_future_job([]{ return 42; });
 *
 * // New way (recommended):
 * #include <kcenon/thread/core/job_builder.h>
 * auto promise = std::make_shared<std::promise<int>>();
 * auto future = promise->get_future();
 * auto job = job_builder()
 *     .name("compute_job")
 *     .work([promise]() {
 *         promise->set_value(42);
 *         return common::ok();
 *     })
 *     .build();
 * @endcode
 *
 * This class will be removed in the next major version.
 *
 * @tparam R The return type of the callable
 *
 * The future_job class extends the base job class to wrap any callable
 * and provide a std::future that can be used to retrieve the result
 * asynchronously.
 *
 * ### Thread Safety
 * - The promise is stored in a shared_ptr to ensure safe access from
 *   both the job execution context and the caller context.
 * - The future can be retrieved before or after job execution.
 *
 * ### Exception Handling
 * - Any exception thrown by the callable is captured and stored in the
 *   promise, to be rethrown when get() is called on the future.
 *
 * ### Usage Example
 * @code
 * // Create a future_job that returns an integer
 * auto job_ptr = std::make_unique<future_job<int>>(
 *     []{ return 42; }, "compute_answer"
 * );
 *
 * // Get the future before submitting the job
 * auto future = job_ptr->get_future();
 *
 * // Submit the job to a thread pool
 * pool->enqueue(std::move(job_ptr));
 *
 * // Wait for and retrieve the result
 * int result = future.get();  // returns 42
 * @endcode
 */
template<typename R>
class [[deprecated("Use job_builder with std::promise instead. See class documentation.")]]
future_job : public job {
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

/**
 * @brief Helper function to create a future_job
 *
 * @deprecated Since v3.0.0. Use job_builder with std::promise instead.
 *
 * @tparam F Callable type
 * @tparam R Return type (automatically deduced)
 * @param callable The function to execute
 * @param name Optional name for the job
 * @return A unique_ptr to the created future_job
 *
 * @example
 * @code
 * auto [job, future] = make_future_job([]{ return 42; });
 * pool->enqueue(std::move(job));
 * int result = future.get();
 * @endcode
 */
template<typename F, typename R = std::invoke_result_t<std::decay_t<F>>>
[[deprecated("Use job_builder with std::promise instead.")]]
[[nodiscard]] auto make_future_job(F&& callable, const std::string& name = "future_job")
    -> std::pair<std::unique_ptr<future_job<R>>, std::future<R>>
{
    auto job_ptr = std::make_unique<future_job<R>>(
        std::forward<F>(callable), name
    );
    auto future = job_ptr->get_future();
    return {std::move(job_ptr), std::move(future)};
}

} // namespace kcenon::thread
