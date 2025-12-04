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
#include <thread>
#include <type_traits>

namespace kcenon::thread {

/**
 * @class queue_factory
 * @brief Factory for creating queue instances based on requirements.
 *
 * This class provides convenient methods for queue creation based on requirements.
 * It is a NEW utility class that does not modify any existing code.
 *
 * @note This is an additive utility. Existing code that creates queues directly
 *       continues to work unchanged:
 * @code
 * // These still work exactly as before:
 * auto q1 = std::make_shared<job_queue>();
 * auto q2 = std::make_unique<lockfree_job_queue>();
 * @endcode
 *
 * ### Usage Examples
 * @code
 * // Convenience factory methods
 * auto standard = queue_factory::create_standard_queue();
 * auto lockfree = queue_factory::create_lockfree_queue();
 * auto adaptive = queue_factory::create_adaptive_queue();
 *
 * // Requirements-based creation
 * queue_factory::requirements reqs;
 * reqs.need_exact_size = true;
 * auto queue = queue_factory::create_for_requirements(reqs);
 *
 * // Environment-optimized creation
 * auto optimal = queue_factory::create_optimal();
 * @endcode
 */
class queue_factory {
public:
    /**
     * @brief Queue selection requirements specification.
     *
     * Use this struct to specify what capabilities you need from a queue.
     * The factory will select the most appropriate queue type based on these requirements.
     */
    struct requirements {
        bool need_exact_size = false;       ///< Require exact size() return value
        bool need_atomic_empty = false;     ///< Require atomic empty() check
        bool prefer_lock_free = false;      ///< Prefer lock-free if requirements allow
        bool need_batch_operations = false; ///< Require batch enqueue/dequeue
        bool need_blocking_wait = false;    ///< Require blocking dequeue
    };

    // ============================================
    // Convenience factory methods
    // ============================================

    /**
     * @brief Create standard mutex-based job_queue.
     * @return Shared pointer to new job_queue
     *
     * @note Same as std::make_shared<job_queue>(), provided for API consistency.
     *
     * Characteristics:
     * - Exact size() and empty()
     * - Blocking dequeue support
     * - Batch operations support
     * - ~300K ops/sec throughput
     */
    [[nodiscard]] static auto create_standard_queue() -> std::shared_ptr<job_queue> {
        return std::make_shared<job_queue>();
    }

    /**
     * @brief Create lock-free queue.
     * @return Unique pointer to new lockfree_job_queue
     *
     * Characteristics:
     * - Approximate size() (may drift under contention)
     * - Non-atomic empty() (snapshot view)
     * - No blocking wait (spin-wait via try_dequeue)
     * - ~1.2M ops/sec throughput
     */
    [[nodiscard]] static auto create_lockfree_queue() -> std::unique_ptr<lockfree_job_queue> {
        return std::make_unique<lockfree_job_queue>();
    }

    /**
     * @brief Create adaptive queue with specified policy.
     * @param p Adaptation policy (default: balanced)
     * @return Unique pointer to new adaptive_job_queue
     *
     * The adaptive queue automatically switches between mutex and lock-free modes
     * based on the policy and usage patterns.
     */
    [[nodiscard]] static auto create_adaptive_queue(
        adaptive_job_queue::policy p = adaptive_job_queue::policy::balanced)
        -> std::unique_ptr<adaptive_job_queue>
    {
        return std::make_unique<adaptive_job_queue>(p);
    }

    // ============================================
    // Requirements-based factory method
    // ============================================

    /**
     * @brief Create queue based on requirements specification.
     * @param reqs Requirements specification
     * @return Scheduler interface pointer to appropriate queue type
     *
     * Selection logic:
     * - If need_exact_size, need_atomic_empty, need_batch_operations,
     *   or need_blocking_wait: returns job_queue
     * - If prefer_lock_free and no accuracy needs: returns lockfree_job_queue
     * - Otherwise: returns adaptive_job_queue
     *
     * @code
     * queue_factory::requirements reqs;
     * reqs.need_exact_size = true;
     * auto queue = queue_factory::create_for_requirements(reqs);
     * // Returns job_queue because exact_size was requested
     * @endcode
     */
    [[nodiscard]] static auto create_for_requirements(const requirements& reqs)
        -> std::unique_ptr<scheduler_interface>;

    // ============================================
    // Environment-based auto-selection
    // ============================================

    /**
     * @brief Create optimal queue for current environment.
     * @return Scheduler interface pointer to queue
     *
     * Considers:
     * - CPU architecture (x86 vs ARM memory model)
     * - Number of CPU cores
     *
     * Selection logic:
     * - ARM/weak memory model: returns job_queue (safer)
     * - Low core count (<=2): returns job_queue (mutex is efficient)
     * - Otherwise: returns adaptive_job_queue (best of both worlds)
     */
    [[nodiscard]] static auto create_optimal() -> std::unique_ptr<scheduler_interface>;

    // Prevent instantiation - all methods are static
    queue_factory() = delete;
};

// ============================================
// Compile-time selection utilities
// ============================================

/**
 * @brief Compile-time queue type selector.
 * @tparam NeedExactSize If true, selects job_queue
 * @tparam PreferLockFree If true and NeedExactSize is false, selects lockfree_job_queue
 *
 * This template allows compile-time selection of queue types based on requirements.
 * Invalid combinations (both exact size AND lock-free) are caught at compile time.
 *
 * @code
 * // Compile-time selection
 * queue_t<true, false> accurate;  // job_queue
 * queue_t<false, true> fast;      // lockfree_job_queue
 * queue_t<false, false> balanced; // adaptive_job_queue
 * @endcode
 */
template<bool NeedExactSize, bool PreferLockFree = false>
struct queue_type_selector {
    static_assert(!(NeedExactSize && PreferLockFree),
        "Cannot require both exact size AND lock-free. These are mutually exclusive.");

    using type = std::conditional_t<
        NeedExactSize,
        job_queue,
        std::conditional_t<PreferLockFree, lockfree_job_queue, adaptive_job_queue>
    >;
};

/**
 * @brief Convenience alias for queue type selection.
 * @tparam NeedExactSize If true, selects job_queue
 * @tparam PreferLockFree If true and NeedExactSize is false, selects lockfree_job_queue
 */
template<bool NeedExactSize, bool PreferLockFree = false>
using queue_t = typename queue_type_selector<NeedExactSize, PreferLockFree>::type;

// ============================================
// Pre-defined type aliases for common cases
// ============================================

/**
 * @brief Queue type for accuracy-first use cases.
 *
 * Use when you need:
 * - Exact size() values
 * - Atomic empty() checks
 * - Batch operations
 * - Blocking wait
 */
using accurate_queue_t = queue_t<true, false>;   // job_queue

/**
 * @brief Queue type for performance-first use cases.
 *
 * Use when you need:
 * - Maximum throughput (>500K ops/sec)
 * - Lock-free operations
 * - Don't need exact size/empty
 */
using fast_queue_t = queue_t<false, true>;       // lockfree_job_queue

/**
 * @brief Queue type for balanced/adaptive use cases.
 *
 * Use when you:
 * - Want automatic optimization
 * - Need temporary accuracy sometimes
 * - Are unsure which to choose
 */
using balanced_queue_t = queue_t<false, false>;  // adaptive_job_queue

} // namespace kcenon::thread
