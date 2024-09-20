#include "priority_job.h"

#include "priority_job_queue.h"

template <typename priority_type>
priority_job<priority_type>::priority_job(
	const std::function<std::tuple<bool, std::optional<std::string>>(void)>& callback,
	priority_type priority,
	const std::string& name)
	: job(callback, name), priority_(priority)
{
}

template <typename priority_type> priority_job<priority_type>::~priority_job(void) {}

template <typename priority_type>
auto priority_job<priority_type>::priority() const -> priority_type
{
	return priority_;
}

template <typename priority_type>
auto priority_job<priority_type>::set_job_queue(const std::shared_ptr<job_queue>& job_queue) -> void
{
	job_queue_ = std::dynamic_pointer_cast<priority_job_queue<priority_type>>(job_queue);
}

template <typename priority_type>
auto priority_job<priority_type>::get_job_queue(void) const -> std::shared_ptr<job_queue>
{
	return std::dynamic_pointer_cast<job_queue>(job_queue_.lock());
}
