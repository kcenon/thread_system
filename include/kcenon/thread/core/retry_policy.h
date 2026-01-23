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
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <string>

namespace kcenon::thread
{
	/**
	 * @enum retry_strategy
	 * @brief Defines the strategy for calculating delay between retry attempts.
	 */
	enum class retry_strategy
	{
		none,                ///< No retry
		fixed,               ///< Fixed delay between retries
		linear,              ///< Linearly increasing delay
		exponential_backoff  ///< Exponentially increasing delay with optional jitter
	};

	/**
	 * @class retry_policy
	 * @brief Encapsulates retry behavior configuration for jobs.
	 *
	 * The retry_policy class provides a flexible way to configure how failed jobs
	 * should be retried. It supports multiple retry strategies:
	 * - **none**: No retry, fail immediately
	 * - **fixed**: Constant delay between each retry
	 * - **linear**: Delay increases linearly (delay * attempt_number)
	 * - **exponential_backoff**: Delay doubles with each attempt (with optional jitter)
	 *
	 * ### Thread Safety
	 * - All getter methods are const and thread-safe
	 * - The policy is typically configured once before job submission
	 *
	 * ### Usage Example
	 * @code
	 * // No retry
	 * auto policy = retry_policy::none();
	 *
	 * // Fixed delay of 100ms, max 3 attempts
	 * auto policy = retry_policy::fixed(3, std::chrono::milliseconds(100));
	 *
	 * // Exponential backoff: 100ms, 200ms, 400ms, 800ms (max 4 attempts)
	 * auto policy = retry_policy::exponential_backoff(4);
	 *
	 * // Custom exponential with jitter
	 * auto policy = retry_policy::exponential_backoff(5, 50ms, 2.0, 10000ms, true);
	 * @endcode
	 */
	class retry_policy
	{
	public:
		/**
		 * @brief Default constructor creates a "no retry" policy.
		 */
		retry_policy()
			: strategy_(retry_strategy::none)
			, max_attempts_(1)
			, initial_delay_(std::chrono::milliseconds(0))
			, multiplier_(1.0)
			, max_delay_(std::chrono::milliseconds(0))
			, use_jitter_(false)
			, current_attempt_(0)
		{
		}

		/**
		 * @brief Creates a policy that disables retry.
		 * @return A retry_policy with no retry behavior
		 */
		[[nodiscard]] static auto no_retry() -> retry_policy
		{
			return retry_policy();
		}

		/**
		 * @brief Creates a fixed delay retry policy.
		 *
		 * @param max_attempts Maximum number of attempts (including initial)
		 * @param delay Fixed delay between attempts
		 * @return A retry_policy with fixed delay
		 *
		 * @note If max_attempts is 1, no retry will occur (same as none())
		 */
		[[nodiscard]] static auto fixed(
			std::size_t max_attempts,
			std::chrono::milliseconds delay) -> retry_policy
		{
			retry_policy policy;
			policy.strategy_ = retry_strategy::fixed;
			policy.max_attempts_ = max_attempts;
			policy.initial_delay_ = delay;
			policy.multiplier_ = 1.0;
			policy.max_delay_ = delay;
			return policy;
		}

		/**
		 * @brief Creates a linear backoff retry policy.
		 *
		 * Delay increases linearly: delay * attempt_number
		 *
		 * @param max_attempts Maximum number of attempts (including initial)
		 * @param initial_delay Base delay for first retry
		 * @param max_delay Maximum delay cap (default: no cap)
		 * @return A retry_policy with linear backoff
		 */
		[[nodiscard]] static auto linear(
			std::size_t max_attempts,
			std::chrono::milliseconds initial_delay,
			std::chrono::milliseconds max_delay = std::chrono::milliseconds::max()) -> retry_policy
		{
			retry_policy policy;
			policy.strategy_ = retry_strategy::linear;
			policy.max_attempts_ = max_attempts;
			policy.initial_delay_ = initial_delay;
			policy.multiplier_ = 1.0;
			policy.max_delay_ = max_delay;
			return policy;
		}

		/**
		 * @brief Creates an exponential backoff retry policy.
		 *
		 * Delay doubles with each attempt: initial_delay * (multiplier ^ attempt)
		 *
		 * @param max_attempts Maximum number of attempts (including initial)
		 * @param initial_delay Base delay for first retry (default: 100ms)
		 * @param multiplier Exponential multiplier (default: 2.0)
		 * @param max_delay Maximum delay cap (default: 30 seconds)
		 * @param use_jitter Add randomness to prevent thundering herd
		 * @return A retry_policy with exponential backoff
		 *
		 * @note With jitter enabled, the actual delay will be uniformly distributed
		 *       between 0 and the calculated delay value.
		 */
		[[nodiscard]] static auto exponential_backoff(
			std::size_t max_attempts,
			std::chrono::milliseconds initial_delay = std::chrono::milliseconds(100),
			double multiplier = 2.0,
			std::chrono::milliseconds max_delay = std::chrono::milliseconds(30000),
			bool use_jitter = false) -> retry_policy
		{
			retry_policy policy;
			policy.strategy_ = retry_strategy::exponential_backoff;
			policy.max_attempts_ = max_attempts;
			policy.initial_delay_ = initial_delay;
			policy.multiplier_ = multiplier;
			policy.max_delay_ = max_delay;
			policy.use_jitter_ = use_jitter;
			return policy;
		}

