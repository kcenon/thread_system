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

#include <kcenon/thread/core/backpressure_job_queue.h>
#include <kcenon/thread/utils/formatter.h>

#include <algorithm>
#include <chrono>

/**
 * @file backpressure_job_queue.cpp
 * @brief Implementation of backpressure-aware job queue.
 *
 * This file contains the implementation of the backpressure_job_queue class,
 * which extends job_queue with comprehensive backpressure mechanisms including
 * multiple policies, rate limiting, and adaptive control.
 */

namespace kcenon::thread
{
	/**
	 * @brief Constructs a backpressure-aware job queue.
	 *
	 * Implementation details:
	 * - Calls base job_queue constructor with max_size
	 * - Stores configuration
	 * - Creates token bucket if rate limiting is enabled
	 * - Initializes pressure state to none
	 *
	 * @param max_size Maximum queue capacity
	 * @param config Backpressure configuration
	 */
	backpressure_job_queue::backpressure_job_queue(
		std::size_t max_size,
		backpressure_config config)
		: job_queue(max_size)
		, config_(std::move(config))
		, current_pressure_(pressure_level::none)
		, current_pressure_ratio_(0.0)
	{
		// Initialize rate limiter if enabled
		if (config_.enable_rate_limiting)
		{
			rate_limiter_ = std::make_unique<token_bucket>(
				config_.rate_limit_tokens_per_second,
				config_.rate_limit_burst_size);
		}
	}

	/**
	 * @brief Destructor.
	 */
	backpressure_job_queue::~backpressure_job_queue() = default;

	/**
	 * @brief Enqueues a job with backpressure handling.
	 *
	 * Implementation details:
	 * - Early validation (stopped, null check)
	 * - Apply rate limiting if enabled
	 * - Route to policy-specific handler based on config
	 * - Update pressure state and statistics
	 *
	 * @param value The job to enqueue
	 * @return VoidResult indicating success or error
	 */
	auto backpressure_job_queue::enqueue(std::unique_ptr<job>&& value) -> common::VoidResult
	{
		// Early validation
		if (is_stopped())
		{
			return make_error_result(error_code::queue_stopped);
		}

		if (value == nullptr)
		{
			return make_error_result(error_code::invalid_argument, "cannot enqueue null job");
		}

		// Apply backpressure logic
		return apply_backpressure(std::move(value));
	}

