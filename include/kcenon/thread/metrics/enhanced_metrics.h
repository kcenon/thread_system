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

#include <kcenon/thread/metrics/latency_histogram.h>
#include <kcenon/thread/metrics/sliding_window_counter.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace kcenon::thread::metrics {

/**
 * @brief Enhanced snapshot with latency percentiles and throughput.
 *
 * This structure contains all metrics data at a point in time,
 * including basic counters, latency percentiles, throughput rates,
 * queue health, and worker utilization.
 */
struct EnhancedSnapshot {
    // =========================================================================
    // Basic Counters (from ThreadPoolMetrics)
    // =========================================================================

    /**
     * @brief Total tasks submitted to the pool.
     */
    std::uint64_t tasks_submitted{0};

    /**
     * @brief Total tasks successfully executed.
     */
    std::uint64_t tasks_executed{0};

    /**
     * @brief Total tasks that failed during execution.
     */
    std::uint64_t tasks_failed{0};

    // =========================================================================
    // Latency Percentiles (microseconds for readability)
    // =========================================================================

    /**
     * @brief Median (P50) enqueue latency in microseconds.
     */
    double enqueue_latency_p50_us{0.0};

    /**
     * @brief 90th percentile enqueue latency in microseconds.
     */
    double enqueue_latency_p90_us{0.0};

    /**
     * @brief 99th percentile enqueue latency in microseconds.
     */
    double enqueue_latency_p99_us{0.0};

    /**
     * @brief Median execution latency in microseconds.
     */
    double execution_latency_p50_us{0.0};

    /**
     * @brief 90th percentile execution latency in microseconds.
     */
    double execution_latency_p90_us{0.0};

    /**
     * @brief 99th percentile execution latency in microseconds.
     */
    double execution_latency_p99_us{0.0};

    /**
     * @brief Median wait time (queue time) in microseconds.
     */
    double wait_time_p50_us{0.0};

    /**
     * @brief 90th percentile wait time in microseconds.
     */
    double wait_time_p90_us{0.0};

    /**
     * @brief 99th percentile wait time in microseconds.
     */
    double wait_time_p99_us{0.0};

    // =========================================================================
    // Throughput
    // =========================================================================

    /**
     * @brief Tasks completed per second (1-second window).
     */
    double throughput_1s{0.0};

    /**
     * @brief Tasks completed per second (1-minute window average).
     */
    double throughput_1m{0.0};

    // =========================================================================
    // Queue Health
    // =========================================================================

    /**
     * @brief Current queue depth.
     */
    std::size_t current_queue_depth{0};

    /**
     * @brief Peak queue depth since last reset.
     */
    std::size_t peak_queue_depth{0};

    /**
     * @brief Average queue depth over the sampling period.
     */
    double avg_queue_depth{0.0};

    // =========================================================================
    // Worker Utilization
    // =========================================================================

    /**
     * @brief Overall worker utilization (0.0 - 1.0).
     *
     * Calculated as: total_busy_time / (total_busy_time + total_idle_time)
     */
    double worker_utilization{0.0};

    /**
     * @brief Per-worker utilization (0.0 - 1.0 each).
     */
    std::vector<double> per_worker_utilization;

    /**
     * @brief Number of active workers.
     */
    std::size_t active_workers{0};

    // =========================================================================
    // Timing Information
    // =========================================================================

    /**
     * @brief Total busy time across all workers in nanoseconds.
     */
    std::uint64_t total_busy_time_ns{0};

    /**
     * @brief Total idle time across all workers in nanoseconds.
     */
    std::uint64_t total_idle_time_ns{0};

    /**
     * @brief Timestamp when this snapshot was taken.
     */
    std::chrono::steady_clock::time_point snapshot_time;
};

/**
 * @brief Per-worker metrics for detailed analysis.
 */
struct WorkerMetrics {
    /**
     * @brief Worker identifier.
     */
    std::size_t worker_id{0};

    /**
     * @brief Total tasks executed by this worker.
     */
    std::uint64_t tasks_executed{0};

