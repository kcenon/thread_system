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

#include <kcenon/thread/resilience/protected_job.h>
#include <kcenon/thread/core/error_handling.h>

#include <stdexcept>

namespace kcenon::thread
{
	protected_job::protected_job(
		std::unique_ptr<job> inner,
		std::shared_ptr<circuit_breaker> cb)
		: job("protected_" + (inner ? inner->get_name() : "unknown"))
		, inner_(std::move(inner))
		, cb_(std::move(cb))
	{
	}

	protected_job::~protected_job() = default;

	auto protected_job::do_work() -> common::VoidResult
	{
		if (!cb_)
		{
			return make_error_result(
				error_code::job_invalid,
				"Circuit breaker is null");
		}

		if (!inner_)
		{
			return make_error_result(
				error_code::job_invalid,
				"Inner job is null");
		}

		// Check if circuit breaker allows the request
		if (!cb_->allow_request())
		{
			return make_error_result(
				error_code::operation_canceled,
				"Circuit breaker is open");
		}

		// Create guard for automatic failure recording
		auto guard = cb_->make_guard();

		try
		{
			auto result = inner_->do_work();
			if (result.is_ok())
			{
				guard.record_success();
			}
			// If result is error, let guard destructor record failure
			return result;
		}
		catch (...)
		{
			// Guard destructor will automatically record failure
			throw;
		}
	}

	auto protected_job::get_name() const -> std::string
	{
		return job::get_name();
	}

} // namespace kcenon::thread
