/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, 🍀☀🌕🌥 🌊
All rights reserved.
*****************************************************************************/

#pragma once

#include <atomic>
#include <chrono>
#include <optional>
#include <thread>

#include "job.h"
#include "job_queue.h"
#include "error_handling.h"

namespace kcenon::thread {

/**
 * @struct queue_metrics
 * @brief Metrics for monitoring queue health and performance
 */
struct queue_metrics {
    std::atomic<uint64_t> total_enqueued{0};
    std::atomic<uint64_t> total_dequeued{0};
    std::atomic<uint64_t> total_rejected{0};
    std::atomic<uint64_t> total_timeouts{0};
    std::atomic<uint64_t> peak_size{0};

    // Default constructor
    queue_metrics() = default;

    // Copy constructor (atomics can't be copied, must load/store)
    queue_metrics(const queue_metrics& other) {
        total_enqueued.store(other.total_enqueued.load(std::memory_order_relaxed), std::memory_order_relaxed);
        total_dequeued.store(other.total_dequeued.load(std::memory_order_relaxed), std::memory_order_relaxed);
        total_rejected.store(other.total_rejected.load(std::memory_order_relaxed), std::memory_order_relaxed);
        total_timeouts.store(other.total_timeouts.load(std::memory_order_relaxed), std::memory_order_relaxed);
        peak_size.store(other.peak_size.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }

    // Copy assignment operator
    queue_metrics& operator=(const queue_metrics& other) {
        if (this != &other) {
            total_enqueued.store(other.total_enqueued.load(std::memory_order_relaxed), std::memory_order_relaxed);
            total_dequeued.store(other.total_dequeued.load(std::memory_order_relaxed), std::memory_order_relaxed);
            total_rejected.store(other.total_rejected.load(std::memory_order_relaxed), std::memory_order_relaxed);
            total_timeouts.store(other.total_timeouts.load(std::memory_order_relaxed), std::memory_order_relaxed);
            peak_size.store(other.peak_size.load(std::memory_order_relaxed), std::memory_order_relaxed);
        }
        return *this;
    }

    void reset() {
        total_enqueued = 0;
        total_dequeued = 0;
        total_rejected = 0;
        total_timeouts = 0;
        peak_size = 0;
    }

    [[nodiscard]] auto get_rejection_rate() const -> double {
        auto total = total_enqueued.load() + total_rejected.load();
        if (total == 0) return 0.0;
        return static_cast<double>(total_rejected.load()) / total;
    }
};

/**
 * @class bounded_job_queue
 * @brief Thread-safe job queue with size limits and backpressure support
 *
 * This enhanced job queue provides:
 * - Maximum queue size enforcement to prevent memory exhaustion
 * - Backpressure signaling when queue is near capacity
 * - Timeout support for enqueue operations
 * - Detailed metrics for monitoring and diagnostics
 * - Graceful degradation under load
 *
 * ### Thread Safety
 * All public methods are thread-safe and can be called concurrently
 * from multiple threads.
 *
 * ### Backpressure Strategy
 * When queue utilization exceeds backpressure_threshold (default 80%),
 * the queue signals backpressure to allow callers to implement
 * flow control strategies like:
 * - Slowing down job submission
 * - Rejecting low-priority work
 * - Triggering alerts
 *
 * ### Production Usage
 * @code
 * // Create bounded queue with 1000 job limit
 * auto queue = std::make_shared<bounded_job_queue>(1000);
 *
 * // Enable backpressure at 80% capacity
 * queue->set_backpressure_threshold(0.8);
 *
 * // Try to enqueue with timeout
 * auto result = queue->enqueue_with_timeout(
 *     std::move(job),
 *     std::chrono::milliseconds(100)
 * );
 *
 * if (!result) {
 *     if (result.error().code == error_codes::thread_system::queue_full) {
 *         // Implement backpressure strategy
 *         slow_down_production();
 *     }
 * }
 *
 * // Monitor metrics
 * auto metrics = queue->get_metrics();
 * if (metrics.get_rejection_rate() > 0.1) {
 *     log_warning("High rejection rate: {}%", metrics.get_rejection_rate() * 100);
 * }
 * @endcode
 */
class bounded_job_queue : public job_queue {
public:
    /**
     * @brief Constructs a bounded job queue
     * @param max_size Maximum number of jobs in queue (0 = unlimited)
     * @param backpressure_threshold Utilization threshold for backpressure (0.0-1.0)
     */
    explicit bounded_job_queue(
        size_t max_size = 10000,
        double backpressure_threshold = 0.8
    )
        : job_queue()
        , max_size_(max_size)
        , backpressure_threshold_(backpressure_threshold)
    {
    }