    /**
     * @brief Total busy time in nanoseconds.
     */
    std::uint64_t busy_time_ns{0};

    /**
     * @brief Total idle time in nanoseconds.
     */
    std::uint64_t idle_time_ns{0};

    /**
     * @brief Current state (true = busy, false = idle).
     */
    bool is_busy{false};
};

/**
 * @brief Enhanced thread pool metrics with histograms and percentiles.
 *
 * @ingroup metrics
 *
 * This class provides production-grade observability for thread pools,
 * including:
 * - Latency histograms for enqueue, execution, and wait times
 * - Percentile calculations (P50, P90, P99)
 * - Sliding window throughput tracking
 * - Per-worker utilization metrics
 * - Queue depth monitoring
 *
 * ### Performance Characteristics
 * - record_* overhead: < 100ns
 * - snapshot() latency: < 10μs
 * - Memory per histogram: < 1KB
 * - Memory per counter: < 4KB (for 60s window)
 *
 * ### Thread Safety
 * All methods are thread-safe. Recording methods use lock-free atomics.
 * Snapshot generation acquires a brief mutex for consistency.
 *
 * ### Usage Example
 * @code
 * auto metrics = std::make_shared<EnhancedThreadPoolMetrics>(8);
 *
 * // Record metrics (called by thread_pool internally)
 * metrics->record_enqueue(std::chrono::nanoseconds{1000});
 * metrics->record_execution(std::chrono::nanoseconds{50000}, true);
 * metrics->record_wait_time(std::chrono::nanoseconds{5000});
 *
 * // Get snapshot
 * auto snap = metrics->snapshot();
 * LOG_INFO("P99 execution: {:.2f}μs", snap.execution_latency_p99_us);
 * LOG_INFO("Throughput: {:.1f} ops/sec", snap.throughput_1s);
 * @endcode
 *
 * @see LatencyHistogram
 * @see SlidingWindowCounter
 * @see ThreadPoolMetrics
 */
class EnhancedThreadPoolMetrics {
public:
    /**
     * @brief Constructs enhanced metrics with the specified worker count.
     * @param worker_count Number of workers to track individually.
     */
    explicit EnhancedThreadPoolMetrics(std::size_t worker_count = 0);

    /**
     * @brief Destructor.
     */
    ~EnhancedThreadPoolMetrics() = default;

    // Non-copyable, non-movable for thread safety
    EnhancedThreadPoolMetrics(const EnhancedThreadPoolMetrics&) = delete;
    EnhancedThreadPoolMetrics& operator=(const EnhancedThreadPoolMetrics&) = delete;
    EnhancedThreadPoolMetrics(EnhancedThreadPoolMetrics&&) = delete;
    EnhancedThreadPoolMetrics& operator=(EnhancedThreadPoolMetrics&&) = delete;

    // =========================================================================
    // Recording Methods (called by thread_pool)
    // =========================================================================

    /**
     * @brief Record a task submission.
     */
    void record_submission();

    /**
     * @brief Record enqueue operation latency.
     * @param latency The time taken to enqueue a task.
     */
    void record_enqueue(std::chrono::nanoseconds latency);

    /**
     * @brief Record task execution completion.
     * @param latency The execution duration.
     * @param success Whether the task completed successfully.
     */
    void record_execution(std::chrono::nanoseconds latency, bool success);

    /**
     * @brief Record wait time (time spent in queue).
     * @param wait The duration the task waited in the queue.
     */
    void record_wait_time(std::chrono::nanoseconds wait);

    /**
     * @brief Record current queue depth.
     * @param depth The current number of tasks in the queue.
     */
    void record_queue_depth(std::size_t depth);

    /**
     * @brief Update worker state.
     * @param worker_id The worker's identifier.
     * @param busy Whether the worker is currently busy.
     * @param duration_ns Duration in the previous state (for utilization calc).
     */
    void record_worker_state(
        std::size_t worker_id,
        bool busy,
        std::uint64_t duration_ns = 0);

