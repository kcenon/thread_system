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

#include <kcenon/thread/dag/dag_job.h>

#include <format>

namespace kcenon::thread
{

std::atomic<job_id> dag_job::next_dag_id_{1};

dag_job::dag_job(const std::string& name)
	: job(name)
	, dag_id_(next_dag_id_.fetch_add(1, std::memory_order_relaxed))
	, submit_time_(std::chrono::steady_clock::now())
{
}

dag_job::~dag_job() = default;

auto dag_job::get_info() const -> dag_job_info
{
	dag_job_info info;
	info.id = dag_id_;
	info.name = get_name();
	info.state = get_state();
	info.dependencies = dependencies_;
	info.submit_time = submit_time_;
	info.start_time = start_time_;
	info.end_time = end_time_;
	info.error_message = error_message_;

	if (has_result())
	{
		info.result = result_;
	}

	return info;
}

auto dag_job::do_work() -> common::VoidResult
{
	// Check cancellation token
	if (cancellation_token_.is_cancelled())
	{
		set_state(dag_job_state::cancelled);
		return make_error_result(error_code::operation_canceled, "Job was cancelled");
	}

	// If no work function is set, return success
	if (!work_func_)
	{
		return common::ok();
	}

	// Execute the work function
	try
	{
		return work_func_();
	}
	catch (const std::exception& e)
	{
		set_error_message(e.what());
		return make_error_result(error_code::job_execution_failed, e.what());
	}
	catch (...)
	{
		set_error_message("Unknown exception");
		return make_error_result(error_code::job_execution_failed, "Unknown exception during job execution");
	}
}

auto dag_job::to_string() const -> std::string
{
	return std::format("[dag_job: {} (id={}, state={})]",
	                   get_name(),
	                   dag_id_,
	                   dag_job_state_to_string(get_state()));
}

} // namespace kcenon::thread
