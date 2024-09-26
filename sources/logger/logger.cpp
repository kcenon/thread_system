#include "logger.h"

#ifdef USE_STD_FORMAT
#include <format>
#else
#include "fmt/format.h"
#endif

#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace log_module
{
#pragma region singleton
	std::unique_ptr<logger> logger::handle_;
	std::once_flag logger::once_;

	auto logger::handle(void) -> logger&
	{
		call_once(once_, []() { handle_.reset(new logger); });

		return *handle_.get();
	}

	auto logger::destroy() -> void { handle_.reset(); }

#pragma endregion

	logger::logger()
		: log_queue_(std::make_shared<job_queue>())
		, file_log_type_(log_types::Error)
		, console_log_type_(log_types::Information)
	{
	}

	auto logger::set_title(const std::string& title) -> void { title_ = title; }

	auto logger::set_file_target(const log_types& type) -> void { file_log_type_ = type; }

	auto logger::get_file_target() const -> log_types { return file_log_type_; }

	auto logger::set_console_target(const log_types& type) -> void { console_log_type_ = type; }

	auto logger::get_console_target() const -> log_types { return console_log_type_; }

	auto logger::set_max_lines(uint32_t max_lines) -> void { max_lines_ = max_lines; }

	auto logger::get_max_lines() const -> uint32_t { return max_lines_; }

	auto logger::set_use_backup(bool use_backup) -> void { use_backup_ = use_backup; }

	auto logger::get_use_backup() const -> bool { return use_backup_; }

	auto logger::time_point() -> std::chrono::time_point<std::chrono::high_resolution_clock>
	{
		return std::chrono::high_resolution_clock::now();
	}

	auto logger::write(log_types type,
					   const std::string& message,
					   std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>>
						   start_time) -> void
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

	[[nodiscard]] auto logger::has_work() const -> bool { return !log_queue_->empty(); }

	auto logger::before_start() -> std::tuple<bool, std::optional<std::string>>
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

		if (file_log_type_ > log_types::None)
		{
			write_to_file(buffer);
		}

		if (console_log_type_ > log_types::None)
		{
			write_to_console(buffer);
		}

		return { true, std::nullopt };
	}

	auto logger::do_work() -> std::tuple<bool, std::optional<std::string>>
	{
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

		write_to_console(console_buffer);
		write_to_file(file_buffer);

		return { true, std::nullopt };
	}

	auto logger::after_stop() -> std::tuple<bool, std::optional<std::string>>
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

		if (file_log_type_ > log_types::None)
		{
			write_to_file(buffer);
		}

		if (console_log_type_ > log_types::None)
		{
			write_to_console(buffer);
		}

		return { true, std::nullopt };
	}

	auto logger::write_to_file(const std::string& message) -> void
	{
		if (message.empty())
		{
			return;
		}

		const auto now = std::chrono::system_clock::now();
		const auto today = std::chrono::floor<std::chrono::days>(now);
		const auto year_month_day = std::chrono::year_month_day{ today };

		const int year = static_cast<int>(year_month_day.year());
		const unsigned month = static_cast<unsigned>(year_month_day.month());
		const unsigned day = static_cast<unsigned>(year_month_day.day());

		const std::string file_name =
#ifdef USE_STD_FORMAT
			std::format
#else
			fmt::format
#endif
			("{}_{:04d}-{:02d}-{:02d}.log", title_, year, month, day);
		const std::string backup_name =
#ifdef USE_STD_FORMAT
			std::format
#else
			fmt::format
#endif
			("{}_{:04d}-{:02d}-{:02d}.backup", title_, year, month, day);

		if (max_lines_ == 0)
		{
			std::ofstream outfile;
			outfile.open(file_name, std::ios_base::app);
			if (!outfile.is_open())
			{
				return;
			}

			outfile << message;
			outfile.close();
		}

		log_buffer_.push_back(message);

		std::ofstream backup_file;
		if (use_backup_)
		{
			backup_file.open(backup_name, std::ios_base::app);
		}

		auto current_lines = log_buffer_.size();
		for (auto i = current_lines; i > max_lines_; --i)
		{
			if (use_backup_ && backup_file.is_open())
			{
				backup_file << log_buffer_.front();
			}
			log_buffer_.pop_front();
		}

		if (backup_file.is_open())
		{
			backup_file.close();
		}

		std::ofstream outfile;
		outfile.open(file_name, std::ios_base::trunc);
		if (!outfile.is_open())
		{
			return;
		}

		for (const auto& line : log_buffer_)
		{
			outfile << line;
		}
		outfile.close();
	}

	auto logger::write_to_console(const std::string& message) -> void
	{
		if (message.empty())
		{
			return;
		}

#ifdef USE_STD_FORMAT
		std::cout << message;
#else
		fmt::print("{}", message);
#endif
	}
} // namespace log_module