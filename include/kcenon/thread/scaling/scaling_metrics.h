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

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace kcenon::thread
{
	/**
	 * @brief Scaling direction for autoscaling decisions.
	 */
	enum class scaling_direction
	{
		none,   ///< No scaling needed
		up,     ///< Scale up (add workers)
		down    ///< Scale down (remove workers)
	};

	/**
	 * @brief Reason for scaling decision.
	 */
	enum class scaling_reason
	{
		queue_depth,        ///< Queue depth threshold exceeded
		worker_utilization, ///< Worker utilization threshold exceeded
		latency,            ///< Latency threshold exceeded
		manual,             ///< Manual trigger via API
		scheduled           ///< Scheduled scaling event
	};

	/**
	 * @brief Metrics sample for autoscaling decisions.
	 *
	 * This structure captures a snapshot of thread pool metrics at a
	 * specific point in time. Multiple samples are aggregated to make
	 * scaling decisions, preventing reactive scaling on transient spikes.
	 *
	 * @see autoscaler
	 */
	struct scaling_metrics_sample
	{
		/// Timestamp when this sample was collected
		std::chrono::steady_clock::time_point timestamp;

		/// Current number of workers in the pool
		std::size_t worker_count{0};

		/// Number of workers currently processing jobs
		std::size_t active_workers{0};

		/// Number of jobs waiting in the queue
		std::size_t queue_depth{0};

		/// Worker utilization ratio (0.0 - 1.0)
		double utilization{0.0};

		/// Jobs per worker ratio
		double queue_depth_per_worker{0.0};

		/// P95 latency in milliseconds
		double p95_latency_ms{0.0};

		/// Jobs completed since last sample
		std::uint64_t jobs_completed{0};

		/// Jobs submitted since last sample
		std::uint64_t jobs_submitted{0};

		/// Throughput in jobs per second
		double throughput_per_second{0.0};
	};

	/**
	 * @brief Scaling decision result.
	 *
	 * Contains the decision made by the autoscaler along with
	 * the reason and explanation for debugging and logging.
	 */
	struct scaling_decision
	{
		/// The scaling direction
		scaling_direction direction{scaling_direction::none};

		/// Reason for the decision
		scaling_reason reason{scaling_reason::queue_depth};

		/// Target worker count after scaling
		std::size_t target_workers{0};

		/// Human-readable explanation
		std::string explanation;

		/**
		 * @brief Checks if scaling should occur.
		 * @return true if direction is not none.
		 */
		[[nodiscard]] auto should_scale() const -> bool
		{
			return direction != scaling_direction::none;
		}
	};

	/**
	 * @brief Statistics for autoscaling operations.
	 *
	 * Tracks historical scaling events and decisions for monitoring
	 * and debugging autoscaling behavior.
	 */
	struct autoscaling_stats
	{
		/// Number of scale-up events
		std::size_t scale_up_count{0};

		/// Number of scale-down events
		std::size_t scale_down_count{0};

		/// Number of decisions evaluated
		std::size_t decisions_evaluated{0};

		/// Time of last scale-up event
		std::chrono::steady_clock::time_point last_scale_up;

		/// Time of last scale-down event
		std::chrono::steady_clock::time_point last_scale_down;

		/// Peak worker count observed
		std::size_t peak_workers{0};

		/// Minimum worker count observed
		std::size_t min_workers{0};
	};

} // namespace kcenon::thread
