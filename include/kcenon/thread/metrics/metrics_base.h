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

#include <atomic>
#include <cstdint>

namespace kcenon::thread::metrics {

/**
 * @brief Base snapshot structure containing common metric values.
 *
 * This structure provides a point-in-time view of the core metrics
 * that are shared across all metrics implementations.
 */
struct BaseSnapshot {
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

    /**
     * @brief Total busy time across all workers in nanoseconds.
     */
    std::uint64_t total_busy_time_ns{0};

    /**
     * @brief Total idle time across all workers in nanoseconds.
     */
    std::uint64_t total_idle_time_ns{0};
};

/**
 * @brief Abstract base class for thread pool metrics.
 *
 * @ingroup metrics
 *
 * This class provides the common atomic counters and recording methods
 * shared between ThreadPoolMetrics and EnhancedThreadPoolMetrics,
 * eliminating code duplication while maintaining a consistent interface.
 *
 * ### Thread Safety
 * All methods are thread-safe using lock-free atomic operations with
 * relaxed memory ordering for optimal performance.
 *
 * ### Performance Characteristics
 * - record_* overhead: < 50ns (single atomic fetch_add)
 * - Memory footprint: 40 bytes (5 atomic counters)
 *
 * ### Usage
 * This class is not intended to be used directly. Use ThreadPoolMetrics
 * for lightweight metrics or EnhancedThreadPoolMetrics for full-featured
 * production observability.
 *
 * @see ThreadPoolMetrics
 * @see EnhancedThreadPoolMetrics
 */
class MetricsBase {
public:
    /**
     * @brief Virtual destructor for proper cleanup in derived classes.
     */
    virtual ~MetricsBase() = default;

    // =========================================================================
    // Recording Methods
    // =========================================================================

    /**
     * @brief Record task submission(s).
     * @param count Number of tasks submitted (default: 1).
     */
    void record_submission(std::size_t count = 1) {
        tasks_submitted_.fetch_add(count, std::memory_order_relaxed);
    }

    /**
     * @brief Record task execution completion.
     * @param duration_ns Execution duration in nanoseconds.
     * @param success Whether the task completed successfully.
     */
    void record_execution(std::uint64_t duration_ns, bool success) {
        if (success) {
            tasks_executed_.fetch_add(1, std::memory_order_relaxed);
        } else {
            tasks_failed_.fetch_add(1, std::memory_order_relaxed);
        }
        total_busy_time_ns_.fetch_add(duration_ns, std::memory_order_relaxed);
    }

    /**
     * @brief Record idle time.
     * @param duration_ns Idle duration in nanoseconds.
     */
    void record_idle_time(std::uint64_t duration_ns) {
        total_idle_time_ns_.fetch_add(duration_ns, std::memory_order_relaxed);
    }

    /**
     * @brief Reset all metrics to their initial state.
     *
     * Derived classes should override this method to reset their
     * additional metrics, but must call the base implementation.
     */
    virtual void reset() {
        tasks_submitted_.store(0, std::memory_order_relaxed);
        tasks_executed_.store(0, std::memory_order_relaxed);
        tasks_failed_.store(0, std::memory_order_relaxed);
        total_busy_time_ns_.store(0, std::memory_order_relaxed);
        total_idle_time_ns_.store(0, std::memory_order_relaxed);
    }

    // =========================================================================
    // Query Methods
    // =========================================================================

    /**
     * @brief Get the total number of tasks submitted.
     * @return Number of submitted tasks.
     */
    [[nodiscard]] std::uint64_t tasks_submitted() const {
        return tasks_submitted_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Get the total number of tasks successfully executed.
     * @return Number of executed tasks.
     */
    [[nodiscard]] std::uint64_t tasks_executed() const {
        return tasks_executed_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Get the total number of failed tasks.
     * @return Number of failed tasks.
     */
    [[nodiscard]] std::uint64_t tasks_failed() const {
        return tasks_failed_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Get the total busy time in nanoseconds.
     * @return Total busy time across all workers.
     */
    [[nodiscard]] std::uint64_t total_busy_time_ns() const {
        return total_busy_time_ns_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Get the total idle time in nanoseconds.
     * @return Total idle time across all workers.
     */
    [[nodiscard]] std::uint64_t total_idle_time_ns() const {
        return total_idle_time_ns_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Get a base snapshot of common metrics.
     * @return BaseSnapshot with current metric values.
     */
    [[nodiscard]] BaseSnapshot base_snapshot() const {
        return BaseSnapshot{
            tasks_submitted_.load(std::memory_order_relaxed),
            tasks_executed_.load(std::memory_order_relaxed),
            tasks_failed_.load(std::memory_order_relaxed),
            total_busy_time_ns_.load(std::memory_order_relaxed),
            total_idle_time_ns_.load(std::memory_order_relaxed),
        };
    }

    /**
     * @brief Calculate worker utilization ratio.
     * @return Utilization ratio (0.0 - 1.0), or 0.0 if no time recorded.
     *
     * Calculated as: busy_time / (busy_time + idle_time)
     */
    [[nodiscard]] double utilization() const {
        const auto busy = total_busy_time_ns_.load(std::memory_order_relaxed);
        const auto idle = total_idle_time_ns_.load(std::memory_order_relaxed);
        const auto total = busy + idle;
        return (total > 0) ? static_cast<double>(busy) / static_cast<double>(total) : 0.0;
    }

    /**
     * @brief Calculate task success rate.
     * @return Success rate (0.0 - 1.0), or 1.0 if no tasks executed.
     *
     * Calculated as: executed / (executed + failed)
     */
    [[nodiscard]] double success_rate() const {
        const auto executed = tasks_executed_.load(std::memory_order_relaxed);
        const auto failed = tasks_failed_.load(std::memory_order_relaxed);
        const auto total = executed + failed;
        return (total > 0) ? static_cast<double>(executed) / static_cast<double>(total) : 1.0;
    }

protected:
    /**
     * @brief Default constructor.
     */
    MetricsBase() = default;

    /**
     * @brief Copy constructor (deleted for thread safety).
     */
    MetricsBase(const MetricsBase&) = delete;

    /**
     * @brief Copy assignment operator (deleted for thread safety).
     */
    MetricsBase& operator=(const MetricsBase&) = delete;

    /**
     * @brief Move constructor (deleted for thread safety).
     */
    MetricsBase(MetricsBase&&) = delete;

    /**
     * @brief Move assignment operator (deleted for thread safety).
     */
    MetricsBase& operator=(MetricsBase&&) = delete;

    // =========================================================================
    // Protected Members (accessible to derived classes)
    // =========================================================================

    /**
     * @brief Counter for submitted tasks.
     */
    std::atomic<std::uint64_t> tasks_submitted_{0};

    /**
     * @brief Counter for successfully executed tasks.
     */
    std::atomic<std::uint64_t> tasks_executed_{0};

    /**
     * @brief Counter for failed tasks.
     */
    std::atomic<std::uint64_t> tasks_failed_{0};

    /**
     * @brief Accumulated busy time in nanoseconds.
     */
    std::atomic<std::uint64_t> total_busy_time_ns_{0};

    /**
     * @brief Accumulated idle time in nanoseconds.
     */
    std::atomic<std::uint64_t> total_idle_time_ns_{0};
};

} // namespace kcenon::thread::metrics
