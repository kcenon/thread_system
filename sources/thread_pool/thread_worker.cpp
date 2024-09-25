#include "thread_worker.h"

#include "logger.h"
#include "job_queue.h"

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#endif

using namespace log_module;

namespace thread_pool_module
{
	thread_worker::thread_worker(const bool& use_time_tag)
		: job_queue_(nullptr), use_time_tag_(use_time_tag)
	{
	}

	thread_worker::~thread_worker(void) {}

	auto thread_worker::set_job_queue(std::shared_ptr<job_queue> job_queue) -> void
	{
		job_queue_ = job_queue;
	}

	auto thread_worker::has_work() const -> bool
	{
		if (job_queue_ == nullptr)
		{
			return false;
		}

		return !job_queue_->empty();
	}

	auto thread_worker::do_work() -> void
	{
		if (job_queue_ == nullptr)
		{
			logger::handle().write(log_types::Error, "there is no job_queue");
		}

		auto [job_opt, error] = job_queue_->dequeue();
		if (!job_opt.has_value())
		{
			if (!job_queue_->is_stopped())
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
						("error dequeue job: {}", error.value_or("unknown error")));
				}
			}
		}

		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> time_point
			= std::nullopt;
		if (use_time_tag_)
		{
			time_point = std::chrono::high_resolution_clock::now();
		}

		job_opt.value()->set_job_queue(job_queue_);
		auto [worked, work_error] = job_opt.value()->do_work();
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
			logger::handle().write(
				log_types::Sequence,
#ifdef USE_STD_FORMAT
				std::format
#else
				fmt::format
#endif
				("job executed successfully: {} on thread_worker", job_opt.value()->get_name()),
				time_point);
		}
	}
} // namespace thread_pool_module