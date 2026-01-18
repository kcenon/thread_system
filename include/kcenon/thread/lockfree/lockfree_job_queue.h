/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
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

#include <kcenon/thread/core/job.h>
#include <kcenon/thread/core/safe_hazard_pointer.h>
#include <kcenon/thread/core/error_handling.h>
#include <kcenon/thread/interfaces/scheduler_interface.h>
#include <kcenon/thread/interfaces/queue_capabilities_interface.h>

#include <atomic>
#include <memory>
#include <optional>

namespace kcenon::thread {

namespace detail {

/**
 * @class lockfree_job_queue
 * @brief Lock-free Multi-Producer Multi-Consumer (MPMC) job queue (Internal implementation)
 *
 * This class implements a lock-free MPMC queue using the Michael-Scott algorithm
 * with Safe Hazard Pointers for memory reclamation. It uses explicit memory
 * ordering to ensure correctness on weak memory model architectures (ARM, etc.)
 *
 * Algorithm: Michael-Scott Queue (1996)
 * Memory Reclamation: Safe Hazard Pointers with explicit memory ordering
 *
 * Key Features:
 * - True lock-free operation (no mutexes, no locks)
 * - Safe concurrent access from multiple producers and consumers
 * - Automatic memory reclamation using Safe Hazard Pointers
 * - Correct memory ordering for weak memory model architectures (ARM)
 * - No TLS node pool (eliminates destructor ordering issues)
 * - ABA problem prevention through HP-based protection
 *
 * Performance Characteristics:
 * - Enqueue: O(1) amortized, wait-free
 * - Dequeue: O(1) amortized, lock-free
 * - Memory overhead: ~256 bytes per thread (hazard pointers)
 *
 * Thread Safety:
 * - All methods are thread-safe
 * - Can be called concurrently from any number of threads
 * - Uses atomic operations with acquire/release semantics
 *
 * @note This implementation is production-safe and resolves TICKET-001 (TLS bug)
 *       and TICKET-002 (weak memory model safety).
 *
 * @see lockfree_job_queue_test.cpp for usage examples
 */
class lockfree_job_queue : public scheduler_interface,
                           public queue_capabilities_interface {
public:
    /**
     * @brief Constructs an empty lock-free job queue
     *
     * Initializes the queue with a dummy node to simplify the algorithm.
     * The dummy node is never removed, allowing concurrent enqueue/dequeue.
     */
    lockfree_job_queue();

    /**
     * @brief Destructor
     *
     * Drains the queue and reclaims all nodes. Thread-safe even if other
     * threads are still accessing the queue (they will get errors).
     */
    ~lockfree_job_queue();

    // Non-copyable and non-movable (queue has shared global state)
    lockfree_job_queue(const lockfree_job_queue&) = delete;
    lockfree_job_queue& operator=(const lockfree_job_queue&) = delete;
    lockfree_job_queue(lockfree_job_queue&&) = delete;
    lockfree_job_queue& operator=(lockfree_job_queue&&) = delete;

    /**
     * @brief Enqueues a job into the queue (thread-safe)
     *
     * @param job Unique pointer to the job to enqueue
     * @return common::VoidResult Success or error
     *
     * @note Wait-free operation (bounded number of steps)
     * @note Takes ownership of the job pointer
     * @note Never blocks, always makes progress
     *
     * Time Complexity: O(1) amortized
     * Memory Ordering: release semantics for visibility
     */
    [[nodiscard]] auto enqueue(std::unique_ptr<job>&& job) -> common::VoidResult;

    /**
     * @brief Dequeues a job from the queue (thread-safe)
     *
     * @return common::Result<std::unique_ptr<job>> The dequeued job or error
     *
     * @note Lock-free operation (system-wide progress guaranteed)
     * @note Returns empty result if queue is empty (not an error)
     * @note Uses Hazard Pointers to protect nodes from premature deletion
     * @note Retired nodes are eventually reclaimed by the HP domain
     *
     * Time Complexity: O(1) amortized
     * Memory Ordering: acquire/release semantics
     */
    [[nodiscard]] auto dequeue() -> common::Result<std::unique_ptr<job>>;

    /**
     * @brief Tries to dequeue a job without blocking
     *
     * @return common::Result<std::unique_ptr<job>> The dequeued job or empty
     *
     * @note Alias for dequeue() (lock-free queues never block)
     * @note Provided for API compatibility with mutex-based queue
     */
    [[nodiscard]] auto try_dequeue() -> common::Result<std::unique_ptr<job>> {
        return dequeue();
    }

    /**
     * @brief Checks if the queue is empty
     *
     * @return true if queue appears empty, false otherwise
     *
     * @note This is a snapshot view; queue may change immediately after
     * @note Use for hints only, not for synchronization
     */
    [[nodiscard]] auto empty() const -> bool;

    /**
     * @brief Gets approximate queue size
     *
     * @return Approximate number of jobs in queue
     *
     * @note This is a best-effort estimate due to concurrent modifications
     * @note Use for monitoring/debugging, not for correctness
     */
    [[nodiscard]] auto size() const -> std::size_t;

    // ============================================
    // scheduler_interface implementation
    // ============================================

    /**
     * @brief Schedule a job (delegates to enqueue)
     *
     * @param work Job to schedule
     * @return common::VoidResult Success or error
     *
     * @note Part of scheduler_interface
     */
    auto schedule(std::unique_ptr<job>&& work) -> common::VoidResult override {
        return enqueue(std::move(work));
    }

    /**
     * @brief Get next job (delegates to dequeue)
     *
     * @return common::Result<std::unique_ptr<job>> The dequeued job or error
     *
     * @note Part of scheduler_interface
     */
    auto get_next_job() -> common::Result<std::unique_ptr<job>> override {
        return dequeue();
    }

    // ============================================
    // queue_capabilities_interface implementation
    // ============================================

    /**
     * @brief Returns capabilities of lockfree_job_queue
     *
     * @return queue_capabilities with lock-free characteristics
     *
     * @warning size() is APPROXIMATE, empty() is NON-ATOMIC
     *
     * Capabilities:
     * - exact_size: false (approximate only due to concurrent modifications)
     * - atomic_empty_check: false (snapshot view, may change immediately)
     * - lock_free: true (uses lock-free Michael-Scott algorithm)
     * - wait_free: false (enqueue is wait-free, dequeue is lock-free)
     * - supports_batch: false (no batch operations available)
     * - supports_blocking_wait: false (spin-wait only via try_dequeue)
     * - supports_stop: false (no stop() method available)
     */
    [[nodiscard]] auto get_capabilities() const -> queue_capabilities override {
        return queue_capabilities{
            .exact_size = false,             // Approximate only
            .atomic_empty_check = false,     // Non-atomic
            .lock_free = true,               // Lock-free implementation
            .wait_free = false,              // Not wait-free
            .supports_batch = false,         // No batch operations
            .supports_blocking_wait = false, // Spin-wait only
            .supports_stop = false           // No stop() method
        };
    }

private:
    /**
     * @brief Internal queue node structure
     *
     * Nodes use atomic pointers for lock-free traversal.
     * Each node holds one job (or nullptr for dummy node).
     */
    struct node {
        std::unique_ptr<job> data;          // Payload (nullptr for dummy node)
        std::atomic<node*> next{nullptr};   // Next node in queue

        explicit node(std::unique_ptr<job>&& job_data)
            : data(std::move(job_data))
            , next(nullptr) {}
    };

    // Queue state: head and tail pointers
    std::atomic<node*> head_;  // Dequeue end (with acquire/release)
    std::atomic<node*> tail_;  // Enqueue end (with acquire/release)

    // Hazard pointer domain for safe memory reclamation
    // Uses safe_hazard_pointer with explicit memory ordering (safe on ARM/weak memory models)
    using node_hp_domain = typed_safe_hazard_domain<node>;

    // Statistics (for monitoring, relaxed memory order)
    mutable std::atomic<std::size_t> approximate_size_{0};

    // Shutdown flag for safe destruction
    std::atomic<bool> shutdown_{false};
};

}  // namespace detail

}  // namespace kcenon::thread
