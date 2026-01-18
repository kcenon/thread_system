/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, kcenon
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

#include "pool_policy.h"
#include <kcenon/thread/scaling/autoscaler.h>
#include <kcenon/thread/scaling/autoscaling_policy.h>

#include <atomic>
#include <memory>
#include <string>

namespace kcenon::thread
{
	/**
	 * @class autoscaling_pool_policy
	 * @brief Pool policy that implements automatic scaling for dynamic worker management.
	 *
	 * @ingroup pool_policies
	 *
	 * This policy wraps the autoscaling functionality as a composable pool policy,
	 * enabling automatic scaling without modifying the thread_pool class.
	 *
	 * ### Autoscaling Behavior
	 * The autoscaler monitors thread pool metrics and adjusts worker count:
	 * - Scale-up triggered when ANY threshold is exceeded (high utilization, queue depth, latency)
	 * - Scale-down triggered when ALL conditions are met (low utilization, low queue depth, idle workers)
	 *
	 * ### Thread Safety
	 * All methods are thread-safe and can be called from any thread.
	 *
	 * ### Usage Example
	 * @code
	 * autoscaling_policy config;
	 * config.min_workers = 2;
	 * config.max_workers = 16;
	 * config.scaling_mode = autoscaling_policy::mode::automatic;
	 *
	 * auto as_policy = std::make_unique<autoscaling_pool_policy>(pool, config);
	 * pool->add_policy(std::move(as_policy));
	 *
	 * // Now workers are automatically scaled based on load
	 * @endcode
	 *
	 * @see pool_policy
	 * @see autoscaler
	 * @see autoscaling_policy
	 */
	class autoscaling_pool_policy : public pool_policy
	{
	public:
		/**
		 * @brief Constructs an autoscaling pool policy with the given configuration.
		 * @param pool Reference to the thread pool to manage.
		 * @param config Autoscaling policy configuration.
		 */
		explicit autoscaling_pool_policy(thread_pool& pool, const autoscaling_policy& config = {});

		/**
		 * @brief Constructs an autoscaling pool policy with an existing autoscaler.
		 * @param scaler Shared pointer to an existing autoscaler.
		 *
		 * This allows sharing an autoscaler across multiple pools or components.
		 */
		explicit autoscaling_pool_policy(std::shared_ptr<autoscaler> scaler);

		/**
		 * @brief Destructor. Stops the autoscaler if running.
		 */
		~autoscaling_pool_policy() override;

		// Non-copyable
		autoscaling_pool_policy(const autoscaling_pool_policy&) = delete;
		autoscaling_pool_policy& operator=(const autoscaling_pool_policy&) = delete;

		// Non-movable (std::atomic is not movable)
		autoscaling_pool_policy(autoscaling_pool_policy&&) = delete;
		autoscaling_pool_policy& operator=(autoscaling_pool_policy&&) = delete;

		// ============================================
		// pool_policy Interface
		// ============================================

		/**
		 * @brief Called before a job is enqueued.
		 * @param j Reference to the job being enqueued.
		 * @return common::ok() - autoscaling always allows jobs.
		 *
		 * Autoscaling does not reject jobs; it adjusts worker count to handle load.
		 */
		auto on_enqueue(job& j) -> common::VoidResult override;

		/**
		 * @brief Called when job starts executing.
		 * @param j Reference to the job.
		 *
		 * Records job start for metrics collection.
		 */
		void on_job_start(job& j) override;

		/**
		 * @brief Called when a job completes.
		 * @param j Reference to the completed job.
		 * @param success True if job succeeded.
		 * @param error Exception pointer if job failed.
		 *
		 * Records completion for metrics used in scaling decisions.
		 */
		void on_job_complete(job& j, bool success, const std::exception* error = nullptr) override;

		/**
		 * @brief Gets the policy name.
		 * @return "autoscaling_pool_policy"
		 */
		[[nodiscard]] auto get_name() const -> std::string override;

		/**
		 * @brief Checks if the policy is enabled.
		 * @return True if enabled.
		 */
		[[nodiscard]] auto is_enabled() const -> bool override;

		/**
		 * @brief Enables or disables the policy.
		 * @param enabled Whether to enable.
		 *
		 * When disabled, the autoscaler is stopped. When enabled, it is started
		 * (if the pool is running).
		 */
		void set_enabled(bool enabled) override;

		// ============================================
		// Autoscaling Specific Methods
		// ============================================

		/**
		 * @brief Starts the autoscaler monitor thread.
		 *
		 * Should be called after the pool starts. This is automatically
		 * managed if the policy is added before pool.start().
		 */
		void start();

		/**
		 * @brief Stops the autoscaler monitor thread.
		 */
		void stop();

		/**
		 * @brief Checks if the autoscaler is currently active.
		 * @return True if the monitor thread is running.
		 */
		[[nodiscard]] auto is_active() const -> bool;

		/**
		 * @brief Gets the underlying autoscaler.
		 * @return Shared pointer to the autoscaler.
		 *
		 * Useful for accessing detailed scaling controls and statistics.
		 */
		[[nodiscard]] auto get_autoscaler() const -> std::shared_ptr<autoscaler>;

		/**
		 * @brief Gets current autoscaling statistics.
		 * @return Statistics about scaling operations.
		 */
		[[nodiscard]] auto get_stats() const -> autoscaling_stats;

		/**
		 * @brief Updates the autoscaling policy configuration.
		 * @param config New policy configuration.
		 */
		void set_policy(const autoscaling_policy& config);

		/**
		 * @brief Gets the current autoscaling policy configuration.
		 * @return Const reference to the policy.
		 */
		[[nodiscard]] auto get_policy() const -> const autoscaling_policy&;

		/**
		 * @brief Manually triggers a scaling evaluation.
		 * @return The scaling decision that would be made.
		 */
		[[nodiscard]] auto evaluate_now() -> scaling_decision;

		/**
		 * @brief Manually scales to a specific worker count.
		 * @param target_workers Desired number of workers.
		 * @return Error if scaling fails.
		 */
		auto scale_to(std::size_t target_workers) -> common::VoidResult;

	private:
		std::shared_ptr<autoscaler> autoscaler_;
		std::atomic<bool> enabled_{true};
	};

} // namespace kcenon::thread
