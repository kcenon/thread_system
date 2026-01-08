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

#include "job_queue.h"
#include "backpressure_config.h"
#include "token_bucket.h"

#include <memory>
#include <mutex>

namespace kcenon::thread
{
	/**
	 * @class backpressure_job_queue
	 * @brief A job queue with comprehensive backpressure mechanisms.
	 *
	 * @ingroup backpressure
	 *
	 * Extends @c job_queue with backpressure handling to prevent resource
	 * exhaustion and provide graceful degradation under high load.
	 *
	 * ### Features
	 * - **Multiple Policies**: Block, drop_oldest, drop_newest, callback, adaptive
	 * - **Watermark-Based Pressure**: Graduated response based on queue depth
	 * - **Rate Limiting**: Token bucket algorithm for sustained throughput control
	 * - **Adaptive Control**: Auto-adjusts based on latency targets
	 * - **Statistics**: Comprehensive metrics for monitoring
	 *
	 * ### Pressure Response
	 * ```
	 * Queue Depth vs Response:
	 *
	 * 0%     50%      80%      100%
	 * |------|--------|--------|
	 *   OK    Warning   High   Critical
	 *        (callback) (slow) (reject)
	 * ```
	 *
	 * ### Thread Safety
	 * All methods are thread-safe. The queue inherits mutex-based
	 * synchronization from @c job_queue.
	 *
	 * ### Usage Example
	 * @code
	 * // Create backpressure queue with 1000 max size
	 * backpressure_config config;
	 * config.policy = backpressure_policy::adaptive;
	 * config.high_watermark = 0.8;
	 * config.enable_rate_limiting = true;
	 * config.rate_limit_tokens_per_second = 5000;
	 *
	 * auto queue = std::make_shared<backpressure_job_queue>(1000, config);
	 *
	 * // Enqueue with backpressure handling
	 * auto result = queue->enqueue(std::make_unique<my_job>());
	 * if (result.is_err()) {
	 *     // Handle backpressure (rejected, timeout, etc.)
	 * }
	 *
	 * // Check pressure level
	 * if (queue->get_pressure_level() == pressure_level::high) {
	 *     // Consider reducing load
	 * }
	 * @endcode
	 *
	 * @see job_queue Base queue class
	 * @see backpressure_config Configuration options
	 * @see token_bucket Rate limiting implementation
	 */
	class backpressure_job_queue : public job_queue
	{
	public:
		/**
		 * @brief Constructs a backpressure-aware job queue.
		 * @param max_size Maximum queue capacity.
		 * @param config Backpressure configuration.
		 *
		 * The queue is immediately ready for use after construction.
		 * If rate limiting is enabled, the token bucket is initialized
		 * with the specified parameters.
		 */
		explicit backpressure_job_queue(
			std::size_t max_size,
			backpressure_config config = backpressure_config{});

		/**
		 * @brief Virtual destructor.
		 */
		~backpressure_job_queue() override;

		// Non-copyable, non-movable (inherits shared_ptr semantics from job_queue)
		backpressure_job_queue(const backpressure_job_queue&) = delete;
		backpressure_job_queue& operator=(const backpressure_job_queue&) = delete;
		backpressure_job_queue(backpressure_job_queue&&) = delete;
		backpressure_job_queue& operator=(backpressure_job_queue&&) = delete;

		// =========================================================================
		// Overridden Queue Operations
		// =========================================================================

		/**
		 * @brief Enqueues a job with backpressure handling.
		 * @param value The job to enqueue.
		 * @return VoidResult indicating success or error.
		 *
		 * Depending on the configured policy:
		 * - block: Waits up to block_timeout for space
		 * - drop_oldest: Removes oldest job if full
		 * - drop_newest: Rejects new job if full
		 * - callback: Calls decision_callback for custom handling
		 * - adaptive: Adjusts behavior based on current conditions
		 *
		 * Rate limiting (if enabled) is applied before policy-based handling.
		 *
		 * Error Codes:
		 * - queue_full: Queue at capacity and job rejected
		 * - queue_stopped: Queue has been stopped
		 * - operation_timeout: Block timeout expired
		 * - invalid_argument: Null job provided
		 */
		[[nodiscard]] auto enqueue(std::unique_ptr<job>&& value) -> common::VoidResult override;

		/**
		 * @brief Enqueues a batch of jobs with backpressure handling.
		 * @param jobs Vector of jobs to enqueue.
		 * @return VoidResult indicating success or error.
		 *
		 * Batch enqueue respects backpressure settings. If the batch
		 * would exceed capacity, behavior depends on policy:
		 * - block/drop_newest: Entire batch rejected
		 * - drop_oldest: Oldest jobs dropped to make room
		 * - callback: Called once for the batch decision
		 * - adaptive: May partially accept (future enhancement)
		 */
		[[nodiscard]] auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs)
			-> common::VoidResult override;

		// =========================================================================
		// Backpressure-Specific Methods
		// =========================================================================

		/**
		 * @brief Returns the current pressure level.
		 * @return Current pressure_level enum value.
		 *
		 * Calculated based on current queue depth relative to watermarks:
		 * - none: depth < low_watermark * max_size
		 * - low: depth < high_watermark * max_size
		 * - high: depth < max_size
		 * - critical: depth >= max_size
		 */
		[[nodiscard]] auto get_pressure_level() const -> pressure_level;

