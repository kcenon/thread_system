/*
 * BSD 3-Clause License
 * Copyright (c) 2025, DongCheol Shin
 */

#pragma once

#include <memory>

#include <kcenon/thread/core/error_handling.h>
#include <kcenon/thread/core/job.h>

namespace kcenon::thread {

/**
 * @class scheduler_interface
 * @brief Scheduler interface for queuing and retrieving jobs.
 *
 * This interface defines the contract for job scheduling implementations,
 * allowing different scheduling strategies (FIFO, priority-based, etc.)
 * to be plugged into the thread system.
 *
 * ### Thread Safety
 * Implementations must ensure thread-safe access to all methods:
 * - schedule() must be safely callable from multiple threads concurrently
 * - get_next_job() must be safely callable from multiple worker threads
 * - Internal queue state must be protected with appropriate synchronization
 * - Implementations should document their specific thread safety guarantees
 *
 * ### Typical Usage
 * @code
 * auto scheduler = get_scheduler();
 *
 * // Thread 1: Schedule jobs
 * auto result = scheduler->schedule(std::make_unique<my_job>());
 *
 * // Thread 2: Retrieve and process jobs
 * auto job_result = scheduler->get_next_job();
 * if (job_result.is_ok()) {
 *     auto job = std::move(job_result).value();
 *     job->execute();
 * }
 * @endcode
 */
class scheduler_interface {
public:
    virtual ~scheduler_interface() = default;

    /**
     * @brief Enqueue a job for processing.
     * @param work Job to schedule for execution
     * @return common::VoidResult indicating success or error
     *
     * Thread Safety: Must be thread-safe, callable from any thread
     */
    virtual auto schedule(std::unique_ptr<job>&& work) -> common::VoidResult = 0;

    /**
     * @brief Dequeue the next available job.
     * @return common::Result containing the next job or error if queue is empty/stopped
     *
     * Thread Safety: Must be thread-safe, callable from multiple worker threads
     */
    virtual auto get_next_job() -> common::Result<std::unique_ptr<job>> = 0;
};

} // namespace kcenon::thread

