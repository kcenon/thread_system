// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

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
