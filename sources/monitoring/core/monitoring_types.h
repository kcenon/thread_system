#pragma once

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

#include <chrono>
#include <atomic>
#include <string>
#include <memory>

/**
 * @namespace monitoring_module
 * @brief Performance monitoring and metrics collection for the thread system.
 *
 * The monitoring_module namespace provides comprehensive monitoring capabilities
 * for thread pools, worker threads, and system resources. It enables real-time
 * performance tracking and metrics collection with minimal overhead.
 *
 * Key components include:
 * - Metric types and structures for various performance indicators
 * - Efficient ring buffer storage for historical data
 * - Thread-safe metrics collection and aggregation
 * - Configurable monitoring intervals and retention policies
 *
 * The monitoring system is designed to:
 * - Track thread pool performance (job throughput, latency, queue depth)
 * - Monitor individual worker thread statistics
 * - Collect system-level metrics (CPU usage, memory consumption)
 * - Provide historical data for trend analysis
 * - Minimize performance impact on the monitored system
 *
 * @see metrics_collector The main collector class for gathering metrics
 * @see ring_buffer Efficient storage for time-series metric data
 */
namespace monitoring_module {

    /**
     * @brief Time point type for metric timestamps.
     * 
     * Uses steady_clock to ensure monotonic time progression,
     * which is important for accurate duration measurements.
     */
    using time_point = std::chrono::steady_clock::time_point;
    
    /**
     * @brief Duration type for time measurements.
     * 
     * Uses nanoseconds for high-precision timing of operations.
     */
    using duration = std::chrono::nanoseconds;

    /**
     * @enum metric_type
     * @brief Categorizes different types of performance metrics.
     * 
     * @ingroup monitoring
     * 
     * Each metric type represents a different measurement pattern:
     * - Counters accumulate values over time
     * - Gauges represent instantaneous values
     * - Histograms track value distributions
     * - Timers measure operation durations
     */
    enum class metric_type {
        counter,      ///< Cumulative counter (e.g., number of completed tasks)
        gauge,        ///< Current value (e.g., queue length, CPU usage)
        histogram,    ///< Distribution data (e.g., latency percentiles)
        timer         ///< Time measurement for operation duration
    };

    /**
     * @struct system_metrics
     * @brief System-level performance metrics.
     * 
     * @ingroup monitoring
     * 
     * Captures overall system resource usage and thread activity.
     * All members use atomic types to ensure thread-safe access
     * without explicit locking.
     * 
     * ### Thread Safety
     * All member variables are atomic, allowing concurrent reads
     * and updates from multiple threads without data races.
     */
    struct system_metrics {
        std::atomic<std::uint64_t> cpu_usage_percent{0};    ///< CPU usage percentage (0-100)
        std::atomic<std::uint64_t> memory_usage_bytes{0};   ///< Memory usage in bytes
        std::atomic<std::uint64_t> active_threads{0};       ///< Number of active threads
        std::atomic<std::uint64_t> total_allocations{0};    ///< Total memory allocations
        time_point timestamp{std::chrono::steady_clock::now()}; ///< Metric collection timestamp

        system_metrics() = default;
        
        system_metrics(const system_metrics& other) 
            : cpu_usage_percent(other.cpu_usage_percent.load())
            , memory_usage_bytes(other.memory_usage_bytes.load())
            , active_threads(other.active_threads.load())
            , total_allocations(other.total_allocations.load())
            , timestamp(other.timestamp) {}

        system_metrics& operator=(const system_metrics& other) {
            if (this != &other) {
                cpu_usage_percent.store(other.cpu_usage_percent.load());
                memory_usage_bytes.store(other.memory_usage_bytes.load());
                active_threads.store(other.active_threads.load());
                total_allocations.store(other.total_allocations.load());
                timestamp = other.timestamp;
            }
            return *this;
        }
    };

