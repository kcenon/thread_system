// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#include <kcenon/thread/dag/dag_job_builder.h>

namespace kcenon::thread
{

dag_job_builder::dag_job_builder(const std::string& name)
	: name_(name)
{
}

auto dag_job_builder::work(std::function<common::VoidResult()> callable) -> dag_job_builder&
{
	work_func_ = std::move(callable);
	return *this;
}

auto dag_job_builder::depends_on(job_id dependency) -> dag_job_builder&
{
	if (dependency != INVALID_JOB_ID)
	{
		dependencies_.push_back(dependency);
	}
	return *this;
}

auto dag_job_builder::depends_on(std::initializer_list<job_id> dependencies) -> dag_job_builder&
{
	for (const auto& dep : dependencies)
	{
		if (dep != INVALID_JOB_ID)
		{
			dependencies_.push_back(dep);
		}
	}
	return *this;
}

auto dag_job_builder::depends_on(const std::vector<job_id>& dependencies) -> dag_job_builder&
{
	for (const auto& dep : dependencies)
	{
		if (dep != INVALID_JOB_ID)
		{
			dependencies_.push_back(dep);
		}
	}
	return *this;
}

auto dag_job_builder::on_failure(std::function<common::VoidResult()> fallback) -> dag_job_builder&
{
	fallback_func_ = std::move(fallback);
	return *this;
}

auto dag_job_builder::is_valid() const -> bool
{
	return work_func_ != nullptr || work_with_result_func_ != nullptr;
}

auto dag_job_builder::get_validation_error() const -> std::optional<std::string>
{
	if (!work_func_ && !work_with_result_func_)
	{
		return "No work function specified. Use work() or work_with_result() before build().";
	}
	return std::nullopt;
}

auto dag_job_builder::build() -> std::unique_ptr<dag_job>
{
	// Validate configuration
	if (!is_valid())
	{
		return nullptr;
	}

	auto job = std::make_unique<dag_job>(name_);

	// Set dependencies
	job->add_dependencies(dependencies_);

	// Set work function
	if (work_with_result_func_)
	{
		// Capture the work_with_result function and create a wrapper
		auto result_func = std::move(work_with_result_func_);
		job->set_work([j = job.get(), func = std::move(result_func)]() -> common::VoidResult {
			return func(*j);
		});
	}
	else if (work_func_)
	{
		job->set_work(std::move(work_func_));
	}

	// Set fallback function
	if (fallback_func_)
	{
		job->set_fallback(std::move(fallback_func_));
	}

	// Reset builder for reuse (keep name)
	reset();

	return job;
}

auto dag_job_builder::reset() -> dag_job_builder&
{
	work_func_ = nullptr;
	work_with_result_func_ = nullptr;
	fallback_func_ = nullptr;
	dependencies_.clear();
	has_return_type_ = false;
	return *this;
}

} // namespace kcenon::thread
