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

#include <kcenon/thread/core/job_queue.h>
#include <kcenon/thread/lockfree/lockfree_job_queue.h>
#include <kcenon/thread/queue/adaptive_job_queue.h>
#include <kcenon/thread/interfaces/scheduler_interface.h>
#include <kcenon/thread/interfaces/queue_traits.h>
#include <kcenon/thread/policies/policy_queue.h>

#include <memory>
#include <type_traits>

namespace kcenon::thread {

/**
 * @class queue_factory
 * @brief Factory for creating queue instances
 *
 * Provides convenient methods for queue creation based on requirements.
 * Following Kent Beck's Simple Design principle, we now offer only 2 public queue types:
 *
 * ## Queue Selection Guide (Simplified)
 *
 * | Queue Type | Use Case | Key Feature |
 * |------------|----------|-------------|
 * | **adaptive_job_queue** (Recommended) | Most use cases | Auto-optimizing, best of both worlds |
 * | **job_queue** | Blocking wait required | Mutex-based, exact size tracking |
 *
 * ## Recommended Usage
 * @code
 * // Default choice: adaptive_job_queue (auto-optimizing)
 * auto queue = queue_factory::create_adaptive_queue();
 *
 * // When blocking dequeue is essential
 * auto blocking_queue = queue_factory::create_standard_queue();
 *
 * // With size limit (bounded queue merged into job_queue)
 * auto bounded = std::make_shared<job_queue>(1000);  // max_size = 1000
 *
 * // Requirements-based creation
 * queue_factory::requirements reqs;
 * reqs.need_exact_size = true;
 * auto queue = queue_factory::create_for_requirements(reqs);
 *
 * // Environment-optimized
 * auto optimal = queue_factory::create_optimal();
 * @endcode
 *
 * @note Internal implementations (lockfree_job_queue, concurrent_queue) are now
 * in the detail:: namespace and should not be used directly.
 */
class queue_factory {
public:
    /**
     * @brief Queue selection requirements
     *
     * Specifies what features are required from the queue.
     * Used by create_for_requirements() to select the appropriate implementation.
     */
    struct requirements {
        bool need_exact_size = false;       ///< Require exact size()
        bool need_atomic_empty = false;     ///< Require atomic empty()
        bool prefer_lock_free = false;      ///< Prefer lock-free if possible
        bool need_batch_operations = false; ///< Require batch enqueue/dequeue
        bool need_blocking_wait = false;    ///< Require blocking dequeue
    };

    // ============================================
    // Convenience factory methods
    // ============================================

    /**
     * @brief Create standard job_queue
     * @return Shared pointer to new job_queue (same as std::make_shared<job_queue>())
     *
     * Use this when you need:
     * - Exact size() and empty() checks
     * - Batch operations
     * - Blocking dequeue with condition variable wait
     *
     * @note For bounded queue functionality, use:
     * @code
     * auto bounded = std::make_shared<job_queue>(1000);  // max_size = 1000
     * @endcode
     *
     * @see create_adaptive_queue() for the recommended default choice
     */
    [[nodiscard]] static auto create_standard_queue() -> std::shared_ptr<job_queue> {
        return std::make_shared<job_queue>();
    }

    /**
     * @brief Create adaptive queue (RECOMMENDED for most use cases)
     * @param policy Adaptation policy (default: balanced)
     * @return Unique pointer to new adaptive_job_queue
     *
     * This is the **recommended default choice** for most applications.
     *
     * Use this when you:
     * - Want automatic optimization between mutex and lock-free modes
     * - Need high throughput with variable workloads
     * - Are unsure which implementation to choose
     * - Want the best of both worlds (safety + performance)
     *
     * @note Internally uses lock-free queue for high-contention scenarios
     * and mutex-based queue for low-contention scenarios.
     */
    [[nodiscard]] static auto create_adaptive_queue(
        adaptive_job_queue::policy policy = adaptive_job_queue::policy::balanced)
        -> std::unique_ptr<adaptive_job_queue>
    {
        return std::make_unique<adaptive_job_queue>(policy);
    }

    // ============================================
    // Requirements-based factory method
    // ============================================

    /**
     * @brief Create queue based on requirements
     * @param reqs Requirements specification
     * @return Scheduler interface pointer to appropriate queue type
     *
     * Selection logic:
     * - If need_exact_size or need_atomic_empty or need_batch_operations
     *   or need_blocking_wait: returns job_queue
     * - If prefer_lock_free and no accuracy needs: returns lockfree_job_queue
     * - Otherwise: returns adaptive_job_queue
     */
    [[nodiscard]] static auto create_for_requirements(const requirements& reqs)
        -> std::unique_ptr<scheduler_interface>;

    // ============================================
    // Environment-based auto-selection
    // ============================================

    /**
     * @brief Create optimal queue for current environment
     * @return Scheduler interface pointer to queue
     *
     * Considers:
     * - CPU architecture (x86 vs ARM memory model)
     * - Number of CPU cores
     * - Default: adaptive_job_queue for flexibility
     *
     * Selection:
     * - ARM/weak memory model: job_queue (safety)
     * - Low core count (<=2): job_queue (mutex efficient enough)
     * - Otherwise: adaptive_job_queue (best of both worlds)
     */
    [[nodiscard]] static auto create_optimal() -> std::unique_ptr<scheduler_interface>;

    // ============================================
    // Policy-based queue factory methods
    // ============================================

    /**
     * @brief Create a standard policy_queue (mutex-based, unbounded)
     * @return Unique pointer to standard_queue
     *
     * This creates a policy_queue with:
     * - mutex_sync_policy: Thread-safe with blocking support
     * - unbounded_policy: No size limits
     * - overflow_reject_policy: Rejects on overflow (not applicable for unbounded)
     *
     * Use this as the modern replacement for legacy job_queue.
     *
     * @code
     * auto queue = queue_factory::create_policy_queue();
     * queue->schedule(std::make_unique<my_job>());
     * @endcode
     */
    [[nodiscard]] static auto create_policy_queue()
        -> std::unique_ptr<standard_queue>;

