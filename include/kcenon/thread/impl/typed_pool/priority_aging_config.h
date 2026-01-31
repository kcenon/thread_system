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

/**
 * @deprecated This header is deprecated. Use thread_config.h instead.
 *
 * For unified configuration, include:
 * @code{.cpp}
 * #include <kcenon/thread/thread_config.h>
 *
 * auto config = thread_system_config::builder()
 *     .enable_priority_aging()
 *     .with_priority_aging_params(std::chrono::seconds{1}, 1, 3)
 *     .build();
 * @endcode
 */

#include <chrono>
#include <functional>
#include <string>
#include <cstdint>

namespace kcenon::thread
{
	/**
	 * @struct job_info
	 * @brief Information about a job for starvation callback.
	 */
	struct job_info
	{
		std::string job_name;
		std::chrono::milliseconds wait_time;
		int priority_boost;
	};

	/**
	 * @enum aging_curve
	 * @brief Defines different aging curve algorithms.
	 *
	 * The aging curve determines how priority boost is calculated over time.
	 */
	enum class aging_curve : uint8_t
	{
		linear,      ///< Constant boost per interval
		exponential, ///< Increasing boost over time
		logarithmic  ///< Decreasing boost (fast initial, slow later)
	};

	/**
	 * @struct priority_aging_config
	 * @brief Configuration for priority aging behavior.
	 *
	 * This structure contains all the parameters needed to configure
	 * priority aging in a typed thread pool. Priority aging prevents
	 * starvation of low-priority jobs by automatically boosting their
	 * priority based on wait time.
	 *
	 * ### Example Usage
	 * @code{.cpp}
	 * priority_aging_config config{
	 *     .enabled = true,
	 *     .aging_interval = std::chrono::seconds{1},
	 *     .priority_boost_per_interval = 1,
	 *     .max_priority_boost = 3,
	 *     .curve = aging_curve::linear
	 * };
	 * @endcode
	 */
	struct priority_aging_config
	{
		/**
		 * @brief Whether priority aging is enabled.
		 *
		 * When disabled, no priority boosting occurs.
		 */
		bool enabled = false;

		/**
		 * @brief Interval at which aging is applied.
		 *
		 * Jobs waiting longer than this interval will receive a priority boost.
		 */
		std::chrono::milliseconds aging_interval{1000};

		/**
		 * @brief Amount of priority boost applied per aging interval.
		 *
		 * Higher values result in faster priority escalation.
		 */
		int priority_boost_per_interval = 1;

		/**
		 * @brief Maximum total priority boost that can be applied.
		 *
		 * Prevents low-priority jobs from exceeding a certain priority level.
		 */
		int max_priority_boost = 3;

		/**
		 * @brief The aging curve algorithm to use.
		 *
		 * - linear: Constant boost per interval
		 * - exponential: Increasing boost over time
		 * - logarithmic: Fast initial boost, slower over time
		 */
		aging_curve curve = aging_curve::linear;

		/**
		 * @brief Exponential factor for exponential aging curve.
		 *
		 * Only used when curve is set to exponential.
		 */
		double exponential_factor = 1.5;

		/**
		 * @brief Threshold for starvation detection.
		 *
		 * If a job waits longer than this threshold, it is considered starving.
		 */
		std::chrono::seconds starvation_threshold{30};

		/**
		 * @brief Callback function invoked when a job is starving.
		 *
		 * This callback is called when a job has been waiting longer than
		 * the starvation_threshold. Can be used for alerting or monitoring.
		 */
		std::function<void(const job_info&)> starvation_callback;

		/**
		 * @brief Whether to reset the boost when a job is dequeued.
		 *
		 * When true, the priority boost is reset after the job is dequeued.
		 */
		bool reset_on_dequeue = true;
	};

	/**
	 * @struct aged_priority
	 * @brief Priority with aging support.
	 *
	 * This structure wraps a base priority value with aging information,
	 * including the current boost level and enqueue time. It provides
	 * methods to calculate the effective priority and wait time.
	 *
	 * @tparam P The base priority type (typically an enum or integral type).
	 *
	 * ### Example Usage
	 * @code{.cpp}
	 * aged_priority<job_types> ap{
	 *     job_types::Background,  // base_priority
	 *     0,                      // initial boost
	 *     std::chrono::steady_clock::now()  // enqueue_time
	 * };
	 *
	 * // Apply boost
	 * ap.apply_boost(1, 3);  // boost by 1, max 3
	 *
	 * // Get effective priority
	 * auto effective = ap.effective_priority();
	 * @endcode
	 */
	template<typename P>
	struct aged_priority
	{
		/**
		 * @brief The original priority level of the job.
		 */
		P base_priority;

		/**
		 * @brief The current priority boost value.
		 *
		 * This value is added to the base priority to get the effective priority.
		 */
		int boost{0};

		/**
		 * @brief The time when the job was enqueued.
		 *
		 * Used to calculate wait time for aging purposes.
		 */
		std::chrono::steady_clock::time_point enqueue_time;

		/**
		 * @brief Calculates the effective priority including boost.
		 *
		 * For enum types, the boost is subtracted from the enum value
		 * (lower enum value = higher priority). For integral types,
		 * the boost is added.
		 *
		 * @return The effective priority value.
		 */
		[[nodiscard]] auto effective_priority() const -> P
		{
			// For enum types, lower values typically mean higher priority
			// Subtracting boost moves toward higher priority (lower enum value)
			auto base_value = static_cast<int>(base_priority);
			auto boosted_value = base_value - boost;
			// Clamp to valid range (minimum 0)
			boosted_value = (boosted_value < 0) ? 0 : boosted_value;
			return static_cast<P>(boosted_value);
		}

		/**
		 * @brief Calculates the time this job has been waiting.
		 *
		 * @return The wait time since enqueue.
		 */
		[[nodiscard]] auto wait_time() const -> std::chrono::milliseconds
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - enqueue_time
			);
		}

		/**
		 * @brief Applies a boost to the priority.
		 *
		 * @param boost_amount The amount to boost.
		 * @param max_boost The maximum allowed boost.
		 */
		auto apply_boost(int boost_amount, int max_boost) -> void
		{
			boost += boost_amount;
			if (boost > max_boost)
			{
				boost = max_boost;
			}
		}

		/**
		 * @brief Resets the boost to zero.
		 */
		auto reset_boost() -> void
		{
			boost = 0;
		}

		/**
		 * @brief Checks if this job has reached max boost.
		 *
		 * @param max_boost The maximum allowed boost.
		 * @return true if boost equals max_boost.
		 */
		[[nodiscard]] auto is_max_boosted(int max_boost) const -> bool
		{
			return boost >= max_boost;
		}
	};

} // namespace kcenon::thread
