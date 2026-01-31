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

/**
 * @file config_example.cpp
 * @brief Examples demonstrating the unified configuration system
 *
 * This file shows various ways to configure thread_system using the
 * new unified configuration structure and builder pattern.
 */

#include <kcenon/thread/thread_config.h>
#include <iostream>

namespace
{
	/**
	 * @brief Example 1: Default configuration
	 *
	 * Creates a configuration with default values for all settings.
	 */
	void example_default_config()
	{
		std::cout << "\n=== Example 1: Default Configuration ===\n";

		kcenon::thread::thread_system_config config;

		std::cout << "Worker count: " << config.pool.worker_count << "\n";
		std::cout << "Queue capacity: " << config.pool.queue_capacity << "\n";
		std::cout << "Backpressure policy: "
		          << kcenon::thread::backpressure_policy_to_string(config.pool.backpressure.policy)
		          << "\n";
	}

	/**
	 * @brief Example 2: Using builder pattern for basic configuration
	 *
	 * Demonstrates the fluent builder API for simple configurations.
	 */
	void example_builder_basic()
	{
		std::cout << "\n=== Example 2: Builder Pattern (Basic) ===\n";

		auto config = kcenon::thread::thread_system_config::builder()
		                  .with_worker_count(8)
		                  .with_queue_capacity(5000)
		                  .build();

		std::cout << "Worker count: " << config.pool.worker_count << "\n";
		std::cout << "Queue capacity: " << config.pool.queue_capacity << "\n";
		std::cout << "Configuration is valid: " << (config.is_valid() ? "yes" : "no") << "\n";
	}

	/**
	 * @brief Example 3: Configuring backpressure
	 *
	 * Shows how to enable and configure backpressure mechanisms.
	 */
	void example_backpressure_config()
	{
		std::cout << "\n=== Example 3: Backpressure Configuration ===\n";

		auto config = kcenon::thread::thread_system_config::builder()
		                  .with_worker_count(4)
		                  .with_queue_capacity(1000)
		                  .enable_backpressure()
		                  .with_backpressure_policy(kcenon::thread::backpressure_policy::adaptive)
		                  .with_watermarks(0.5, 0.8)
		                  .build();

		std::cout << "Backpressure enabled: yes\n";
		std::cout << "Policy: "
		          << kcenon::thread::backpressure_policy_to_string(config.pool.backpressure.policy)
		          << "\n";
		std::cout << "Low watermark: " << config.pool.backpressure.low_watermark << "\n";
		std::cout << "High watermark: " << config.pool.backpressure.high_watermark << "\n";
	}

	/**
	 * @brief Example 4: Configuring circuit breaker
	 *
	 * Demonstrates resilience configuration with circuit breaker.
	 */
	void example_circuit_breaker_config()
	{
		std::cout << "\n=== Example 4: Circuit Breaker Configuration ===\n";

		auto config = kcenon::thread::thread_system_config::builder()
		                  .with_worker_count(4)
		                  .enable_circuit_breaker()
		                  .with_failure_threshold(5)
		                  .with_open_duration(std::chrono::seconds{30})
		                  .build();

		std::cout << "Circuit breaker enabled: yes\n";
		std::cout << "Failure threshold: "
		          << config.resilience.circuit_breaker.failure_threshold << "\n";
		std::cout << "Open duration: "
		          << config.resilience.circuit_breaker.timeout.count() << "s\n";
		std::cout << "Half-open max requests: "
		          << config.resilience.circuit_breaker.half_open_max_requests << "\n";
	}

	/**
	 * @brief Example 5: Configuring work stealing
	 *
	 * Shows how to enable and tune work stealing parameters.
	 */
	void example_work_stealing_config()
	{
		std::cout << "\n=== Example 5: Work Stealing Configuration ===\n";

		auto config = kcenon::thread::thread_system_config::builder()
		                  .with_worker_count(8)
		                  .enable_work_stealing()
		                  .with_work_stealing_params(3, std::chrono::microseconds{50})
		                  .build();

		std::cout << "Work stealing enabled: " << (config.pool.enable_work_stealing ? "yes" : "no")
		          << "\n";
		std::cout << "Max steal attempts: " << config.pool.max_steal_attempts << "\n";
		std::cout << "Steal backoff: " << config.pool.steal_backoff.count() << "μs\n";
	}

	/**
	 * @brief Example 6: Configuring priority aging
	 *
	 * Demonstrates dynamic scaling with priority aging.
	 */
	void example_priority_aging_config()
	{
		std::cout << "\n=== Example 6: Priority Aging Configuration ===\n";

		auto config = kcenon::thread::thread_system_config::builder()
		                  .with_worker_count(4)
		                  .enable_priority_aging()
		                  .with_priority_aging_params(std::chrono::seconds{1}, 1, 3)
		                  .build();

		std::cout << "Priority aging enabled: "
		          << (config.scaling.priority_aging.enabled ? "yes" : "no") << "\n";
		std::cout << "Aging interval: " << config.scaling.priority_aging.aging_interval.count()
		          << "ms\n";
		std::cout << "Boost per interval: "
		          << config.scaling.priority_aging.priority_boost_per_interval << "\n";
		std::cout << "Max boost: " << config.scaling.priority_aging.max_priority_boost << "\n";
	}

