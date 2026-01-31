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

/**
 * @file thread_system_config.h
 * @brief Unified configuration structure for thread_system
 *
 * This file consolidates all configuration options for the thread_system
 * into a hierarchical structure with builder pattern support.
 */

#include "../core/backpressure_config.h"
#include "../resilience/circuit_breaker_config.h"
#include "../dag/dag_config.h"
#include "../impl/typed_pool/priority_aging_config.h"

#include <chrono>
#include <cstddef>
#include <thread>

namespace kcenon::thread
{
	// Forward declaration for builder pattern
	class config_builder;

	/**
	 * @struct thread_system_config
	 * @brief Unified configuration for thread_system
	 *
	 * This structure consolidates all configuration options into a
	 * hierarchical, easy-to-use format. It supports partial configuration
	 * with sensible defaults for unspecified values.
	 *
	 * ### Example Usage
	 * @code{.cpp}
	 * // Using builder pattern
	 * auto config = thread_system_config::builder()
	 *     .with_worker_count(8)
	 *     .with_queue_capacity(5000)
	 *     .enable_backpressure()
	 *     .enable_circuit_breaker()
	 *     .build();
	 *
	 * // Or direct initialization
	 * thread_system_config config;
	 * config.pool.worker_count = 8;
	 * config.pool.queue_capacity = 5000;
	 * config.resilience.circuit_breaker.failure_threshold = 5;
	 * @endcode
	 */
	struct thread_system_config
	{
		/**
		 * @struct pool_config
		 * @brief Configuration for thread pool behavior
		 */
		struct pool_config
		{
			/**
			 * @brief Number of worker threads in the pool
			 *
			 * Default: hardware_concurrency() (number of CPU cores)
			 */
			std::size_t worker_count = std::thread::hardware_concurrency();

			/**
			 * @brief Maximum number of jobs in the queue
			 *
			 * Default: 10000 jobs
			 * Set to 0 for unlimited (not recommended for production)
			 */
			std::size_t queue_capacity = 10000;

			/**
			 * @brief Backpressure configuration
			 *
			 * Controls how the queue handles overflow conditions.
			 * Default: blocking policy with 5s timeout
			 */
			backpressure_config backpressure{};

			/**
			 * @brief Wake interval for idle workers
			 *
			 * How often idle workers check for new jobs.
			 * Default: 100ms
			 */
			std::chrono::milliseconds wake_interval{100};

			/**
			 * @brief Shutdown timeout
			 *
			 * Maximum time to wait for workers to complete during shutdown.
			 * Default: 5s
			 */
			std::chrono::seconds shutdown_timeout{5};

			/**
			 * @brief Worker idle timeout
			 *
			 * Workers will be stopped after being idle for this duration.
			 * Default: 30s
			 */
			std::chrono::seconds worker_idle_timeout{30};

			/**
			 * @brief Yield on idle
			 *
			 * When true, idle workers yield CPU to other threads.
			 * Default: true
			 */
			bool yield_on_idle = true;

			/**
			 * @brief Enable work stealing
			 *
			 * When true, idle workers can steal jobs from busy workers.
			 * Default: false (can be enabled via THREAD_WORK_STEALING_ENABLED)
			 */
			bool enable_work_stealing = false;

			/**
			 * @brief Maximum steal attempts before backing off
			 *
			 * Only used when work stealing is enabled.
			 * Default: 3
			 */
			std::size_t max_steal_attempts = 3;

			/**
			 * @brief Backoff duration between steal attempts
			 *
			 * Only used when work stealing is enabled.
			 * Default: 50μs
			 */
			std::chrono::microseconds steal_backoff{50};
		} pool;

		/**
		 * @struct resilience_config
		 * @brief Configuration for resilience features
		 */
		struct resilience_config
		{
			/**
			 * @brief Circuit breaker configuration
			 *
			 * Prevents cascading failures by opening the circuit
			 * when failure threshold is exceeded.
			 * Default: disabled (failure_threshold = 5, rate = 0.5)
			 */
			circuit_breaker_config circuit_breaker{};
		} resilience;

