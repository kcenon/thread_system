// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/**
 * @file circuit_breaker_policy.h
 * @brief Pool policy implementing circuit breaker pattern for failure protection.
 *
 * @see pool_policy For the base interface
 */

#pragma once

#include "pool_policy.h"
#include <kcenon/common/resilience/circuit_breaker.h>
#include <kcenon/common/resilience/circuit_breaker_config.h>
#include <kcenon/common/resilience/circuit_state.h>

#include <atomic>
#include <memory>
#include <string>

namespace kcenon::thread
{
	// Import resilience types from common_system
	using common::resilience::circuit_breaker;
	using common::resilience::circuit_breaker_config;
	using common::resilience::circuit_state;

	/**
	 * @class circuit_breaker_policy
	 * @brief Pool policy that implements circuit breaker pattern for failure protection.
	 *
	 * @ingroup pool_policies
	 *
	 * This policy wraps the circuit breaker functionality as a composable pool policy,
	 * enabling circuit breaker protection without modifying the thread_pool class.
	 *
	 * ### Circuit Breaker Pattern
	 * The circuit breaker monitors job failures and automatically opens when a
	 * threshold is exceeded, preventing cascading failures:
	 * - CLOSED: Normal operation, all jobs allowed
	 * - OPEN: Failure threshold exceeded, jobs rejected immediately
	 * - HALF_OPEN: Testing recovery, limited jobs allowed
	 *
	 * ### Thread Safety
	 * All methods are thread-safe and can be called from any thread.
	 *
	 * ### Usage Example
	 * @code
	 * circuit_breaker_config config;
	 * config.failure_threshold = 5;
	 * config.open_duration = std::chrono::seconds{30};
	 *
	 * auto cb_policy = std::make_unique<circuit_breaker_policy>(config);
	 * pool->add_policy(std::move(cb_policy));
	 *
	 * // Now all jobs are protected by the circuit breaker
	 * // Jobs will be rejected when circuit is open
	 * @endcode
	 *
	 * @see pool_policy
	 * @see circuit_breaker
	 * @see circuit_breaker_config
	 */
	class circuit_breaker_policy : public pool_policy
	{
	public:
		/**
		 * @brief Constructs a circuit breaker policy with the given configuration.
		 * @param config Circuit breaker configuration.
		 */
		explicit circuit_breaker_policy(const circuit_breaker_config& config = {});

		/**
		 * @brief Constructs a circuit breaker policy with an existing circuit breaker.
		 * @param cb Shared pointer to an existing circuit breaker.
		 *
		 * This allows sharing a circuit breaker across multiple pools or components.
		 */
		explicit circuit_breaker_policy(std::shared_ptr<circuit_breaker> cb);

		/**
		 * @brief Destructor.
		 */
		~circuit_breaker_policy() override = default;

		// Non-copyable
		circuit_breaker_policy(const circuit_breaker_policy&) = delete;
		circuit_breaker_policy& operator=(const circuit_breaker_policy&) = delete;

		// Non-movable (std::atomic is not movable)
		circuit_breaker_policy(circuit_breaker_policy&&) = delete;
		circuit_breaker_policy& operator=(circuit_breaker_policy&&) = delete;

		// ============================================
		// pool_policy Interface
		// ============================================

		/**
		 * @brief Checks circuit state before allowing job enqueue.
		 * @param j Reference to the job being enqueued.
		 * @return common::ok() if allowed, error if circuit is open.
		 *
		 * When the circuit is open, jobs are rejected immediately with
		 * an error indicating the circuit breaker state.
		 */
		auto on_enqueue(job& j) -> common::VoidResult override;

		/**
		 * @brief Called when job starts executing.
		 * @param j Reference to the job.
		 *
		 * Records the start time for latency tracking.
		 */
		void on_job_start(job& j) override;

		/**
		 * @brief Records job completion in the circuit breaker.
		 * @param j Reference to the completed job.
		 * @param success True if job succeeded.
		 * @param error Exception pointer if job failed.
		 *
		 * This updates the circuit breaker state based on success/failure.
		 */
		void on_job_complete(job& j, bool success, const std::exception* error = nullptr) override;

		/**
		 * @brief Gets the policy name.
		 * @return "circuit_breaker_policy"
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
		 */
		void set_enabled(bool enabled) override;

		// ============================================
		// Circuit Breaker Specific Methods
		// ============================================

		/**
		 * @brief Checks if the circuit is accepting work.
		 * @return True if circuit is closed or half-open with capacity.
		 */
		[[nodiscard]] auto is_accepting_work() const -> bool;

		/**
		 * @brief Gets the current circuit state.
		 * @return Current circuit_state.
		 */
		[[nodiscard]] auto get_state() const -> circuit_state;

		/**
		 * @brief Gets the underlying circuit breaker.
		 * @return Shared pointer to the circuit breaker.
		 *
		 * Useful for sharing the circuit breaker with other components
		 * or for advanced circuit breaker operations.
		 */
		[[nodiscard]] auto get_circuit_breaker() const -> std::shared_ptr<circuit_breaker>;

	private:
		std::shared_ptr<circuit_breaker> circuit_breaker_;
		std::atomic<bool> enabled_{true};
	};

} // namespace kcenon::thread
