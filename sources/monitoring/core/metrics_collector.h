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

#include "monitoring_types.h"
#include "../storage/ring_buffer.h"
#include "../../thread_base/synchronization/error_handling.h"

#include <memory>
#include <thread>
#include <atomic>
#include <chrono>

namespace monitoring_module {

    /**
     * @struct collection_statistics
     * @brief Statistics about the metrics collection process.
     * 
     * @ingroup monitoring
     * 
     * Tracks the performance and health of the metrics collector itself,
     * including collection frequency, errors, and buffer overflows.
     * Useful for monitoring the monitoring system.
     * 
     * ### Thread Safety
     * All members are atomic, allowing concurrent access without locks.
     */
    struct collection_statistics {
        std::atomic<std::uint64_t> total_collections{0};  ///< Total collection cycles completed
        std::atomic<std::uint64_t> collection_errors{0};   ///< Number of collection errors
        std::atomic<std::uint64_t> buffer_overflows{0};    ///< Times buffer was full on push
        std::atomic<std::uint64_t> collection_time_ns{0};  ///< Total time spent collecting (nanoseconds)

        collection_statistics() = default;

        collection_statistics(const collection_statistics& other)
            : total_collections(other.total_collections.load())
            , collection_errors(other.collection_errors.load())
            , buffer_overflows(other.buffer_overflows.load())
            , collection_time_ns(other.collection_time_ns.load()) {}

        collection_statistics& operator=(const collection_statistics& other) {
            if (this != &other) {
                total_collections.store(other.total_collections.load());
                collection_errors.store(other.collection_errors.load());
                buffer_overflows.store(other.buffer_overflows.load());
                collection_time_ns.store(other.collection_time_ns.load());
            }
            return *this;
        }
    };

    /**
     * @class metrics_collector
     * @brief Central metrics collection and aggregation service.
     * 
     * @ingroup monitoring
     * 
     * The metrics_collector class provides a comprehensive monitoring solution
     * for thread pools and system resources. It runs a dedicated collection
     * thread that periodically gathers metrics from registered sources and
     * stores them in a ring buffer for historical analysis.
     * 
     * ### Key Features
     * - Periodic metrics collection at configurable intervals
     * - Thread-safe registration of metric sources
     * - Efficient ring buffer storage for historical data
     * - Low-overhead design suitable for production use
     * - Support for system, thread pool, and worker metrics
     * 
     * ### Design Principles
     * - **Non-intrusive**: Minimal impact on monitored components
     * - **Asynchronous**: Collection runs in a separate thread
     * - **Configurable**: Adjust collection frequency and retention
     * - **Extensible**: Easy to add new metric types
     * 
     * ### Thread Safety
     * All public methods are thread-safe and can be called from any thread.
     * The collector manages its own synchronization internally.
     * 
     * ### Example Usage
     * @code
     * // Create and configure collector
     * monitoring_config config;
     * config.collection_interval = std::chrono::milliseconds(100);
     * config.buffer_size = 1000;
     * 
     * auto collector = std::make_unique<metrics_collector>(config);
     * 
     * // Register metric sources
     * collector->register_thread_pool_metrics(pool_metrics);
     * 
     * // Start collection
     * if (auto result = collector->start(); !result) {
     *     // Handle error
     * }
     * 
     * // Query metrics
     * auto snapshot = collector->get_current_snapshot();
     * @endcode
     * 
     * @see monitoring_config For configuration options
     * @see metrics_snapshot For the data structure returned by queries
     */
    class metrics_collector {
    public:
        /**
         * @brief Constructs a metrics collector with specified configuration.
         * @param config Monitoring configuration parameters
         */
        explicit metrics_collector(monitoring_config config = {});
        
        /**
         * @brief Destructor ensures collection thread is properly stopped.
         */
        ~metrics_collector();

        /**
         * @brief Starts the metrics collection thread.
         * @return result_void containing error on failure, or success value
         * 
         * Begins periodic collection of metrics according to the configured
         * interval. The collector must be started before metrics can be gathered.
         * Calling start() on an already running collector returns an error.
         */
        auto start() -> thread_module::result_void;
        
