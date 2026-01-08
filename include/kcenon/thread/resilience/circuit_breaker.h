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

#include "circuit_breaker_config.h"
#include "failure_window.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>

namespace kcenon::thread
{
	/**
	 * @class circuit_breaker
	 * @brief Implements the Circuit Breaker pattern for failure detection and recovery.
	 *
	 * The circuit breaker monitors for failures and automatically opens to prevent
	 * cascading failures when a threshold is exceeded. After a timeout, it enters
	 * a half-open state to test if recovery is possible.
	 *
	 * ### State Machine
	 * @code
	 *                     failure_threshold
	 *                     or failure_rate > 50%
	 *         ┌───────────────────────────────────────────┐
	 *         │                                           │
	 *         ▼                                           │
	 *    ┌─────────┐         open_duration          ┌─────────┐
	 *    │  OPEN   │ ───────────────────────────▶   │HALF_OPEN│
	 *    └─────────┘         elapsed                └─────────┘
	 *         ▲                                           │
	 *         │                                           │
	 *         │    half_open failures                     │
	 *         │    >= threshold                           │
	 *         └───────────────────────────────────────────┤
	 *                                                     │
	 *                                                     │ half_open successes
	 *                                                     │ >= threshold
	 *                                                     ▼
	 *                                                ┌─────────┐
	 *                                                │ CLOSED  │
	 *                                                └─────────┘
	 * @endcode
	 *
	 * ### Thread Safety
	 * All public methods are thread-safe and can be called from any thread.
	 *
	 * ### Usage Example
	 * @code
	 * auto cb = std::make_shared<circuit_breaker>(circuit_breaker_config{
	 *     .failure_threshold = 5,
	 *     .open_duration = std::chrono::seconds{30}
	 * });
	 *
	 * // Use with guard for automatic recording
	 * {
	 *     auto guard = cb->make_guard();
	 *     if (!guard.is_allowed()) {
	 *         return error("Circuit open");
	 *     }
	 *
	 *     try {
	 *         do_operation();
	 *         guard.mark_success();
	 *     } catch (const std::exception& e) {
	 *         guard.mark_failure(&e);
	 *         throw;
	 *     }
	 * }
	 * @endcode
	 *
	 * @see circuit_breaker_config
	 * @see failure_window
	 */
	class circuit_breaker
	{
	public:
		/**
		 * @brief Statistics about the circuit breaker state.
		 */
		struct stats
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

		/**
		 * @brief RAII guard for automatic success/failure recording.
		 *
		 * The guard automatically tracks whether the operation succeeded or failed,
		 * ensuring the circuit breaker state is updated correctly even when
		 * exceptions occur.
		 */
		class guard
		{
		public:
			/**
			 * @brief Constructs a guard and checks if request is allowed.
			 * @param cb Reference to the circuit breaker.
			 */
			explicit guard(circuit_breaker& cb);

			/**
			 * @brief Destructor records failure if not explicitly marked.
			 */
			~guard();

			guard(const guard&) = delete;
			guard& operator=(const guard&) = delete;
			guard(guard&&) noexcept;
			guard& operator=(guard&&) noexcept;

			/**
			 * @brief Checks if the request was allowed.
			 * @return true if allowed, false if rejected (circuit open).
			 */
			[[nodiscard]] auto is_allowed() const -> bool;

			/**
			 * @brief Marks the operation as successful.
			 */
			auto mark_success() -> void;

			/**
			 * @brief Marks the operation as failed.
			 * @param e Optional exception for failure predicate evaluation.
			 */
			auto mark_failure(const std::exception* e = nullptr) -> void;

		private:
			circuit_breaker* cb_;
			bool allowed_;
			bool recorded_;
		};

		/**
		 * @brief Constructs a circuit breaker with the given configuration.
		 * @param config Configuration parameters (defaults used if not specified).
		 */
		explicit circuit_breaker(circuit_breaker_config config = {});

		/**
		 * @brief Destructor.
		 */
		~circuit_breaker();

		// ============================================
		// State Management
		// ============================================

		/**
		 * @brief Checks if a request is allowed to proceed.
		 * @return true if request can proceed, false if circuit is open.
		 *
		 * This method also handles state transitions:
		 * - In OPEN state: transitions to HALF_OPEN if timeout elapsed
		 * - In HALF_OPEN state: allows up to half_open_max_requests
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 */
		[[nodiscard]] auto allow_request() -> bool;

		/**
		 * @brief Records a successful operation.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 */
		auto record_success() -> void;

		/**
		 * @brief Records a failed operation.
		 * @param error Optional exception for failure predicate evaluation.
		 *
		 * If a failure_predicate is configured and returns false, the failure
		 * is not counted against the circuit breaker.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 */
		auto record_failure(const std::exception* error = nullptr) -> void;

		/**
		 * @brief Gets the current circuit state.
		 * @return Current circuit_state.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 */
		[[nodiscard]] auto get_state() const -> circuit_state;

		/**
		 * @brief Manually trips (opens) the circuit.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 */
		auto trip() -> void;

		/**
		 * @brief Manually resets (closes) the circuit.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 */
		auto reset() -> void;

		// ============================================
		// Statistics
		// ============================================

		/**
		 * @brief Gets current statistics.
		 * @return Statistics structure with current state and counters.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 */
		[[nodiscard]] auto get_stats() const -> stats;

		// ============================================
		// Guard Creation
		// ============================================

		/**
		 * @brief Creates an RAII guard for request handling.
		 * @return Guard object for automatic recording.
		 *
		 * The returned guard checks allow_request() on construction and
		 * will record failure on destruction if not explicitly marked.
		 */
		[[nodiscard]] auto make_guard() -> guard;

	private:
		/**
		 * @brief Transitions to a new state.
		 * @param new_state The state to transition to.
		 */
		auto transition_to(circuit_state new_state) -> void;

		/**
		 * @brief Checks if circuit should transition to OPEN.
		 * @return true if transition should occur.
		 */
		[[nodiscard]] auto should_transition_to_open() const -> bool;

		/**
		 * @brief Checks if circuit should transition from OPEN to HALF_OPEN.
		 * @return true if transition should occur.
		 */
		[[nodiscard]] auto should_transition_to_half_open() const -> bool;

		/**
		 * @brief Checks if circuit should transition from HALF_OPEN to CLOSED.
		 * @return true if transition should occur.
		 */
		[[nodiscard]] auto should_transition_to_closed() const -> bool;

		circuit_breaker_config config_;
		std::atomic<circuit_state> state_{circuit_state::closed};
		std::unique_ptr<failure_window> window_;

		std::atomic<std::size_t> consecutive_failures_{0};
		std::atomic<std::size_t> half_open_requests_{0};
		std::atomic<std::size_t> half_open_successes_{0};
		std::atomic<std::size_t> rejected_requests_{0};
		std::atomic<std::size_t> total_requests_{0};
		std::atomic<std::size_t> successful_requests_{0};
		std::atomic<std::size_t> failed_requests_{0};
		std::atomic<std::size_t> state_transitions_{0};

		std::chrono::steady_clock::time_point open_time_;
		std::chrono::steady_clock::time_point state_change_time_;

		mutable std::mutex state_mutex_;
	};

} // namespace kcenon::thread
