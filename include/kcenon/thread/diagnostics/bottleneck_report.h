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

#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace kcenon::thread::diagnostics
{
	/**
	 * @enum bottleneck_type
	 * @brief Type of bottleneck detected in the thread pool.
	 *
	 * @ingroup diagnostics
	 *
	 * Categorizes different types of performance bottlenecks that can
	 * occur in a thread pool system.
	 */
	enum class bottleneck_type
	{
		none,                  ///< No bottleneck detected
		queue_full,            ///< Queue is at capacity
		slow_consumer,         ///< Workers can't keep up with job submission rate
		worker_starvation,     ///< Not enough workers for the workload
		lock_contention,       ///< High mutex wait times affecting throughput
		uneven_distribution,   ///< Work is not evenly distributed (work stealing needed)
		memory_pressure        ///< Excessive memory allocations causing slowdown
	};

	/**
	 * @brief Converts bottleneck_type to human-readable string.
	 * @param type The bottleneck type to convert.
	 * @return String representation of the bottleneck type.
	 */
	[[nodiscard]] inline auto bottleneck_type_to_string(bottleneck_type type) -> std::string
	{
		switch (type)
		{
			case bottleneck_type::none: return "none";
			case bottleneck_type::queue_full: return "queue_full";
			case bottleneck_type::slow_consumer: return "slow_consumer";
			case bottleneck_type::worker_starvation: return "worker_starvation";
			case bottleneck_type::lock_contention: return "lock_contention";
			case bottleneck_type::uneven_distribution: return "uneven_distribution";
			case bottleneck_type::memory_pressure: return "memory_pressure";
			default: return "unknown";
		}
	}

	/**
	 * @struct bottleneck_report
	 * @brief Analysis report of bottlenecks in the thread pool.
	 *
	 * @ingroup diagnostics
	 *
	 * Contains the results of bottleneck analysis including the type of
	 * bottleneck detected, supporting metrics, and actionable recommendations.
	 *
	 * ### Diagnosis Logic
	 * ```
	 * queue_saturation > 0.9  → queue_full
	 * avg_wait_time > threshold && worker_utilization > 0.9  → slow_consumer
	 * worker_utilization > 0.95 && queue_saturation > 0.5   → worker_starvation
	 * utilization variance high  → uneven_distribution
	 * ```
	 *
	 * ### Usage Example
	 * @code
	 * auto report = pool->diagnostics().detect_bottlenecks();
	 * if (report.has_bottleneck) {
	 *     LOG_WARN("Bottleneck: {} - {}", bottleneck_type_to_string(report.type),
	 *              report.description);
	 *     for (const auto& rec : report.recommendations) {
	 *         LOG_INFO("  Recommendation: {}", rec);
	 *     }
	 * }
	 * @endcode
	 */
	struct bottleneck_report
	{
		/**
		 * @brief Whether a bottleneck was detected.
		 */
		bool has_bottleneck{false};

		/**
		 * @brief Human-readable description of the bottleneck.
		 *
		 * Empty if no bottleneck detected.
		 */
		std::string description;

		/**
		 * @brief Type of bottleneck detected.
		 */
		bottleneck_type type{bottleneck_type::none};

		// =========================================================================
		// Supporting Metrics
		// =========================================================================

		/**
		 * @brief Queue saturation level.
		 *
		 * Current queue depth as a ratio of maximum capacity (0.0 to 1.0+).
		 * Values above 1.0 indicate queue overflow attempts.
		 */
		double queue_saturation{0.0};

		/**
		 * @brief Average wait time in milliseconds.
		 *
		 * Average time jobs spend waiting in the queue before execution.
		 */
		double avg_wait_time_ms{0.0};

		/**
		 * @brief Average worker utilization.
		 *
		 * Average ratio of busy time across all workers (0.0 to 1.0).
		 */
		double worker_utilization{0.0};

		/**
		 * @brief Estimated time to process the current backlog.
		 *
		 * Based on current processing rate and queue depth.
		 */
		std::size_t estimated_backlog_time_ms{0};

		/**
		 * @brief Variance in worker utilization.
		 *
		 * High variance indicates uneven work distribution.
		 */
		double utilization_variance{0.0};

		/**
		 * @brief Jobs rejected due to queue full.
		 *
		 * Count of jobs rejected since last reset.
		 */
		std::uint64_t jobs_rejected{0};

		/**
		 * @brief Current queue depth.
		 */
		std::size_t queue_depth{0};

		/**
		 * @brief Number of idle workers.
		 */
		std::size_t idle_workers{0};

		/**
		 * @brief Total number of workers.
		 */
		std::size_t total_workers{0};

		// =========================================================================
		// Recommendations
		// =========================================================================

		/**
		 * @brief Actionable recommendations to resolve the bottleneck.
		 *
		 * List of suggestions based on the detected bottleneck type.
		 */
		std::vector<std::string> recommendations;

		// =========================================================================
		// Computed Properties
		// =========================================================================

		/**
		 * @brief Gets the severity level of the bottleneck.
		 * @return Severity from 0 (none) to 3 (critical).
		 */
		[[nodiscard]] auto severity() const -> int
		{
			if (!has_bottleneck)
			{
				return 0;
			}

			// Critical: queue full or severe worker starvation
			if (queue_saturation > 0.95 || worker_utilization > 0.98)
			{
				return 3;
			}

			// High: approaching capacity
			if (queue_saturation > 0.8 || worker_utilization > 0.9)
			{
				return 2;
			}

			// Medium: noticeable but not urgent
			return 1;
		}

		/**
		 * @brief Gets severity as a string.
		 * @return Severity level string.
		 */
		[[nodiscard]] auto severity_string() const -> std::string
		{
			switch (severity())
			{
				case 0: return "none";
				case 1: return "low";
				case 2: return "medium";
				case 3: return "critical";
				default: return "unknown";
			}
		}

		/**
		 * @brief Checks if immediate action is required.
		 * @return true if severity is critical (3).
		 */
		[[nodiscard]] auto requires_immediate_action() const -> bool
		{
			return severity() >= 3;
		}

		/**
		 * @brief Converts the bottleneck report to a JSON string.
		 * @return JSON representation of the bottleneck report.
		 *
		 * Output format:
		 * @code
		 * {
		 *   "has_bottleneck": true,
		 *   "type": "slow_consumer",
		 *   "severity": "medium",
		 *   "description": "Workers cannot keep up with job submission rate",
		 *   "metrics": {
		 *     "queue_saturation": 0.75,
		 *     "avg_wait_time_ms": 150.5,
		 *     "worker_utilization": 0.92,
		 *     "utilization_variance": 0.05,
		 *     "estimated_backlog_time_ms": 5000,
		 *     "queue_depth": 100,
		 *     "idle_workers": 1,
		 *     "total_workers": 8,
		 *     "jobs_rejected": 0
		 *   },
		 *   "recommendations": [...]
		 * }
		 * @endcode
		 */
		[[nodiscard]] auto to_json() const -> std::string
		{
			std::ostringstream oss;
			oss << "{\n";
			oss << "  \"has_bottleneck\": " << (has_bottleneck ? "true" : "false") << ",\n";
			oss << "  \"type\": \"" << bottleneck_type_to_string(type) << "\",\n";
			oss << "  \"severity\": \"" << severity_string() << "\",\n";
			oss << "  \"description\": \"" << description << "\",\n";

			// Metrics
			oss << "  \"metrics\": {\n";
			oss << std::fixed << std::setprecision(4);
			oss << "    \"queue_saturation\": " << queue_saturation << ",\n";
			oss << std::setprecision(3);
			oss << "    \"avg_wait_time_ms\": " << avg_wait_time_ms << ",\n";
			oss << std::setprecision(4);
			oss << "    \"worker_utilization\": " << worker_utilization << ",\n";
			oss << "    \"utilization_variance\": " << utilization_variance << ",\n";
			oss << "    \"estimated_backlog_time_ms\": " << estimated_backlog_time_ms << ",\n";
			oss << "    \"queue_depth\": " << queue_depth << ",\n";
			oss << "    \"idle_workers\": " << idle_workers << ",\n";
			oss << "    \"total_workers\": " << total_workers << ",\n";
			oss << "    \"jobs_rejected\": " << jobs_rejected << "\n";
			oss << "  },\n";

			// Recommendations
			oss << "  \"recommendations\": [";
			for (std::size_t i = 0; i < recommendations.size(); ++i)
			{
				oss << "\n    \"" << recommendations[i] << "\"";
				if (i < recommendations.size() - 1)
				{
					oss << ",";
				}
			}
			if (!recommendations.empty())
			{
				oss << "\n  ";
			}
			oss << "]\n";

			oss << "}";
			return oss.str();
		}

		/**
		 * @brief Converts the bottleneck report to a human-readable string.
		 * @return String representation of the bottleneck report.
		 *
		 * Output format:
		 * ```
		 * === Bottleneck Report ===
		 * Status: DETECTED (medium severity)
		 * Type: slow_consumer
		 * Description: Workers cannot keep up with job submission rate
		 *
		 * Metrics:
		 *   Queue: 100 items (75.0% saturated)
		 *   Workers: 7/8 active (1 idle)
		 *   Utilization: 92.0% (variance: 0.0500)
		 *   Wait time: 150.500ms avg
		 *   Backlog: ~5000ms to clear
		 *
		 * Recommendations:
		 *   - Add more worker threads
		 *   - Optimize job execution time
		 * ```
		 */
		[[nodiscard]] auto to_string() const -> std::string
		{
			std::ostringstream oss;

			oss << "=== Bottleneck Report ===\n";
			if (has_bottleneck)
			{
				oss << "Status: DETECTED (" << severity_string() << " severity)\n";
				oss << "Type: " << bottleneck_type_to_string(type) << "\n";
				oss << "Description: " << description << "\n\n";
			}
			else
			{
				oss << "Status: No bottleneck detected\n\n";
			}

			// Metrics
			oss << "Metrics:\n";
			oss << std::fixed;
			oss << "  Queue: " << queue_depth << " items ("
			    << std::setprecision(1) << (queue_saturation * 100.0) << "% saturated)\n";
			oss << "  Workers: " << (total_workers - idle_workers) << "/" << total_workers
			    << " active (" << idle_workers << " idle)\n";
			oss << "  Utilization: " << std::setprecision(1) << (worker_utilization * 100.0)
			    << "% (variance: " << std::setprecision(4) << utilization_variance << ")\n";
			oss << "  Wait time: " << std::setprecision(3) << avg_wait_time_ms << "ms avg\n";
			oss << "  Backlog: ~" << estimated_backlog_time_ms << "ms to clear\n";

			if (jobs_rejected > 0)
			{
				oss << "  Jobs rejected: " << jobs_rejected << "\n";
			}

			// Recommendations
			if (!recommendations.empty())
			{
				oss << "\nRecommendations:\n";
				for (const auto& rec : recommendations)
				{
					oss << "  - " << rec << "\n";
				}
			}

			return oss.str();
		}
	};

} // namespace kcenon::thread::diagnostics
