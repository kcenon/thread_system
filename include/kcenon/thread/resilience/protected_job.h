// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/**
 * @file protected_job.h
 * @brief Job wrapper integrating circuit breaker protection.
 *
 * @see circuit_breaker_policy
 */

#pragma once

#include <kcenon/common/resilience/circuit_breaker.h>
#include <kcenon/thread/core/job.h>

#include <memory>

namespace kcenon::thread
{
	using common::resilience::circuit_breaker;

	/**
	 * @class protected_job
	 * @brief A job wrapper that integrates circuit breaker protection.
	 *
	 * This class wraps an existing job and adds circuit breaker protection.
	 * Before executing the inner job, it checks if the circuit breaker allows
	 * the request. After execution, it records success or failure.
	 *
	 * ### Thread Safety
	 * Thread safety depends on the wrapped job. The circuit breaker integration
	 * itself is thread-safe.
	 *
	 * ### Usage Example
	 * @code
	 * auto cb = std::make_shared<circuit_breaker>();
	 * auto inner = std::make_unique<my_job>();
	 * auto protected = std::make_unique<protected_job>(std::move(inner), cb);
	 * pool->enqueue(std::move(protected));
	 * @endcode
	 *
	 * @see circuit_breaker
	 * @see job
	 */
	class protected_job : public job
	{
	public:
		/**
		 * @brief Constructs a protected job wrapper.
		 * @param inner The job to wrap and protect.
		 * @param cb The circuit breaker to use for protection.
		 */
		protected_job(
			std::unique_ptr<job> inner,
			std::shared_ptr<circuit_breaker> cb);

		/**
		 * @brief Destructor.
		 */
		~protected_job() override;

		/**
		 * @brief Executes the wrapped job with circuit breaker protection.
		 * @return Result from the inner job, or error if circuit is open.
		 *
		 * Behavior:
		 * 1. Checks circuit breaker - returns error if open
		 * 2. Executes inner job
		 * 3. Records success or failure to circuit breaker
		 * 4. Returns inner job result
		 */
		auto do_work() -> common::VoidResult override;

		/**
		 * @brief Gets the name of this job.
		 * @return Name of the protected job (includes inner job name).
		 */
		[[nodiscard]] auto get_name() const -> std::string;

	private:
		std::unique_ptr<job> inner_;
		std::shared_ptr<circuit_breaker> cb_;
	};

} // namespace kcenon::thread
