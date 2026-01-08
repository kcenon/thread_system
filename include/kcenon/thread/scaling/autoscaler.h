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

#include "autoscaling_policy.h"
#include "scaling_metrics.h"

#include <kcenon/thread/core/error_handling.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

namespace kcenon::thread
{
	// Forward declaration
	class thread_pool;

	/**
	 * @class autoscaler
	 * @brief Manages automatic scaling of thread pool workers based on load metrics.
	 *
	 * The autoscaler monitors thread pool metrics and automatically adjusts the
	 * number of workers to match workload demands. It uses a background monitor
	 * thread to periodically collect metrics and make scaling decisions.
	 *
	 * ### Design Principles
	 * - **Non-intrusive**: Scaling decisions are made asynchronously
	 * - **Configurable**: All thresholds and behaviors are customizable
	 * - **Graceful**: Scale-down removes workers only when safe
	 * - **Observable**: Provides statistics and callbacks for monitoring
	 *
	 * ### State Machine
	 * @code
	 * ┌─────────────────────────────────────────────────────────────┐
	 * │                    Autoscaler Loop                          │
	 * │                                                             │
	 * │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐     │
	 * │  │ Collect     │───▶│ Aggregate   │───▶│ Make        │     │
	 * │  │ Metrics     │    │ Samples     │    │ Decision    │     │
	 * │  └─────────────┘    └─────────────┘    └─────────────┘     │
	 * │         │                                    │              │
	 * │         │                                    ▼              │
	 * │         │                         ┌─────────────────┐       │
	 * │         │                         │ Check Cooldown  │       │
	 * │         │                         └─────────────────┘       │
	 * │         │                                    │              │
	 * │         │                                    ▼              │
	 * │         │                         ┌─────────────────┐       │
	 * │         │                         │ Execute Scale   │       │
	 * │         │                         └─────────────────┘       │
	 * │         │                                    │              │
	 * │         ▼                                    │              │
	 * │  ┌─────────────┐                            │              │
	 * │  │   Sleep     │◀───────────────────────────┘              │
	 * │  │  Interval   │                                           │
	 * │  └─────────────┘                                           │
	 * └─────────────────────────────────────────────────────────────┘
	 * @endcode
	 *
	 * ### Thread Safety
	 * All public methods are thread-safe and can be called from any thread.
	 *
	 * ### Usage Example
	 * @code
	 * auto pool = std::make_shared<thread_pool>("MyPool");
	 * auto scaler = std::make_shared<autoscaler>(*pool, autoscaling_policy{
	 *     .min_workers = 2,
	 *     .max_workers = 16,
	 *     .scaling_mode = autoscaling_policy::mode::automatic
	 * });
	 *
	 * scaler->start();
	 * // ... pool automatically scales ...
	 * scaler->stop();
	 * @endcode
	 *
	 * @see autoscaling_policy
	 * @see scaling_metrics_sample
	 */
	class autoscaler
	{
	public:
		/**
		 * @brief Constructs an autoscaler for the given thread pool.
		 * @param pool Reference to the thread pool to manage.
		 * @param policy Autoscaling policy configuration.
		 */
		explicit autoscaler(thread_pool& pool, autoscaling_policy policy = {});

		/**
		 * @brief Destructor. Stops the monitor thread if running.
		 */
		~autoscaler();

		// Non-copyable, non-movable
		autoscaler(const autoscaler&) = delete;
		autoscaler& operator=(const autoscaler&) = delete;
		autoscaler(autoscaler&&) = delete;
		autoscaler& operator=(autoscaler&&) = delete;

		// ============================================
		// Control
		// ============================================

		/**
		 * @brief Starts the autoscaling monitor thread.
		 *
		 * The monitor thread periodically collects metrics and makes
		 * scaling decisions based on the configured policy.
		 */
		auto start() -> void;

		/**
		 * @brief Stops the autoscaling monitor thread.
		 *
		 * Waits for the monitor thread to complete before returning.
		 */
		auto stop() -> void;

		/**
		 * @brief Checks if the autoscaler is currently active.
		 * @return true if the monitor thread is running.
		 */
		[[nodiscard]] auto is_active() const -> bool;

		// ============================================
		// Manual Triggers
		// ============================================

		/**
		 * @brief Manually triggers a scaling evaluation.
		 * @return The scaling decision that would be made.
		 *
		 * This does not actually execute the scaling; use scale_to()
		 * or scale_up()/scale_down() to actually modify worker count.
		 */
		[[nodiscard]] auto evaluate_now() -> scaling_decision;

