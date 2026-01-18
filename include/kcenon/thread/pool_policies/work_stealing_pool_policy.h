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
#include <kcenon/thread/core/worker_policy.h>

#include <atomic>
#include <memory>
#include <string>

namespace kcenon::thread
{
	// Forward declarations
	class thread_pool;

	/**
	 * @class work_stealing_pool_policy
	 * @brief Pool policy that implements work-stealing behavior for load balancing.
	 *
	 * @ingroup pool_policies
	 *
	 * This policy wraps the work-stealing functionality as a composable pool policy,
	 * enabling work-stealing configuration without modifying the thread_pool class.
	 *
	 * ### Work-Stealing Pattern
	 * Work-stealing enables idle workers to "steal" jobs from busy workers' local
	 * queues, improving load balancing and throughput:
	 * - Workers first check their local queue for work
	 * - If empty, they attempt to steal from other workers
	 * - Victim selection can be random, round-robin, or adaptive
	 *
	 * ### Thread Safety
	 * All methods are thread-safe and can be called from any thread.
	 *
	 * ### Usage Example
	 * @code
	 * worker_policy config;
	 * config.enable_work_stealing = true;
	 * config.victim_selection = steal_policy::adaptive;
	 * config.max_steal_attempts = 5;
	 *
	 * auto pool = std::make_shared<thread_pool>("my_pool");
	 * auto ws_policy = std::make_unique<work_stealing_pool_policy>(config);
	 * pool->add_policy(std::move(ws_policy));
	 *
	 * pool->start();
	 * @endcode
	 *
	 * @see pool_policy
	 * @see worker_policy
	 */
	class work_stealing_pool_policy : public pool_policy
	{
	public:
		/**
		 * @brief Constructs a work-stealing policy with the given configuration.
		 * @param config Worker policy configuration containing work-stealing settings.
		 */
		explicit work_stealing_pool_policy(const worker_policy& config = {});

		/**
		 * @brief Destructor.
		 */
		~work_stealing_pool_policy() override = default;

		// Non-copyable
		work_stealing_pool_policy(const work_stealing_pool_policy&) = delete;
		work_stealing_pool_policy& operator=(const work_stealing_pool_policy&) = delete;

		// Non-movable (std::atomic is not movable)
		work_stealing_pool_policy(work_stealing_pool_policy&&) = delete;
		work_stealing_pool_policy& operator=(work_stealing_pool_policy&&) = delete;

		// ============================================
		// pool_policy Interface
		// ============================================

		/**
		 * @brief Called before a job is enqueued.
		 * @param j Reference to the job being enqueued.
		 * @return common::ok() - work stealing does not reject jobs.
		 *
		 * Work-stealing policy does not modify enqueue behavior.
		 * Jobs are always accepted.
		 */
		auto on_enqueue(job& j) -> common::VoidResult override;

		/**
		 * @brief Called when job starts executing.
		 * @param j Reference to the job.
		 *
		 * Can be used to track job execution for steal decisions.
		 */
		void on_job_start(job& j) override;

		/**
		 * @brief Called when a job completes.
		 * @param j Reference to the completed job.
		 * @param success True if job succeeded.
		 * @param error Exception pointer if job failed.
		 *
		 * Updates internal statistics for adaptive stealing.
		 */
		void on_job_complete(job& j, bool success, const std::exception* error = nullptr) override;

		/**
		 * @brief Gets the policy name.
		 * @return "work_stealing_pool_policy"
		 */
		[[nodiscard]] auto get_name() const -> std::string override;

		/**
		 * @brief Checks if the policy is enabled.
		 * @return True if work-stealing is enabled.
		 */
		[[nodiscard]] auto is_enabled() const -> bool override;

		/**
		 * @brief Enables or disables the policy.
		 * @param enabled Whether to enable work-stealing.
		 */
		void set_enabled(bool enabled) override;

		// ============================================
		// Work-Stealing Specific Methods
		// ============================================

		/**
		 * @brief Gets the current worker policy configuration.
		 * @return Const reference to the worker policy.
		 */
		[[nodiscard]] auto get_policy() const -> const worker_policy&;

		/**
		 * @brief Updates the worker policy configuration.
		 * @param config New worker policy configuration.
		 *
		 * Note: Changes take effect for subsequent operations.
		 */
		void set_policy(const worker_policy& config);

		/**
		 * @brief Gets the steal policy (victim selection strategy).
		 * @return Current steal policy.
		 */
		[[nodiscard]] auto get_steal_policy() const -> steal_policy;

		/**
		 * @brief Sets the steal policy (victim selection strategy).
		 * @param policy New steal policy.
		 */
		void set_steal_policy(steal_policy policy);

		/**
		 * @brief Gets the maximum steal attempts per steal cycle.
		 * @return Maximum steal attempts.
		 */
		[[nodiscard]] auto get_max_steal_attempts() const -> std::size_t;

		/**
		 * @brief Sets the maximum steal attempts per steal cycle.
		 * @param attempts New maximum steal attempts.
		 */
		void set_max_steal_attempts(std::size_t attempts);

		/**
		 * @brief Gets the steal backoff duration.
		 * @return Backoff duration between steal attempts.
		 */
		[[nodiscard]] auto get_steal_backoff() const -> std::chrono::microseconds;

		/**
		 * @brief Sets the steal backoff duration.
		 * @param backoff New backoff duration.
		 */
		void set_steal_backoff(std::chrono::microseconds backoff);

		/**
		 * @brief Gets the total number of successful steals.
		 * @return Number of successful steal operations.
		 */
		[[nodiscard]] auto get_successful_steals() const -> std::uint64_t;

		/**
		 * @brief Gets the total number of failed steal attempts.
		 * @return Number of failed steal attempts.
		 */
		[[nodiscard]] auto get_failed_steals() const -> std::uint64_t;

		/**
		 * @brief Resets the steal statistics.
		 */
		void reset_stats();

		/**
		 * @brief Records a successful steal operation.
		 *
		 * Call this from the thread_pool when a steal succeeds.
		 */
		void record_successful_steal();

		/**
		 * @brief Records a failed steal attempt.
		 *
		 * Call this from the thread_pool when a steal fails.
		 */
		void record_failed_steal();

	private:
		worker_policy policy_;
		std::atomic<bool> enabled_;
		std::atomic<std::uint64_t> successful_steals_{0};
		std::atomic<std::uint64_t> failed_steals_{0};
	};

} // namespace kcenon::thread