        /**
         * @brief Stops the metrics collection thread.
         * 
         * Signals the collection thread to stop and waits for it to exit.
         * Safe to call even if the collector is not running.
         */
        auto stop() -> void;

        /**
         * @brief Registers a system metrics source.
         * @param metrics Shared pointer to system metrics structure
         * 
         * The collector will periodically read from this metrics structure.
         * Only one system metrics source can be registered at a time.
         */
        auto register_system_metrics(std::shared_ptr<system_metrics> metrics) -> void;
        
        /**
         * @brief Registers a thread pool metrics source.
         * @param metrics Shared pointer to thread pool metrics structure
         * 
         * The collector will periodically read from this metrics structure.
         * Only one thread pool metrics source can be registered at a time.
         */
        auto register_thread_pool_metrics(std::shared_ptr<thread_pool_metrics> metrics) -> void;
        
        /**
         * @brief Registers a worker thread metrics source.
         * @param metrics Shared pointer to worker metrics structure
         * 
         * The collector will periodically read from this metrics structure.
         * Only one worker metrics source can be registered at a time.
         */
        auto register_worker_metrics(std::shared_ptr<worker_metrics> metrics) -> void;

        /**
         * @brief Retrieves the most recent metrics snapshot.
         * @return Current metrics snapshot
         * 
         * Returns the latest collected metrics. If no metrics have been
         * collected yet, returns a snapshot with default values.
         */
        auto get_current_snapshot() const -> metrics_snapshot;
        
        /**
         * @brief Retrieves multiple recent metrics snapshots.
         * @param count Number of snapshots to retrieve
         * @return Vector of metrics snapshots, newest first
         * 
         * Returns up to count recent snapshots. If fewer snapshots are
         * available, returns all available snapshots.
         */
        auto get_recent_snapshots(std::size_t count) const -> std::vector<metrics_snapshot>;

        /**
         * @brief Retrieves statistics about the collection process.
         * @return Collection statistics structure
         * 
         * Provides insights into collector performance and health,
         * including error counts and timing information.
         */
        auto get_collection_stats() const -> collection_statistics;

        /**
         * @brief Updates the monitoring configuration.
         * @param config New configuration parameters
         * 
         * Changes take effect on the next collection cycle. Note that
         * changing buffer_size will clear existing historical data.
         */
        auto update_config(const monitoring_config& config) -> void;

        /**
         * @brief Checks if the collector is currently running.
         * @return true if collection thread is active
         */
        auto is_running() const -> bool { return running_.load(std::memory_order_acquire); }

    private:
        /**
         * @brief Main collection loop executed by the collection thread.
         * 
         * Runs continuously while the collector is active, gathering metrics
         * at the configured interval and storing them in the ring buffer.
         */
        auto collection_loop() -> void;
        
        /**
         * @brief Performs a single metrics collection cycle.
         * 
         * Gathers metrics from all registered sources and stores
         * a snapshot in the ring buffer.
         */
        auto collect_metrics() -> void;
        
        /**
         * @brief Collects system-level metrics.
         */
        auto collect_system_metrics() -> void;
        
        /**
         * @brief Collects thread pool metrics.
         */
        auto collect_thread_pool_metrics() -> void;
        
        /**
         * @brief Collects worker thread metrics.
         */
        auto collect_worker_metrics() -> void;

        monitoring_config config_;                          ///< Current configuration
        std::atomic<bool> running_{false};                  ///< Collection thread status
        std::atomic<bool> stop_requested_{false};           ///< Stop request flag

        /// Ring buffer for storing historical snapshots
        std::unique_ptr<thread_safe_ring_buffer<metrics_snapshot>> snapshot_buffer_;
        
        // Registered metrics sources
        std::shared_ptr<system_metrics> system_metrics_;         ///< System metrics source
        std::shared_ptr<thread_pool_metrics> thread_pool_metrics_; ///< Thread pool metrics source
        std::shared_ptr<worker_metrics> worker_metrics_;         ///< Worker metrics source

