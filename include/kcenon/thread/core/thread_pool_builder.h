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

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/worker_policy.h>
#include <kcenon/thread/interfaces/thread_context.h>
#include <kcenon/thread/pool_policies/circuit_breaker_policy.h>
#include <kcenon/thread/pool_policies/autoscaling_pool_policy.h>
#include <kcenon/thread/pool_policies/work_stealing_pool_policy.h>
#include <kcenon/thread/resilience/circuit_breaker_config.h>
#include <kcenon/thread/scaling/autoscaling_policy.h>

#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace kcenon::thread
{
	/**
	 * @class thread_pool_builder
	 * @brief Fluent builder for creating and configuring thread pools.
	 *
	 * @ingroup thread_pools
	 *
	 * The `thread_pool_builder` provides a fluent API for constructing thread pools
	 * with various configuration options. This pattern improves readability and
	 * makes configuration immutable until the pool is built.
	 *
	 * ### Design Principles
	 * - **Fluent Interface**: All `with_*()` methods return `*this` for chaining
	 * - **Immutable Configuration**: Settings are accumulated before building
	 * - **Sensible Defaults**: Unconfigured options use reasonable defaults
	 * - **Policy Composition**: Multiple policies can be combined
	 *
	 * ### Usage Example
	 * @code
	 * // Basic usage
	 * auto pool = thread_pool_builder("my_pool")
	 *     .with_workers(8)
	 *     .build();
	 *
	 * // With policies
	 * auto pool = thread_pool_builder("resilient_pool")
	 *     .with_workers(4)
	 *     .with_circuit_breaker(circuit_breaker_config{
	 *         .failure_threshold = 5,
	 *         .open_duration = std::chrono::seconds{30}
	 *     })
	 *     .with_autoscaling(autoscaling_policy{
	 *         .min_workers = 2,
	 *         .max_workers = 16,
	 *         .scaling_mode = autoscaling_policy::mode::automatic
	 *     })
	 *     .with_work_stealing()
	 *     .build();
	 *
	 * pool->start();
	 * @endcode
	 *
	 * @see thread_pool
	 * @see circuit_breaker_policy
	 * @see autoscaling_pool_policy
	 * @see work_stealing_pool_policy
	 */
	class thread_pool_builder
	{
	public:
		/**
		 * @brief Constructs a builder with the given pool name.
		 * @param name Name/title for the thread pool.
		 *
		 * The name is used for identification, logging, and debugging.
		 */
		explicit thread_pool_builder(const std::string& name = "thread_pool");

		/**
		 * @brief Sets the number of worker threads.
		 * @param count Number of workers to create.
		 * @return Reference to this builder for chaining.
		 *
		 * If not specified, defaults to `std::thread::hardware_concurrency()`.
		 */
		thread_pool_builder& with_workers(std::size_t count);

		/**
		 * @brief Sets the thread context for logging and monitoring.
		 * @param context Thread context with logger and monitoring services.
		 * @return Reference to this builder for chaining.
		 */
		thread_pool_builder& with_context(const thread_context& context);

		/**
		 * @brief Sets a custom job queue.
		 * @param queue Custom job queue implementation.
		 * @return Reference to this builder for chaining.
		 *
		 * Use this to inject specialized queues like `backpressure_job_queue`.
		 */
		thread_pool_builder& with_queue(std::shared_ptr<job_queue> queue);

		/**
		 * @brief Sets a policy-based queue adapter.
		 * @param adapter Queue adapter for policy_queue.
		 * @return Reference to this builder for chaining.
		 *
		 * Use this for the new policy-based queue system.
		 */
		thread_pool_builder& with_queue_adapter(
			std::unique_ptr<pool_queue_adapter_interface> adapter);

		/**
		 * @brief Adds circuit breaker protection.
		 * @param config Circuit breaker configuration.
		 * @return Reference to this builder for chaining.
		 *
		 * The circuit breaker monitors job failures and automatically opens
		 * when a threshold is exceeded, preventing cascading failures.
		 *
		 * @see circuit_breaker_config
		 * @see circuit_breaker_policy
		 */
		thread_pool_builder& with_circuit_breaker(
			const circuit_breaker_config& config = {});

		/**
		 * @brief Adds circuit breaker with an existing circuit breaker instance.
		 * @param cb Shared circuit breaker for multiple pools.
		 * @return Reference to this builder for chaining.
		 *
		 * Use this to share a circuit breaker across multiple pools.
		 */
		thread_pool_builder& with_circuit_breaker(
			std::shared_ptr<circuit_breaker> cb);

		/**
		 * @brief Enables autoscaling with the specified policy.
		 * @param config Autoscaling policy configuration.
		 * @return Reference to this builder for chaining.
		 *
		 * The autoscaler automatically adjusts worker count based on
		 * load metrics (utilization, queue depth, latency).
		 *
		 * @see autoscaling_policy
		 * @see autoscaling_pool_policy
		 */
		thread_pool_builder& with_autoscaling(
			const autoscaling_policy& config = {});

		/**
		 * @brief Enables work-stealing with default configuration.
		 * @return Reference to this builder for chaining.
		 *
		 * Work-stealing enables idle workers to steal jobs from busy workers,
		 * improving load balancing and throughput.
		 *
		 * @see work_stealing_pool_policy
		 */
		thread_pool_builder& with_work_stealing();

		/**
		 * @brief Enables work-stealing with custom configuration.
		 * @param config Worker policy with work-stealing settings.
		 * @return Reference to this builder for chaining.
		 *
		 * @see worker_policy
		 * @see work_stealing_pool_policy
		 */
		thread_pool_builder& with_work_stealing(const worker_policy& config);

		/**
		 * @brief Enables diagnostics for the pool.
		 * @return Reference to this builder for chaining.
		 *
		 * Diagnostics provide thread dumps, job inspection, and
		 * bottleneck detection capabilities.
		 */
		thread_pool_builder& with_diagnostics();

		/**
		 * @brief Enables enhanced metrics collection.
		 * @return Reference to this builder for chaining.
		 *
		 * Enhanced metrics include latency histograms, percentiles,
		 * and sliding window throughput tracking.
		 */
		thread_pool_builder& with_enhanced_metrics();

		/**
		 * @brief Adds a custom policy to the pool.
		 * @param policy Custom pool policy.
		 * @return Reference to this builder for chaining.
		 *
		 * Use this to add custom policies that implement pool_policy.
		 *
		 * @see pool_policy
		 */
		thread_pool_builder& with_policy(std::unique_ptr<pool_policy> policy);

		/**
		 * @brief Builds and returns the configured thread pool.
		 * @return Shared pointer to the constructed thread pool.
		 *
		 * After calling build(), the builder is reset and can be reused
		 * to build another pool with different settings.
		 *
		 * @note The pool is NOT started automatically. Call pool->start()
		 *       to begin processing jobs.
		 */
		[[nodiscard]] std::shared_ptr<thread_pool> build();

		/**
		 * @brief Builds the pool and starts it immediately.
		 * @return Shared pointer to the started thread pool.
		 *
		 * Convenience method equivalent to:
		 * @code
		 * auto pool = builder.build();
		 * pool->start();
		 * return pool;
		 * @endcode
		 */
		[[nodiscard]] std::shared_ptr<thread_pool> build_and_start();

	private:
		std::string name_;
		std::size_t worker_count_{0};
		thread_context context_;
		std::shared_ptr<job_queue> custom_queue_;
		std::unique_ptr<pool_queue_adapter_interface> queue_adapter_;
		std::vector<std::unique_ptr<pool_policy>> policies_;
		bool enable_diagnostics_{false};
		bool enable_enhanced_metrics_{false};

		std::optional<circuit_breaker_config> circuit_breaker_config_;
		std::shared_ptr<circuit_breaker> shared_circuit_breaker_;
		std::optional<autoscaling_policy> autoscaling_config_;
		std::optional<worker_policy> work_stealing_config_;

		void reset();
	};

} // namespace kcenon::thread