	/**
	 * @brief Enqueues a batch of jobs with backpressure handling.
	 *
	 * Implementation details:
	 * - Validates batch (stopped, empty, null jobs)
	 * - Checks if batch fits within capacity constraints
	 * - For drop_oldest: drops enough jobs to fit batch
	 * - For other policies: rejects entire batch if won't fit
	 *
	 * @param jobs Vector of jobs to enqueue
	 * @return VoidResult indicating success or error
	 */
	auto backpressure_job_queue::enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs)
		-> common::VoidResult
	{
		if (is_stopped())
		{
			return make_error_result(error_code::queue_stopped);
		}

		if (jobs.empty())
		{
			return make_error_result(error_code::invalid_argument, "cannot enqueue empty batch");
		}

		// Validate all jobs
		for (const auto& job_ptr : jobs)
		{
			if (job_ptr == nullptr)
			{
				return make_error_result(error_code::invalid_argument,
					"cannot enqueue null job in batch");
			}
		}

		// Apply rate limiting for batch
		if (config_.enable_rate_limiting && rate_limiter_)
		{
			if (!rate_limiter_->try_acquire(jobs.size()))
			{
				stats_.rate_limit_waits.fetch_add(1, std::memory_order_relaxed);

				// Try waiting for tokens
				auto start = std::chrono::steady_clock::now();
				if (!rate_limiter_->try_acquire_for(jobs.size(), config_.block_timeout))
				{
					stats_.jobs_rejected.fetch_add(jobs.size(), std::memory_order_relaxed);
					return make_error_result(error_code::operation_timeout,
						"rate limit timeout for batch");
				}
				auto elapsed = std::chrono::steady_clock::now() - start;
				stats_.total_block_time_ns.fetch_add(
					std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count(),
					std::memory_order_relaxed);
			}
		}

		// Check capacity
		auto max_sz = get_max_size();
		if (!max_sz.has_value())
		{
			// Unbounded queue, just enqueue
			auto result = job_queue::enqueue_batch(std::move(jobs));
			if (result.is_ok())
			{
				stats_.jobs_accepted.fetch_add(jobs.size(), std::memory_order_relaxed);
			}
			update_pressure_state();
			return result;
		}

		std::size_t current_size = size();
		std::size_t available_space = max_sz.value() > current_size
			? max_sz.value() - current_size
			: 0;

		if (jobs.size() <= available_space)
		{
			// Batch fits
			auto result = job_queue::enqueue_batch(std::move(jobs));
			if (result.is_ok())
			{
				stats_.jobs_accepted.fetch_add(jobs.size(), std::memory_order_relaxed);
			}
			update_pressure_state();
			return result;
		}

		// Batch doesn't fit - handle based on policy
		switch (config_.policy)
		{
			case backpressure_policy::drop_oldest:
			{
				// Drop enough jobs to make room
				std::size_t to_drop = jobs.size() - available_space;
				{
					std::scoped_lock<std::mutex> lock(mutex_);
					for (std::size_t i = 0; i < to_drop && !empty(); ++i)
					{
						auto batch = dequeue_batch_limited(1);
						stats_.jobs_dropped.fetch_add(batch.size(), std::memory_order_relaxed);
					}
				}
				auto result = job_queue::enqueue_batch(std::move(jobs));
				if (result.is_ok())
				{
					stats_.jobs_accepted.fetch_add(jobs.size(), std::memory_order_relaxed);
				}
				update_pressure_state();
				return result;
			}

			case backpressure_policy::block:
			{
				// Wait for space (simplified: just check once with timeout)
				auto start = std::chrono::steady_clock::now();
				std::unique_lock<std::mutex> lock(mutex_);
				bool got_space = space_available_.wait_for(
					lock,
					config_.block_timeout,
					[this, needed = jobs.size(), max = max_sz.value()]() {
						return size() + needed <= max || is_stopped();
					});

				if (!got_space || is_stopped())
				{
					stats_.jobs_rejected.fetch_add(jobs.size(), std::memory_order_relaxed);
					auto elapsed = std::chrono::steady_clock::now() - start;
					stats_.total_block_time_ns.fetch_add(
						std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count(),
						std::memory_order_relaxed);
					return make_error_result(error_code::operation_timeout,
						"timeout waiting for space for batch");
				}

				lock.unlock();
				auto result = job_queue::enqueue_batch(std::move(jobs));
				if (result.is_ok())
				{
					stats_.jobs_accepted.fetch_add(jobs.size(), std::memory_order_relaxed);
				}
				update_pressure_state();
				return result;
			}

			default:
				// drop_newest, callback, adaptive - reject batch
				stats_.jobs_rejected.fetch_add(jobs.size(), std::memory_order_relaxed);
				stats_.pressure_events.fetch_add(1, std::memory_order_relaxed);
				update_pressure_state();
				return make_error_result(error_code::queue_full,
					"queue cannot fit entire batch");
		}
	}

	/**
	 * @brief Core backpressure implementation for single job.
	 *
	 * Implementation details:
	 * - Applies rate limiting first
	 * - Checks capacity before attempting enqueue
	 * - Routes to policy handler if queue is full
	 * - Updates statistics
	 *
	 * @param value The job to enqueue
	 * @return VoidResult indicating success or error
	 */
	auto backpressure_job_queue::apply_backpressure(std::unique_ptr<job>&& value)
		-> common::VoidResult
	{
		// Apply rate limiting
		if (!apply_rate_limiting())
		{
			stats_.jobs_rejected.fetch_add(1, std::memory_order_relaxed);
			return make_error_result(error_code::operation_timeout,
				"rate limit exceeded");
		}

		// Check if queue is full before deciding on policy
		bool queue_is_full = is_full();

		// If not full, try direct enqueue
		if (!queue_is_full)
		{
			auto result = direct_enqueue(std::move(value));
			if (result.is_ok())
			{
				stats_.jobs_accepted.fetch_add(1, std::memory_order_relaxed);
				update_pressure_state();
				return result;
			}
			// If direct_enqueue failed but queue wasn't reported as full,
			// it might be a race condition - treat as full
			queue_is_full = true;
		}

		// Queue is full, apply policy
		switch (config_.policy)
		{
			case backpressure_policy::block:
				return handle_block_policy(std::move(value));

			case backpressure_policy::drop_oldest:
				return handle_drop_oldest_policy(std::move(value));

			case backpressure_policy::drop_newest:
				stats_.jobs_rejected.fetch_add(1, std::memory_order_relaxed);
				stats_.pressure_events.fetch_add(1, std::memory_order_relaxed);
				update_pressure_state();
				return make_error_result(error_code::queue_full,
					"queue full: drop_newest policy rejected job");

			case backpressure_policy::callback:
				return handle_callback_policy(std::move(value));

			case backpressure_policy::adaptive:
				return handle_adaptive_policy(std::move(value));

			default:
				stats_.jobs_rejected.fetch_add(1, std::memory_order_relaxed);
				return make_error_result(error_code::queue_full,
					"queue full: unknown policy");
		}
	}

	/**
	 * @brief Applies rate limiting using token bucket.
	 *
	 * @return true if allowed to proceed, false if rate limited
	 */
	auto backpressure_job_queue::apply_rate_limiting() -> bool
	{
		if (!config_.enable_rate_limiting || !rate_limiter_)
		{
			return true;  // No rate limiting
		}

		// Try immediate acquire
		if (rate_limiter_->try_acquire())
		{
			return true;
		}

		// Rate limited - record and try waiting
		stats_.rate_limit_waits.fetch_add(1, std::memory_order_relaxed);

		auto start = std::chrono::steady_clock::now();
		bool acquired = rate_limiter_->try_acquire_for(1, config_.block_timeout);
		auto elapsed = std::chrono::steady_clock::now() - start;

		stats_.total_block_time_ns.fetch_add(
			std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count(),
			std::memory_order_relaxed);

		return acquired;
	}

	/**
	 * @brief Updates pressure state and triggers callbacks.
	 */
	auto backpressure_job_queue::update_pressure_state() -> void
	{
		auto max_sz = get_max_size();
		if (!max_sz.has_value() || max_sz.value() == 0)
		{
			current_pressure_.store(pressure_level::none, std::memory_order_relaxed);
			current_pressure_ratio_.store(0.0, std::memory_order_relaxed);
			return;
		}

		std::size_t current = size();
		double ratio = static_cast<double>(current) / static_cast<double>(max_sz.value());
		current_pressure_ratio_.store(ratio, std::memory_order_relaxed);

		pressure_level new_level;
		if (current >= max_sz.value())
		{
			new_level = pressure_level::critical;
		}
		else if (ratio >= config_.high_watermark)
		{
			new_level = pressure_level::high;
		}
		else if (ratio >= config_.low_watermark)
		{
			new_level = pressure_level::low;
		}
		else
		{
			new_level = pressure_level::none;
		}

		pressure_level old_level = current_pressure_.exchange(
			new_level, std::memory_order_acq_rel);

		// Trigger callback on level change or when entering high/critical
		if (config_.pressure_callback)
		{
			if (new_level != old_level ||
				new_level == pressure_level::high ||
				new_level == pressure_level::critical)
			{
				if (new_level >= pressure_level::high)
				{
					stats_.pressure_events.fetch_add(1, std::memory_order_relaxed);
				}
				config_.pressure_callback(current, ratio);
			}
		}

		// Notify waiting threads if space became available
		if (new_level < old_level)
		{
			space_available_.notify_all();
		}
	}

	/**
	 * @brief Handles blocking policy with timeout.
	 */
	auto backpressure_job_queue::handle_block_policy(std::unique_ptr<job>&& value)
		-> common::VoidResult
	{
		auto start = std::chrono::steady_clock::now();
		auto deadline = start + config_.block_timeout;

		while (std::chrono::steady_clock::now() < deadline)
		{
			// Try to enqueue
			auto result = direct_enqueue(std::move(value));
			if (result.is_ok())
			{
				stats_.jobs_accepted.fetch_add(1, std::memory_order_relaxed);
				update_pressure_state();
				return result;
			}

			// Wait for space
			std::unique_lock<std::mutex> lock(mutex_);
			auto remaining = deadline - std::chrono::steady_clock::now();
			if (remaining <= std::chrono::milliseconds{0})
			{
				break;
			}

			space_available_.wait_for(lock, remaining, [this]() {
				return !is_full() || is_stopped();
			});

			if (is_stopped())
			{
				break;
			}

			// Job was moved, need to return error
			// This is a design issue - we can't retry after move
			break;
		}

		auto elapsed = std::chrono::steady_clock::now() - start;
		stats_.total_block_time_ns.fetch_add(
			std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count(),
			std::memory_order_relaxed);
		stats_.jobs_rejected.fetch_add(1, std::memory_order_relaxed);

		return make_error_result(error_code::operation_timeout,
			"timeout waiting for queue space");
	}

	/**
	 * @brief Handles drop_oldest policy.
	 */
	auto backpressure_job_queue::handle_drop_oldest_policy(std::unique_ptr<job>&& value)
		-> common::VoidResult
	{
		// Drop oldest job - dequeue_batch_limited handles its own locking
		auto dropped = dequeue_batch_limited(1);
		stats_.jobs_dropped.fetch_add(dropped.size(), std::memory_order_relaxed);

		// Now enqueue new job
		auto result = direct_enqueue(std::move(value));
		if (result.is_ok())
		{
			stats_.jobs_accepted.fetch_add(1, std::memory_order_relaxed);
		}
		update_pressure_state();
		return result;
	}

	/**
	 * @brief Handles callback policy.
	 */
	auto backpressure_job_queue::handle_callback_policy(std::unique_ptr<job>&& value)
		-> common::VoidResult
	{
		if (!config_.decision_callback)
		{
			// No callback, fall back to reject
			stats_.jobs_rejected.fetch_add(1, std::memory_order_relaxed);
			return make_error_result(error_code::queue_full,
				"queue full and no decision callback");
		}

		backpressure_decision decision = config_.decision_callback(value);

		switch (decision)
		{
			case backpressure_decision::accept:
				// Force accept (may exceed max_size temporarily)
				return job_queue::enqueue(std::move(value));

			case backpressure_decision::reject:
				stats_.jobs_rejected.fetch_add(1, std::memory_order_relaxed);
				return make_error_result(error_code::queue_full,
					"callback rejected job");

			case backpressure_decision::drop_and_accept:
				return handle_drop_oldest_policy(std::move(value));

			case backpressure_decision::delay:
				// For now, treat as reject with retry hint
				stats_.jobs_rejected.fetch_add(1, std::memory_order_relaxed);
				return make_error_result(error_code::queue_busy,
					"callback requested delay, retry later");

			default:
				stats_.jobs_rejected.fetch_add(1, std::memory_order_relaxed);
				return make_error_result(error_code::queue_full,
					"unknown callback decision");
		}
	}

	/**
	 * @brief Handles adaptive policy.
	 *
	 * Implementation details:
	 * - Below high_watermark: accept normally
	 * - Above high_watermark: probabilistic acceptance based on pressure
	 * - At capacity: block briefly then reject
	 */
	auto backpressure_job_queue::handle_adaptive_policy(std::unique_ptr<job>&& value)
		-> common::VoidResult
	{
		double ratio = get_pressure_ratio();

		// Below high watermark: accept
		if (ratio < config_.high_watermark)
		{
			auto result = direct_enqueue(std::move(value));
			if (result.is_ok())
			{
				stats_.jobs_accepted.fetch_add(1, std::memory_order_relaxed);
			}
			update_pressure_state();
			return result;
		}

		// Above high watermark but not full: probabilistic acceptance
		// Acceptance probability decreases as we approach capacity
		if (ratio < 1.0)
		{
			// Simple linear decrease from 1.0 at high_watermark to 0 at 1.0
			double accept_prob = (1.0 - ratio) / (1.0 - config_.high_watermark);

			// Use simple threshold (not truly random, but deterministic)
			// This avoids random number generation overhead
			static thread_local std::size_t counter = 0;
			bool should_accept = (counter++ % 100) < (accept_prob * 100);

			if (should_accept)
			{
				auto result = direct_enqueue(std::move(value));
				if (result.is_ok())
				{
					stats_.jobs_accepted.fetch_add(1, std::memory_order_relaxed);
				}
				update_pressure_state();
				return result;
			}
		}

		// At or above capacity: brief wait then reject
		{
			std::unique_lock<std::mutex> lock(mutex_);
			auto brief_wait = std::chrono::milliseconds{10};
			bool got_space = space_available_.wait_for(
				lock,
				brief_wait,
				[this]() { return !is_full() || is_stopped(); });

			if (got_space && !is_stopped())
			{
				lock.unlock();
				auto result = direct_enqueue(std::move(value));
				if (result.is_ok())
				{
					stats_.jobs_accepted.fetch_add(1, std::memory_order_relaxed);
				}
				update_pressure_state();
				return result;
			}
		}

		stats_.jobs_rejected.fetch_add(1, std::memory_order_relaxed);
		stats_.pressure_events.fetch_add(1, std::memory_order_relaxed);
		update_pressure_state();
		return make_error_result(error_code::queue_full,
			"adaptive policy rejected job due to high pressure");
	}

	/**
	 * @brief Directly enqueues to base class without backpressure.
	 */
	auto backpressure_job_queue::direct_enqueue(std::unique_ptr<job>&& value)
		-> common::VoidResult
	{
		return job_queue::enqueue(std::move(value));
	}

	/**
	 * @brief Returns current pressure level.
	 */
	auto backpressure_job_queue::get_pressure_level() const -> pressure_level
	{
		return current_pressure_.load(std::memory_order_acquire);
	}

	/**
	 * @brief Returns current pressure ratio.
	 */
	auto backpressure_job_queue::get_pressure_ratio() const -> double
	{
		// Recalculate for accuracy
		auto max_sz = get_max_size();
		if (!max_sz.has_value() || max_sz.value() == 0)
		{
			return 0.0;
		}
		return static_cast<double>(size()) / static_cast<double>(max_sz.value());
	}

	/**
	 * @brief Sets backpressure configuration.
	 */
	auto backpressure_job_queue::set_backpressure_config(backpressure_config config) -> void
	{
		std::scoped_lock<std::mutex> lock(config_mutex_);

		bool rate_limiting_changed =
			config.enable_rate_limiting != config_.enable_rate_limiting ||
			config.rate_limit_tokens_per_second != config_.rate_limit_tokens_per_second ||
			config.rate_limit_burst_size != config_.rate_limit_burst_size;

		config_ = std::move(config);

		// Update rate limiter if needed
		if (rate_limiting_changed)
		{
			if (config_.enable_rate_limiting)
			{
				rate_limiter_ = std::make_unique<token_bucket>(
					config_.rate_limit_tokens_per_second,
					config_.rate_limit_burst_size);
			}
			else
			{
				rate_limiter_.reset();
			}
		}

		// Update pressure state with new config
		update_pressure_state();
	}

	/**
	 * @brief Returns current configuration.
	 */
	auto backpressure_job_queue::get_backpressure_config() const -> const backpressure_config&
	{
		return config_;
	}

	/**
	 * @brief Checks if rate limiting is active.
	 */
	auto backpressure_job_queue::is_rate_limited() const -> bool
	{
		if (!config_.enable_rate_limiting || !rate_limiter_)
		{
			return false;
		}
		// Rate limited if tokens are low (less than burst size * 10%)
		return rate_limiter_->available_tokens() <
			(config_.rate_limit_burst_size / 10);
	}

	/**
	 * @brief Returns available rate limit tokens.
	 */
	auto backpressure_job_queue::get_available_tokens() const -> std::size_t
	{
		if (!config_.enable_rate_limiting || !rate_limiter_)
		{
			return std::numeric_limits<std::size_t>::max();
		}
		return rate_limiter_->available_tokens();
	}

	/**
	 * @brief Returns backpressure statistics snapshot.
	 */
	auto backpressure_job_queue::get_backpressure_stats() const -> backpressure_stats_snapshot
	{
		return stats_.snapshot();
	}

	/**
	 * @brief Resets statistics.
	 */
	auto backpressure_job_queue::reset_stats() -> void
	{
		stats_.reset();
	}

	/**
	 * @brief Returns string representation.
	 */
	auto backpressure_job_queue::to_string() const -> std::string
	{
		return utility_module::formatter::format(
			"backpressure_job_queue[size={}, pressure={}, policy={}, ratio={:.1f}%]",
			size(),
			pressure_level_to_string(get_pressure_level()),
			backpressure_policy_to_string(config_.policy),
			get_pressure_ratio() * 100.0);
	}

} // namespace kcenon::thread