        std::unique_ptr<std::thread> collection_thread_;         ///< Dedicated collection thread
        mutable collection_statistics stats_;                    ///< Collection statistics
        mutable std::mutex metrics_mutex_;                       ///< Protects metrics access
    };

    /**
     * @class global_metrics_collector
     * @brief Singleton instance for application-wide metrics collection.
     * 
     * @ingroup monitoring
     * 
     * Provides a convenient global access point for metrics collection
     * functionality. Useful for applications that need a single, shared
     * metrics collector accessible from multiple components.
     * 
     * ### Thread Safety
     * All methods are thread-safe. The singleton instance is lazily
     * initialized with thread-safe guarantees.
     * 
     * ### Example
     * @code
     * // Initialize global collector
     * monitoring_config config;
     * config.collection_interval = std::chrono::milliseconds(50);
     * 
     * auto& global = global_metrics_collector::instance();
     * if (auto result = global.initialize(config); !result) {
     *     // Handle error
     * }
     * 
     * // Use from anywhere in the application
     * auto collector = global.get_collector();
     * auto metrics = collector->get_current_snapshot();
     * @endcode
     */
    class global_metrics_collector {
    public:
        /**
         * @brief Returns the singleton instance.
         * @return Reference to the global metrics collector
         */
        static auto instance() -> global_metrics_collector&;
        
        /**
         * @brief Initializes the global collector with configuration.
         * @param config Monitoring configuration
         * @return result_void containing error on failure
         * 
         * Must be called before using the global collector. Subsequent
         * calls will return an error unless shutdown() is called first.
         */
        auto initialize(monitoring_config config = {}) -> thread_module::result_void;
        
        /**
         * @brief Shuts down the global collector.
         * 
         * Stops collection and releases resources. After shutdown,
         * initialize() can be called again with new configuration.
         */
        auto shutdown() -> void;
        
        /**
         * @brief Retrieves the underlying metrics collector.
         * @return Shared pointer to the metrics collector, or nullptr if not initialized
         */
        auto get_collector() -> std::shared_ptr<metrics_collector>;
        
        /**
         * @brief Checks if the global collector is initialized.
         * @return true if initialized and ready for use
         */
        auto is_initialized() const -> bool { return collector_ != nullptr; }

    private:
        /**
         * @brief Private constructor for singleton pattern.
         */
        global_metrics_collector() = default;
        
        /**
         * @brief Private destructor.
         */
        ~global_metrics_collector() = default;

        std::shared_ptr<metrics_collector> collector_; ///< The actual collector instance
        std::mutex init_mutex_;                        ///< Protects initialization
    };

    /**
     * @namespace metrics
     * @brief Convenience functions for global metrics operations.
     * 
     * @ingroup monitoring
     * 
     * Provides simplified access to common monitoring operations using
     * the global metrics collector. These functions handle initialization
     * and access to the singleton instance internally.
     */
    namespace metrics {
        /**
         * @brief Starts global monitoring with specified configuration.
         * @param config Monitoring configuration
         * @return result_void containing error on failure
         */
        auto start_global_monitoring(monitoring_config config = {}) -> thread_module::result_void;
        
        /**
         * @brief Stops global monitoring.
         */
        auto stop_global_monitoring() -> void;
        
        /**
         * @brief Retrieves current metrics from the global collector.
         * @return Current metrics snapshot
         * @note Returns default snapshot if monitoring is not active
         */
        auto get_current_metrics() -> metrics_snapshot;
        
        /**
         * @brief Retrieves recent metrics from the global collector.
         * @param count Number of snapshots to retrieve
         * @return Vector of recent metrics snapshots
         * @note Returns empty vector if monitoring is not active
         */
        auto get_recent_metrics(std::size_t count) -> std::vector<metrics_snapshot>;
        
        /**
         * @brief Checks if global monitoring is active.
         * @return true if monitoring is running
         */
        auto is_monitoring_active() -> bool;
    }

} // namespace monitoring_module