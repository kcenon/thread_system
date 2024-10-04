#include <iostream>

#include "logger.h"
#include "formatter.h"
#include "thread_pool.h"

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#endif

using namespace log_module;
using namespace utility_module;
using namespace thread_pool_module;

bool use_backup_ = false;
uint32_t max_lines_ = 0;
uint16_t wait_interval_ = 100;
uint32_t test_line_count_ = 1000000;
log_types file_target_ = log_types::Debug;
log_types console_target_ = log_types::Error;

uint16_t thread_counts_ = 10;

auto initialize_logger() -> std::tuple<bool, std::optional<std::string>>
{
	logger::handle().set_title("thread_pool_sample");
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

auto create_default(const uint16_t& worker_counts)
	-> std::tuple<std::shared_ptr<thread_pool>, std::optional<std::string>>
{
	std::shared_ptr<thread_pool> pool;

	try
	{
		pool = std::make_shared<thread_pool>();
	}
	catch (const std::bad_alloc& e)
	{
		return { nullptr, std::string(e.what()) };
	}

	for (uint16_t i = 0; i < worker_counts; ++i)
	{
		auto [enqueued, euqueue_error] = pool->enqueue(std::make_unique<thread_worker>());
		if (!enqueued)
		{
			return { nullptr, formatter::format("cannot enqueue to worker: {}",
												euqueue_error.value_or("unknown error")) };
		}
	}

	return { pool, std::nullopt };
}

auto store_job(std::shared_ptr<thread_pool> thread_pool)
	-> std::tuple<bool, std::optional<std::string>>
{
	for (auto index = 0; index < test_line_count_; ++index)
	{
		auto [enqueued, enqueue_error] = thread_pool->enqueue(std::make_unique<job>(
			[index](void) -> std::tuple<bool, std::optional<std::string>>
			{
				if (logger::handle().get_file_target() >= log_types::Debug
					|| logger::handle().get_console_target() >= log_types::Debug)
				{
					logger::handle().write(log_types::Debug,
										   formatter::format("Hello, World!: {}", index));
				}

				return { true, std::nullopt };
			}));
		if (!enqueued)
		{
			if (logger::handle().get_file_target() >= log_types::Error
				|| logger::handle().get_console_target() >= log_types::Error)
			{
				logger::handle().write(log_types::Error,
									   formatter::format("error enqueuing job: {}",
														 enqueue_error.value_or("unknown error")));
			}

			break;
		}

		if (logger::handle().get_file_target() >= log_types::Sequence
			|| logger::handle().get_console_target() >= log_types::Sequence)
		{
			logger::handle().write(log_types::Sequence,
								   formatter::format("enqueued job: {}", index));
		}
	}

	return { true, std::nullopt };
}

auto main() -> int
{
	auto [started, start_error] = initialize_logger();
	if (!started)
	{
		std::cerr << formatter::format("error starting logger: {}\n",
									   start_error.value_or("unknown error"));
		return 0;
	}

	auto [thread_pool, create_error] = create_default(thread_counts_);
	if (thread_pool == nullptr)
	{
		if (logger::handle().get_file_target() >= log_types::Error
			|| logger::handle().get_console_target() >= log_types::Error)
		{
			logger::handle().write(log_types::Error,
								   formatter::format("error creating thread pool: {}",
													 create_error.value_or("unknown error")));
		}

		return 0;
	}

	if (logger::handle().get_file_target() >= log_types::Information
		|| logger::handle().get_console_target() >= log_types::Information)
	{
		logger::handle().write(log_types::Information, "created thread pool");
	}

	auto [stored, store_error] = store_job(thread_pool);
	if (!stored)
	{
		if (logger::handle().get_file_target() >= log_types::Error
			|| logger::handle().get_console_target() >= log_types::Error)
		{
			logger::handle().write(
				log_types::Error,
				formatter::format("error storing job: {}", store_error.value_or("unknown error")));
		}

		thread_pool.reset();

		return 0;
	}

	auto [thread_started, thread_start_error] = thread_pool->start();
	if (!thread_started)
	{
		if (logger::handle().get_file_target() >= log_types::Error
			|| logger::handle().get_console_target() >= log_types::Error)
		{
			logger::handle().write(log_types::Error,
								   formatter::format("error starting thread pool: {}",
													 thread_start_error.value_or("unknown error")));
		}

		thread_pool.reset();

		return 0;
	}

	if (logger::handle().get_file_target() >= log_types::Information
		|| logger::handle().get_console_target() >= log_types::Information)
	{
		logger::handle().write(log_types::Information, "started thread pool");
	}

	thread_pool->stop();

	if (logger::handle().get_file_target() >= log_types::Information
		|| logger::handle().get_console_target() >= log_types::Information)
	{
		logger::handle().write(log_types::Information, "stopped thread pool");
	}

	thread_pool.reset();

	logger::handle().stop();
	logger::destroy();

	return 0;
}