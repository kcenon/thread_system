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

#include <kcenon/common/patterns/result.h>

#include <memory>
#include <string>

namespace kcenon::thread
{
	// Forward declarations
	class job;

	/**
	 * @class pool_policy
	 * @brief Base interface for thread pool policies.
	 *
	 * @ingroup pool_policies
	 *
	 * Policies provide a way to extend thread pool behavior without modifying
	 * the thread_pool class itself. This follows the Strategy pattern and
	 * Single Responsibility Principle (SRP).
	 *
	 * ### Design Principles
	 * - **Extensibility**: New behaviors can be added by implementing this interface
	 * - **Composability**: Multiple policies can be combined in a thread pool
	 * - **Non-intrusive**: Policies don't require changes to core thread_pool code
	 * - **Testability**: Each policy can be unit tested independently
	 *
	 * ### Lifecycle Hooks
	 * Policies receive callbacks at key points in the job lifecycle:
	 * - `on_enqueue()`: Called before a job is added to the queue
	 * - `on_job_start()`: Called when a worker begins executing a job
	 * - `on_job_complete()`: Called when a job finishes (success or failure)
	 *
	 * ### Thread Safety
	 * All methods must be thread-safe as they may be called from multiple workers.
	 *
	 * ### Usage Example
	 * @code
	 * class my_logging_policy : public pool_policy {
	 * public:
	 *     auto on_enqueue(job& j) -> common::VoidResult override {
	 *         LOG_INFO("Job {} enqueued", j.get_name());
	 *         return common::ok();
	 *     }
	 *
	 *     void on_job_start(job& j) override {
	 *         LOG_INFO("Job {} started", j.get_name());
	 *     }
	 *
	 *     void on_job_complete(job& j, bool success, const std::exception* error) override {
	 *         LOG_INFO("Job {} completed: {}", j.get_name(), success ? "success" : "failed");
	 *     }
	 *
	 *     auto get_name() const -> std::string override { return "logging_policy"; }
	 * };
	 *
	 * pool->add_policy(std::make_unique<my_logging_policy>());
	 * @endcode
	 *
	 * @see circuit_breaker_policy
	 */
	class pool_policy
	{
	public:
		/**
		 * @brief Virtual destructor for proper cleanup.
		 */
		virtual ~pool_policy() = default;

		/**
		 * @brief Called before a job is enqueued.
		 * @param j Reference to the job being enqueued.
		 * @return common::VoidResult - ok() to allow, error to reject the job.
		 *
		 * Policies can use this to:
		 * - Validate the job
		 * - Apply transformations
		 * - Reject jobs based on policy rules (e.g., circuit breaker open)
		 *
		 * Thread Safety:
		 * - Must be thread-safe
		 * - Called from the enqueueing thread
		 */
		virtual auto on_enqueue(job& j) -> common::VoidResult = 0;

		/**
		 * @brief Called when a worker starts executing a job.
		 * @param j Reference to the job being started.
		 *
		 * Policies can use this to:
		 * - Start timing
		 * - Update metrics
		 * - Log job start
		 *
		 * Thread Safety:
		 * - Must be thread-safe
		 * - Called from the worker thread
		 */
		virtual void on_job_start(job& j) = 0;

		/**
		 * @brief Called when a job completes (success or failure).
		 * @param j Reference to the completed job.
		 * @param success True if job completed successfully.
		 * @param error Pointer to exception if job failed, nullptr otherwise.
		 *
		 * Policies can use this to:
		 * - Record success/failure metrics
		 * - Update circuit breaker state
		 * - Log completion
		 *
		 * Thread Safety:
		 * - Must be thread-safe
		 * - Called from the worker thread
		 */
		virtual void on_job_complete(job& j, bool success, const std::exception* error = nullptr) = 0;

		/**
		 * @brief Gets the policy name for identification and logging.
		 * @return Policy name string.
		 */
		[[nodiscard]] virtual auto get_name() const -> std::string = 0;

		/**
		 * @brief Checks if the policy is currently enabled.
		 * @return True if policy is active.
		 */
		[[nodiscard]] virtual auto is_enabled() const -> bool { return true; }

		/**
		 * @brief Enables or disables the policy.
		 * @param enabled Whether to enable the policy.
		 *
		 * Disabled policies have their hooks called but should no-op.
		 */
		virtual void set_enabled(bool enabled) { (void)enabled; }
	};

} // namespace kcenon::thread