		/**
		 * @brief DAG (Directed Acyclic Graph) configuration
		 *
		 * Configuration for DAG-based task scheduling.
		 * Default: fail-fast policy, cycle detection enabled
		 */
		dag_config dag{};

		/**
		 * @struct scaling_config
		 * @brief Configuration for dynamic scaling features
		 */
		struct scaling_config
		{
			/**
			 * @brief Enable automatic scaling based on load
			 *
			 * When true, the pool can dynamically adjust worker count.
			 * Default: false
			 */
			bool auto_scaling_enabled = false;

			/**
			 * @brief Priority aging configuration
			 *
			 * Prevents starvation by boosting priority of waiting jobs.
			 * Default: disabled
			 */
			priority_aging_config priority_aging{};
		} scaling;

		/**
		 * @brief Validates the entire configuration
		 *
		 * Checks all sub-configurations for validity.
		 *
		 * @return true if configuration is valid, false otherwise
		 */
		[[nodiscard]] auto is_valid() const -> bool
		{
			// Pool validation
			if (pool.worker_count == 0)
			{
				return false;
			}

			// Backpressure validation
			if (!pool.backpressure.is_valid())
			{
				return false;
			}

			// DAG validation
			if (dag.max_retries > 10)
			{
				return false; // Sanity check to prevent infinite retries
			}

			return true;
		}

		/**
		 * @brief Creates a builder for fluent configuration
		 *
		 * @return A new builder instance
		 */
		static auto builder() -> config_builder;
	};

	/**
	 * @class config_builder
	 * @brief Builder for thread_system_config
	 *
	 * Provides a fluent interface for constructing thread_system_config
	 * with sensible defaults.
	 *
	 * ### Example Usage
	 * @code{.cpp}
	 * auto config = thread_system_config::builder()
	 *     .with_worker_count(8)
	 *     .with_queue_capacity(5000)
	 *     .enable_backpressure()
	 *     .with_backpressure_policy(backpressure_policy::adaptive)
	 *     .enable_circuit_breaker()
	 *     .with_failure_threshold(5)
	 *     .enable_priority_aging()
	 *     .build();
	 * @endcode
	 */
	class config_builder
	{
	public:
		config_builder() = default;

		/**
		 * @brief Sets the number of worker threads
		 * @param count Number of workers
		 * @return Reference to this builder
		 */
		auto with_worker_count(std::size_t count) -> config_builder&
		{
			config_.pool.worker_count = count;
			return *this;
		}

		/**
		 * @brief Sets the queue capacity
		 * @param capacity Maximum queue size
		 * @return Reference to this builder
		 */
		auto with_queue_capacity(std::size_t capacity) -> config_builder&
		{
			config_.pool.queue_capacity = capacity;
			return *this;
		}

		/**
		 * @brief Enables backpressure with default settings
		 * @return Reference to this builder
		 */
		auto enable_backpressure() -> config_builder&
		{
			config_.pool.backpressure.policy = backpressure_policy::block;
			return *this;
		}

		/**
		 * @brief Sets the backpressure policy
		 * @param policy The backpressure policy to use
		 * @return Reference to this builder
		 */
		auto with_backpressure_policy(backpressure_policy policy) -> config_builder&
		{
			config_.pool.backpressure.policy = policy;
			return *this;
		}

		/**
		 * @brief Sets backpressure watermarks
		 * @param low Low watermark (0.0 to 1.0)
		 * @param high High watermark (0.0 to 1.0)
		 * @return Reference to this builder
		 */
		auto with_watermarks(double low, double high) -> config_builder&
		{
			config_.pool.backpressure.low_watermark = low;
			config_.pool.backpressure.high_watermark = high;
			return *this;
		}

