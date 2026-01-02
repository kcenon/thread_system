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

#include <memory>
#include <type_traits>

namespace kcenon::thread {

/**
 * @class queue_factory
 * @brief Factory for creating queue instances
 *
 * Provides convenient methods for queue creation based on requirements.
 * This is a NEW utility class that does not break any existing code.
 *
 * @note Existing code that creates queues directly continues to work unchanged:
 * @code
 * // These still work exactly as before:
 * auto q1 = std::make_shared<job_queue>();
 * auto q2 = std::make_unique<lockfree_job_queue>();
 * @endcode
 *
 * ### Factory Usage
 * @code
 * // Convenience methods
 * auto standard = queue_factory::create_standard_queue();
 * auto lockfree = queue_factory::create_lockfree_queue();
 * auto adaptive = queue_factory::create_adaptive_queue();
 *
 * // Requirements-based creation
 * queue_factory::requirements reqs;
 * reqs.need_exact_size = true;
 * auto queue = queue_factory::create_for_requirements(reqs);
 *
 * // Environment-optimized
 * auto optimal = queue_factory::create_optimal();
 * @endcode
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
     * - Blocking dequeue
     */
    [[nodiscard]] static auto create_standard_queue() -> std::shared_ptr<job_queue> {
        return std::make_shared<job_queue>();
    }

    /**
     * @brief Create lock-free queue
     * @return Unique pointer to new adaptive_job_queue with performance_first policy
     *
     * @deprecated Use create_adaptive_queue(policy::performance_first) instead.
     * lockfree_job_queue is now an internal implementation detail.
     *
     * Use this when you need:
     * - Maximum throughput (>500K ops/sec)
     * - Many concurrent producers/consumers
     * - Non-blocking try_dequeue() pattern
     *
     * @note size() returns approximate values, empty() is non-atomic
     */
    [[deprecated("Use create_adaptive_queue(policy::performance_first) instead")]]
    [[nodiscard]] static auto create_lockfree_queue() -> std::unique_ptr<adaptive_job_queue> {
        return std::make_unique<adaptive_job_queue>(adaptive_job_queue::policy::performance_first);
    }

    /**
     * @brief Create adaptive queue
     * @param policy Adaptation policy (default: balanced)
     * @return Unique pointer to new adaptive_job_queue
     *
     * Use this when you:
     * - Want automatic optimization
     * - Need temporary accuracy sometimes
     * - Are unsure which implementation to choose
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
