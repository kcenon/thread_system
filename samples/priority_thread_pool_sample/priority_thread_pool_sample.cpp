/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#include <iostream>

#include "logger.h"
#include "formatter.h"
#include "test_priority.h"
#include "priority_thread_pool.h"

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#endif

using namespace log_module;
using namespace utility_module;
using namespace priority_thread_pool_module;

bool use_backup_ = false;
uint32_t max_lines_ = 0;
uint16_t wait_interval_ = 100;
uint32_t test_line_count_ = 1000000;
log_types file_target_ = log_types::Debug;
log_types console_target_ = log_types::Error;

uint16_t top_priority_workers_ = 3;
uint16_t middle_priority_workers_ = 2;
uint16_t bottom_priority_workers_ = 1;

auto initialize_logger() -> std::tuple<bool, std::optional<std::string>>
{
	logger::handle().set_title("priority_thread_pool_sample");
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

auto create_default(const uint16_t& top_priority_workers,
					const uint16_t& middle_priority_workers,
					const uint16_t& bottom_priority_workers)
	-> std::tuple<std::shared_ptr<priority_thread_pool<test_priority>>, std::optional<std::string>>
{
	std::shared_ptr<priority_thread_pool<test_priority>> pool;

	try
	{
		pool = std::make_shared<priority_thread_pool<test_priority>>();
	}
	catch (const std::bad_alloc& e)
	{
		return { nullptr, std::string(e.what()) };
	}

	for (uint16_t i = 0; i < top_priority_workers; ++i)
	{
		auto [enqueued, enqueue_error]
			= pool->enqueue(std::make_unique<priority_thread_worker<test_priority>>(
				std::vector<test_priority>{ test_priority::Top }, "top priority worker"));
		if (!enqueued)
		{
			return { nullptr, formatter::format("cannot enqueue to top priority worker: {}",
												enqueue_error.value_or("unknown error")) };
		}
	}
	for (uint16_t i = 0; i < middle_priority_workers; ++i)
	{
		auto [enqueued, enqueue_error]
			= pool->enqueue(std::make_unique<priority_thread_worker<test_priority>>(
				std::vector<test_priority>{ test_priority::Middle }, "middle priority worker"));
		if (!enqueued)
		{
			return { nullptr, formatter::format("cannot enqueue to middle priority worker: {}",
												enqueue_error.value_or("unknown error")) };
		}
	}
	for (uint16_t i = 0; i < bottom_priority_workers; ++i)
	{
		auto [enqueued, enqueue_error]
			= pool->enqueue(std::make_unique<priority_thread_worker<test_priority>>(
				std::vector<test_priority>{ test_priority::Bottom }, "bottom priority worker"));
		if (!enqueued)
		{
			return { nullptr, formatter::format("cannot enqueue to bottom priority worker: {}",
												enqueue_error.value_or("unknown error")) };
		}
	}

	return { pool, std::nullopt };
}

auto store_job(std::shared_ptr<priority_thread_pool<test_priority>> thread_pool)
	-> std::tuple<bool, std::optional<std::string>>
{
	for (auto index = 0; index < test_line_count_; ++index)
	{
		auto target = index % 3;
		auto [enqueued, enqueue_error]
			= thread_pool->enqueue(std::make_unique<priority_job<test_priority>>(
				[target](void) -> std::tuple<bool, std::optional<std::string>>
				{
					logger::handle().write(log_types::Debug,
										   formatter::format("Hello, World!: {} priority", target));

					return { true, std::nullopt };
				},
				static_cast<test_priority>(target)));
		if (!enqueued)
		{
			logger::handle().write(log_types::Error,
								   formatter::format("error enqueuing job: {}",
													 enqueue_error.value_or("unknown error")));

			break;
		}

		logger::handle().write(log_types::Sequence, formatter::format("enqueued job: {}", index));
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

	auto [thread_pool, create_error]
		= create_default(top_priority_workers_, middle_priority_workers_, bottom_priority_workers_);
	if (thread_pool == nullptr)
	{
		logger::handle().write(log_types::Error,
							   formatter::format("error creating thread pool: {}",
												 create_error.value_or("unknown error")));

		return 0;
	}

	logger::handle().write(log_types::Information, "created priority thread pool");

	auto [stored, store_error] = store_job(thread_pool);
	if (!stored)
	{
		logger::handle().write(
			log_types::Error,
			formatter::format("error storing job: {}", store_error.value_or("unknown error")));

		thread_pool.reset();

		return 0;
	}

	auto [thread_started, thread_start_error] = thread_pool->start();
	if (!thread_started)
	{
		logger::handle().write(log_types::Error,
							   formatter::format("error starting thread pool: {}",
												 thread_start_error.value_or("unknown error")));

		thread_pool.reset();

		return 0;
	}

	logger::handle().write(log_types::Information, "started thread pool");

	thread_pool->stop();

	logger::handle().write(log_types::Information, "stopped thread pool");

	thread_pool.reset();

	logger::handle().stop();
	logger::destroy();

	return 0;
}