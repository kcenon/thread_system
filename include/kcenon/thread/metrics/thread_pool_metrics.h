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

#include <kcenon/thread/metrics/metrics_base.h>

namespace kcenon::thread::metrics {

/**
 * @brief Lightweight metrics container shared between thread_pool and workers.
 *
 * @ingroup metrics
 *
 * This class extends MetricsBase with an additional enqueue counter,
 * providing minimal overhead for basic metrics tracking.
 *
 * ### Performance Characteristics
 * - record_* overhead: < 50ns
 * - Memory footprint: 48 bytes (6 atomic counters)
 * - No histograms or percentiles (use EnhancedThreadPoolMetrics for those)
 *
 * ### Thread Safety
 * All methods are thread-safe using lock-free atomic operations.
 *
 * ### Usage Example
 * @code
 * auto metrics = std::make_shared<ThreadPoolMetrics>();
 *
 * // Record metrics
 * metrics->record_submission();
 * metrics->record_enqueue();
 * metrics->record_execution(50000, true);  // 50μs, success
 *
 * // Get snapshot
 * auto snap = metrics->snapshot();
 * LOG_INFO("Executed: {}", snap.tasks_executed);
 * @endcode
 *
 * @see MetricsBase
 * @see EnhancedThreadPoolMetrics
 */
class ThreadPoolMetrics : public MetricsBase {
public:
    /**
     * @brief Snapshot structure containing all metric values.
     *
     * Extends base metrics with the enqueue counter.
     */
    struct Snapshot {
        std::uint64_t tasks_submitted;
        std::uint64_t tasks_enqueued;
        std::uint64_t tasks_executed;
        std::uint64_t tasks_failed;
        std::uint64_t total_busy_time_ns;
        std::uint64_t total_idle_time_ns;
    };

    /**
     * @brief Default constructor.
     */
    ThreadPoolMetrics() = default;

    /**
     * @brief Virtual destructor.
     */
    ~ThreadPoolMetrics() override = default;

    /**
     * @brief Record enqueue operation(s).
     * @param count Number of tasks enqueued (default: 1).
     */
    void record_enqueue(std::size_t count = 1) {
        tasks_enqueued_.fetch_add(count, std::memory_order_relaxed);
    }

    /**
     * @brief Get the total number of tasks enqueued.
     * @return Number of enqueued tasks.
     */
    [[nodiscard]] std::uint64_t tasks_enqueued() const {
        return tasks_enqueued_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Reset all metrics to their initial state.
     */
    void reset() override {
        MetricsBase::reset();
        tasks_enqueued_.store(0, std::memory_order_relaxed);
    }

    /**
     * @brief Get a snapshot of all metrics.
     * @return Snapshot with current metric values.
     */
    [[nodiscard]] Snapshot snapshot() const {
        return Snapshot{
            tasks_submitted(),
            tasks_enqueued_.load(std::memory_order_relaxed),
            tasks_executed(),
            tasks_failed(),
            total_busy_time_ns(),
            total_idle_time_ns(),
        };
    }

private:
    /**
     * @brief Counter for enqueued tasks.
     *
     * This counter tracks tasks that were successfully added to the queue,
     * which may differ from submitted tasks if some are rejected.
     */
    std::atomic<std::uint64_t> tasks_enqueued_{0};
};

} // namespace kcenon::thread::metrics