    /**
     * @brief Create a lock-free policy_queue
     * @return Unique pointer to policy_lockfree_queue
     *
     * This creates a policy_queue with:
     * - lockfree_sync_policy: High-throughput, non-blocking
     * - unbounded_policy: No size limits
     * - overflow_reject_policy: Rejects on overflow (not applicable for unbounded)
     *
     * Use this for high-contention scenarios.
     *
     * @note size() returns approximate values, empty() is non-atomic
     */
    [[nodiscard]] static auto create_lockfree_policy_queue()
        -> std::unique_ptr<policy_lockfree_queue>;

    /**
     * @brief Create a bounded policy_queue with specified size
     * @param max_size Maximum number of jobs the queue can hold
     * @return Unique pointer to bounded queue
     *
     * This creates a policy_queue with:
     * - mutex_sync_policy: Thread-safe with blocking support
     * - bounded_policy: Limited to max_size items
     * - overflow_reject_policy: Returns error when full
     *
     * @code
     * auto queue = queue_factory::create_bounded_policy_queue(1000);
     * auto result = queue->schedule(std::make_unique<my_job>());
     * if (result.is_err()) {
     *     // Queue was full
     * }
     * @endcode
     */
    [[nodiscard]] static auto create_bounded_policy_queue(std::size_t max_size)
        -> std::unique_ptr<policy_queue<
            policies::mutex_sync_policy,
            policies::bounded_policy,
            policies::overflow_reject_policy>>;

    /**
     * @brief Create a policy_queue with custom policies
     * @tparam SyncPolicy Synchronization policy type
     * @tparam BoundPolicy Bounding policy type
     * @tparam OverflowPolicy Overflow handling policy type
     * @param sync_policy Sync policy instance
     * @param bound_policy Bound policy instance
     * @param overflow_policy Overflow policy instance
     * @return Unique pointer to the configured policy_queue
     *
     * This factory method allows full customization of queue behavior.
     *
     * @code
     * auto queue = queue_factory::create_custom_policy_queue(
     *     policies::mutex_sync_policy{},
     *     policies::bounded_policy{100},
     *     policies::overflow_drop_oldest_policy{}
     * );
     * @endcode
     */
    template<typename SyncPolicy, typename BoundPolicy, typename OverflowPolicy>
    [[nodiscard]] static auto create_custom_policy_queue(
        SyncPolicy sync_policy,
        BoundPolicy bound_policy,
        OverflowPolicy overflow_policy)
        -> std::unique_ptr<policy_queue<SyncPolicy, BoundPolicy, OverflowPolicy>>
    {
        static_assert(is_sync_policy_v<SyncPolicy>,
            "SyncPolicy must be a valid sync policy type");
        static_assert(is_bound_policy_v<BoundPolicy>,
            "BoundPolicy must be a valid bound policy type");
        static_assert(is_overflow_policy_v<OverflowPolicy>,
            "OverflowPolicy must be a valid overflow policy type");

        return std::make_unique<policy_queue<SyncPolicy, BoundPolicy, OverflowPolicy>>(
            std::move(sync_policy),
            std::move(bound_policy),
            std::move(overflow_policy)
        );
    }
};

// ============================================
// Compile-time selection utilities
// ============================================

/**
 * @brief Compile-time queue type selector
 * @tparam NeedExactSize If true, selects job_queue
 * @tparam PreferLockFree If true and NeedExactSize is false, selects adaptive_job_queue
 *
 * This template allows compile-time selection of queue types based on requirements.
 * Invalid combinations (exact size + lock-free) trigger a static_assert.
 *
 * @note lockfree_job_queue is now an internal implementation detail.
 * PreferLockFree now selects adaptive_job_queue which wraps lockfree_job_queue internally.
 *
 * @code
 * // Compile-time selection
 * queue_type_selector<true, false>::type accurate_queue;   // job_queue
 * queue_type_selector<false, true>::type fast_queue;       // adaptive_job_queue
 * queue_type_selector<false, false>::type balanced_queue;  // adaptive_job_queue
 * @endcode
 */
template<bool NeedExactSize, bool PreferLockFree = false>
struct queue_type_selector {
    static_assert(!(NeedExactSize && PreferLockFree),
        "Cannot require both exact size AND lock-free. These are mutually exclusive. "
        "Lock-free queues cannot provide exact size due to concurrent modifications.");

    // Both PreferLockFree and default case now use adaptive_job_queue
    // (lockfree_job_queue is an internal implementation detail)
    using type = std::conditional_t<
        NeedExactSize,
        job_queue,
        adaptive_job_queue
    >;
};

/**
 * @brief Convenience alias for queue type selection
 * @tparam NeedExactSize If true, selects job_queue
 * @tparam PreferLockFree If true and NeedExactSize is false, selects lockfree_job_queue
 */
template<bool NeedExactSize, bool PreferLockFree = false>
using queue_t = typename queue_type_selector<NeedExactSize, PreferLockFree>::type;

// ============================================
// Pre-defined type aliases
// ============================================

/// @brief Queue type for accurate size/empty operations (job_queue)
using accurate_queue_t = queue_t<true, false>;

/// @brief Queue type for maximum throughput (adaptive_job_queue with performance_first policy)
/// @note lockfree_job_queue is now an internal implementation detail
using fast_queue_t = queue_t<false, true>;

/// @brief Queue type for balanced performance (adaptive_job_queue)
using balanced_queue_t = queue_t<false, false>;

} // namespace kcenon::thread
