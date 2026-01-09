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

auto dag_job_builder::build() -> std::unique_ptr<dag_job>
{
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

	return job;
}

} // namespace kcenon::thread
