#include "console_writer.h"

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#endif

namespace log_module
{
	console_writer::console_writer(void) : job_queue_(std::make_shared<job_queue>()) {}

	auto console_writer::has_work() const -> bool { return !job_queue_->empty(); }

	auto console_writer::do_work() -> std::tuple<bool, std::optional<std::string>>
	{
		if (job_queue_ == nullptr)
		{
			return { false, "there is no job_queue" };
		}

		auto [job_opt, error] = job_queue_->dequeue();
		if (!job_opt.has_value())
		{
			if (!job_queue_->is_stopped())
			{
				return { false,
#ifdef USE_STD_FORMAT
						 std::format
#else
						 fmt::format
#endif
						 ("error dequeue job: {}", error.value_or("unknown error")) };
			}

			return { true, std::nullopt };
		}

		auto current_job = std::move(job_opt.value());
		if (current_job == nullptr)
		{
			return { false, "error executing job: nullptr" };
		}

		return current_job->do_work();
	}
}