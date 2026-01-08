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

#include "job.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace kcenon::thread
{
	/**
	 * @enum backpressure_policy
	 * @brief Policy for handling queue overflow conditions.
	 *
	 * @ingroup backpressure
	 *
	 * Defines how the queue should respond when it reaches capacity or
	 * encounters pressure situations.
	 */
	enum class backpressure_policy
	{
		block,        ///< Block until space is available (with timeout)
		drop_oldest,  ///< Drop the oldest job when full to make room
		drop_newest,  ///< Reject the new job when full
		callback,     ///< Call user callback for custom decision
		adaptive      ///< Automatically adjust based on load conditions
	};

	/**
	 * @enum backpressure_decision
	 * @brief Decision returned by callback policy handler.
	 *
	 * @ingroup backpressure
	 *
	 * When using @c backpressure_policy::callback, the user-provided callback
	 * function returns one of these decisions to determine how to handle
	 * the incoming job.
	 */
	enum class backpressure_decision
	{
		accept,           ///< Accept the job into the queue
		reject,           ///< Reject with error (queue_full)
		drop_and_accept,  ///< Drop the oldest job, then accept new one
		delay             ///< Delay processing (attempt later)
	};

	/**
	 * @enum pressure_level
	 * @brief Current pressure level for graduated response.
	 *
	 * @ingroup backpressure
	 *
	 * Indicates the current load level of the queue based on watermark
	 * thresholds. Used by adaptive policies and for monitoring.
	 */
	enum class pressure_level
	{
		none,      ///< Below low_watermark, queue is healthy
		low,       ///< Between low and high watermark
		high,      ///< Above high_watermark, approaching capacity
		critical   ///< At or above max_size, queue is full
	};

	/**
	 * @brief Converts pressure_level to human-readable string.
	 * @param level The pressure level to convert.
	 * @return String representation of the pressure level.
	 */
	[[nodiscard]] inline auto pressure_level_to_string(pressure_level level) -> std::string
	{
		switch (level)
		{
			case pressure_level::none: return "none";
			case pressure_level::low: return "low";
			case pressure_level::high: return "high";
			case pressure_level::critical: return "critical";
			default: return "unknown";
		}
	}

	/**
	 * @brief Converts backpressure_policy to human-readable string.
	 * @param policy The policy to convert.
	 * @return String representation of the policy.
	 */
	[[nodiscard]] inline auto backpressure_policy_to_string(backpressure_policy policy) -> std::string
	{
		switch (policy)
		{
			case backpressure_policy::block: return "block";
			case backpressure_policy::drop_oldest: return "drop_oldest";
			case backpressure_policy::drop_newest: return "drop_newest";
			case backpressure_policy::callback: return "callback";
			case backpressure_policy::adaptive: return "adaptive";
			default: return "unknown";
		}
	}

	/**
	 * @struct backpressure_config
	 * @brief Configuration for backpressure mechanisms.
	 *
	 * @ingroup backpressure
	 *
	 * This structure contains all configurable parameters for the
	 * backpressure system, including policy selection, watermarks,
	 * rate limiting, and callbacks.
	 *
	 * ### Watermarks
	 * Watermarks define pressure thresholds as percentages of max_size:
	 * ```
	 * 0%                    50%              80%           100%
	 * |------ none ---------|---- low -------|--- high ----|critical|
	 *                    low_watermark    high_watermark  max_size
	 * ```
	 *
	 * ### Rate Limiting
	 * When enabled, the token bucket algorithm limits the rate of job
	 * acceptance regardless of queue capacity.
	 *
	 * ### Adaptive Mode
	 * In adaptive mode, the system monitors latency and automatically
	 * adjusts acceptance rate to maintain target latency.
	 *
	 * ### Usage Example
	 * @code
	 * backpressure_config config;
	 * config.policy = backpressure_policy::adaptive;
	 * config.high_watermark = 0.75;
	 * config.enable_rate_limiting = true;
	 * config.rate_limit_tokens_per_second = 5000;
	 * config.pressure_callback = [](std::size_t depth, double ratio) {
	 *     LOG_WARN("Queue pressure: {:.1f}%", ratio * 100);
	 * };
	 * @endcode
	 */
	struct backpressure_config
	{
		// =========================================================================
		// Policy Selection
		// =========================================================================

		/**
		 * @brief The backpressure policy to use.
		 *
		 * Determines how the queue handles overflow conditions:
		 * - block: Wait with timeout
		 * - drop_oldest: Remove oldest job to make room
		 * - drop_newest: Reject new job
		 * - callback: Call user callback for decision
		 * - adaptive: Auto-adjust based on conditions
		 */
		backpressure_policy policy = backpressure_policy::block;

		// =========================================================================
		// Watermarks
		// =========================================================================

		/**
		 * @brief High watermark threshold (percentage of max_size).
		 *
		 * When queue depth exceeds this threshold, pressure is considered "high".
		 * Range: 0.0 to 1.0 (e.g., 0.8 = 80% of capacity)
		 */
		double high_watermark = 0.8;

		/**
		 * @brief Low watermark threshold (percentage of max_size).
		 *
		 * When queue depth falls below this threshold, pressure returns to "none".
		 * Used for hysteresis to prevent oscillation.
		 * Range: 0.0 to 1.0 (e.g., 0.5 = 50% of capacity)
		 */
		double low_watermark = 0.5;

		// =========================================================================
		// Blocking Behavior
		// =========================================================================

		/**
		 * @brief Maximum time to block when using block policy.
		 *
		 * If the timeout expires before space becomes available, the enqueue
		 * operation returns an error.
		 */
		std::chrono::milliseconds block_timeout{5000};

		// =========================================================================
		// Rate Limiting (Token Bucket)
		// =========================================================================

		/**
		 * @brief Enable token bucket rate limiting.
		 *
		 * When enabled, job acceptance is limited by a token bucket
		 * regardless of queue capacity.
		 */
		bool enable_rate_limiting = false;

		/**
		 * @brief Token refill rate (tokens added per second).
		 *
		 * Determines the sustained throughput limit.
		 * Only effective when enable_rate_limiting is true.
		 */
		std::size_t rate_limit_tokens_per_second = 10000;

		/**
		 * @brief Maximum tokens that can accumulate (burst capacity).
		 *
		 * Allows short bursts above the sustained rate.
		 * Only effective when enable_rate_limiting is true.
		 */
		std::size_t rate_limit_burst_size = 1000;

		// =========================================================================
		// Callbacks
		// =========================================================================

		/**
		 * @brief Callback for pressure events.
		 *
		 * Called when pressure level changes. Receives:
		 * - queue_depth: Current number of jobs in queue
		 * - pressure_ratio: Current depth as ratio of max_size (0.0 to 1.0+)
		 *
		 * Useful for logging, alerting, or triggering external actions.
		 */
		std::function<void(std::size_t queue_depth, double pressure_ratio)> pressure_callback;

		/**
		 * @brief Custom decision callback for callback policy.
		 *
		 * Called when policy is backpressure_policy::callback.
		 * Receives reference to the job being enqueued.
		 * Returns backpressure_decision indicating how to proceed.
		 *
		 * @note The callback should be fast as it's called in the enqueue path.
		 */
		std::function<backpressure_decision(std::unique_ptr<job>&)> decision_callback;

		// =========================================================================
		// Adaptive Mode Settings
		// =========================================================================

		/**
		 * @brief Sampling interval for adaptive mode.
		 *
		 * How frequently the adaptive controller samples queue state
		 * to adjust its parameters.
		 */
		std::chrono::milliseconds adaptive_sample_interval{100};

		/**
		 * @brief Target latency for adaptive mode (milliseconds).
		 *
		 * The adaptive controller tries to maintain average queue wait time
		 * at or below this target by adjusting acceptance rate.
		 */
		double adaptive_target_latency_ms = 10.0;

		/**
		 * @brief Validates the configuration.
		 * @return true if configuration is valid, false otherwise.
		 *
		 * Checks:
		 * - Watermarks are in valid range (0.0 to 1.0)
		 * - low_watermark < high_watermark
		 * - Required callbacks are set for callback policy
		 */
		[[nodiscard]] auto is_valid() const -> bool
		{
			// Watermark validation
			if (low_watermark < 0.0 || low_watermark > 1.0)
			{
				return false;
			}
			if (high_watermark < 0.0 || high_watermark > 1.0)
			{
				return false;
			}
			if (low_watermark >= high_watermark)
			{
				return false;
			}

			// Callback policy requires decision_callback
			if (policy == backpressure_policy::callback && !decision_callback)
			{
				return false;
			}

			// Rate limiting validation
			if (enable_rate_limiting)
			{
				if (rate_limit_tokens_per_second == 0)
				{
					return false;
				}
				if (rate_limit_burst_size == 0)
				{
					return false;
				}
			}

			return true;
		}
	};

	/**
	 * @struct backpressure_stats_snapshot
	 * @brief Snapshot of backpressure statistics (copyable).
	 *
	 * @ingroup backpressure
	 *
	 * This is a copyable snapshot of statistics, suitable for returning
	 * from getter methods.
	 */
	struct backpressure_stats_snapshot
	{
		std::uint64_t jobs_accepted{0};
		std::uint64_t jobs_rejected{0};
		std::uint64_t jobs_dropped{0};
		std::uint64_t rate_limit_waits{0};
		std::uint64_t pressure_events{0};
		std::uint64_t total_block_time_ns{0};

		/**
		 * @brief Returns acceptance rate (accepted / total attempts).
		 * @return Acceptance ratio (0.0 to 1.0), or 1.0 if no attempts.
		 */
		[[nodiscard]] auto acceptance_rate() const -> double
		{
			auto total = jobs_accepted + jobs_rejected;
			if (total == 0)
			{
				return 1.0;
			}
			return static_cast<double>(jobs_accepted) / static_cast<double>(total);
		}

		/**
		 * @brief Returns average block time per blocked operation.
		 * @return Average block time in milliseconds.
		 */
		[[nodiscard]] auto avg_block_time_ms() const -> double
		{
			if (rate_limit_waits == 0)
			{
				return 0.0;
			}
			return static_cast<double>(total_block_time_ns) /
				static_cast<double>(rate_limit_waits) / 1e6;
		}
	};

	/**
	 * @struct backpressure_stats
	 * @brief Thread-safe statistics for backpressure operations.
	 *
	 * @ingroup backpressure
	 *
	 * Tracks various counters and timing information about backpressure
	 * events for monitoring and debugging. Uses atomics for thread-safety.
	 */
	struct backpressure_stats
	{
		/**
		 * @brief Total jobs accepted into the queue.
		 */
		std::atomic<std::uint64_t> jobs_accepted{0};

		/**
		 * @brief Total jobs rejected due to backpressure.
		 */
		std::atomic<std::uint64_t> jobs_rejected{0};

		/**
		 * @brief Total jobs dropped (oldest dropped for new).
		 */
		std::atomic<std::uint64_t> jobs_dropped{0};

		/**
		 * @brief Number of times rate limiting caused a wait.
		 */
		std::atomic<std::uint64_t> rate_limit_waits{0};

		/**
		 * @brief Number of times high watermark was crossed.
		 */
		std::atomic<std::uint64_t> pressure_events{0};

		/**
		 * @brief Total time spent blocking in nanoseconds.
		 */
		std::atomic<std::uint64_t> total_block_time_ns{0};

		/**
		 * @brief Resets all statistics to zero.
		 */
		auto reset() -> void
		{
			jobs_accepted.store(0, std::memory_order_relaxed);
			jobs_rejected.store(0, std::memory_order_relaxed);
			jobs_dropped.store(0, std::memory_order_relaxed);
			rate_limit_waits.store(0, std::memory_order_relaxed);
			pressure_events.store(0, std::memory_order_relaxed);
			total_block_time_ns.store(0, std::memory_order_relaxed);
		}

		/**
		 * @brief Creates a copyable snapshot of current statistics.
		 * @return Snapshot with current values.
		 */
		[[nodiscard]] auto snapshot() const -> backpressure_stats_snapshot
		{
			backpressure_stats_snapshot snap;
			snap.jobs_accepted = jobs_accepted.load(std::memory_order_relaxed);
			snap.jobs_rejected = jobs_rejected.load(std::memory_order_relaxed);
			snap.jobs_dropped = jobs_dropped.load(std::memory_order_relaxed);
			snap.rate_limit_waits = rate_limit_waits.load(std::memory_order_relaxed);
			snap.pressure_events = pressure_events.load(std::memory_order_relaxed);
			snap.total_block_time_ns = total_block_time_ns.load(std::memory_order_relaxed);
			return snap;
		}
	};

} // namespace kcenon::thread
