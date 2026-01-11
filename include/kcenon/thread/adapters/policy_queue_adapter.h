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

#include <kcenon/thread/interfaces/pool_queue_adapter.h>
#include <kcenon/thread/policies/policy_queue.h>
#include <kcenon/thread/utils/formatter.h>

namespace kcenon::thread {

/**
 * @class policy_queue_adapter
 * @brief Adapter for policy_queue to pool_queue_adapter_interface
 *
 * This template adapter wraps any policy_queue instantiation and provides
 * the unified interface expected by thread_pool. It enables thread_pool
 * to work with the new policy-based queue system.
 *
 * ### Template Parameters
 * - **SyncPolicy**: Controls thread synchronization
 * - **BoundPolicy**: Controls queue size limits
 * - **OverflowPolicy**: Controls behavior when queue is full
 *
 * ### Usage
 * @code
 * // Using standard_queue (mutex-based, unbounded)
 * auto adapter = std::make_unique<policy_queue_adapter<
 *     policies::mutex_sync_policy,
 *     policies::unbounded_policy,
 *     policies::overflow_reject_policy>>();
 *
 * // Or using type alias
 * auto adapter = make_policy_queue_adapter<standard_queue>();
 * @endcode
 *
 * @tparam SyncPolicy Synchronization policy type
 * @tparam BoundPolicy Bounding policy type
 * @tparam OverflowPolicy Overflow handling policy type
 */
template<
    typename SyncPolicy,
    typename BoundPolicy = policies::unbounded_policy,
    typename OverflowPolicy = policies::overflow_reject_policy
>
class policy_queue_adapter : public pool_queue_adapter_interface {
public:
    using queue_type = policy_queue<SyncPolicy, BoundPolicy, OverflowPolicy>;

    /**
     * @brief Construct adapter with new policy_queue using default policies
     */
    policy_queue_adapter()
        : queue_(std::make_unique<queue_type>()) {}

    /**
     * @brief Construct adapter with bound policy
     * @param bound_policy Bound policy configuration
     */
    explicit policy_queue_adapter(BoundPolicy bound_policy)
        : queue_(std::make_unique<queue_type>(std::move(bound_policy))) {}

    /**
     * @brief Construct adapter with all policies
     * @param sync_policy Sync policy configuration
     * @param bound_policy Bound policy configuration
     * @param overflow_policy Overflow policy configuration
     */
    policy_queue_adapter(SyncPolicy sync_policy,
                         BoundPolicy bound_policy,
                         OverflowPolicy overflow_policy)
        : queue_(std::make_unique<queue_type>(
            std::move(sync_policy),
            std::move(bound_policy),
            std::move(overflow_policy))) {}

    /**
     * @brief Construct adapter with existing policy_queue
     * @param queue Unique pointer to policy_queue
     */
    explicit policy_queue_adapter(std::unique_ptr<queue_type> queue)
        : queue_(std::move(queue)) {}

    ~policy_queue_adapter() override = default;

    // Non-copyable
    policy_queue_adapter(const policy_queue_adapter&) = delete;
    policy_queue_adapter& operator=(const policy_queue_adapter&) = delete;

    // Movable
    policy_queue_adapter(policy_queue_adapter&&) = default;
    policy_queue_adapter& operator=(policy_queue_adapter&&) = default;

    [[nodiscard]] auto enqueue(std::unique_ptr<job>&& j) -> common::VoidResult override {
        return queue_->enqueue(std::move(j));
    }

    [[nodiscard]] auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> common::VoidResult override {
        // policy_queue doesn't have enqueue_batch, so we implement it as a loop
        for (auto& j : jobs) {
            auto result = queue_->enqueue(std::move(j));
            if (result.is_err()) {
                return result;
            }
        }
        return common::ok();
    }

    [[nodiscard]] auto dequeue() -> common::Result<std::unique_ptr<job>> override {
        return queue_->dequeue();
    }

    [[nodiscard]] auto try_dequeue() -> common::Result<std::unique_ptr<job>> override {
        return queue_->try_dequeue();
    }

    [[nodiscard]] auto empty() const -> bool override {
        return queue_->empty();
    }

    [[nodiscard]] auto size() const -> std::size_t override {
        return queue_->size();
    }

