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
				logger::handle().write(log_types::Debug, "Hello, World!: {}", index);

				return { true, std::nullopt };
			}));
		if (!enqueued)
		{
			logger::handle().write(log_types::Error, "error enqueuing job: {}",
								   enqueue_error.value_or("unknown error"));

			break;
		}

		logger::handle().write(log_types::Sequence, "enqueued job: {}", index);
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
		logger::handle().write(log_types::Error, "error creating thread pool: {}",
							   create_error.value_or("unknown error"));

		return 0;
	}

	logger::handle().write(log_types::Information, "created thread pool");

	auto [stored, store_error] = store_job(thread_pool);
	if (!stored)
	{
		logger::handle().write(log_types::Error, "error storing job: {}",
							   store_error.value_or("unknown error"));

		thread_pool.reset();

		return 0;
	}

	auto [thread_started, thread_start_error] = thread_pool->start();
	if (!thread_started)
	{
		logger::handle().write(log_types::Error, "error starting thread pool: {}",
							   thread_start_error.value_or("unknown error"));

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