    /**
     * @struct thread_pool_metrics
     * @brief Performance metrics for thread pool operations.
     * 
     * @ingroup monitoring
     * 
     * Tracks job processing statistics, queue depth, and worker
     * thread utilization. Provides insights into thread pool
     * efficiency and throughput.
     * 
     * ### Thread Safety
     * All member variables are atomic, ensuring thread-safe
     * concurrent access without explicit synchronization.
     */
    struct thread_pool_metrics {
        std::atomic<std::uint64_t> jobs_completed{0};           ///< Total jobs completed
        std::atomic<std::uint64_t> jobs_pending{0};             ///< Jobs waiting in queue
        std::atomic<std::uint64_t> total_execution_time_ns{0};  ///< Total job execution time (nanoseconds)
        std::atomic<std::uint64_t> average_latency_ns{0};       ///< Average job latency (nanoseconds)
        std::atomic<std::uint64_t> worker_threads{0};           ///< Total worker threads
        std::atomic<std::uint64_t> idle_threads{0};             ///< Currently idle threads
        time_point timestamp{std::chrono::steady_clock::now()}; ///< Metric collection timestamp

        thread_pool_metrics() = default;
        
        thread_pool_metrics(const thread_pool_metrics& other)
            : jobs_completed(other.jobs_completed.load())
            , jobs_pending(other.jobs_pending.load())
            , total_execution_time_ns(other.total_execution_time_ns.load())
            , average_latency_ns(other.average_latency_ns.load())
            , worker_threads(other.worker_threads.load())
            , idle_threads(other.idle_threads.load())
            , timestamp(other.timestamp) {}

        thread_pool_metrics& operator=(const thread_pool_metrics& other) {
            if (this != &other) {
                jobs_completed.store(other.jobs_completed.load());
                jobs_pending.store(other.jobs_pending.load());
                total_execution_time_ns.store(other.total_execution_time_ns.load());
                average_latency_ns.store(other.average_latency_ns.load());
                worker_threads.store(other.worker_threads.load());
                idle_threads.store(other.idle_threads.load());
                timestamp = other.timestamp;
            }
            return *this;
        }
    };

    /**
     * @struct worker_metrics
     * @brief Performance metrics for individual worker threads.
     * 
     * @ingroup monitoring
     * 
     * Tracks per-worker statistics including job processing count,
     * processing time, idle time, and context switches. Useful for
     * identifying worker load imbalances or performance issues.
     * 
     * ### Thread Safety
     * All member variables are atomic for lock-free concurrent access.
     */
    struct worker_metrics {
        std::atomic<std::uint64_t> jobs_processed{0};              ///< Jobs processed by this worker
        std::atomic<std::uint64_t> total_processing_time_ns{0};    ///< Total time spent processing (nanoseconds)
        std::atomic<std::uint64_t> idle_time_ns{0};                ///< Total idle time (nanoseconds)
        std::atomic<std::uint64_t> context_switches{0};            ///< Number of context switches
        time_point timestamp{std::chrono::steady_clock::now()};    ///< Metric collection timestamp

        worker_metrics() = default;
        
        worker_metrics(const worker_metrics& other)
            : jobs_processed(other.jobs_processed.load())
            , total_processing_time_ns(other.total_processing_time_ns.load())
            , idle_time_ns(other.idle_time_ns.load())
            , context_switches(other.context_switches.load())
            , timestamp(other.timestamp) {}

        worker_metrics& operator=(const worker_metrics& other) {
            if (this != &other) {
                jobs_processed.store(other.jobs_processed.load());
                total_processing_time_ns.store(other.total_processing_time_ns.load());
                idle_time_ns.store(other.idle_time_ns.load());
                context_switches.store(other.context_switches.load());
                timestamp = other.timestamp;
            }
            return *this;
        }
    };

    /**
     * @struct metrics_snapshot
     * @brief Read-only snapshot of all metrics at a point in time.
     * 
     * @ingroup monitoring
     * 
     * Provides a consistent view of all metrics captured at the same
     * moment. Used for querying historical data and generating reports.
     * Since this is a snapshot, it doesn't require atomic members.
     */
    struct metrics_snapshot {
        system_metrics system;           ///< System-level metrics
        thread_pool_metrics thread_pool; ///< Thread pool metrics
        worker_metrics worker;           ///< Worker thread metrics
        time_point capture_time{std::chrono::steady_clock::now()}; ///< Snapshot capture time
    };

