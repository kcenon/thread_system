/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, DongCheol Shin
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#pragma once

#include <memory>
#include <vector>
#include <string>

#include <kcenon/thread/core/job.h>
#include <kcenon/thread/core/error_handling.h>
#include <kcenon/thread/interfaces/scheduler_interface.h>
#include <kcenon/thread/interfaces/queue_capabilities_interface.h>

namespace kcenon::thread {

// Forward declarations
class job_queue;

template<typename SyncPolicy, typename BoundPolicy, typename OverflowPolicy>
class policy_queue;

/**
 * @class pool_queue_adapter_interface
 * @brief Abstract interface for queue adapters used by thread_pool
 *
 * This interface provides a unified API for both job_queue and policy_queue,
 * allowing thread_pool to work with either queue type seamlessly.
 *
 * ### Design Pattern
 * Uses the Adapter pattern to provide a common interface for different
 * queue implementations. This enables thread_pool to be decoupled from
 * specific queue implementations while maintaining type safety.
 *
 * ### Thread Safety
 * All methods delegate to the underlying queue implementation, which
 * must provide its own thread safety guarantees.
 */
class pool_queue_adapter_interface {
public:
    virtual ~pool_queue_adapter_interface() = default;

    /**
     * @brief Enqueue a job
     * @param job Job to enqueue
     * @return VoidResult indicating success or error
     */
    [[nodiscard]] virtual auto enqueue(std::unique_ptr<job>&& job) -> common::VoidResult = 0;

    /**
     * @brief Enqueue a batch of jobs
     * @param jobs Jobs to enqueue
     * @return VoidResult indicating success or error
     */
    [[nodiscard]] virtual auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> common::VoidResult = 0;

    /**
     * @brief Dequeue a job (blocking)
     * @return Result containing the job or error
     */
    [[nodiscard]] virtual auto dequeue() -> common::Result<std::unique_ptr<job>> = 0;

    /**
     * @brief Try to dequeue a job (non-blocking)
     * @return Result containing the job or error
     */
    [[nodiscard]] virtual auto try_dequeue() -> common::Result<std::unique_ptr<job>> = 0;

    /**
     * @brief Check if queue is empty
     * @return true if empty
     */
    [[nodiscard]] virtual auto empty() const -> bool = 0;

    /**
     * @brief Get queue size
     * @return Number of jobs in queue
     */
    [[nodiscard]] virtual auto size() const -> std::size_t = 0;

    /**
     * @brief Clear all jobs from queue
     */
    virtual auto clear() -> void = 0;

    /**
     * @brief Stop the queue
     */
    virtual auto stop() -> void = 0;

    /**
     * @brief Check if queue is stopped
     * @return true if stopped
     */
    [[nodiscard]] virtual auto is_stopped() const -> bool = 0;

    /**
     * @brief Get queue capabilities
     * @return Capabilities structure
     */
    [[nodiscard]] virtual auto get_capabilities() const -> queue_capabilities = 0;

    /**
     * @brief Get string representation
     * @return String describing the queue
     */
    [[nodiscard]] virtual auto to_string() const -> std::string = 0;

    /**
     * @brief Get the underlying job_queue if this adapter wraps one
     * @return Shared pointer to job_queue, or nullptr if not applicable
     *
     * This method is provided for backward compatibility with code that
     * needs direct access to job_queue. Returns nullptr for policy_queue
     * adapters.
     */
    [[nodiscard]] virtual auto get_job_queue() const -> std::shared_ptr<job_queue> = 0;

    /**
     * @brief Get the underlying scheduler interface
     * @return Reference to scheduler_interface
     */
    [[nodiscard]] virtual auto get_scheduler() -> scheduler_interface& = 0;

    /**
     * @brief Get the underlying scheduler interface (const)
     * @return Const reference to scheduler_interface
     */
    [[nodiscard]] virtual auto get_scheduler() const -> const scheduler_interface& = 0;
};

} // namespace kcenon::thread
