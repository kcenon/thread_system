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

#include <kcenon/thread/core/token_bucket.h>

#include <algorithm>
#include <thread>

/**
 * @file token_bucket.cpp
 * @brief Implementation of lock-free token bucket rate limiter.
 *
 * This file contains the implementation of the token_bucket class, which
 * provides a lock-free rate limiting mechanism using the token bucket
 * algorithm with continuous refill.
 */

namespace kcenon::thread
{
	/**
	 * @brief Constructs a token bucket with the specified rate and capacity.
	 *
	 * Implementation details:
	 * - Initializes tokens to maximum capacity (full bucket)
	 * - Calculates refill rate as tokens per nanosecond (for high precision)
	 * - Records initial timestamp for refill calculations
	 * - Uses fixed-point arithmetic (PRECISION_FACTOR = 1000) for sub-token precision
	 *
	 * @param tokens_per_second Number of tokens to add per second
	 * @param burst_size Maximum tokens that can accumulate
	 */
	token_bucket::token_bucket(std::size_t tokens_per_second, std::size_t burst_size)
		: tokens_(static_cast<std::int64_t>(burst_size) * PRECISION_FACTOR)
		, max_tokens_(static_cast<std::int64_t>(burst_size) * PRECISION_FACTOR)
		, refill_rate_(static_cast<double>(tokens_per_second) * PRECISION_FACTOR / 1e9)
		, last_refill_(std::chrono::steady_clock::now().time_since_epoch().count())
	{
	}

	/**
	 * @brief Refills tokens based on elapsed time since last refill.
	 *
	 * Implementation details:
	 * - Uses CAS loop to atomically update tokens and timestamp
	 * - Calculates tokens to add based on elapsed nanoseconds
	 * - Caps tokens at max_tokens_ to prevent overflow
	 * - Lock-free: multiple threads can refill concurrently
	 *
	 * Algorithm:
	 * 1. Read current timestamp
	 * 2. Calculate elapsed time since last_refill_
	 * 3. Calculate new tokens = current + (elapsed * rate)
	 * 4. Cap at max_tokens_
	 * 5. CAS update (retry if another thread modified)
	 */
	auto token_bucket::refill() -> void
	{
		auto now = std::chrono::steady_clock::now().time_since_epoch().count();
		auto last = last_refill_.load(std::memory_order_acquire);

		// Calculate elapsed nanoseconds
		auto elapsed_ns = now - last;
		if (elapsed_ns <= 0)
		{
			return;  // No time passed, nothing to refill
		}

		// Try to update last_refill_ atomically
		if (!last_refill_.compare_exchange_weak(
				last, now,
				std::memory_order_acq_rel,
				std::memory_order_relaxed))
		{
			// Another thread updated, our calculation is stale
			return;
		}

		// Calculate tokens to add
		double rate = refill_rate_.load(std::memory_order_relaxed);
		auto new_tokens = static_cast<std::int64_t>(elapsed_ns * rate);

		if (new_tokens <= 0)
		{
			return;
		}

		// Add tokens (capped at max)
		std::int64_t max = max_tokens_.load(std::memory_order_relaxed);
		std::int64_t current = tokens_.load(std::memory_order_relaxed);
		std::int64_t updated = std::min(current + new_tokens, max);

		// Relaxed CAS is fine here since we're just accumulating
		tokens_.compare_exchange_weak(
			current, updated,
			std::memory_order_relaxed,
			std::memory_order_relaxed);
	}

	/**
	 * @brief Attempts to acquire tokens without waiting.
	 *
	 * Implementation details:
	 * - First refills bucket based on elapsed time
	 * - Then attempts atomic decrement if sufficient tokens
	 * - Uses CAS loop to handle concurrent acquisitions
	 * - Returns immediately if insufficient tokens
	 *
	 * Thread Safety:
	 * - Lock-free via CAS loop
	 * - Multiple threads can acquire concurrently
	 * - Fair in the sense that all competing threads have equal chance
	 *
	 * @param tokens Number of tokens to acquire
	 * @return true if acquired, false if insufficient
	 */
	auto token_bucket::try_acquire(std::size_t tokens) -> bool
	{
		// First, refill based on elapsed time
		refill();

		// Scale requested tokens by precision factor
		std::int64_t needed = static_cast<std::int64_t>(tokens) * PRECISION_FACTOR;

		// CAS loop to atomically decrement tokens
		std::int64_t current = tokens_.load(std::memory_order_acquire);
		while (current >= needed)
		{
			if (tokens_.compare_exchange_weak(
					current, current - needed,
					std::memory_order_acq_rel,
					std::memory_order_acquire))
			{
				return true;  // Successfully acquired
			}
			// CAS failed, current has been updated, retry
		}

		return false;  // Insufficient tokens
	}

