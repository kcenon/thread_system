/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, DongCheol Shin
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

#include <type_traits>
#include <memory>

#include <kcenon/thread/core/job.h>
#include <kcenon/thread/core/error_handling.h>
#include <kcenon/thread/interfaces/scheduler_interface.h>
#include <kcenon/thread/interfaces/queue_capabilities_interface.h>
#include <kcenon/thread/policies/sync_policies.h>
#include <kcenon/thread/policies/bound_policies.h>
#include <kcenon/thread/policies/overflow_policies.h>

namespace kcenon::thread {

/**
 * @class policy_queue
 * @brief Policy-based job queue template with customizable behavior
 *
 * This template class provides a flexible job queue where synchronization,
 * bounding, and overflow behaviors are controlled by policy classes.
 *
 * ### Template Parameters
 * - **SyncPolicy**: Controls thread synchronization (mutex, lock-free, adaptive)
 * - **BoundPolicy**: Controls queue size limits (bounded, unbounded)
 * - **OverflowPolicy**: Controls behavior when queue is full (reject, block, drop)
 *
 * ### Design Pattern
 * Uses compile-time policy-based design for:
 * - Zero-overhead abstractions (no virtual dispatch for policies)
 * - Type-safe policy combinations
 * - Explicit capability documentation
 *
 * ### Thread Safety
 * Thread safety depends on the SyncPolicy:
 * - mutex_sync_policy: Full thread safety with blocking support
 * - lockfree_sync_policy: Thread-safe without blocking
 * - adaptive_sync_policy: Configurable thread safety
 *
 * ### Usage Example
 * @code
 * // Bounded queue with mutex sync and blocking on overflow
 * using my_queue = policy_queue<
 *     policies::mutex_sync_policy,
 *     policies::bounded_policy,
 *     policies::overflow_block_policy
 * >;
 *
 * my_queue queue(policies::bounded_policy(1000));
 * queue.enqueue(std::make_unique<my_job>());
 * auto job = queue.dequeue();
 * @endcode
 *
 * ### Policy Compatibility Matrix
 * | SyncPolicy | BoundPolicy | OverflowPolicy | Compatible |
 * |------------|-------------|----------------|------------|
 * | mutex      | any         | any            | Yes        |
 * | lockfree   | unbounded   | N/A            | Yes        |
 * | lockfree   | bounded     | reject/drop    | Yes        |
 * | lockfree   | bounded     | block          | No         |
 * | adaptive   | any         | any            | Yes        |
 *
 * @tparam SyncPolicy Synchronization policy type
 * @tparam BoundPolicy Bounding policy type (default: unbounded_policy)
 * @tparam OverflowPolicy Overflow handling policy type (default: overflow_reject_policy)
 */
template<
    typename SyncPolicy,
    typename BoundPolicy = policies::unbounded_policy,
    typename OverflowPolicy = policies::overflow_reject_policy
>
class policy_queue : public scheduler_interface,
                     public queue_capabilities_interface {
public:
    using sync_policy_type = SyncPolicy;
    using bound_policy_type = BoundPolicy;
    using overflow_policy_type = OverflowPolicy;

    // ============================================
    // Construction
    // ============================================

    /**
     * @brief Construct queue with default policies
     */
    policy_queue()
        : sync_policy_()
        , bound_policy_()
        , overflow_policy_() {}

    /**
     * @brief Construct queue with bound policy
     * @param bound_policy Bound policy configuration
     */
    explicit policy_queue(BoundPolicy bound_policy)
        : sync_policy_()
        , bound_policy_(std::move(bound_policy))
        , overflow_policy_() {}

    /**
     * @brief Construct queue with all policies
     * @param sync_policy Sync policy configuration
     * @param bound_policy Bound policy configuration
     * @param overflow_policy Overflow policy configuration
     */
    policy_queue(SyncPolicy sync_policy,
                 BoundPolicy bound_policy,
                 OverflowPolicy overflow_policy)
        : sync_policy_(std::move(sync_policy))
        , bound_policy_(std::move(bound_policy))
        , overflow_policy_(std::move(overflow_policy)) {}

    /**
     * @brief Destructor
     */
    ~policy_queue() = default;

    // Non-copyable, non-movable
    policy_queue(const policy_queue&) = delete;
    policy_queue& operator=(const policy_queue&) = delete;
    policy_queue(policy_queue&&) = delete;
    policy_queue& operator=(policy_queue&&) = delete;

    // ============================================
    // scheduler_interface implementation
    // ============================================

    /**
     * @brief Schedule a job (delegates to enqueue)
     * @param work Job to schedule
     * @return common::VoidResult Success or error
     */
    auto schedule(std::unique_ptr<job>&& work) -> common::VoidResult override {
        return enqueue(std::move(work));
    }

    /**
     * @brief Get next job (delegates to dequeue)
     * @return common::Result<std::unique_ptr<job>> The dequeued job or error
     */
    auto get_next_job() -> common::Result<std::unique_ptr<job>> override {
        return dequeue();
    }

    // ============================================
    // Queue operations
    // ============================================

    /**
     * @brief Enqueue a job into the queue
     * @param value Job to enqueue
     * @return VoidResult indicating success or error
     *
     * Behavior depends on policies:
     * - If queue is bounded and full, overflow policy is invoked
     * - Sync policy controls thread safety
     */
    [[nodiscard]] auto enqueue(std::unique_ptr<job>&& value) -> common::VoidResult {
        if (!value) {
            return common::error_info{-105, "cannot enqueue null job", "thread_system"};
        }

        // Check bound policy
        if constexpr (BoundPolicy::is_bounded()) {
            if (bound_policy_.is_full(sync_policy_.size())) {
                return handle_overflow(std::move(value));
            }
        }

        return sync_policy_.enqueue(std::move(value));
    }

    /**
     * @brief Type-safe enqueue for job subclasses
     * @tparam JobType A type derived from job
     * @param value Job to enqueue
     * @return VoidResult indicating success or error
     */
    template<typename JobType, typename = std::enable_if_t<std::is_base_of_v<job, JobType>>>
    [[nodiscard]] auto enqueue(std::unique_ptr<JobType>&& value) -> common::VoidResult {
        return enqueue(std::unique_ptr<job>(std::move(value)));
    }

    /**
     * @brief Dequeue a job (blocking if sync policy supports it)
     * @return Result containing the job or error
     */
    [[nodiscard]] auto dequeue() -> common::Result<std::unique_ptr<job>> {
        return sync_policy_.dequeue();
    }

    /**
     * @brief Try to dequeue a job (non-blocking)
     * @return Result containing the job or error
     */
    [[nodiscard]] auto try_dequeue() -> common::Result<std::unique_ptr<job>> {
        return sync_policy_.try_dequeue();
    }

    /**
     * @brief Check if queue is empty
     * @return true if empty
     */
    [[nodiscard]] auto empty() const -> bool {
        return sync_policy_.empty();
    }

    /**
     * @brief Get queue size
     * @return Number of jobs in queue (exact or approximate based on sync policy)
     */
    [[nodiscard]] auto size() const -> std::size_t {
        return sync_policy_.size();
    }

    /**
     * @brief Clear all jobs from queue
     */
    auto clear() -> void {
        sync_policy_.clear();
    }

    /**
     * @brief Stop the queue
     */
    auto stop() -> void {
        sync_policy_.stop();
    }

    /**
     * @brief Check if queue is stopped
     * @return true if stopped
     */
    [[nodiscard]] auto is_stopped() const -> bool {
        return sync_policy_.is_stopped();
    }

    // ============================================
    // queue_capabilities_interface implementation
    // ============================================

    /**
     * @brief Get queue capabilities
     * @return Capabilities based on sync policy
     */
    [[nodiscard]] auto get_capabilities() const -> queue_capabilities override {
        return sync_policy_.get_capabilities();
    }

    // ============================================
    // Policy access
    // ============================================

    /**
     * @brief Get reference to sync policy
     * @return Sync policy reference
     */
    [[nodiscard]] auto sync_policy() -> SyncPolicy& {
        return sync_policy_;
    }

    /**
     * @brief Get const reference to sync policy
     * @return Sync policy const reference
     */
    [[nodiscard]] auto sync_policy() const -> const SyncPolicy& {
        return sync_policy_;
    }

    /**
     * @brief Get reference to bound policy
     * @return Bound policy reference
     */
    [[nodiscard]] auto bound_policy() -> BoundPolicy& {
        return bound_policy_;
    }

    /**
     * @brief Get const reference to bound policy
     * @return Bound policy const reference
     */
    [[nodiscard]] auto bound_policy() const -> const BoundPolicy& {
        return bound_policy_;
    }

    /**
     * @brief Get reference to overflow policy
     * @return Overflow policy reference
     */
    [[nodiscard]] auto overflow_policy() -> OverflowPolicy& {
        return overflow_policy_;
    }

    /**
     * @brief Get const reference to overflow policy
     * @return Overflow policy const reference
     */
    [[nodiscard]] auto overflow_policy() const -> const OverflowPolicy& {
        return overflow_policy_;
    }

    // ============================================
    // Bounded queue convenience methods
    // ============================================

    /**
     * @brief Check if queue is bounded
     * @return true if bound policy limits size
     */
    [[nodiscard]] auto is_bounded() const -> bool {
        if constexpr (requires { bound_policy_.is_bounded(); }) {
            return bound_policy_.is_bounded();
        }
        return BoundPolicy::is_bounded();
    }

    /**
     * @brief Check if queue is at capacity
     * @return true if bounded and at max size
     */
    [[nodiscard]] auto is_full() const -> bool {
        if constexpr (BoundPolicy::is_bounded()) {
            return bound_policy_.is_full(sync_policy_.size());
        }
        return false;
    }

    /**
     * @brief Get remaining capacity
     * @return Number of items that can still be added
     */
    [[nodiscard]] auto remaining_capacity() const -> std::size_t {
        return bound_policy_.remaining_capacity(sync_policy_.size());
    }

private:
    /**
     * @brief Handle overflow based on overflow policy
     * @param value Job that triggered overflow
     * @return VoidResult from overflow handling
     */
    [[nodiscard]] auto handle_overflow(std::unique_ptr<job>&& value) -> common::VoidResult {
        if constexpr (std::is_same_v<OverflowPolicy, policies::overflow_reject_policy>) {
            return overflow_policy_.handle_overflow(std::move(value));
        } else if constexpr (std::is_same_v<OverflowPolicy, policies::overflow_drop_newest_policy>) {
            return overflow_policy_.handle_overflow(std::move(value));
        } else if constexpr (std::is_same_v<OverflowPolicy, policies::overflow_drop_oldest_policy>) {
            // Drop oldest by dequeuing and discarding
            auto oldest = sync_policy_.try_dequeue();
            (void)oldest;  // Discard
            return sync_policy_.enqueue(std::move(value));
        } else {
            // For blocking policies, just return the error
            // Actual blocking must be implemented at a higher level
            return common::error_info{-120, "queue is full", "thread_system"};
        }
    }

    SyncPolicy sync_policy_;
    BoundPolicy bound_policy_;
    OverflowPolicy overflow_policy_;
};

// ============================================
// Type aliases for common configurations
// ============================================

/**
 * @brief Standard mutex-based unbounded queue
 */
using standard_queue = policy_queue<
    policies::mutex_sync_policy,
    policies::unbounded_policy,
    policies::overflow_reject_policy
>;

/**
 * @brief Lock-free unbounded queue
 */
using lockfree_queue = policy_queue<
    policies::lockfree_sync_policy,
    policies::unbounded_policy,
    policies::overflow_reject_policy
>;

/**
 * @brief Bounded queue with blocking on overflow
 */
template<std::size_t MaxSize>
using bounded_blocking_queue = policy_queue<
    policies::mutex_sync_policy,
    policies::bounded_policy,
    policies::overflow_block_policy
>;

/**
 * @brief Bounded queue that rejects on overflow
 */
template<std::size_t MaxSize>
using bounded_rejecting_queue = policy_queue<
    policies::mutex_sync_policy,
    policies::bounded_policy,
    policies::overflow_reject_policy
>;

/**
 * @brief Bounded queue that drops oldest on overflow (ring buffer behavior)
 */
template<std::size_t MaxSize>
using ring_buffer_queue = policy_queue<
    policies::mutex_sync_policy,
    policies::bounded_policy,
    policies::overflow_drop_oldest_policy
>;

} // namespace kcenon::thread
