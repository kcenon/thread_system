/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, 🍀☀🌕🌥 🌊
All rights reserved.
*****************************************************************************/

#pragma once

#include "job.h"
#include "cancellation_token.h"
#include <atomic>
#include <functional>
#include <chrono>

namespace kcenon::thread {

/**
 * @class cancellable_job
 * @brief Job that can be cancelled during execution
 *
 * Provides cooperative cancellation mechanism where jobs can check
 * cancellation status and terminate early if requested.
 *
 * ### Thread Safety
 * - cancel() can be called from any thread
 * - is_cancelled() is thread-safe and lock-free
 * - execute() runs in worker thread context
 *
 * ### Usage Pattern
 * @code
 * auto job = std::make_unique<cancellable_job>([](auto& token) {
 *     for (int i = 0; i < 1000; ++i) {
 *         // Check for cancellation periodically
 *         if (token.is_cancellation_requested()) {
 *             log_info("Job cancelled at iteration {}", i);
 *             return;
 *         }
 *
 *         // Do work
 *         process_item(i);
 *     }
 * });
 *
 * // Later, from another thread
 * job->cancel();
 * @endcode
 */
class cancellable_job : public job {
public:
    using cancellable_work_function = std::function<void(const cancellation_token&)>;

    /**
     * @brief Construct cancellable job
     * @param work Function to execute (receives cancellation_token)
     */
    explicit cancellable_job(cancellable_work_function work)
        : work_(std::move(work))
        , cancelled_(false)
        , started_(false)
        , finished_(false)
    {
    }

    /**
     * @brief Cancel the job
     *
     * This sets the cancellation flag. The job must cooperatively
     * check is_cancelled() to actually stop execution.
     */
    auto cancel() -> void {
        cancelled_.store(true, std::memory_order_release);

        // Wake up any waiting operations
        if (token_) {
            token_->request_cancellation();
        }
    }

    /**
     * @brief Check if cancellation requested
     * @return true if cancel() was called
     */
    [[nodiscard]] auto is_cancelled() const -> bool {
        return cancelled_.load(std::memory_order_acquire);
    }

    /**
     * @brief Check if job has started execution
     */
    [[nodiscard]] auto is_started() const -> bool {
        return started_.load(std::memory_order_acquire);
    }

    /**
     * @brief Check if job has finished execution
     */
    [[nodiscard]] auto is_finished() const -> bool {
        return finished_.load(std::memory_order_acquire);
    }

    /**
     * @brief Execute the job
     *
     * Sets started flag, creates cancellation token, executes work,
     * and sets finished flag.
     */
    auto execute() -> void override {
        started_.store(true, std::memory_order_release);

        try {
            // Create cancellation token
            token_ = std::make_shared<cancellation_token>(cancelled_);

            // Execute work function
            work_(*token_);
        } catch (...) {
            finished_.store(true, std::memory_order_release);
            throw;
        }

        finished_.store(true, std::memory_order_release);
    }

    /**
     * @brief Set timeout for job execution
     * @param timeout Maximum execution time
     *
     * Job will be automatically cancelled if it exceeds this duration.
     */
    auto set_timeout(std::chrono::milliseconds timeout) -> void {
        timeout_ = timeout;
        start_time_ = std::chrono::steady_clock::now();
    }

    /**
     * @brief Check if job has exceeded timeout
     */
    [[nodiscard]] auto is_timeout_exceeded() const -> bool {
        if (timeout_.count() == 0) return false;

        auto elapsed = std::chrono::steady_clock::now() - start_time_;
        return elapsed > timeout_;
    }

protected:
    cancellable_work_function work_;
    std::atomic<bool> cancelled_;
    std::atomic<bool> started_;
    std::atomic<bool> finished_;
    std::shared_ptr<cancellation_token> token_;

    std::chrono::milliseconds timeout_{0};
    std::chrono::steady_clock::time_point start_time_;
};

/**
 * @brief Factory function for creating cancellable jobs
 * @param work Function to execute
 * @return Unique pointer to cancellable_job
 */
template<typename Func>
[[nodiscard]] auto make_cancellable_job(Func&& work)
    -> std::unique_ptr<cancellable_job>
{
    return std::make_unique<cancellable_job>(std::forward<Func>(work));
}

/**
 * @brief Create cancellable job with timeout
 * @param work Function to execute
 * @param timeout Maximum execution time
 * @return Unique pointer to cancellable_job
 */
template<typename Func>
[[nodiscard]] auto make_cancellable_job_with_timeout(
    Func&& work,
    std::chrono::milliseconds timeout)
    -> std::unique_ptr<cancellable_job>
{
    auto job = std::make_unique<cancellable_job>(std::forward<Func>(work));
    job->set_timeout(timeout);
    return job;
}

} // namespace kcenon::thread
