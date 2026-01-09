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

#include "thread_info.h"
#include "job_info.h"
#include "bottleneck_report.h"
#include "health_status.h"
#include "execution_event.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace kcenon::thread
{
	// Forward declarations
	class thread_pool;
	class thread_worker;

namespace diagnostics
{
	/**
	 * @struct diagnostics_config
	 * @brief Configuration options for thread pool diagnostics.
	 *
	 * @ingroup diagnostics
	 */
	struct diagnostics_config
	{
		/**
		 * @brief Maximum number of recent jobs to track.
		 */
		std::size_t recent_jobs_capacity{1000};

		/**
		 * @brief Maximum number of events to retain in history.
		 */
		std::size_t event_history_size{1000};

		/**
		 * @brief Enable automatic event tracing.
		 */
		bool enable_tracing{false};

		/**
		 * @brief High watermark threshold for queue saturation (0.0 to 1.0).
		 */
		double queue_saturation_high{0.8};

		/**
		 * @brief Worker utilization threshold for bottleneck detection.
		 */
		double utilization_high_threshold{0.9};

		/**
		 * @brief Wait time threshold (ms) for slow consumer detection.
		 */
		double wait_time_threshold_ms{100.0};

		/**
		 * @brief Configurable thresholds for health status determination.
		 */
		health_thresholds health_thresholds_config{};
	};

	/**
	 * @class thread_pool_diagnostics
	 * @brief Comprehensive diagnostics API for thread pool monitoring.
	 *
	 * @ingroup diagnostics
	 *
	 * Provides thread dump capabilities, job tracing, bottleneck detection,
	 * and health check integration for thread pools.
	 *
	 * ### Design Principles
	 * - **Non-intrusive**: Minimal overhead when not actively used
	 * - **Thread-safe**: All methods can be called from any thread
	 * - **Read-only**: Never modifies thread pool state
	 * - **Snapshot-based**: Returns point-in-time snapshots
	 *
	 * ### Thread Safety
	 * All public methods are thread-safe and can be called concurrently.
	 * Internal state is protected by appropriate synchronization.
	 *
	 * ### Performance Considerations
	 * - Thread dump: O(n) where n is worker count
	 * - Job inspection: O(1) for active jobs, O(n) for history
	 * - Bottleneck detection: O(n) where n is worker count
	 * - Health check: O(n) including all component checks
	 * - Event tracing: < 1μs overhead per event when enabled
	 *
	 * ### Usage Example
	 * @code
	 * auto pool = std::make_shared<thread_pool>("MyPool");
	 * pool->start();
	 *
	 * // Get thread dump
	 * std::cout << pool->diagnostics().format_thread_dump() << std::endl;
	 *
	 * // Check for bottlenecks
	 * auto report = pool->diagnostics().detect_bottlenecks();
	 * if (report.has_bottleneck) {
	 *     LOG_WARN("Bottleneck: {}", report.description);
	 * }
	 *
	 * // Health check for HTTP endpoint
	 * auto health = pool->diagnostics().health_check();
	 * return http_response(health.http_status_code(), health.to_json());
	 * @endcode
	 */
	class thread_pool_diagnostics
	{
	public:
		/**
		 * @brief Constructs diagnostics for a thread pool.
		 * @param pool Reference to the thread pool to diagnose.
		 * @param config Optional configuration for diagnostics.
		 */
		explicit thread_pool_diagnostics(thread_pool& pool,
		                                const diagnostics_config& config = {});

		/**
		 * @brief Destructor.
		 */
		~thread_pool_diagnostics();

		// Non-copyable, non-movable
		thread_pool_diagnostics(const thread_pool_diagnostics&) = delete;
		thread_pool_diagnostics& operator=(const thread_pool_diagnostics&) = delete;
		thread_pool_diagnostics(thread_pool_diagnostics&&) = delete;
		thread_pool_diagnostics& operator=(thread_pool_diagnostics&&) = delete;

		// =========================================================================
		// Thread Dump
		// =========================================================================

		/**
		 * @brief Gets current state of all worker threads.
		 * @return Vector of thread information.
		 *
		 * Thread-safe: Can be called from any thread.
		 */
		[[nodiscard]] auto dump_thread_states() const -> std::vector<thread_info>;

		/**
		 * @brief Gets formatted thread dump (human-readable).
		 * @return Multi-line string with thread dump.
		 *
		 * Output format:
		 * ```
		 * === Thread Pool Dump: MyPool ===
		 * Time: 2025-01-08T10:30:00Z
		 * Workers: 8, Active: 5, Idle: 3
		 *
		 * Worker-0 [tid:12345] ACTIVE (2.5s)
		 *   Current Job: ProcessOrder#1234 (running 150ms)
		 *   Jobs: 1523 completed, 2 failed
		 *   Utilization: 87.3%
		 * ...
		 * ```
		 */
		[[nodiscard]] auto format_thread_dump() const -> std::string;

		// =========================================================================
		// Job Inspection
		// =========================================================================

		/**
		 * @brief Gets currently executing jobs.
		 * @return Vector of active job information.
		 */
		[[nodiscard]] auto get_active_jobs() const -> std::vector<job_info>;

		/**
		 * @brief Gets pending jobs in queue.
		 * @param limit Maximum number to return (0 = all).
		 * @return Vector of pending job information.
		 */
		[[nodiscard]] auto get_pending_jobs(std::size_t limit = 100) const
		    -> std::vector<job_info>;

		/**
		 * @brief Gets recent completed/failed jobs.
		 * @param limit Maximum number to return.
		 * @return Vector of recent job information.
		 */
		[[nodiscard]] auto get_recent_jobs(std::size_t limit = 100) const
		    -> std::vector<job_info>;

		/**
		 * @brief Records a job completion for history tracking.
		 * @param info The job information to record.
		 *
		 * Called internally by the thread pool when jobs complete.
		 */
		void record_job_completion(const job_info& info);

		// =========================================================================
		// Bottleneck Detection
		// =========================================================================

		/**
		 * @brief Analyzes for bottlenecks.
		 * @return Bottleneck analysis report.
		 */
		[[nodiscard]] auto detect_bottlenecks() const -> bottleneck_report;

		// =========================================================================
		// Health Checks
		// =========================================================================

		/**
		 * @brief Performs comprehensive health check.
		 * @return Health status with all component states.
		 */
		[[nodiscard]] auto health_check() const -> health_status;

		/**
		 * @brief Quick check if pool is healthy.
		 * @return true if pool is operational.
		 */
		[[nodiscard]] auto is_healthy() const -> bool;

		// =========================================================================
		// Event Tracing
		// =========================================================================

		/**
		 * @brief Enables or disables job execution tracing.
		 * @param enable Enable or disable tracing.
		 * @param history_size Number of events to retain.
		 */
		void enable_tracing(bool enable, std::size_t history_size = 1000);

		/**
		 * @brief Checks if tracing is enabled.
		 * @return true if tracing is enabled.
		 */
		[[nodiscard]] auto is_tracing_enabled() const -> bool;

		/**
		 * @brief Adds an event listener.
		 * @param listener Listener to add.
		 */
		void add_event_listener(std::shared_ptr<execution_event_listener> listener);

		/**
		 * @brief Removes an event listener.
		 * @param listener Listener to remove.
		 */
		void remove_event_listener(std::shared_ptr<execution_event_listener> listener);

		/**
		 * @brief Records a job execution event.
		 * @param event The event to record.
		 *
		 * Called internally by the thread pool on job lifecycle events.
		 */
		void record_event(const job_execution_event& event);

		/**
		 * @brief Gets recent execution events.
		 * @param limit Maximum events to return.
		 * @return Vector of recent events.
		 */
		[[nodiscard]] auto get_recent_events(std::size_t limit = 100) const
		    -> std::vector<job_execution_event>;

		// =========================================================================
		// Export
		// =========================================================================

		/**
		 * @brief Exports diagnostics as JSON.
		 * @return JSON string with all diagnostic data.
		 */
		[[nodiscard]] auto to_json() const -> std::string;

		/**
		 * @brief Exports diagnostics as formatted string.
		 * @return Human-readable string.
		 */
		[[nodiscard]] auto to_string() const -> std::string;

		// =========================================================================
		// Configuration
		// =========================================================================

		/**
		 * @brief Gets the current configuration.
		 * @return Current diagnostics configuration.
		 */
		[[nodiscard]] auto get_config() const -> diagnostics_config;

		/**
		 * @brief Updates the configuration.
		 * @param config New configuration to apply.
		 */
		void set_config(const diagnostics_config& config);

	private:
		/**
		 * @brief Reference to the monitored thread pool.
		 */
		thread_pool& pool_;

		/**
		 * @brief Configuration for diagnostics.
		 */
		diagnostics_config config_;

		/**
		 * @brief Whether event tracing is enabled.
		 */
		std::atomic<bool> tracing_enabled_{false};

		/**
		 * @brief Mutex for event history access.
		 */
		mutable std::mutex events_mutex_;

		/**
		 * @brief Ring buffer for event history.
		 */
		std::deque<job_execution_event> event_history_;

		/**
		 * @brief Mutex for recent jobs access.
		 */
		mutable std::mutex jobs_mutex_;

		/**
		 * @brief Ring buffer for recent job completions.
		 */
		std::deque<job_info> recent_jobs_;

		/**
		 * @brief Mutex for event listeners.
		 */
		mutable std::mutex listeners_mutex_;

		/**
		 * @brief Event listeners.
		 */
		std::vector<std::shared_ptr<execution_event_listener>> listeners_;

		/**
		 * @brief Counter for event IDs.
		 */
		std::atomic<std::uint64_t> next_event_id_{0};

		/**
		 * @brief Time when the pool was started.
		 */
		std::chrono::steady_clock::time_point start_time_;

		// =========================================================================
		// Internal Helpers
		// =========================================================================

		/**
		 * @brief Gets thread info for a single worker.
		 * @param worker The worker to query.
		 * @param index Worker index in the pool.
		 * @return Thread information.
		 */
		[[nodiscard]] auto get_worker_info(const thread_worker& worker,
		                                   std::size_t index) const -> thread_info;

		/**
		 * @brief Notifies all event listeners.
		 * @param event The event to broadcast.
		 */
		void notify_listeners(const job_execution_event& event);

		/**
		 * @brief Generates recommendations for a bottleneck.
		 * @param report The bottleneck report to add recommendations to.
		 */
		void generate_recommendations(bottleneck_report& report) const;

		/**
		 * @brief Checks worker component health.
		 * @return Component health status for workers.
		 */
		[[nodiscard]] auto check_worker_health() const -> component_health;

		/**
		 * @brief Checks queue component health.
		 * @return Component health status for queue.
		 */
		[[nodiscard]] auto check_queue_health() const -> component_health;

		/**
		 * @brief Checks metrics component health.
		 * @param avg_latency_ms Current average latency.
		 * @param success_rate Current success rate.
		 * @return Component health status for metrics.
		 */
		[[nodiscard]] auto check_metrics_health(double avg_latency_ms,
		                                        double success_rate) const -> component_health;
	};

} // namespace diagnostics
} // namespace kcenon::thread
