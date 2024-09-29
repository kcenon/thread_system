#include <iostream>

#include "logger.h"

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#endif

using namespace log_module;

bool use_backup_ = true;
uint32_t max_lines_ = 0;
uint32_t test_line_count_ = 10000000;
log_types file_target_ = log_types::Error;
log_types console_target_ = log_types::Debug;

auto main() -> int
{
	logger::handle().set_title("logger_sample");
	logger::handle().set_use_backup(use_backup_);
	logger::handle().set_max_lines(max_lines_);
	logger::handle().set_file_target(file_target_);
	logger::handle().set_console_target(console_target_);
	logger::handle().set_wake_interval(std::chrono::milliseconds(100));

	auto [started, start_error] = logger::handle().start();
	if (!started)
	{
		std::cerr << "error starting logger: " << start_error.value_or("unknown error")
				  << std::endl;
		return 0;
	}

	for (auto index = 0; index < test_line_count_; ++index)
	{
		if (logger::handle().get_file_target() >= log_types::Debug
			|| logger::handle().get_console_target() >= log_types::Debug)
		{
			logger::handle().write(log_types::Debug,
#ifdef USE_STD_FORMAT
								   std::format
#else
								   fmt::format
#endif
								   ("Hello, World!: {}", index));
		}
	}

	logger::handle().stop();
	logger::destroy();

	return 0;
}
