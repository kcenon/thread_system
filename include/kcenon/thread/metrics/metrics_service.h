/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, kcenon
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

#include <kcenon/thread/metrics/thread_pool_metrics.h>
#include <kcenon/thread/metrics/enhanced_metrics.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>

namespace kcenon::thread::metrics {

/**
 * @brief Centralized metrics service for thread pool metrics management.
 *
 * @ingroup metrics
 *
 * This class provides a unified interface for metrics handling, consolidating
 * metrics management that was previously duplicated between thread_pool and
 * thread_worker classes.
 *
 * ### Ownership Semantics
 * - thread_pool owns the metrics_service (via shared_ptr)
 * - thread_worker receives a non-owning pointer to the service
 *
 * ### Thread Safety
 * All methods are thread-safe using lock-free atomic operations and
 * internal synchronization.
 *
 * ### Performance Characteristics
 * - record_* overhead: < 100ns
 * - Minimal memory footprint due to lazy initialization
 *
 * ### Usage Example
 * @code
 * // In thread_pool constructor
 * metrics_service_ = std::make_shared<metrics_service>();
 *
 * // In thread_pool::set_enhanced_metrics_enabled
 * metrics_service_->set_enhanced_metrics_enabled(true, worker_count);
 *
 * // In thread_worker (non-owning reference)
 * metrics_service_->record_execution(duration_ns, success);
 * @endcode
 *
 * @see ThreadPoolMetrics
 * @see EnhancedThreadPoolMetrics
 */
class metrics_service {
public:
    /**
     * @brief Default constructor.
     *
     * Creates the basic ThreadPoolMetrics immediately.
     * EnhancedThreadPoolMetrics is lazily initialized when enabled.
     */
    metrics_service();

    /**
     * @brief Destructor.
     */
    ~metrics_service() = default;

    // Non-copyable, non-movable for thread safety
    metrics_service(const metrics_service&) = delete;
    metrics_service& operator=(const metrics_service&) = delete;
    metrics_service(metrics_service&&) = delete;
    metrics_service& operator=(metrics_service&&) = delete;

    // =========================================================================
    // Recording Methods (called by thread_pool and thread_worker)
    // =========================================================================

    /**
     * @brief Record task submission(s).
     * @param count Number of tasks submitted (default: 1).
     */
    void record_submission(std::size_t count = 1);

    /**
     * @brief Record enqueue operation(s).
     * @param count Number of tasks enqueued (default: 1).
     */
    void record_enqueue(std::size_t count = 1);

    /**
     * @brief Record enqueue operation with latency measurement.
     * @param latency Time taken to enqueue the task.
     * @param count Number of tasks enqueued (default: 1).
     *
     * Used when enhanced metrics are enabled to track enqueue latency histogram.
     */
    void record_enqueue_with_latency(std::chrono::nanoseconds latency, std::size_t count = 1);

    /**
     * @brief Record task execution completion.
     * @param duration_ns Execution duration in nanoseconds.
     * @param success Whether the task completed successfully.
     */
    void record_execution(std::uint64_t duration_ns, bool success);

    /**
     * @brief Record task execution with wait time tracking.
     * @param duration Execution duration.
     * @param wait_time Time the task waited in queue.
     * @param success Whether the task completed successfully.
     *
     * Used when enhanced metrics are enabled.
     */
    void record_execution_with_wait_time(
        std::chrono::nanoseconds duration,
        std::chrono::nanoseconds wait_time,
        bool success);

    /**
     * @brief Record idle time.
     * @param duration_ns Idle duration in nanoseconds.
     */
    void record_idle_time(std::uint64_t duration_ns);

    /**
     * @brief Record current queue depth.
     * @param depth The current number of tasks in the queue.
     */
    void record_queue_depth(std::size_t depth);

    /**
     * @brief Update worker state for per-worker metrics.
     * @param worker_id The worker's identifier.
     * @param busy Whether the worker is currently busy.
     * @param duration_ns Duration in the previous state.
     */
    void record_worker_state(
        std::size_t worker_id,
        bool busy,
        std::uint64_t duration_ns = 0);

    // =========================================================================
    // Enhanced Metrics Control
    // =========================================================================

    /**
     * @brief Enable or disable enhanced metrics collection.
     * @param enabled True to enable enhanced metrics.
     * @param worker_count Number of workers to track (for initialization).
     *
     * When enabled for the first time, initializes EnhancedThreadPoolMetrics.
     * Subsequent calls only toggle the enabled flag.
     */
    void set_enhanced_metrics_enabled(bool enabled, std::size_t worker_count = 0);

    /**
     * @brief Check if enhanced metrics is enabled.
     * @return True if enhanced metrics collection is enabled.
     */
    [[nodiscard]] bool is_enhanced_metrics_enabled() const;

    /**
     * @brief Update the worker count for enhanced metrics.
     * @param count New number of workers.
     *
     * Call this when the thread pool scales up or down.
     */
    void update_worker_count(std::size_t count);

    /**
     * @brief Set the number of active workers.
     * @param count Number of currently active workers.
     */
    void set_active_workers(std::size_t count);

    // =========================================================================
    // Query Methods
    // =========================================================================

    /**
     * @brief Access basic metrics (read-only reference).
     * @return Reference to ThreadPoolMetrics.
     */
    [[nodiscard]] const ThreadPoolMetrics& basic_metrics() const noexcept;

    /**
     * @brief Access enhanced metrics (read-only reference).
     * @return Reference to EnhancedThreadPoolMetrics.
     * @throw std::runtime_error if enhanced metrics is not enabled.
     */
    [[nodiscard]] const EnhancedThreadPoolMetrics& enhanced_metrics() const;

    /**
     * @brief Get enhanced metrics snapshot.
     * @return EnhancedSnapshot with all current metric values.
     *
     * Returns empty snapshot if enhanced metrics is not enabled.
     */
    [[nodiscard]] EnhancedSnapshot enhanced_snapshot() const;

    /**
     * @brief Get the shared pointer to basic metrics.
     * @return Shared pointer to ThreadPoolMetrics.
     *
     * Used by thread_worker for direct metrics recording.
     */
    [[nodiscard]] std::shared_ptr<ThreadPoolMetrics> get_basic_metrics() const noexcept;

    // =========================================================================
    // Management Methods
    // =========================================================================

    /**
     * @brief Reset all metrics to their initial state.
     *
     * Resets both basic and enhanced metrics (if enabled).
     */
    void reset();

private:
    /**
     * @brief Basic metrics collector.
     *
     * Always initialized; provides core metrics functionality.
     */
    std::shared_ptr<ThreadPoolMetrics> basic_metrics_;

    /**
     * @brief Enhanced metrics collector.
     *
     * Lazily initialized when enhanced metrics is first enabled.
     */
    std::shared_ptr<EnhancedThreadPoolMetrics> enhanced_metrics_;

    /**
     * @brief Flag indicating if enhanced metrics collection is enabled.
     */
    std::atomic<bool> enhanced_enabled_{false};

    /**
     * @brief Mutex for thread-safe enhanced metrics initialization.
     */
    mutable std::mutex init_mutex_;
};

} // namespace kcenon::thread::metrics
