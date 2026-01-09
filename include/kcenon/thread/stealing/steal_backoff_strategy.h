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

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <random>

namespace kcenon::thread
{

/**
 * @enum steal_backoff_strategy
 * @brief Backoff strategies for work-stealing operations
 *
 * When a steal attempt fails, workers use backoff strategies to reduce
 * contention and allow other threads to make progress. Different strategies
 * offer different trade-offs between responsiveness and CPU usage.
 *
 * ### Strategy Comparison
 * | Strategy | Behavior | Best For |
 * |----------|----------|----------|
 * | fixed | Constant delay | Predictable workloads |
 * | linear | Increasing delay | Moderate contention |
 * | exponential | Rapidly increasing | High contention |
 * | adaptive_jitter | Exponential + random | Variable workloads |
 */
enum class steal_backoff_strategy : std::uint8_t
{
	fixed,           ///< Constant delay between steal attempts
	linear,          ///< Linear increase: delay = initial * (attempt + 1)
	exponential,     ///< Exponential increase: delay = initial * 2^attempt
	adaptive_jitter  ///< Exponential with random jitter for anti-correlation
};

/**
 * @struct steal_backoff_config
 * @brief Configuration for backoff behavior
 */
struct steal_backoff_config
{
	steal_backoff_strategy strategy = steal_backoff_strategy::exponential;
	std::chrono::microseconds initial_backoff{50};
	std::chrono::microseconds max_backoff{1000};
	double multiplier = 2.0;        ///< Multiplier for exponential backoff
	double jitter_factor = 0.5;     ///< Jitter range as fraction of delay (0.0 - 1.0)
};

/**
 * @class backoff_calculator
 * @brief Calculates backoff delays for work-stealing operations
 *
 * This class provides thread-safe backoff delay calculation using various
 * strategies. It is designed to be used by workers when steal attempts fail.
 *
 * ### Thread Safety
 * Each worker should have its own backoff_calculator instance to avoid
 * contention on the random number generator. All methods are thread-safe
 * when used with separate instances.
 *
 * ### Usage Example
 * @code
 * steal_backoff_config config;
 * config.strategy = steal_backoff_strategy::exponential;
 * config.initial_backoff = std::chrono::microseconds{50};
 * config.max_backoff = std::chrono::microseconds{1000};
 *
 * backoff_calculator calculator(config);
 *
 * // On steal failure
 * std::size_t attempt = 0;
 * while (!steal_successful && attempt < max_attempts) {
 *     auto delay = calculator.calculate(attempt);
 *     std::this_thread::sleep_for(delay);
 *     ++attempt;
 * }
 * @endcode
 */
class backoff_calculator
{
public:
	/**
	 * @brief Construct a backoff calculator with given configuration
	 * @param config Backoff configuration
	 */
	explicit backoff_calculator(steal_backoff_config config = {})
		: config_(config)
		, rng_(std::random_device{}())
	{
	}

	/**
	 * @brief Calculate backoff delay for a given attempt number
	 * @param attempt The attempt number (0-indexed)
	 * @return Calculated backoff delay
	 *
	 * The delay is calculated based on the configured strategy and capped
	 * at max_backoff.
	 */
	[[nodiscard]] auto calculate(std::size_t attempt) -> std::chrono::microseconds
	{
		auto delay = calculate_base_delay(attempt);

		if (config_.strategy == steal_backoff_strategy::adaptive_jitter)
		{
			delay = apply_jitter(delay);
		}

		return cap_delay(delay);
	}

	/**
	 * @brief Get the current configuration
	 * @return Reference to the backoff configuration
	 */
	[[nodiscard]] auto get_config() const -> const steal_backoff_config&
	{
		return config_;
	}

	/**
	 * @brief Update the configuration
	 * @param config New configuration to use
	 */
	void set_config(steal_backoff_config config)
	{
		config_ = config;
	}

private:
	/**
	 * @brief Calculate base delay without jitter or capping
	 */
	[[nodiscard]] auto calculate_base_delay(std::size_t attempt) const
		-> std::chrono::microseconds
	{
		auto initial = static_cast<double>(config_.initial_backoff.count());
		auto max_count = static_cast<double>(config_.max_backoff.count());

		switch (config_.strategy)
		{
		case steal_backoff_strategy::fixed:
			return config_.initial_backoff;

		case steal_backoff_strategy::linear:
			return std::chrono::microseconds{
				static_cast<std::int64_t>(initial * static_cast<double>(attempt + 1))};

		case steal_backoff_strategy::exponential:
		case steal_backoff_strategy::adaptive_jitter:
		{
			// Calculate multiplier^attempt with overflow protection
			double factor = 1.0;
			for (std::size_t i = 0; i < attempt; ++i)
			{
				factor *= config_.multiplier;
				// Early exit if we've exceeded max to avoid unnecessary computation
				if (factor * initial > max_count)
				{
					return config_.max_backoff;
				}
			}
			return std::chrono::microseconds{
				static_cast<std::int64_t>(initial * factor)};
		}

		default:
			return config_.initial_backoff;
		}
	}

	/**
	 * @brief Apply random jitter to a delay
	 */
	[[nodiscard]] auto apply_jitter(std::chrono::microseconds delay)
		-> std::chrono::microseconds
	{
		using rep = std::chrono::microseconds::rep;
		rep base = delay.count();
		rep jitter_range =
			static_cast<rep>(static_cast<double>(base) * config_.jitter_factor);

		if (jitter_range <= 0)
		{
			return delay;
		}

		std::uniform_int_distribution<rep> dist(-jitter_range, jitter_range);
		rep jittered = base + dist(rng_);

		// Ensure we don't go negative
		return std::chrono::microseconds{std::max(rep{1}, jittered)};
	}

	/**
	 * @brief Cap delay at maximum
	 */
	[[nodiscard]] auto cap_delay(std::chrono::microseconds delay) const
		-> std::chrono::microseconds
	{
		return std::min(delay, config_.max_backoff);
	}

	steal_backoff_config config_;
	std::mt19937_64 rng_;
};

/**
 * @brief Convert steal_backoff_strategy to string representation
 * @param strategy The strategy to convert
 * @return String name of the strategy
 */
constexpr const char* to_string(steal_backoff_strategy strategy)
{
	switch (strategy)
	{
	case steal_backoff_strategy::fixed:
		return "fixed";
	case steal_backoff_strategy::linear:
		return "linear";
	case steal_backoff_strategy::exponential:
		return "exponential";
	case steal_backoff_strategy::adaptive_jitter:
		return "adaptive_jitter";
	default:
		return "unknown";
	}
}

} // namespace kcenon::thread