    /**
     * @brief Set the number of active workers.
     * @param count Number of currently active workers.
     */
    void set_active_workers(std::size_t count);

    // =========================================================================
    // Query Methods
    // =========================================================================

    /**
     * @brief Get a comprehensive snapshot of all metrics.
     * @return EnhancedSnapshot with current metric values.
     *
     * This method is thread-safe and provides a consistent view of all metrics.
     */
    [[nodiscard]] EnhancedSnapshot snapshot() const;

    /**
     * @brief Get the enqueue latency histogram (read-only).
     * @return Reference to the enqueue latency histogram.
     */
    [[nodiscard]] const LatencyHistogram& enqueue_latency() const;

    /**
     * @brief Get the execution latency histogram (read-only).
     * @return Reference to the execution latency histogram.
     */
    [[nodiscard]] const LatencyHistogram& execution_latency() const;

    /**
     * @brief Get the wait time histogram (read-only).
     * @return Reference to the wait time histogram.
     */
    [[nodiscard]] const LatencyHistogram& wait_time() const;

    /**
     * @brief Get per-worker metrics.
     * @return Vector of WorkerMetrics for each tracked worker.
     */
    [[nodiscard]] std::vector<WorkerMetrics> worker_metrics() const;

    /**
     * @brief Get the 1-second throughput counter (read-only).
     * @return Reference to the 1-second throughput counter.
     */
    [[nodiscard]] const SlidingWindowCounter& throughput_1s() const;

    /**
     * @brief Get the 1-minute throughput counter (read-only).
     * @return Reference to the 1-minute throughput counter.
     */
    [[nodiscard]] const SlidingWindowCounter& throughput_1m() const;

    // =========================================================================
    // Management Methods
    // =========================================================================

    /**
     * @brief Reset all metrics to initial state.
     *
     * Clears all histograms, counters, and per-worker stats.
     */
    void reset();

    /**
     * @brief Update worker count.
     * @param count New number of workers to track.
     *
     * Call this when the thread pool scales up or down.
     */
    void update_worker_count(std::size_t count);

    // =========================================================================
    // Export Methods
    // =========================================================================

    /**
     * @brief Export metrics as JSON string.
     * @return JSON representation of the current metrics.
     */
    [[nodiscard]] std::string to_json() const;

    /**
     * @brief Export metrics in Prometheus/OpenMetrics format.
     * @param prefix Metric name prefix (e.g., "thread_pool").
     * @return Prometheus-compatible metrics string.
     */
    [[nodiscard]] std::string to_prometheus(
        const std::string& prefix = "thread_pool") const;

private:
    // Latency histograms
    LatencyHistogram enqueue_latency_;
    LatencyHistogram execution_latency_;
    LatencyHistogram wait_time_;

    // Throughput counters
    SlidingWindowCounter throughput_1s_;
    SlidingWindowCounter throughput_1m_;

    // Basic counters
    std::atomic<std::uint64_t> tasks_submitted_{0};
    std::atomic<std::uint64_t> tasks_executed_{0};
    std::atomic<std::uint64_t> tasks_failed_{0};

    // Queue depth tracking
    std::atomic<std::size_t> current_queue_depth_{0};
    std::atomic<std::size_t> peak_queue_depth_{0};
    std::atomic<std::uint64_t> queue_depth_sum_{0};
    std::atomic<std::uint64_t> queue_depth_samples_{0};

    // Worker tracking
    std::atomic<std::size_t> active_workers_{0};
    std::atomic<std::uint64_t> total_busy_time_ns_{0};
    std::atomic<std::uint64_t> total_idle_time_ns_{0};

    // Per-worker metrics
    mutable std::mutex workers_mutex_;
    std::vector<WorkerMetrics> per_worker_metrics_;

    // Helper to convert nanoseconds to microseconds
    [[nodiscard]] static double ns_to_us(double ns) { return ns / 1000.0; }
};

} // namespace kcenon::thread::metrics