		/**
		 * @brief Returns the current pressure as a ratio.
		 * @return Ratio of current depth to max_size (0.0 to 1.0+).
		 *
		 * Can exceed 1.0 if queue somehow exceeds max_size (shouldn't happen
		 * with proper backpressure, but included for robustness).
		 */
		[[nodiscard]] auto get_pressure_ratio() const -> double;

		/**
		 * @brief Sets the backpressure configuration.
		 * @param config New configuration to apply.
		 *
		 * Updates take effect immediately. If rate limiting is being
		 * enabled or its parameters change, the token bucket is
		 * recreated or updated.
		 */
		auto set_backpressure_config(backpressure_config config) -> void;

		/**
		 * @brief Returns the current backpressure configuration.
		 * @return Reference to current configuration.
		 */
		[[nodiscard]] auto get_backpressure_config() const -> const backpressure_config&;

		// =========================================================================
		// Rate Limiting
		// =========================================================================

		/**
		 * @brief Checks if rate limiting is causing delays.
		 * @return true if rate limiter is constraining throughput.
		 *
		 * Returns false if rate limiting is disabled.
		 */
		[[nodiscard]] auto is_rate_limited() const -> bool;

		/**
		 * @brief Returns available rate limit tokens.
		 * @return Available tokens, or max if rate limiting disabled.
		 */
		[[nodiscard]] auto get_available_tokens() const -> std::size_t;

		// =========================================================================
		// Statistics
		// =========================================================================

		/**
		 * @brief Returns backpressure statistics snapshot.
		 * @return Snapshot of current statistics.
		 *
		 * Returns a snapshot of current stats. For ongoing monitoring,
		 * call periodically.
		 */
		[[nodiscard]] auto get_backpressure_stats() const -> backpressure_stats_snapshot;

		/**
		 * @brief Resets backpressure statistics.
		 */
		auto reset_stats() -> void;

		// =========================================================================
		// String Representation
		// =========================================================================

		/**
		 * @brief Returns string representation including backpressure state.
		 * @return Formatted string with queue and backpressure information.
		 */
		[[nodiscard]] auto to_string() const -> std::string override;

	private:
		/**
		 * @brief Applies backpressure logic for a single job.
		 * @param value The job to potentially enqueue.
		 * @return VoidResult indicating success or error.
		 *
		 * This is the core backpressure implementation that handles
		 * all policy logic, rate limiting, and statistics tracking.
		 */
		[[nodiscard]] auto apply_backpressure(std::unique_ptr<job>&& value) -> common::VoidResult;

		/**
		 * @brief Applies rate limiting check.
		 * @return true if rate limit allows operation, false if should wait.
		 */
		[[nodiscard]] auto apply_rate_limiting() -> bool;

		/**
		 * @brief Updates pressure level and triggers callbacks if changed.
		 */
		auto update_pressure_state() -> void;

		/**
		 * @brief Handles blocking policy with timeout.
		 * @param value The job to enqueue.
		 * @return VoidResult indicating success or timeout.
		 */
		[[nodiscard]] auto handle_block_policy(std::unique_ptr<job>&& value) -> common::VoidResult;

		/**
		 * @brief Handles drop_oldest policy.
		 * @param value The job to enqueue after dropping oldest.
		 * @return VoidResult (always succeeds unless stopped).
		 */
		[[nodiscard]] auto handle_drop_oldest_policy(std::unique_ptr<job>&& value) -> common::VoidResult;

		/**
		 * @brief Handles callback policy.
		 * @param value The job to potentially enqueue.
		 * @return VoidResult based on callback decision.
		 */
		[[nodiscard]] auto handle_callback_policy(std::unique_ptr<job>&& value) -> common::VoidResult;

		/**
		 * @brief Handles adaptive policy.
		 * @param value The job to potentially enqueue.
		 * @return VoidResult based on adaptive decision.
		 */
		[[nodiscard]] auto handle_adaptive_policy(std::unique_ptr<job>&& value) -> common::VoidResult;

		/**
		 * @brief Directly enqueues without backpressure (internal use).
		 * @param value The job to enqueue.
		 * @return VoidResult from base class enqueue.
		 */
		[[nodiscard]] auto direct_enqueue(std::unique_ptr<job>&& value) -> common::VoidResult;

		/**
		 * @brief Backpressure configuration.
		 */
		backpressure_config config_;

		/**
		 * @brief Mutex for configuration access.
		 */
		mutable std::mutex config_mutex_;

		/**
		 * @brief Token bucket rate limiter (nullptr if disabled).
		 */
		std::unique_ptr<token_bucket> rate_limiter_;

		/**
		 * @brief Current pressure level (atomic for lock-free reads).
		 */
		std::atomic<pressure_level> current_pressure_{pressure_level::none};

		/**
		 * @brief Current pressure ratio (atomic for lock-free reads).
		 */
		std::atomic<double> current_pressure_ratio_{0.0};

		/**
		 * @brief Backpressure statistics.
		 */
		backpressure_stats stats_;

		/**
		 * @brief Condition variable for blocking policy wait.
		 */
		std::condition_variable space_available_;
	};

} // namespace kcenon::thread
