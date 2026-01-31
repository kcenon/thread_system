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

// Redirect to common_system implementation (Issue #524)
// The circuit_breaker_config and circuit_state have been consolidated into common_system.
#include <kcenon/common/resilience/circuit_breaker_config.h>
#include <kcenon/common/resilience/circuit_state.h>

#include <chrono>
#include <cstddef>

namespace kcenon::thread
{
	// Deprecated aliases for backward compatibility
	// These will be removed in a future version. Please migrate to kcenon::common::resilience types

	/**
	 * @brief Deprecated alias for kcenon::common::resilience::circuit_state
	 * @deprecated Use kcenon::common::resilience::circuit_state instead
	 */
	using circuit_state [[deprecated("Use kcenon::common::resilience::circuit_state instead")]] =
		common::resilience::circuit_state;

	/**
	 * @brief Deprecated alias for kcenon::common::resilience::circuit_breaker_config
	 * @deprecated Use kcenon::common::resilience::circuit_breaker_config instead
	 */
	using circuit_breaker_config [[deprecated("Use kcenon::common::resilience::circuit_breaker_config instead")]] =
		common::resilience::circuit_breaker_config;

	/**
	 * @brief Deprecated string conversion function for circuit_state
	 * @deprecated Use kcenon::common::resilience::to_string instead
	 * @note Returns pointer to static string for backward compatibility
	 */
	[[deprecated("Use kcenon::common::resilience::to_string instead")]]
	inline const char* to_string(circuit_state state)
	{
		// Convert common_system's uppercase enum to thread_system's lowercase strings
		switch (state)
		{
			case circuit_state::CLOSED:    return "closed";
			case circuit_state::OPEN:      return "open";
			case circuit_state::HALF_OPEN: return "half_open";
			default:                       return "unknown";
		}
	}

	/**
	 * @brief Deprecated statistics structure for backward compatibility
	 * @deprecated Use circuit_breaker::get_stats() map interface instead
	 * @note This structure is maintained for backward compatibility only
	 */
	struct [[deprecated("Use circuit_breaker::get_stats() map interface instead")]] circuit_breaker_stats
	{
		circuit_state current_state;
		std::chrono::steady_clock::time_point state_since;

		std::size_t total_requests;
		std::size_t successful_requests;
		std::size_t failed_requests;
		std::size_t rejected_requests;

		double failure_rate;
		std::size_t consecutive_failures;
		std::size_t state_transitions;
	};

} // namespace kcenon::thread