		/**
		 * @brief Manually scales to a specific worker count.
		 * @param target_workers Desired number of workers.
		 * @return Error if scaling fails.
		 *
		 * The target is clamped to [min_workers, max_workers] from the policy.
		 */
		auto scale_to(std::size_t target_workers) -> common::VoidResult;

		/**
		 * @brief Manually scales up by the configured increment.
		 * @return Error if scaling fails.
		 */
		auto scale_up() -> common::VoidResult;

		/**
		 * @brief Manually scales down by the configured increment.
		 * @return Error if scaling fails.
		 */
		auto scale_down() -> common::VoidResult;

		// ============================================
		// Configuration
		// ============================================

		/**
		 * @brief Updates the autoscaling policy.
		 * @param policy New policy configuration.
		 */
		auto set_policy(autoscaling_policy policy) -> void;

		/**
		 * @brief Gets the current autoscaling policy.
		 * @return Const reference to the policy.
		 */
		[[nodiscard]] auto get_policy() const -> const autoscaling_policy&;

		// ============================================
		// Metrics & Statistics
		// ============================================

		/**
		 * @brief Collects current metrics from the thread pool.
		 * @return Current metrics sample.
		 */
		[[nodiscard]] auto get_current_metrics() const -> scaling_metrics_sample;

		/**
		 * @brief Gets historical metrics samples.
		 * @param count Maximum number of samples to return.
		 * @return Vector of recent metrics samples.
		 */
		[[nodiscard]] auto get_metrics_history(std::size_t count = 60) const
			-> std::vector<scaling_metrics_sample>;

		/**
		 * @brief Gets autoscaling statistics.
		 * @return Statistics about scaling operations.
		 */
		[[nodiscard]] auto get_stats() const -> autoscaling_stats;

		/**
		 * @brief Resets autoscaling statistics.
		 */
		auto reset_stats() -> void;

	private:
		/**
		 * @brief Main monitoring loop running in the background thread.
		 */
		auto monitor_loop() -> void;

		/**
		 * @brief Collects current metrics from the pool.
		 * @return Collected metrics sample.
		 */
		[[nodiscard]] auto collect_metrics() const -> scaling_metrics_sample;

		/**
		 * @brief Makes a scaling decision based on recent samples.
		 * @param samples Recent metrics samples.
		 * @return Scaling decision.
		 */
		[[nodiscard]] auto make_decision(const std::vector<scaling_metrics_sample>& samples) const
			-> scaling_decision;

		/**
		 * @brief Executes a scaling decision.
		 * @param decision The decision to execute.
		 */
		auto execute_scaling(const scaling_decision& decision) -> void;

		/**
		 * @brief Checks if scale-up cooldown has elapsed.
		 * @return true if scale-up is allowed.
		 */
		[[nodiscard]] auto can_scale_up() const -> bool;

		/**
		 * @brief Checks if scale-down cooldown has elapsed.
		 * @return true if scale-down is allowed.
		 */
		[[nodiscard]] auto can_scale_down() const -> bool;

		/**
		 * @brief Adds workers to the pool.
		 * @param count Number of workers to add.
		 * @return Error if operation fails.
		 */
		auto add_workers(std::size_t count) -> common::VoidResult;

		/**
		 * @brief Removes workers from the pool.
		 * @param count Number of workers to remove.
		 * @return Error if operation fails.
		 */
		auto remove_workers(std::size_t count) -> common::VoidResult;

		thread_pool& pool_;
		autoscaling_policy policy_;

		std::atomic<bool> running_{false};
		std::unique_ptr<std::thread> monitor_thread_;

		mutable std::mutex mutex_;
		std::condition_variable cv_;

		std::deque<scaling_metrics_sample> metrics_history_;
		mutable std::mutex history_mutex_;

		std::chrono::steady_clock::time_point last_scale_up_time_;
		std::chrono::steady_clock::time_point last_scale_down_time_;

		autoscaling_stats stats_;
		mutable std::mutex stats_mutex_;

		// Cached values from previous sample for delta calculations
		std::uint64_t last_jobs_completed_{0};
		std::uint64_t last_jobs_submitted_{0};
		std::chrono::steady_clock::time_point last_sample_time_;
	};

} // namespace kcenon::thread
