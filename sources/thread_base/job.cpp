#include "job.h"

#include "job_queue.h"

job::job(const std::function<std::tuple<bool, std::optional<std::string>>(void)>& callback,
		 const std::string& name)
	: name_(name), callback_(callback)
{
}

job::~job(void) {}

auto job::get_ptr(void) -> std::shared_ptr<job> { return shared_from_this(); }

auto job::get_name(void) const -> std::string { return name_; }

auto job::do_work(void) -> std::tuple<bool, std::optional<std::string>>
{
	if (callback_ == nullptr)
	{
		return { false, "cannot execute job without callback" };
	}

	try
	{
		return callback_();
	}
	catch (const std::exception& e)
	{
		return { false, std::string(e.what()) };
	}
	catch (...)
	{
		return { false, "unknown error" };
	}
}

auto job::set_job_queue(const std::shared_ptr<job_queue>& job_queue) -> void
{
	job_queue_ = job_queue;
}

auto job::get_job_queue(void) const -> std::shared_ptr<job_queue> { return job_queue_.lock(); }