		/**
		 * @brief Enables circuit breaker with default settings
		 * @return Reference to this builder
		 */
		auto enable_circuit_breaker() -> config_builder&
		{
			config_.resilience.circuit_breaker.failure_threshold = 5;
			return *this;
		}

		/**
		 * @brief Sets circuit breaker failure threshold
		 * @param threshold Number of consecutive failures to open circuit
		 * @return Reference to this builder
		 */
		auto with_failure_threshold(std::size_t threshold) -> config_builder&
		{
			config_.resilience.circuit_breaker.failure_threshold = threshold;
			return *this;
		}

		/**
		 * @brief Sets circuit breaker open duration
		 * @param duration Time in open state before half-open
		 * @return Reference to this builder
		 */
		auto with_open_duration(std::chrono::seconds duration) -> config_builder&
		{
			// Note: common_system uses 'timeout' instead of 'open_duration'
			config_.resilience.circuit_breaker.timeout = duration;
			return *this;
		}

		/**
		 * @brief Enables work stealing
		 * @return Reference to this builder
		 */
		auto enable_work_stealing() -> config_builder&
		{
			config_.pool.enable_work_stealing = true;
			return *this;
		}

		/**
		 * @brief Sets work stealing parameters
		 * @param max_attempts Maximum steal attempts
		 * @param backoff Backoff duration between attempts
		 * @return Reference to this builder
		 */
		auto with_work_stealing_params(std::size_t max_attempts,
		                                std::chrono::microseconds backoff) -> config_builder&
		{
			config_.pool.max_steal_attempts = max_attempts;
			config_.pool.steal_backoff = backoff;
			return *this;
		}

		/**
		 * @brief Enables priority aging with default settings
		 * @return Reference to this builder
		 */
		auto enable_priority_aging() -> config_builder&
		{
			config_.scaling.priority_aging.enabled = true;
			return *this;
		}

		/**
		 * @brief Sets priority aging parameters
		 * @param interval Aging interval
		 * @param boost Boost per interval
		 * @param max_boost Maximum boost
		 * @return Reference to this builder
		 */
		auto with_priority_aging_params(std::chrono::milliseconds interval,
		                                 int boost,
		                                 int max_boost) -> config_builder&
		{
			config_.scaling.priority_aging.aging_interval = interval;
			config_.scaling.priority_aging.priority_boost_per_interval = boost;
			config_.scaling.priority_aging.max_priority_boost = max_boost;
			return *this;
		}

		/**
		 * @brief Enables auto-scaling
		 * @return Reference to this builder
		 */
		auto enable_auto_scaling() -> config_builder&
		{
			config_.scaling.auto_scaling_enabled = true;
			return *this;
		}

		/**
		 * @brief Sets DAG failure policy
		 * @param policy The failure policy to use
		 * @return Reference to this builder
		 */
		auto with_dag_failure_policy(dag_failure_policy policy) -> config_builder&
		{
			config_.dag.failure_policy = policy;
			return *this;
		}

		/**
		 * @brief Sets DAG retry parameters
		 * @param max_retries Maximum retry attempts
		 * @param delay Delay between retries
		 * @return Reference to this builder
		 */
		auto with_dag_retry_params(std::size_t max_retries,
		                            std::chrono::milliseconds delay) -> config_builder&
		{
			config_.dag.max_retries = max_retries;
			config_.dag.retry_delay = delay;
			return *this;
		}

		/**
		 * @brief Builds the final configuration
		 *
		 * Validates the configuration and returns it.
		 *
		 * @return The constructed configuration
		 * @throws std::invalid_argument if configuration is invalid
		 */
		auto build() -> thread_system_config
		{
			if (!config_.is_valid())
			{
				throw std::invalid_argument("Invalid thread_system_config");
			}
			return config_;
		}

	private:
		thread_system_config config_;
	};

	inline auto thread_system_config::builder() -> config_builder
	{
		return config_builder{};
	}

} // namespace kcenon::thread
