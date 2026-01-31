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
 *     .enable_circuit_breaker()
 *     .with_failure_threshold(5)
 *     .build();
 * @endcode
 */

#include <chrono>
#include <cstddef>
#include <functional>
#include <exception>

namespace kcenon::thread
{
	/**
	 * @enum circuit_state
	 * @brief Represents the current state of a circuit breaker.
	 *
	 * The circuit breaker follows a state machine with three states:
	 * - closed: Normal operation, all requests allowed
	 * - open: Failure threshold exceeded, requests blocked
	 * - half_open: Testing recovery, limited requests allowed
	 */
	enum class circuit_state
	{
		closed,     ///< Normal operation, requests allowed
		open,       ///< Failing, requests blocked
		half_open   ///< Testing recovery, limited requests
	};

	/**
	 * @brief Converts circuit_state to string representation.
	 * @param state The circuit state to convert.
	 * @return String representation of the state.
	 */
	inline const char* to_string(circuit_state state)
	{
		switch (state)
		{
			case circuit_state::closed:    return "closed";
			case circuit_state::open:      return "open";
			case circuit_state::half_open: return "half_open";
			default:                       return "unknown";
		}
	}

	/**
	 * @struct circuit_breaker_config
	 * @brief Configuration for the circuit breaker.
	 *
	 * This structure contains all configurable parameters for the circuit breaker
	 * behavior, including failure thresholds, recovery settings, and callbacks.
	 */
	struct circuit_breaker_config
	{
		// Failure thresholds
		std::size_t failure_threshold = 5;          ///< Consecutive failures to open circuit
		double failure_rate_threshold = 0.5;        ///< Failure rate (0.0-1.0) to open circuit
		std::size_t minimum_requests = 10;          ///< Minimum requests before rate check

		// Recovery settings
		std::chrono::seconds open_duration{30};     ///< Time in open state before half-open
		std::size_t half_open_max_requests = 3;     ///< Max requests allowed in half-open
		std::size_t half_open_success_threshold = 2; ///< Successes needed to close circuit

		// Sliding window
		std::chrono::seconds window_size{60};       ///< Sliding window for failure rate calculation

		// Callbacks
		std::function<void(circuit_state, circuit_state)> state_change_callback;
		std::function<bool(const std::exception&)> failure_predicate;  ///< What counts as failure
	};

} // namespace kcenon::thread
