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

using namespace utility_module;
using namespace priority_thread_pool_module;

bool use_backup_ = false;
uint32_t max_lines_ = 0;
uint16_t wait_interval_ = 100;
uint32_t test_line_count_ = 1000000;
log_module::log_types file_target_ = log_module::log_types::None;
log_module::log_types console_target_ = log_module::log_types::Error;
log_module::log_types callback_target_ = log_module::log_types::None;

uint16_t top_priority_workers_ = 3;
uint16_t middle_priority_workers_ = 2;
uint16_t bottom_priority_workers_ = 1;

auto initialize_logger() -> std::optional<std::string>
{
	log_module::set_title("priority_thread_pool_sample_2");
	log_module::set_use_backup(use_backup_);
	log_module::set_max_lines(max_lines_);
	log_module::file_target(file_target_);
	log_module::console_target(console_target_);
	log_module::callback_target(callback_target_);
	log_module::message_callback(
		[](const log_module::log_types& type, const std::string& datetime,
		   const std::string& message)
		{ std::cout << formatter::format("[{}][{}] {}\n", datetime, type, message); });
	if (wait_interval_ > 0)
	{
		log_module::set_wake_interval(std::chrono::milliseconds(wait_interval_));
	}

	return log_module::start();
}

auto create_default(const uint16_t& top_priority_workers,
					const uint16_t& middle_priority_workers,
					const uint16_t& bottom_priority_workers)
	-> std::tuple<std::shared_ptr<priority_thread_pool_t<test_priority>>,
				  std::optional<std::string>>
{
	std::shared_ptr<priority_thread_pool_t<test_priority>> pool;

	try
	{
		pool = std::make_shared<priority_thread_pool_t<test_priority>>();
	}
	catch (const std::bad_alloc& e)
	{
		return { nullptr, std::string(e.what()) };
	}

	std::optional<std::string> error_message = std::nullopt;
	for (uint16_t i = 0; i < top_priority_workers; ++i)
	{
		error_message = pool->enqueue(std::make_unique<priority_thread_worker_t<test_priority>>(
			std::vector<test_priority>{ test_priority::Top }, "top priority worker"));
		if (error_message.has_value())
		{
			return { nullptr, formatter::format("cannot enqueue to top priority worker: {}",
												error_message.value_or("unknown error")) };
		}
	}
	for (uint16_t i = 0; i < middle_priority_workers; ++i)
	{
		error_message = pool->enqueue(std::make_unique<priority_thread_worker_t<test_priority>>(
			std::vector<test_priority>{ test_priority::Middle }, "middle priority worker"));
		if (error_message.has_value())
		{
			return { nullptr, formatter::format("cannot enqueue to middle priority worker: {}",
												error_message.value_or("unknown error")) };
		}
	}
	for (uint16_t i = 0; i < bottom_priority_workers; ++i)
	{
		error_message = pool->enqueue(std::make_unique<priority_thread_worker_t<test_priority>>(
			std::vector<test_priority>{ test_priority::Bottom }, "bottom priority worker"));
		if (error_message.has_value())
		{
			return { nullptr, formatter::format("cannot enqueue to bottom priority worker: {}",
												error_message.value_or("unknown error")) };
		}
	}

	return { pool, std::nullopt };
}

auto store_job(std::shared_ptr<priority_thread_pool_t<test_priority>> thread_pool)
	-> std::optional<std::string>
{
	int target = 0;
	std::optional<std::string> error_message = std::nullopt;
	for (auto index = 0; index < test_line_count_; ++index)
	{
		target = index % 3;
		error_message = thread_pool->enqueue(std::make_unique<priority_job_t<test_priority>>(
			[target](void) -> std::optional<std::string>
			{
				log_module::write_debug("Hello, World!: {} priority", target);

				return std::nullopt;
			},
			static_cast<test_priority>(target)));
		if (error_message.has_value())
		{
			return formatter::format("error enqueuing job: {}",
									 error_message.value_or("unknown error"));
		}

		log_module::write_sequence("enqueued job: {}", index);
	}

	return std::nullopt;
}

auto main() -> int
{
	auto error_message = initialize_logger();
	if (error_message.has_value())
	{
		std::cerr << formatter::format("error starting logger: {}\n",
									   error_message.value_or("unknown error"));
		return 0;
	}

	std::shared_ptr<priority_thread_pool_t<test_priority>> thread_pool = nullptr;
	std::tie(thread_pool, error_message)
		= create_default(top_priority_workers_, middle_priority_workers_, bottom_priority_workers_);
	if (error_message.has_value())
	{
		log_module::write_error("error creating thread pool: {}",
								error_message.value_or("unknown error"));

		return 0;
	}

	log_module::write_information("created priority thread pool");

	error_message = store_job(thread_pool);
	if (error_message.has_value())
	{
		log_module::write_error("error storing job: {}", error_message.value_or("unknown error"));

		thread_pool.reset();

		return 0;
	}

	error_message = thread_pool->start();
	if (error_message.has_value())
	{
		log_module::write_error("error starting thread pool: {}",
								error_message.value_or("unknown error"));

		thread_pool.reset();

		return 0;
	}

	log_module::write_information("started thread pool");

	thread_pool->stop();

	log_module::write_information("stopped thread pool");

	thread_pool.reset();

	log_module::stop();

	return 0;
}