    /**
     * @struct monitoring_config
     * @brief Configuration parameters for the monitoring system.
     * 
     * @ingroup monitoring
     * 
     * Controls monitoring behavior including collection frequency,
     * data retention, and which metrics to collect. Can be adjusted
     * at runtime to balance monitoring detail against performance impact.
     */
    struct monitoring_config {
        std::chrono::milliseconds collection_interval{100};  ///< Metrics collection interval (default: 100ms)
        std::size_t buffer_size{3600};                       ///< Ring buffer size for historical data
        bool enable_system_metrics{true};                    ///< Enable system-level metrics collection
        bool enable_thread_pool_metrics{true};               ///< Enable thread pool metrics collection
        bool enable_worker_metrics{true};                    ///< Enable per-worker metrics collection
        bool low_overhead_mode{false};                       ///< Minimize monitoring overhead when true
    };

    /**
     * @class scoped_timer
     * @brief RAII timer for automatic duration measurement.
     * 
     * @ingroup monitoring
     * 
     * Measures elapsed time from construction to destruction,
     * automatically adding the duration to a target atomic counter.
     * Useful for measuring function or scope execution time.
     * 
     * ### Example
     * @code
     * void process_job() {
     *     scoped_timer timer(processing_time_counter);
     *     // ... do work ...
     * } // Timer destructor adds elapsed time to counter
     * @endcode
     * 
     * ### Thread Safety
     * Thread-safe when used with atomic target counters.
     */
    class scoped_timer {
    public:
        /**
         * @brief Constructs a timer and starts measurement.
         * @param target Atomic counter to update with elapsed time on destruction
         */
        explicit scoped_timer(std::atomic<std::uint64_t>& target)
            : target_(target), start_time_(std::chrono::steady_clock::now()) {}

        /**
         * @brief Destructor calculates elapsed time and updates target.
         * 
         * Automatically adds the elapsed nanoseconds to the target counter
         * using atomic fetch_add for thread safety.
         */
        ~scoped_timer() {
            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
                end_time - start_time_).count();
            target_.fetch_add(static_cast<std::uint64_t>(duration), std::memory_order_relaxed);
        }

        // Copy and move disabled
        scoped_timer(const scoped_timer&) = delete;
        scoped_timer& operator=(const scoped_timer&) = delete;
        scoped_timer(scoped_timer&&) = delete;
        scoped_timer& operator=(scoped_timer&&) = delete;

    private:
        std::atomic<std::uint64_t>& target_; ///< Target counter to update
        time_point start_time_;              ///< Timer start time
    };

    /**
     * @class metrics_updater
     * @brief Utility class for thread-safe metric updates.
     * 
     * @ingroup monitoring
     * 
     * Provides static helper methods for common metric update operations.
     * All methods use appropriate memory ordering for optimal performance
     * while maintaining thread safety.
     */
    class metrics_updater {
    public:
        /**
         * @brief Increments a counter by one.
         * @param counter The atomic counter to increment
         */
        static auto increment_counter(std::atomic<std::uint64_t>& counter) -> void {
            counter.fetch_add(1, std::memory_order_relaxed);
        }

        /**
         * @brief Adds a value to a counter.
         * @param target The atomic counter to update
         * @param value The value to add
         */
        static auto add_value(std::atomic<std::uint64_t>& target, std::uint64_t value) -> void {
            target.fetch_add(value, std::memory_order_relaxed);
        }

        /**
         * @brief Sets a counter to a specific value.
         * @param target The atomic counter to update
         * @param value The new value
         */
        static auto set_value(std::atomic<std::uint64_t>& target, std::uint64_t value) -> void {
            target.store(value, std::memory_order_relaxed);
        }

        /**
         * @brief Creates a scoped timer for duration measurement.
         * @param target The atomic counter to update with elapsed time
         * @return A scoped_timer instance
         */
        static auto create_timer(std::atomic<std::uint64_t>& target) -> scoped_timer {
            return scoped_timer(target);
        }
    };

} // namespace monitoring_module