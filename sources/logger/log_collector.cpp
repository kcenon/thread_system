#include "log_collector.h"

#include "log_job.h"
#include "file_job.h"
#include "console_job.h"

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#endif

namespace log_module
{
	log_collector::log_collector(void)
		: log_queue_(std::make_shared<job_queue>())
		, title_("log")
		, use_backup_(false)
		, max_lines_(0)
		, file_log_type_(log_types::Error)
		, console_log_type_(log_types::Information)
	{
	}

	auto log_collector::set_title(const std::string& title) -> void { title_ = title; }

	auto log_collector::get_title() const -> std::string { return title_; }

	auto log_collector::set_use_backup(const bool& use_backup) -> void { use_backup_ = use_backup; }

	auto log_collector::get_use_backup() const -> bool { return use_backup_; }

	auto log_collector::set_max_lines(const uint32_t& max_lines) -> void { max_lines_ = max_lines; }

	auto log_collector::get_max_lines() const -> uint32_t { return max_lines_; }

	auto log_collector::set_console_target(const log_types& type) -> void
	{
		console_log_type_ = type;
	}

	auto log_collector::get_console_target() const -> log_types { return console_log_type_; }

	auto log_collector::set_file_target(const log_types& type) -> void { file_log_type_ = type; }

	auto log_collector::get_file_target() const -> log_types { return file_log_type_; }

	auto log_collector::set_console_queue(std::shared_ptr<job_queue> queue) -> void
	{
		console_queue_ = queue;
	}

	auto log_collector::set_file_queue(std::shared_ptr<job_queue> queue) -> void
	{
		file_queue_ = queue;
	}

	auto log_collector::write(
		log_types type,
		const std::string& message,
		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time)
		-> void
	{
		std::unique_ptr<log_job> new_log_job;

		try
		{
			new_log_job = std::make_unique<log_job>(message, type, start_time);
		}
		catch (const std::bad_alloc& e)
		{
			std::cerr << "error allocating log job: " << e.what() << std::endl;
			return;
		}

		auto [enqueued, enqueue_error] = log_queue_->enqueue(std::move(new_log_job));
		if (!enqueued)
		{
			std::cerr << "error enqueuing log job: " << enqueue_error.value_or("unknown error")
					  << std::endl;
		}
	}

	auto log_collector::has_work() const -> bool { return !log_queue_->empty(); }

	auto log_collector::before_start() -> std::tuple<bool, std::optional<std::string>>
	{
		log_queue_->set_notify(!wake_interval_.has_value());

		log_job job("START");
		auto [worked, work_error] = job.do_work();
		std::string log
			= (worked) ? job.log() : work_error.value_or("Unknown error to convert to log message");

		std::string buffer = "";

#ifdef USE_STD_FORMAT
		std::format_to
#else
		fmt::format_to
#endif
			(std::back_inserter(buffer), "{}\n", log);

		if (console_log_type_ > log_types::None)
		{
			auto console_queue = console_queue_.lock();
			if (console_queue != nullptr && !buffer.empty())
			{
				console_queue->enqueue(std::make_unique<console_job>(buffer));
			}
		}

		if (file_log_type_ > log_types::None)
		{
			auto file_queue = file_queue_.lock();
			if (file_queue != nullptr && !buffer.empty())
			{
				file_queue->enqueue(
					std::make_unique<file_job>(title_, buffer, max_lines_, use_backup_));
			}
		}

		return { true, std::nullopt };
	}

	auto log_collector::do_work() -> std::tuple<bool, std::optional<std::string>>
	{
		if (log_queue_ == nullptr)
		{
			return { false, "there is no job_queue" };
		}

		auto remaining_logs = log_queue_->dequeue_all();

		std::string file_buffer = "";
		std::string console_buffer = "";

		while (!remaining_logs.empty())
		{
			auto current_job = std::move(remaining_logs.front());
			remaining_logs.pop();

			auto current_log
				= std::unique_ptr<log_job>(static_cast<log_job*>(current_job.release()));

			auto [worked, work_error] = current_log->do_work();
			std::string log = (worked)
								  ? current_log->log()
								  : work_error.value_or("Unknown error to convert to log message");

			if (current_log->get_type() <= file_log_type_)
			{
#ifdef USE_STD_FORMAT
				std::format_to
#else
				fmt::format_to
#endif
					(std::back_inserter(file_buffer), "{}\n", log);
			}

			if (current_log->get_type() <= console_log_type_)
			{
#ifdef USE_STD_FORMAT
				std::format_to
#else
				fmt::format_to
#endif
					(std::back_inserter(console_buffer), "{}\n", log);
			}
		}

		auto console_queue = console_queue_.lock();
		if (console_queue != nullptr && !console_buffer.empty())
		{
			console_queue->enqueue(std::make_unique<console_job>(console_buffer));
		}

		auto file_queue = file_queue_.lock();
		if (file_queue != nullptr && !file_buffer.empty())
		{
			file_queue->enqueue(
				std::make_unique<file_job>(title_, file_buffer, max_lines_, use_backup_));
		}

		return { true, std::nullopt };
	}

	auto log_collector::after_stop() -> std::tuple<bool, std::optional<std::string>>
	{
		log_job job("STOP");
		auto [worked, work_error] = job.do_work();
		std::string log
			= (worked) ? job.log() : work_error.value_or("Unknown error to convert to log message");

		std::string buffer = "";

#ifdef USE_STD_FORMAT
		std::format_to
#else
		fmt::format_to
#endif
			(std::back_inserter(buffer), "{}\n", log);

		if (console_log_type_ > log_types::None)
		{
			auto console_queue = console_queue_.lock();
			if (console_queue != nullptr && !buffer.empty())
			{
				console_queue->enqueue(std::make_unique<console_job>(buffer));
			}
		}

		if (file_log_type_ > log_types::None)
		{
			auto file_queue = file_queue_.lock();
			if (file_queue != nullptr && !buffer.empty())
			{
				file_queue->enqueue(
					std::make_unique<file_job>(title_, buffer, max_lines_, use_backup_));
			}
		}

		return { true, std::nullopt };
	}
}