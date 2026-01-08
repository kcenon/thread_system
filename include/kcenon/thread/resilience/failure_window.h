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

#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace kcenon::thread
{
	/**
	 * @class failure_window
	 * @brief Sliding window failure tracker for circuit breaker.
	 *
	 * This class implements a time-bucketed sliding window to track success and
	 * failure counts over a configurable time period. It provides efficient
	 * failure rate calculation with automatic bucket rotation.
	 *
	 * ### Thread Safety
	 * All public methods are thread-safe and can be called concurrently.
	 *
	 * ### Implementation Details
	 * The window is divided into multiple time buckets (default 10). Each bucket
	 * covers window_size/bucket_count seconds. Old buckets are automatically
	 * expired when new requests arrive.
	 *
	 * @see circuit_breaker
	 */
	class failure_window
	{
	public:
		/**
		 * @brief Constructs a failure window with the specified window size.
		 * @param window_size Duration of the sliding window.
		 * @param bucket_count Number of time buckets (default 10).
		 */
		explicit failure_window(
			std::chrono::seconds window_size,
			std::size_t bucket_count = 10);

		/**
		 * @brief Records a successful operation.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 */
		auto record_success() -> void;

		/**
		 * @brief Records a failed operation.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 */
		auto record_failure() -> void;

		/**
		 * @brief Gets the total number of requests in the window.
		 * @return Total requests (successes + failures).
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 */
		[[nodiscard]] auto total_requests() const -> std::size_t;

		/**
		 * @brief Gets the number of failed requests in the window.
		 * @return Number of failures.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 */
		[[nodiscard]] auto failure_count() const -> std::size_t;

		/**
		 * @brief Gets the number of successful requests in the window.
		 * @return Number of successes.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 */
		[[nodiscard]] auto success_count() const -> std::size_t;

		/**
		 * @brief Calculates the failure rate in the window.
		 * @return Failure rate between 0.0 and 1.0, or 0.0 if no requests.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 */
		[[nodiscard]] auto failure_rate() const -> double;

		/**
		 * @brief Resets all counters in the window.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 */
		auto reset() -> void;

	private:
		struct bucket
		{
			std::atomic<std::size_t> successes{0};
			std::atomic<std::size_t> failures{0};
			std::atomic<std::int64_t> timestamp{0};  // epoch seconds when bucket started
		};

		/**
		 * @brief Gets the current bucket index and expires old buckets.
		 * @return Index of the current bucket.
		 */
		auto get_current_bucket_index() const -> std::size_t;

		/**
		 * @brief Expires buckets older than the window size.
		 */
		auto expire_old_buckets() const -> void;

		std::chrono::seconds window_size_;
		std::chrono::seconds bucket_duration_;
		std::size_t bucket_count_;
		mutable std::vector<bucket> buckets_;
		mutable std::mutex mutex_;
	};

} // namespace kcenon::thread