	/**
	 * @brief Example 7: DAG configuration
	 *
	 * Shows how to configure DAG scheduler behavior.
	 */
	void example_dag_config()
	{
		std::cout << "\n=== Example 7: DAG Configuration ===\n";

		auto config = kcenon::thread::thread_system_config::builder()
		                  .with_worker_count(4)
		                  .with_dag_failure_policy(kcenon::thread::dag_failure_policy::retry)
		                  .with_dag_retry_params(3, std::chrono::milliseconds{1000})
		                  .build();

		std::cout << "DAG failure policy: "
		          << kcenon::thread::dag_failure_policy_to_string(config.dag.failure_policy)
		          << "\n";
		std::cout << "Max retries: " << config.dag.max_retries << "\n";
		std::cout << "Retry delay: " << config.dag.retry_delay.count() << "ms\n";
		std::cout << "Detect cycles: " << (config.dag.detect_cycles ? "yes" : "no") << "\n";
		std::cout << "Execute in parallel: " << (config.dag.execute_in_parallel ? "yes" : "no")
		          << "\n";
	}

	/**
	 * @brief Example 8: Production configuration
	 *
	 * A realistic production configuration combining multiple features.
	 */
	void example_production_config()
	{
		std::cout << "\n=== Example 8: Production Configuration ===\n";

		auto config = kcenon::thread::thread_system_config::builder()
		                  .with_worker_count(std::thread::hardware_concurrency())
		                  .with_queue_capacity(10000)
		                  .enable_backpressure()
		                  .with_backpressure_policy(kcenon::thread::backpressure_policy::adaptive)
		                  .with_watermarks(0.6, 0.85)
		                  .enable_circuit_breaker()
		                  .with_failure_threshold(10)
		                  .with_open_duration(std::chrono::seconds{60})
		                  .enable_work_stealing()
		                  .with_work_stealing_params(5, std::chrono::microseconds{100})
		                  .enable_priority_aging()
		                  .with_priority_aging_params(std::chrono::seconds{2}, 1, 5)
		                  .with_dag_failure_policy(kcenon::thread::dag_failure_policy::continue_others)
		                  .build();

		std::cout << "Worker count: " << config.pool.worker_count << "\n";
		std::cout << "Queue capacity: " << config.pool.queue_capacity << "\n";
		std::cout << "Backpressure: adaptive (low=" << config.pool.backpressure.low_watermark
		          << ", high=" << config.pool.backpressure.high_watermark << ")\n";
		std::cout << "Circuit breaker: threshold=" << config.resilience.circuit_breaker.failure_threshold
		          << ", open_duration=" << config.resilience.circuit_breaker.timeout.count()
		          << "s\n";
		std::cout << "Work stealing: enabled, max_attempts=" << config.pool.max_steal_attempts
		          << "\n";
		std::cout << "Priority aging: enabled, interval="
		          << config.scaling.priority_aging.aging_interval.count() << "ms\n";
		std::cout << "DAG failure policy: "
		          << kcenon::thread::dag_failure_policy_to_string(config.dag.failure_policy)
		          << "\n";
		std::cout << "Configuration is valid: " << (config.is_valid() ? "yes" : "no") << "\n";
	}

	/**
	 * @brief Example 9: Direct struct initialization
	 *
	 * Shows that you can still initialize the config struct directly.
	 */
	void example_direct_initialization()
	{
		std::cout << "\n=== Example 9: Direct Initialization ===\n";

		kcenon::thread::thread_system_config config;
		config.pool.worker_count = 6;
		config.pool.queue_capacity = 2000;
		config.pool.backpressure.policy = kcenon::thread::backpressure_policy::drop_oldest;
		config.resilience.circuit_breaker.failure_threshold = 3;
		config.scaling.priority_aging.enabled = true;

		std::cout << "Worker count: " << config.pool.worker_count << "\n";
		std::cout << "Queue capacity: " << config.pool.queue_capacity << "\n";
		std::cout << "Backpressure policy: "
		          << kcenon::thread::backpressure_policy_to_string(config.pool.backpressure.policy)
		          << "\n";
		std::cout << "Circuit breaker threshold: "
		          << config.resilience.circuit_breaker.failure_threshold << "\n";
		std::cout << "Priority aging enabled: "
		          << (config.scaling.priority_aging.enabled ? "yes" : "no") << "\n";
	}
} // namespace

int main()
{
	std::cout << "========================================\n";
	std::cout << "Thread System Configuration Examples\n";
	std::cout << "========================================\n";

	example_default_config();
	example_builder_basic();
	example_backpressure_config();
	example_circuit_breaker_config();
	example_work_stealing_config();
	example_priority_aging_config();
	example_dag_config();
	example_production_config();
	example_direct_initialization();

	std::cout << "\n========================================\n";
	std::cout << "All examples completed successfully!\n";
	std::cout << "========================================\n";

	return 0;
}
