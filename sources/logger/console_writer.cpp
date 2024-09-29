#include "console_writer.h"

#include "message_job.h"

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#endif

namespace log_module
{
	console_writer::console_writer(void) : job_queue_(std::make_shared<job_queue>()) {}

	auto console_writer::has_work() const -> bool { return !job_queue_->empty(); }

	auto console_writer::before_start() -> std::tuple<bool, std::optional<std::string>>
	{
		if (job_queue_ == nullptr)
		{
			return { false, "error creating job_queue" };
		}

		job_queue_->set_notify(!wake_interval_.has_value());

		return { true, std::nullopt };
	}

	auto console_writer::do_work() -> std::tuple<bool, std::optional<std::string>>
	{
		if (job_queue_ == nullptr)
		{
			return { false, "there is no job_queue" };
		}

		std::string console_buffer = "";
		auto remaining_logs = job_queue_->dequeue_all();
		while (!remaining_logs.empty())
		{
			auto current_job = std::move(remaining_logs.front());
			remaining_logs.pop();

			auto current_log
				= std::unique_ptr<message_job>(static_cast<message_job*>(current_job.release()));

			auto [worked, work_error] = current_log->do_work();
			if (!worked)
			{
				continue;
			}

#ifdef USE_STD_FORMAT
			std::format_to
#else
			fmt::format_to
#endif
				(std::back_inserter(console_buffer), "{}", current_log->message(true));
		}

#ifdef USE_STD_FORMAT
		std::cout << console_buffer;
#else
		fmt::print("{}", console_buffer);
#endif

		return { true, std::nullopt };
	}
}