    auto clear() -> void override {
        queue_->clear();
    }

    auto stop() -> void override {
        queue_->stop();
    }

    [[nodiscard]] auto is_stopped() const -> bool override {
        return queue_->is_stopped();
    }

    [[nodiscard]] auto get_capabilities() const -> queue_capabilities override {
        return queue_->get_capabilities();
    }

    [[nodiscard]] auto to_string() const -> std::string override {
        return utility_module::formatter::format(
            "policy_queue[size={}, stopped={}]",
            queue_->size(),
            queue_->is_stopped());
    }

    [[nodiscard]] auto get_job_queue() const -> std::shared_ptr<job_queue> override {
        // policy_queue is not a job_queue, return nullptr
        return nullptr;
    }

    [[nodiscard]] auto get_scheduler() -> scheduler_interface& override {
        return *queue_;
    }

    [[nodiscard]] auto get_scheduler() const -> const scheduler_interface& override {
        return *queue_;
    }

    /**
     * @brief Get direct access to the underlying policy_queue
     * @return Reference to the policy_queue
     */
    [[nodiscard]] auto get_policy_queue() -> queue_type& {
        return *queue_;
    }

    /**
     * @brief Get direct access to the underlying policy_queue (const)
     * @return Const reference to the policy_queue
     */
    [[nodiscard]] auto get_policy_queue() const -> const queue_type& {
        return *queue_;
    }

private:
    std::unique_ptr<queue_type> queue_;
};

// ============================================
// Type aliases for common configurations
// ============================================

/**
 * @brief Adapter for standard_queue (mutex-based, unbounded)
 */
using standard_queue_adapter = policy_queue_adapter<
    policies::mutex_sync_policy,
    policies::unbounded_policy,
    policies::overflow_reject_policy
>;

/**
 * @brief Adapter for lock-free queue
 */
using lockfree_queue_adapter = policy_queue_adapter<
    policies::lockfree_sync_policy,
    policies::unbounded_policy,
    policies::overflow_reject_policy
>;

// ============================================
// Factory functions
// ============================================

/**
 * @brief Create a pool_queue_adapter from a policy_queue type
 * @tparam PolicyQueueType A policy_queue instantiation
 * @return Unique pointer to pool_queue_adapter_interface
 *
 * @code
 * auto adapter = make_policy_queue_adapter<standard_queue>();
 * @endcode
 */
template<typename PolicyQueueType>
[[nodiscard]] auto make_policy_queue_adapter()
    -> std::unique_ptr<pool_queue_adapter_interface>
{
    using adapter_type = policy_queue_adapter<
        typename PolicyQueueType::sync_policy_type,
        typename PolicyQueueType::bound_policy_type,
        typename PolicyQueueType::overflow_policy_type
    >;
    return std::make_unique<adapter_type>();
}

/**
 * @brief Create a pool_queue_adapter with bound policy
 * @tparam PolicyQueueType A policy_queue instantiation
 * @param bound_policy Bound policy configuration
 * @return Unique pointer to pool_queue_adapter_interface
 */
template<typename PolicyQueueType>
[[nodiscard]] auto make_policy_queue_adapter(
    typename PolicyQueueType::bound_policy_type bound_policy)
    -> std::unique_ptr<pool_queue_adapter_interface>
{
    using adapter_type = policy_queue_adapter<
        typename PolicyQueueType::sync_policy_type,
        typename PolicyQueueType::bound_policy_type,
        typename PolicyQueueType::overflow_policy_type
    >;
    return std::make_unique<adapter_type>(std::move(bound_policy));
}

/**
 * @brief Create a standard_queue adapter
 * @return Unique pointer to pool_queue_adapter_interface
 */
[[nodiscard]] inline auto make_standard_queue_adapter()
    -> std::unique_ptr<pool_queue_adapter_interface>
{
    return std::make_unique<standard_queue_adapter>();
}

/**
 * @brief Create a lock-free queue adapter
 * @return Unique pointer to pool_queue_adapter_interface
 */
[[nodiscard]] inline auto make_lockfree_queue_adapter()
    -> std::unique_ptr<pool_queue_adapter_interface>
{
    return std::make_unique<lockfree_queue_adapter>();
}

} // namespace kcenon::thread