	/**
	 * @brief Attempts to acquire tokens with timeout and backoff.
	 *
	 * Implementation details:
	 * - Uses exponential backoff to reduce CPU usage while waiting
	 * - Backoff starts at 1μs, doubles each iteration, caps at 1ms
	 * - Checks deadline after each failed attempt
	 * - Returns as soon as tokens are acquired
	 *
	 * Backoff Strategy:
	 * - Initial: 1μs sleep
	 * - Max: 1ms sleep
	 * - Multiplier: 2x per iteration
	 * - Purpose: Balance responsiveness vs CPU usage
	 *
	 * @param tokens Number of tokens to acquire
	 * @param timeout Maximum time to wait
	 * @return true if acquired within timeout, false otherwise
	 */
	auto token_bucket::try_acquire_for(
		std::size_t tokens,
		std::chrono::milliseconds timeout) -> bool
	{
		auto deadline = std::chrono::steady_clock::now() + timeout;

		// Start with small backoff, increase exponentially
		auto backoff = std::chrono::microseconds{1};
		constexpr auto max_backoff = std::chrono::milliseconds{1};

		while (std::chrono::steady_clock::now() < deadline)
		{
			if (try_acquire(tokens))
			{
				return true;
			}

			// Sleep with exponential backoff
			std::this_thread::sleep_for(backoff);

			// Double backoff, cap at max
			backoff = std::min(
				backoff * 2,
				std::chrono::duration_cast<std::chrono::microseconds>(max_backoff));
		}

		// Final attempt after loop
		return try_acquire(tokens);
	}

	/**
	 * @brief Returns current available tokens.
	 *
	 * Implementation details:
	 * - First refills to get accurate count
	 * - Converts from fixed-point to integer
	 * - Returns 0 if tokens are negative (shouldn't happen)
	 *
	 * @return Available token count
	 */
	auto token_bucket::available_tokens() const -> std::size_t
	{
		// Need non-const refill, but we're returning a snapshot anyway
		const_cast<token_bucket*>(this)->refill();

		std::int64_t current = tokens_.load(std::memory_order_acquire);
		if (current <= 0)
		{
			return 0;
		}
		return static_cast<std::size_t>(current / PRECISION_FACTOR);
	}

	/**
	 * @brief Calculates time until tokens become available.
	 *
	 * Implementation details:
	 * - If sufficient tokens exist, returns 0
	 * - Otherwise, calculates time based on deficit and refill rate
	 * - Returns duration in nanoseconds for maximum precision
	 *
	 * @param tokens Number of tokens needed
	 * @return Duration until tokens available (0 if already available)
	 */
	auto token_bucket::time_until_available(std::size_t tokens) const
		-> std::chrono::nanoseconds
	{
		const_cast<token_bucket*>(this)->refill();

		std::int64_t needed = static_cast<std::int64_t>(tokens) * PRECISION_FACTOR;
		std::int64_t current = tokens_.load(std::memory_order_acquire);

		if (current >= needed)
		{
			return std::chrono::nanoseconds{0};
		}

		// Calculate deficit
		std::int64_t deficit = needed - current;

		// Time = deficit / rate (rate is in tokens per nanosecond)
		double rate = refill_rate_.load(std::memory_order_relaxed);
		if (rate <= 0)
		{
			// Infinite wait if rate is zero
			return std::chrono::nanoseconds::max();
		}

		auto wait_ns = static_cast<std::int64_t>(deficit / rate);
		return std::chrono::nanoseconds{wait_ns};
	}

	/**
	 * @brief Updates the token refill rate.
	 *
	 * Implementation details:
	 * - First refills with old rate to preserve accumulated tokens
	 * - Then updates rate for future refills
	 * - Converts tokens/second to tokens/nanosecond
	 *
	 * @param tokens_per_second New refill rate
	 */
	auto token_bucket::set_rate(std::size_t tokens_per_second) -> void
	{
		// Refill with current rate before changing
		refill();

		// Update rate (tokens per nanosecond, scaled by precision)
		double new_rate = static_cast<double>(tokens_per_second) * PRECISION_FACTOR / 1e9;
		refill_rate_.store(new_rate, std::memory_order_release);
	}

	/**
	 * @brief Updates the maximum bucket capacity.
	 *
	 * Implementation details:
	 * - Updates max_tokens_ atomically
	 * - If current tokens exceed new max, caps them
	 *
	 * @param burst_size New maximum capacity
	 */
	auto token_bucket::set_burst_size(std::size_t burst_size) -> void
	{
		std::int64_t new_max = static_cast<std::int64_t>(burst_size) * PRECISION_FACTOR;
		max_tokens_.store(new_max, std::memory_order_release);

		// Cap current tokens if they exceed new max
		std::int64_t current = tokens_.load(std::memory_order_acquire);
		while (current > new_max)
		{
			if (tokens_.compare_exchange_weak(
					current, new_max,
					std::memory_order_acq_rel,
					std::memory_order_acquire))
			{
				break;
			}
		}
	}

	/**
	 * @brief Returns the current refill rate.
	 * @return Tokens per second
	 */
	auto token_bucket::get_rate() const -> std::size_t
	{
		double rate = refill_rate_.load(std::memory_order_acquire);
		// Convert back: rate * 1e9 / PRECISION_FACTOR
		return static_cast<std::size_t>(rate * 1e9 / PRECISION_FACTOR);
	}

	/**
	 * @brief Returns the maximum bucket capacity.
	 * @return Burst size (maximum tokens)
	 */
	auto token_bucket::get_burst_size() const -> std::size_t
	{
		std::int64_t max = max_tokens_.load(std::memory_order_acquire);
		return static_cast<std::size_t>(max / PRECISION_FACTOR);
	}

	/**
	 * @brief Resets the bucket to full capacity.
	 *
	 * Implementation details:
	 * - Sets tokens to max_tokens_
	 * - Updates last_refill_ to current time
	 * - Useful for manual intervention or testing
	 */
	auto token_bucket::reset() -> void
	{
		std::int64_t max = max_tokens_.load(std::memory_order_acquire);
		tokens_.store(max, std::memory_order_release);
		last_refill_.store(
			std::chrono::steady_clock::now().time_since_epoch().count(),
			std::memory_order_release);
	}

} // namespace kcenon::thread
