// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

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