		/**
		 * @brief Gets the retry strategy type.
		 * @return The current retry strategy
		 */
		[[nodiscard]] auto get_strategy() const -> retry_strategy
		{
			return strategy_;
		}

		/**
		 * @brief Gets the maximum number of attempts.
		 * @return Maximum attempts (1 means no retry)
		 */
		[[nodiscard]] auto get_max_attempts() const -> std::size_t
		{
			return max_attempts_;
		}

		/**
		 * @brief Gets the initial delay between retries.
		 * @return Initial delay duration
		 */
		[[nodiscard]] auto get_initial_delay() const -> std::chrono::milliseconds
		{
			return initial_delay_;
		}

		/**
		 * @brief Gets the multiplier used for exponential backoff.
		 * @return Exponential multiplier
		 */
		[[nodiscard]] auto get_multiplier() const -> double
		{
			return multiplier_;
		}

		/**
		 * @brief Gets the maximum delay cap.
		 * @return Maximum delay duration
		 */
		[[nodiscard]] auto get_max_delay() const -> std::chrono::milliseconds
		{
			return max_delay_;
		}

		/**
		 * @brief Checks if jitter is enabled.
		 * @return true if jitter is enabled
		 */
		[[nodiscard]] auto uses_jitter() const -> bool
		{
			return use_jitter_;
		}

		/**
		 * @brief Checks if retry is enabled.
		 * @return true if max_attempts > 1 and strategy is not none
		 */
		[[nodiscard]] auto is_retry_enabled() const -> bool
		{
			return strategy_ != retry_strategy::none && max_attempts_ > 1;
		}

		/**
		 * @brief Gets the current attempt number (0-based).
		 * @return Current attempt number
		 */
		[[nodiscard]] auto get_current_attempt() const -> std::size_t
		{
			return current_attempt_;
		}

		/**
		 * @brief Checks if more retry attempts are available.
		 * @return true if current_attempt < max_attempts - 1
		 */
		[[nodiscard]] auto has_attempts_remaining() const -> bool
		{
			return current_attempt_ < max_attempts_ - 1;
		}

		/**
		 * @brief Increments the attempt counter.
		 *
		 * Call this after each failed attempt. If the counter reaches
		 * max_attempts, has_attempts_remaining() will return false.
		 */
		auto record_attempt() -> void
		{
			++current_attempt_;
		}

		/**
		 * @brief Resets the attempt counter to zero.
		 */
		auto reset() -> void
		{
			current_attempt_ = 0;
		}

		/**
		 * @brief Calculates the delay for the current retry attempt.
		 *
		 * @return Delay duration for the current attempt
		 *
		 * @note This does not increment the attempt counter.
		 */
		[[nodiscard]] auto get_delay_for_current_attempt() const -> std::chrono::milliseconds
		{
			if (strategy_ == retry_strategy::none || current_attempt_ == 0)
			{
				return std::chrono::milliseconds(0);
			}

			std::chrono::milliseconds delay;

			switch (strategy_)
			{
				case retry_strategy::fixed:
					delay = initial_delay_;
					break;

				case retry_strategy::linear:
					delay = std::chrono::milliseconds(
						initial_delay_.count() * static_cast<long long>(current_attempt_)
					);
					break;

				case retry_strategy::exponential_backoff:
					{
						double factor = std::pow(multiplier_, static_cast<double>(current_attempt_ - 1));
						auto calculated = static_cast<long long>(
							static_cast<double>(initial_delay_.count()) * factor
						);
						delay = std::chrono::milliseconds(calculated);
					}
					break;

				default:
					delay = std::chrono::milliseconds(0);
					break;
			}

			// Apply max_delay cap
			if (delay > max_delay_)
			{
				delay = max_delay_;
			}

			return delay;
		}

		/**
		 * @brief Provides a string representation of the policy.
		 * @return Human-readable description of the policy
		 */
		[[nodiscard]] auto to_string() const -> std::string
		{
			switch (strategy_)
			{
				case retry_strategy::none:
					return "retry_policy(none)";
				case retry_strategy::fixed:
					return "retry_policy(fixed, attempts=" + std::to_string(max_attempts_) +
						   ", delay=" + std::to_string(initial_delay_.count()) + "ms)";
				case retry_strategy::linear:
					return "retry_policy(linear, attempts=" + std::to_string(max_attempts_) +
						   ", initial=" + std::to_string(initial_delay_.count()) + "ms)";
				case retry_strategy::exponential_backoff:
					return "retry_policy(exponential, attempts=" + std::to_string(max_attempts_) +
						   ", initial=" + std::to_string(initial_delay_.count()) +
						   "ms, multiplier=" + std::to_string(multiplier_) + ")";
				default:
					return "retry_policy(unknown)";
			}
		}

	private:
		retry_strategy strategy_;
		std::size_t max_attempts_;
		std::chrono::milliseconds initial_delay_;
		double multiplier_;
		std::chrono::milliseconds max_delay_;
		bool use_jitter_;
		std::size_t current_attempt_;
	};

} // namespace kcenon::thread
