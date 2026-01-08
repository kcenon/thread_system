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

#include "circuit_breaker.h"
#include <kcenon/thread/core/job.h>

#include <memory>

namespace kcenon::thread
{
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
