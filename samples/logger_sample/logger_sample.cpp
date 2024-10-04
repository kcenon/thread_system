#include <iostream>

#include "logger.h"

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#endif

using namespace log_module;

bool use_backup_ = false;
uint32_t max_lines_ = 0;
uint16_t wait_interval_ = 100;
uint32_t test_line_count_ = 1000000;
log_types file_target_ = log_types::Debug;
log_types console_target_ = log_types::Error;

template <typename... Args>
auto message_formatter(
#ifdef USE_STD_FORMAT
	std::format_string<Args...> format_str,
#else
	fmt::format_string<Args...> format_str,
#endif
	Args&&... args) -> std::string
{
	return
#ifdef USE_STD_FORMAT
		std::format
#else
		fmt::format
#endif
		(format_str, std::forward<Args>(args)...);
}

template <typename... Args>
auto log_message(log_types log_level,
#ifdef USE_STD_FORMAT
				 std::format_string<Args...> format_str,
#else
				 fmt::format_string<Args...> format_str,
#endif
				 Args&&... args) -> void
{
	if (logger::handle().get_file_target() >= log_level
		|| logger::handle().get_console_target() >= log_level)
	{
		logger::handle().write(log_level,
							   message_formatter(format_str, std::forward<Args>(args)...));
	}
}

auto initialize_logger() -> std::tuple<bool, std::optional<std::string>>
{
	logger::handle().set_title("logger_sample");
	logger::handle().set_use_backup(use_backup_);
	logger::handle().set_max_lines(max_lines_);
	logger::handle().set_file_target(file_target_);
	logger::handle().set_console_target(console_target_);
	if (wait_interval_ > 0)
	{
		logger::handle().set_wake_interval(std::chrono::milliseconds(wait_interval_));
	}

	return logger::handle().start();
}

auto main() -> int
{
	auto [started, start_error] = initialize_logger();
	if (!started)
	{
		std::cerr << message_formatter("error starting logger: {}\n",
									   start_error.value_or("unknown error"));
		return 0;
	}

	for (auto index = 0; index < test_line_count_; ++index)
	{
		if (logger::handle().get_file_target() >= log_types::Debug
			|| logger::handle().get_console_target() >= log_types::Debug)
		{
			log_message(log_types::Debug, "Hello, World!: {}", index);
		}
	}

	logger::handle().stop();
	logger::destroy();

	return 0;
}
