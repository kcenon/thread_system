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
#include <kcenon/thread/interfaces/queue_capabilities_interface.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>

namespace kcenon::thread {

/**
 * @class adaptive_job_queue
 * @brief Adaptive queue that switches between mutex and lock-free modes
 *
 * This class WRAPS existing job_queue and lockfree_job_queue,
 * providing automatic or manual mode switching based on requirements.
 *
 * @note This is a NEW class. Existing job_queue and lockfree_job_queue
 *       are UNCHANGED and can still be used directly.
 *
 * ### Key Features
 * - Wraps both mutex-based and lock-free queue implementations
 * - Supports multiple selection policies (accuracy, performance, balanced, manual)
 * - Provides RAII guard for temporary accuracy mode
 * - Thread-safe mode switching with data migration
 * - Statistics tracking for mode switches and time spent in each mode
 *
 * ### Usage
 * @code
 * // Create adaptive queue (defaults to balanced policy)
 * auto queue = std::make_unique<adaptive_job_queue>();
 *
 * // Use like any other queue
 * queue->enqueue(std::make_unique<my_job>());
 * auto job = queue->dequeue();
 *
 * // Temporarily require accuracy
 * {
 *     auto guard = queue->require_accuracy();
 *     size_t exact = queue->size();  // Now guaranteed exact
 * }
 * @endcode
 */
class adaptive_job_queue : public scheduler_interface,
                           public queue_capabilities_interface {
public:
    /**
     * @brief Operating mode
     */
    enum class mode {
        mutex,     ///< Using job_queue (accuracy mode)
        lock_free  ///< Using lockfree_job_queue (performance mode)
    };

    /**
     * @brief Selection policy
     */
    enum class policy {
        accuracy_first,    ///< Always use mutex mode
        performance_first, ///< Always use lock-free mode
        balanced,          ///< Auto-switch based on usage
        manual             ///< User controls mode
    };

    // ============================================
    // Construction
    // ============================================

    /**
     * @brief Create adaptive queue with specified policy
     * @param p Selection policy (default: balanced)
     */
    explicit adaptive_job_queue(policy p = policy::balanced);

    /**
     * @brief Destructor - cleans up both queue implementations
     */
    ~adaptive_job_queue();

    // Non-copyable
    adaptive_job_queue(const adaptive_job_queue&) = delete;
    adaptive_job_queue& operator=(const adaptive_job_queue&) = delete;

    // Non-movable (contains shared state)
    adaptive_job_queue(adaptive_job_queue&&) = delete;
    adaptive_job_queue& operator=(adaptive_job_queue&&) = delete;

    // ============================================
    // scheduler_interface implementation
    // ============================================

    /**
     * @brief Schedule a job (delegates to current queue)
     * @param work Job to schedule
     * @return common::VoidResult Success or error
     */
    auto schedule(std::unique_ptr<job>&& work) -> common::VoidResult override;

    /**
     * @brief Get next job (delegates to current queue)
     * @return common::Result<std::unique_ptr<job>> The dequeued job or error
     */
    auto get_next_job() -> common::Result<std::unique_ptr<job>> override;

    // ============================================
    // Standard queue operations
    // ============================================

    /**
     * @brief Enqueues a job into the current active queue
     * @param j Unique pointer to the job being added
     * @return common::VoidResult indicating success or error
     */
    [[nodiscard]] auto enqueue(std::unique_ptr<job>&& j) -> common::VoidResult;

    /**
     * @brief Dequeues a job from the current active queue
     * @return common::Result<std::unique_ptr<job>> The dequeued job or error
     */
    [[nodiscard]] auto dequeue() -> common::Result<std::unique_ptr<job>>;

    /**
     * @brief Tries to dequeue a job without blocking
     * @return common::Result<std::unique_ptr<job>> The dequeued job or error
     */
    [[nodiscard]] auto try_dequeue() -> common::Result<std::unique_ptr<job>>;

    /**
     * @brief Checks if the queue is empty
     * @return true if queue is empty, false otherwise
     * @note Accuracy depends on current mode
     */
    [[nodiscard]] auto empty() const -> bool;

    /**
     * @brief Returns the current number of jobs in the queue
     * @return Number of pending jobs
     * @note Exact in mutex mode, approximate in lock-free mode
     */
    [[nodiscard]] auto size() const -> std::size_t;

    /**
     * @brief Clears all jobs from the queue
     */
    auto clear() -> void;

    /**
     * @brief Signals the queue to stop
     */
    auto stop() -> void;

    /**
     * @brief Checks if the queue is stopped
     * @return true if stopped, false otherwise
     */
    [[nodiscard]] auto is_stopped() const -> bool;

    // ============================================
    // queue_capabilities_interface implementation
    // ============================================

    /**
     * @brief Returns capabilities based on current mode
     * @return Queue capabilities struct
     *
     * @note Capabilities change based on current operating mode:
     * - Mutex mode: exact_size=true, atomic_empty_check=true, lock_free=false
     * - Lock-free mode: exact_size=false, atomic_empty_check=false, lock_free=true
     */
    [[nodiscard]] auto get_capabilities() const -> queue_capabilities override;

    // ============================================
    // Adaptive-specific API
    // ============================================

    /**
     * @brief Get current operating mode
     * @return Current mode (mutex or lock_free)
     */
    [[nodiscard]] auto current_mode() const -> mode;

    /**
     * @brief Get current policy
     * @return Current selection policy
     */
    [[nodiscard]] auto current_policy() const -> policy;

    /**
     * @brief Manually switch mode (only if policy is manual)
     * @param m Target mode to switch to
     * @return result_void Success or error if policy is not manual
     */
    auto switch_mode(mode m) -> common::VoidResult;

    /**
     * @brief Statistics about mode switching
     */
    struct stats {
        uint64_t mode_switches{0};        ///< Total number of mode switches
        uint64_t time_in_mutex_ms{0};     ///< Cumulative time in mutex mode (ms)
        uint64_t time_in_lockfree_ms{0};  ///< Cumulative time in lock-free mode (ms)
        uint64_t enqueue_count{0};        ///< Total enqueue operations
        uint64_t dequeue_count{0};        ///< Total dequeue operations
    };

    /**
     * @brief Get statistics about queue usage
     * @return Current statistics
     */
    [[nodiscard]] auto get_stats() const -> stats;

    // ============================================
    // RAII Guard for temporary accuracy
    // ============================================

    class accuracy_guard;

    /**
     * @brief Request temporary accuracy mode
     * @return Guard object - mode reverts when guard is destroyed
     *
     * @note While the guard is active:
     * - Queue operates in mutex mode
     * - size() returns exact count
     * - empty() is atomically consistent
     *
     * @code
     * auto queue = std::make_unique<adaptive_job_queue>();
     * {
     *     auto guard = queue->require_accuracy();
     *     // In this scope, size() is guaranteed exact
     *     if (queue->size() > 1000) {
     *         apply_backpressure();
     *     }
     * }
     * // After scope, may revert to lock-free mode
     * @endcode
     */
    [[nodiscard]] auto require_accuracy() -> accuracy_guard;

    /**
     * @class accuracy_guard
     * @brief RAII guard that temporarily switches to accuracy mode
     */
    class accuracy_guard {
    public:
        /**
         * @brief Construct guard and switch queue to mutex mode
         * @param queue Reference to the adaptive queue
         */
        explicit accuracy_guard(adaptive_job_queue& queue);

        /**
         * @brief Destructor - restores previous mode
         */
        ~accuracy_guard();

        // Move-only
        accuracy_guard(accuracy_guard&& other) noexcept;
        accuracy_guard& operator=(accuracy_guard&&) = delete;

        // Non-copyable
        accuracy_guard(const accuracy_guard&) = delete;
        accuracy_guard& operator=(const accuracy_guard&) = delete;

    private:
        adaptive_job_queue* queue_;
        mode previous_mode_;
        bool active_;
    };

private:
    // Policy and mode
    policy policy_;
    std::atomic<mode> current_mode_;
    std::atomic<bool> stopped_{false};

    // Wrapped queues (existing classes, unchanged)
    std::shared_ptr<job_queue> mutex_queue_;
    std::unique_ptr<lockfree_job_queue> lockfree_queue_;

    // Synchronization for mode switching
    mutable std::mutex migration_mutex_;

    // Accuracy guard reference count
    std::atomic<int> accuracy_guard_count_{0};

    // Statistics tracking
    mutable std::mutex stats_mutex_;
    stats stats_;
    std::chrono::steady_clock::time_point mode_start_time_;

    // Private methods
    void migrate_to_mode(mode target);
    void update_mode_time();
    auto determine_mode_for_balanced() const -> mode;
};

} // namespace kcenon::thread