    /**
     * @brief Enqueue job with capacity checking
     * @param value Job to enqueue
     * @return Result indicating success or queue_full error
     */
    [[nodiscard]] auto enqueue(std::unique_ptr<job>&& value) -> result_void override {
        if (max_size_ > 0 && size() >= max_size_) {
            metrics_.total_rejected.fetch_add(1, std::memory_order_relaxed);
            return result_void(error(error_code::queue_full, "Job queue is at maximum capacity"));
        }

        auto result = job_queue::enqueue(std::move(value));
        if (!result.has_error()) {
            metrics_.total_enqueued.fetch_add(1, std::memory_order_relaxed);

            // Update peak size with retry limit to prevent infinite loops
            auto current_size = size();
            auto peak = metrics_.peak_size.load(std::memory_order_acquire);

            // Limit retries to prevent infinite loop in case of high contention
            // or spurious failures with compare_exchange_weak
            constexpr size_t max_retries = 10;
            size_t retry_count = 0;

            while (current_size > peak && retry_count < max_retries) {
                // Use compare_exchange_strong to avoid spurious failures
                // Use acq_rel ordering to ensure proper synchronization
                if (metrics_.peak_size.compare_exchange_strong(
                        peak, current_size,
                        std::memory_order_acq_rel,
                        std::memory_order_acquire)) {
                    break;  // Successfully updated peak
                }
                ++retry_count;
                // Peak was updated by another thread, reload and retry if still smaller
            }

            // If we hit max retries, it's not critical - peak tracking is best-effort
            // The metric might be slightly inaccurate but won't cause infinite loops
        }

        return result;
    }

    /**
     * @brief Enqueue job with timeout
     * @param value Job to enqueue
     * @param timeout Maximum time to wait if queue is full
     * @return Result indicating success, timeout, or error
     */
    [[nodiscard]] auto enqueue_with_timeout(
        std::unique_ptr<job>&& value,
        std::chrono::milliseconds timeout
    ) -> result_void {
        auto start = std::chrono::steady_clock::now();

        while (true) {
            // Try to enqueue
            if (max_size_ == 0 || size() < max_size_) {
                return enqueue(std::move(value));
            }

            // Check timeout
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed >= timeout) {
                metrics_.total_timeouts.fetch_add(1, std::memory_order_relaxed);
                return result_void(error(error_code::operation_timeout, "Enqueue operation timed out"));
            }

            // Wait a bit before retry
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    /**
     * @brief Dequeue job and update metrics
     */
    [[nodiscard]] auto dequeue() -> result<std::unique_ptr<job>> override {
        auto result = job_queue::dequeue();
        if (!!result.has_value()) {
            metrics_.total_dequeued.fetch_add(1, std::memory_order_relaxed);
        }
        return result;
    }

    /**
     * @brief Check if backpressure should be applied
     * @return true if queue utilization exceeds backpressure threshold
     */
    [[nodiscard]] auto is_backpressure_active() const -> bool {
        if (max_size_ == 0) return false;

        auto current_size = size();
        auto threshold_size = static_cast<size_t>(max_size_ * backpressure_threshold_);

        return current_size >= threshold_size;
    }

    /**
     * @brief Get current queue utilization
     * @return Utilization as a value between 0.0 (empty) and 1.0 (full)
     */
    [[nodiscard]] auto get_utilization() const -> double {
        if (max_size_ == 0) return 0.0;
        return static_cast<double>(size()) / max_size_;
    }

    /**
     * @brief Set maximum queue size
     * @param max_size New maximum size (0 = unlimited)
     */
    auto set_max_size(size_t max_size) -> void {
        max_size_.store(max_size, std::memory_order_release);
    }

    /**
     * @brief Get maximum queue size
     * @return Current maximum size
     */
    [[nodiscard]] auto get_max_size() const -> size_t {
        return max_size_.load(std::memory_order_acquire);
    }

    /**
     * @brief Set backpressure threshold
     * @param threshold Threshold as percentage (0.0 to 1.0)
     */
    auto set_backpressure_threshold(double threshold) -> void {
        backpressure_threshold_.store(
            std::clamp(threshold, 0.0, 1.0),
            std::memory_order_release
        );
    }

    /**
     * @brief Get current backpressure threshold
     * @return Threshold percentage
     */
    [[nodiscard]] auto get_backpressure_threshold() const -> double {
        return backpressure_threshold_.load(std::memory_order_acquire);
    }

    /**
     * @brief Get queue metrics
     * @return Current metrics snapshot
     */
    [[nodiscard]] auto get_metrics() const -> queue_metrics {
        return metrics_;
    }

    /**
     * @brief Reset all metrics
     */
    auto reset_metrics() -> void {
        metrics_.reset();
    }

    /**
     * @brief Get estimated memory usage
     * @return Approximate memory usage in bytes
     */
    [[nodiscard]] auto get_memory_usage() const -> size_t {
        // Rough estimate: queue size * average job size
        constexpr size_t avg_job_size = 256; // bytes
        return size() * avg_job_size;
    }

private:
    std::atomic<size_t> max_size_;
    std::atomic<double> backpressure_threshold_;
    queue_metrics metrics_;
};

} // namespace kcenon::thread
