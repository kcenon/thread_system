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

namespace kcenon::thread
{
	/**
	 * @class token_bucket
	 * @brief Lock-free token bucket rate limiter for controlling throughput.
	 *
	 * @ingroup backpressure
	 *
	 * The token bucket algorithm is a metering mechanism that controls the rate
	 * at which operations can proceed. Tokens are added to a bucket at a fixed
	 * rate, and operations consume tokens. If no tokens are available, the
	 * operation either waits or is rejected.
	 *
	 * ### Design Principles
	 * - **Lock-free**: Uses atomic operations for thread-safe token management
	 * - **Continuous Refill**: Tokens are calculated on-demand, not via timer
	 * - **Burst Support**: Allows bursts up to bucket capacity
	 * - **Configurable**: Rate and burst size can be adjusted at runtime
	 *
	 * ### Algorithm
	 * ```
	 * tokens = min(max_tokens, tokens + elapsed_time * refill_rate)
	 * if (tokens >= requested) {
	 *     tokens -= requested
	 *     return success
	 * }
	 * return failure
	 * ```
	 *
	 * ### Thread Safety
	 * All methods are thread-safe and lock-free. Multiple threads can
	 * concurrently acquire tokens without blocking each other.
	 *
	 * ### Usage Example
	 * @code
	 * // Create bucket: 1000 tokens/sec, burst of 100
	 * token_bucket bucket(1000, 100);
	 *
	 * // Try to acquire token (non-blocking)
	 * if (bucket.try_acquire()) {
	 *     process_request();
	 * }
	 *
	 * // Wait up to 100ms for token
	 * if (bucket.try_acquire_for(1, std::chrono::milliseconds{100})) {
	 *     process_request();
	 * }
	 * @endcode
	 *
	 * @see backpressure_config For integration with job queues
	 */
	class token_bucket
	{
	public:
		/**
		 * @brief Constructs a token bucket with the specified rate and burst size.
		 * @param tokens_per_second Number of tokens added per second.
		 * @param burst_size Maximum tokens that can accumulate (bucket capacity).
		 *
		 * The bucket starts full (burst_size tokens available).
		 */
		token_bucket(std::size_t tokens_per_second, std::size_t burst_size);

		/**
		 * @brief Default destructor.
		 */
		~token_bucket() = default;

		// Non-copyable, non-movable for thread safety
		token_bucket(const token_bucket&) = delete;
		token_bucket& operator=(const token_bucket&) = delete;
		token_bucket(token_bucket&&) = delete;
		token_bucket& operator=(token_bucket&&) = delete;

		/**
		 * @brief Attempts to acquire tokens without waiting.
		 * @param tokens Number of tokens to acquire (default: 1).
		 * @return true if tokens were acquired, false if insufficient tokens.
		 *
		 * This method is non-blocking and returns immediately. If the bucket
		 * doesn't have enough tokens, the operation fails without waiting.
		 *
		 * Thread Safety: Lock-free, safe for concurrent calls.
		 */
		[[nodiscard]] auto try_acquire(std::size_t tokens = 1) -> bool;

		/**
		 * @brief Attempts to acquire tokens with a timeout.
		 * @param tokens Number of tokens to acquire.
		 * @param timeout Maximum time to wait for tokens.
		 * @return true if tokens were acquired within timeout, false otherwise.
		 *
		 * This method will spin-wait (with backoff) until either:
		 * - Enough tokens become available (returns true)
		 * - The timeout expires (returns false)
		 *
		 * Implementation uses exponential backoff to reduce CPU usage while
		 * waiting for token refill.
		 *
		 * Thread Safety: Lock-free with cooperative spin-waiting.
		 */
		[[nodiscard]] auto try_acquire_for(
			std::size_t tokens,
			std::chrono::milliseconds timeout) -> bool;

		/**
		 * @brief Returns the current number of available tokens.
		 * @return Current token count (may be fractional, returned as integer).
		 *
		 * This is a snapshot that may become stale immediately in a
		 * multi-threaded environment.
		 *
		 * Thread Safety: Lock-free read.
		 */
		[[nodiscard]] auto available_tokens() const -> std::size_t;

		/**
		 * @brief Calculates time until the specified tokens become available.
		 * @param tokens Number of tokens needed.
		 * @return Duration until tokens will be available (0 if already available).
		 *
		 * Useful for implementing waiting strategies or displaying estimated
		 * wait times to users.
		 *
		 * Thread Safety: Lock-free calculation.
		 */
		[[nodiscard]] auto time_until_available(std::size_t tokens) const
			-> std::chrono::nanoseconds;

		/**
		 * @brief Updates the token refill rate.
		 * @param tokens_per_second New tokens per second rate.
		 *
		 * Takes effect immediately. Does not affect currently accumulated tokens.
		 *
		 * Thread Safety: Lock-free write.
		 */
		auto set_rate(std::size_t tokens_per_second) -> void;

		/**
		 * @brief Updates the maximum bucket capacity.
		 * @param burst_size New maximum tokens.
		 *
		 * If current tokens exceed new capacity, excess tokens are discarded.
		 *
		 * Thread Safety: Lock-free write.
		 */
		auto set_burst_size(std::size_t burst_size) -> void;

		/**
		 * @brief Returns the current refill rate.
		 * @return Tokens per second.
		 */
		[[nodiscard]] auto get_rate() const -> std::size_t;

		/**
		 * @brief Returns the maximum bucket capacity.
		 * @return Maximum tokens (burst size).
		 */
		[[nodiscard]] auto get_burst_size() const -> std::size_t;

		/**
		 * @brief Resets the bucket to full capacity.
		 *
		 * Restores tokens to burst_size and resets the last refill time.
		 *
		 * Thread Safety: Lock-free write.
		 */
		auto reset() -> void;

	private:
		/**
		 * @brief Refills tokens based on elapsed time since last refill.
		 *
		 * Uses CAS loop to atomically update both tokens and timestamp.
		 * Called internally before each token acquisition attempt.
		 */
		auto refill() -> void;

		/**
		 * @brief Current token count (scaled by 1000 for sub-token precision).
		 *
		 * We use fixed-point arithmetic to avoid floating-point atomics.
		 * Actual tokens = tokens_ / PRECISION_FACTOR
		 */
		std::atomic<std::int64_t> tokens_;

		/**
		 * @brief Maximum tokens (burst size) scaled by precision factor.
		 */
		std::atomic<std::int64_t> max_tokens_;

		/**
		 * @brief Token refill rate in nano-tokens per nanosecond.
		 *
		 * Calculated as: (tokens_per_second * PRECISION_FACTOR) / 1e9
		 */
		std::atomic<double> refill_rate_;

		/**
		 * @brief Timestamp of last token refill.
		 */
		std::atomic<std::chrono::steady_clock::time_point::rep> last_refill_;

		/**
		 * @brief Precision factor for fixed-point token calculations.
		 *
		 * Using 1000 allows for milli-token precision without floating point.
		 */
		static constexpr std::int64_t PRECISION_FACTOR = 1000;
	};

} // namespace kcenon::thread
