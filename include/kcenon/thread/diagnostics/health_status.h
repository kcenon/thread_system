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

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace kcenon::thread::diagnostics
{
	/**
	 * @struct health_thresholds
	 * @brief Configurable thresholds for health status determination.
	 *
	 * @ingroup diagnostics
	 *
	 * Defines the thresholds used to determine if components are
	 * healthy, degraded, or unhealthy. These can be customized
	 * based on application requirements.
	 */
	struct health_thresholds
	{
		/**
		 * @brief Minimum success rate for healthy status (0.0 to 1.0).
		 *
		 * Below this threshold, the pool is considered degraded.
		 */
		double min_success_rate{0.95};

		/**
		 * @brief Success rate below which pool is unhealthy (0.0 to 1.0).
		 */
		double unhealthy_success_rate{0.8};

		/**
		 * @brief Maximum average latency (ms) for healthy status.
		 */
		double max_healthy_latency_ms{100.0};

		/**
		 * @brief Latency (ms) above which pool is considered degraded.
		 */
		double degraded_latency_ms{500.0};

		/**
		 * @brief Queue saturation threshold for degraded status (0.0 to 1.0).
		 */
		double queue_saturation_warning{0.8};

		/**
		 * @brief Queue saturation threshold for unhealthy status (0.0 to 1.0).
		 */
		double queue_saturation_critical{0.95};

		/**
		 * @brief Worker utilization threshold for degraded status (0.0 to 1.0).
		 */
		double worker_utilization_warning{0.9};

		/**
		 * @brief Minimum number of idle workers required for healthy status.
		 *
		 * Set to 0 to disable this check.
		 */
		std::size_t min_idle_workers{0};
	};

	/**
	 * @enum health_state
	 * @brief Overall health state of a component or system.
	 *
	 * @ingroup diagnostics
	 *
	 * Standard health states compatible with most health check frameworks
	 * and Kubernetes-style health probes.
	 */
	enum class health_state
	{
		healthy,    ///< Component is fully operational
		degraded,   ///< Component is operational but with reduced capacity/performance
		unhealthy,  ///< Component is not operational or failing
		unknown     ///< Health state cannot be determined
	};

	/**
	 * @brief Converts health_state to human-readable string.
	 * @param state The state to convert.
	 * @return String representation of the health state.
	 */
	[[nodiscard]] inline auto health_state_to_string(health_state state) -> std::string
	{
		switch (state)
		{
			case health_state::healthy: return "healthy";
			case health_state::degraded: return "degraded";
			case health_state::unhealthy: return "unhealthy";
			case health_state::unknown: return "unknown";
			default: return "unknown";
		}
	}

	/**
	 * @brief Gets HTTP status code for health state.
	 *
	 * Useful for implementing health check HTTP endpoints.
	 * @param state The health state.
	 * @return HTTP status code (200, 503, etc.).
	 */
	[[nodiscard]] inline auto health_state_to_http_code(health_state state) -> int
	{
		switch (state)
		{
			case health_state::healthy: return 200;
			case health_state::degraded: return 200;  // Still operational
			case health_state::unhealthy: return 503;
			case health_state::unknown: return 503;
			default: return 503;
		}
	}

	/**
	 * @struct component_health
	 * @brief Health status of a single component.
	 *
	 * @ingroup diagnostics
	 *
	 * Represents the health of a single subsystem or component
	 * within the thread pool (e.g., workers, queue, metrics).
	 */
	struct component_health
	{
		/**
		 * @brief Name of the component (e.g., "workers", "queue", "metrics").
		 */
		std::string name;

		/**
		 * @brief Current health state of this component.
		 */
		health_state state{health_state::unknown};

		/**
		 * @brief Human-readable message describing the current state.
		 */
		std::string message;

		/**
		 * @brief Additional details about this component's health.
		 *
		 * Key-value pairs with component-specific metrics or information.
		 */
		std::map<std::string, std::string> details;

		/**
		 * @brief Checks if this component is operational.
		 * @return true if healthy or degraded.
		 */
		[[nodiscard]] auto is_operational() const -> bool
		{
			return state == health_state::healthy ||
			       state == health_state::degraded;
		}
	};

	/**
	 * @struct health_status
	 * @brief Comprehensive health status of the thread pool.
	 *
	 * @ingroup diagnostics
	 *
	 * Contains overall health status, individual component health,
	 * and summary metrics. Designed to be compatible with standard
	 * health check frameworks and easily serializable to JSON.
	 *
	 * ### Health Check Integration
	 * This structure is designed to integrate with:
	 * - Kubernetes liveness/readiness probes
	 * - Spring Boot Actuator style health endpoints
	 * - Prometheus health metrics
	 *
	 * ### Usage Example
	 * @code
	 * auto health = pool->diagnostics().health_check();
	 * if (health.overall_status == health_state::healthy) {
	 *     return http_response(200, health.to_json());
	 * } else {
	 *     return http_response(health_state_to_http_code(health.overall_status),
	 *                         health.to_json());
	 * }
	 * @endcode
	 */
	struct health_status
	{
		/**
		 * @brief Overall health state of the thread pool.
		 *
		 * Aggregated from all component health states.
		 * If any component is unhealthy, overall is unhealthy.
		 * If any component is degraded, overall is degraded.
		 */
		health_state overall_status{health_state::unknown};

		/**
		 * @brief Human-readable message about overall status.
		 */
		std::string status_message;

		/**
		 * @brief Time when this health check was performed.
		 */
		std::chrono::steady_clock::time_point check_time;

		/**
		 * @brief Health status of individual components.
		 */
		std::vector<component_health> components;

		// =========================================================================
		// Summary Metrics
		// =========================================================================

		/**
		 * @brief Time since the thread pool was started (seconds).
		 */
		double uptime_seconds{0.0};

		/**
		 * @brief Total number of jobs processed since startup.
		 */
		std::uint64_t total_jobs_processed{0};

		/**
		 * @brief Job success rate (0.0 to 1.0).
		 */
		double success_rate{1.0};

		/**
		 * @brief Average job latency in milliseconds.
		 */
		double avg_latency_ms{0.0};

		/**
		 * @brief Number of active workers.
		 */
		std::size_t active_workers{0};

		/**
		 * @brief Total number of workers.
		 */
		std::size_t total_workers{0};

		/**
		 * @brief Current queue depth.
		 */
		std::size_t queue_depth{0};

		/**
		 * @brief Queue capacity (if bounded).
		 */
		std::size_t queue_capacity{0};

		// =========================================================================
		// Computed Properties
		// =========================================================================

		/**
		 * @brief Checks if the thread pool is operational.
		 * @return true if overall status is healthy or degraded.
		 */
		[[nodiscard]] auto is_operational() const -> bool
		{
			return overall_status == health_state::healthy ||
			       overall_status == health_state::degraded;
		}

		/**
		 * @brief Checks if the thread pool is fully healthy.
		 * @return true if overall status is healthy.
		 */
		[[nodiscard]] auto is_healthy() const -> bool
		{
			return overall_status == health_state::healthy;
		}

		/**
		 * @brief Gets HTTP status code for this health status.
		 * @return Appropriate HTTP status code.
		 */
		[[nodiscard]] auto http_status_code() const -> int
		{
			return health_state_to_http_code(overall_status);
		}

		/**
		 * @brief Finds a component by name.
		 * @param name Component name to search for.
		 * @return Pointer to component health, or nullptr if not found.
		 */
		[[nodiscard]] auto find_component(const std::string& name) const
		    -> const component_health*
		{
			for (const auto& comp : components)
			{
				if (comp.name == name)
				{
					return &comp;
				}
			}
			return nullptr;
		}

		/**
		 * @brief Calculates overall status from component states.
		 *
		 * Updates overall_status based on component health:
		 * - If any unhealthy → unhealthy
		 * - If any degraded → degraded
		 * - If all healthy → healthy
		 * - If empty → unknown
		 */
		auto calculate_overall_status() -> void
		{
			if (components.empty())
			{
				overall_status = health_state::unknown;
				status_message = "No components registered";
				return;
			}

			bool has_unhealthy = false;
			bool has_degraded = false;
			bool has_unknown = false;

			for (const auto& comp : components)
			{
				switch (comp.state)
				{
					case health_state::unhealthy:
						has_unhealthy = true;
						break;
					case health_state::degraded:
						has_degraded = true;
						break;
					case health_state::unknown:
						has_unknown = true;
						break;
					default:
						break;
				}
			}

			if (has_unhealthy)
			{
				overall_status = health_state::unhealthy;
				status_message = "One or more components are unhealthy";
			}
			else if (has_degraded)
			{
				overall_status = health_state::degraded;
				status_message = "One or more components are degraded";
			}
			else if (has_unknown)
			{
				overall_status = health_state::degraded;
				status_message = "One or more components have unknown status";
			}
			else
			{
				overall_status = health_state::healthy;
				status_message = "All components are healthy";
			}
		}

		// =========================================================================
		// Serialization
		// =========================================================================

		/**
		 * @brief Converts health status to JSON string.
		 *
		 * Output format is compatible with standard health check endpoints
		 * and monitoring tools like Kubernetes, Spring Boot Actuator, etc.
		 *
		 * @return JSON string representation of the health status.
		 */
		[[nodiscard]] auto to_json() const -> std::string
		{
			std::ostringstream oss;
			oss << std::fixed;

			oss << "{\n";
			oss << "  \"status\": \"" << health_state_to_string(overall_status) << "\",\n";
			oss << "  \"message\": \"" << status_message << "\",\n";
			oss << "  \"http_code\": " << http_status_code() << ",\n";

			// Metrics
			oss << "  \"metrics\": {\n";
			oss << "    \"uptime_seconds\": " << std::setprecision(2) << uptime_seconds << ",\n";
			oss << "    \"total_jobs_processed\": " << total_jobs_processed << ",\n";
			oss << "    \"success_rate\": " << std::setprecision(4) << success_rate << ",\n";
			oss << "    \"avg_latency_ms\": " << std::setprecision(3) << avg_latency_ms << "\n";
			oss << "  },\n";

			// Workers
			oss << "  \"workers\": {\n";
			oss << "    \"total\": " << total_workers << ",\n";
			oss << "    \"active\": " << active_workers << ",\n";
			oss << "    \"idle\": " << (total_workers - active_workers) << "\n";
			oss << "  },\n";

			// Queue
			oss << "  \"queue\": {\n";
			oss << "    \"depth\": " << queue_depth << ",\n";
			oss << "    \"capacity\": " << queue_capacity << "\n";
			oss << "  },\n";

			// Components
			oss << "  \"components\": [\n";
			for (std::size_t i = 0; i < components.size(); ++i)
			{
				const auto& comp = components[i];
				oss << "    {\n";
				oss << "      \"name\": \"" << comp.name << "\",\n";
				oss << "      \"status\": \"" << health_state_to_string(comp.state) << "\",\n";
				oss << "      \"message\": \"" << comp.message << "\"";

				if (!comp.details.empty())
				{
					oss << ",\n      \"details\": {\n";
					std::size_t detail_idx = 0;
					for (const auto& [key, value] : comp.details)
					{
						oss << "        \"" << key << "\": \"" << value << "\"";
						if (++detail_idx < comp.details.size())
						{
							oss << ",";
						}
						oss << "\n";
					}
					oss << "      }\n";
				}
				else
				{
					oss << "\n";
				}

				oss << "    }";
				if (i < components.size() - 1)
				{
					oss << ",";
				}
				oss << "\n";
			}
			oss << "  ]\n";
			oss << "}";

			return oss.str();
		}

		/**
		 * @brief Converts health status to human-readable string.
		 *
		 * Provides a formatted text representation suitable for logging
		 * or console output.
		 *
		 * @return Human-readable string representation.
		 */
		[[nodiscard]] auto to_string() const -> std::string
		{
			std::ostringstream oss;
			oss << std::fixed;

			oss << "=== Health Status: " << health_state_to_string(overall_status)
			    << " (HTTP " << http_status_code() << ") ===\n";
			oss << "Message: " << status_message << "\n\n";

			oss << "Metrics:\n";
			oss << "  Uptime: " << std::setprecision(1) << uptime_seconds << " seconds\n";
			oss << "  Jobs processed: " << total_jobs_processed << "\n";
			oss << "  Success rate: " << std::setprecision(1) << (success_rate * 100.0) << "%\n";
			oss << "  Avg latency: " << std::setprecision(2) << avg_latency_ms << " ms\n\n";

			oss << "Workers: " << active_workers << "/" << total_workers << " active";
			if (total_workers > 0)
			{
				oss << " (" << (total_workers - active_workers) << " idle)";
			}
			oss << "\n";

			oss << "Queue: " << queue_depth;
			if (queue_capacity > 0)
			{
				double saturation = static_cast<double>(queue_depth) /
				                    static_cast<double>(queue_capacity) * 100.0;
				oss << "/" << queue_capacity << " (" << std::setprecision(1)
				    << saturation << "% full)";
			}
			oss << "\n\n";

			oss << "Components:\n";
			for (const auto& comp : components)
			{
				oss << "  [" << health_state_to_string(comp.state) << "] "
				    << comp.name << ": " << comp.message << "\n";
			}

			return oss.str();
		}
	};

} // namespace kcenon::thread::diagnostics
