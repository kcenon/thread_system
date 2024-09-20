#include "priority_thread_worker.h"

#include "logger.h"
#include "priority_job_queue.h"

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#endif

template <typename priority_type>
priority_thread_worker<priority_type>::priority_thread_worker(std::vector<priority_type> priorities,
															  const bool& use_time_tag)
	: job_queue_(nullptr), priorities_(priorities), use_time_tag_(use_time_tag)
{
}

template <typename priority_type>
priority_thread_worker<priority_type>::~priority_thread_worker(void)
{
}

template <typename priority_type>
auto priority_thread_worker<priority_type>::set_job_queue(
	std::shared_ptr<priority_job_queue<priority_type>> job_queue) -> void
{
	job_queue_ = job_queue;
}

template <typename priority_type>
auto priority_thread_worker<priority_type>::priorities(void) const -> std::vector<priority_type>
{
	return priorities_;
}

template <typename priority_type>
auto priority_thread_worker<priority_type>::has_work() const -> bool
{
	if (job_queue_ == nullptr)
	{
		return false;
	}

	return !job_queue_->empty(priorities_);
}

template <typename priority_type> auto priority_thread_worker<priority_type>::do_work() -> void
{
	if (job_queue_ == nullptr)
	{
		logger::handle().write(log_types::Error, "there is no job_queue");
	}

	auto [job_opt, error] = job_queue_->dequeue(priorities_);
	if (!job_opt.has_value())
	{
		if (!job_queue_->is_stopped())
		{
			if (logger::handle().get_file_target() >= log_types::Error
				|| logger::handle().get_console_target() >= log_types::Error)
			{
				logger::handle().write(log_types::Error,
#ifdef USE_STD_FORMAT
									   std::format
#else
									   fmt::format
#endif
									   ("cannot dequeue job: {}", error.value_or("unknown error")));
			}
		}

		return;
	}

	auto current_job = std::move(job_opt.value());
	if (current_job == nullptr)
	{
		if (logger::handle().get_file_target() >= log_types::Error
			|| logger::handle().get_console_target() >= log_types::Error)
		{
			logger::handle().write(log_types::Error, "error executing job: nullptr");
		}

		return;
	}

	std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> time_point
		= std::nullopt;
	if (use_time_tag_)
	{
		time_point = std::chrono::high_resolution_clock::now();
	}

	current_job->set_job_queue(job_queue_);
	auto [worked, work_error] = current_job->do_work();
	if (!worked)
	{
		if (logger::handle().get_file_target() >= log_types::Error
			|| logger::handle().get_console_target() >= log_types::Error)
		{
			logger::handle().write(
				log_types::Error,
#ifdef USE_STD_FORMAT
				std::format
#else
				fmt::format
#endif
				("error executing job: {}", work_error.value_or("unknown error")),
				time_point);
		}
	}

	if (logger::handle().get_file_target() >= log_types::Sequence
		|| logger::handle().get_console_target() >= log_types::Sequence)
	{
		logger::handle().write(log_types::Sequence,
#ifdef USE_STD_FORMAT
							   std::format
#else
							   fmt::format
#endif
							   ("job executed successfully: {}[{}] on priority_thread_worker",
								current_job->get_name(), current_job->priority()),
							   time_point);
	}
}