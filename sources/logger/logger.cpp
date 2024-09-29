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
		: collector_(std::make_shared<log_collector>())
		, console_writer_(std::make_shared<console_writer>())
		, file_writer_(std::make_shared<file_writer>())
	{
	}

	auto logger::set_title(const std::string& title) -> void
	{
		if (file_writer_ == nullptr)
		{
			return;
		}

		file_writer_->set_title(title);
	}

	auto logger::set_file_target(const log_types& type) -> void
	{
		if (collector_ == nullptr)
		{
			return;
		}

		collector_->set_file_target(type);
	}

	auto logger::get_file_target() const -> log_types
	{
		if (collector_ == nullptr)
		{
			return log_types::None;
		}

		return collector_->get_file_target();
	}

	auto logger::set_console_target(const log_types& type) -> void
	{
		if (collector_ == nullptr)
		{
			return;
		}

		collector_->set_console_target(type);
	}

	auto logger::get_console_target() const -> log_types
	{
		if (collector_ == nullptr)
		{
			return log_types::None;
		}

		return collector_->get_console_target();
	}

	auto logger::set_max_lines(uint32_t max_lines) -> void
	{
		if (file_writer_ == nullptr)
		{
			return;
		}

		file_writer_->set_max_lines(max_lines);
	}

	auto logger::get_max_lines() const -> uint32_t
	{
		if (file_writer_ == nullptr)
		{
			return 0;
		}

		return file_writer_->get_max_lines();
	}

	auto logger::set_use_backup(bool use_backup) -> void
	{
		if (file_writer_ == nullptr)
		{
			return;
		}

		file_writer_->set_use_backup(use_backup);
	}

	auto logger::get_use_backup() const -> bool
	{
		if (file_writer_ == nullptr)
		{
			return false;
		}

		return file_writer_->get_use_backup();
	}

	auto logger::set_wake_interval(std::chrono::milliseconds interval) -> void
	{
		if (console_writer_ != nullptr)
		{
			console_writer_->set_wake_interval(interval);
		}

		if (file_writer_ != nullptr)
		{
			file_writer_->set_wake_interval(interval);
		}
	}

	auto logger::start() -> std::tuple<bool, std::optional<std::string>>
	{
		if (collector_ == nullptr)
		{
			return { false, "there is no collector" };
		}

		collector_->set_console_queue(console_writer_->get_job_queue());
		collector_->set_file_queue(file_writer_->get_job_queue());

		auto [started, start_error] = collector_->start();
		if (!started)
		{
			return { false, start_error };
		}

		auto [console_started, console_start_error] = console_writer_->start();
		if (!console_started)
		{
			return { false, console_start_error };
		}

		auto [file_started, file_start_error] = file_writer_->start();
		if (!file_started)
		{
			return { false, file_start_error };
		}

		return { true, std::nullopt };
	}

	auto logger::stop() -> void
	{
		if (collector_ != nullptr)
		{
			collector_->stop();
		}

		if (console_writer_ != nullptr)
		{
			console_writer_->stop();
		}

		if (file_writer_ != nullptr)
		{
			file_writer_->stop();
		}
	}

	auto logger::time_point() -> std::chrono::time_point<std::chrono::high_resolution_clock>
	{
		return std::chrono::high_resolution_clock::now();
	}

	auto logger::write(log_types type,
					   const std::string& message,
					   std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>>
						   start_time) -> void
	{
		if (collector_ == nullptr)
		{
			return;
		}

		collector_->write(type, message, start_time);
	}
} // namespace log_module