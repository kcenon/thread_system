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

#include "log_collector.h"

#include "log_job.h"
#include "formatter.h"
#include "message_job.h"

using namespace utility_module;

namespace log_module
{
	log_collector::log_collector(void)
		: log_queue_(std::make_shared<job_queue>())
		, file_log_type_(log_types::Error)
		, console_log_type_(log_types::Information)
	{
	}

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
		write_string(type, message, start_time);
	}

#ifdef _WIN32_BUT_NOT_TESTED_NOT_TESTED
	auto log_collector::write(
		log_types type,
		const std::wstring& message,
		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time)
		-> void
	{
		write_string(type, message, start_time);
	}
#endif

	auto log_collector::write(
		log_types type,
		const std::u16string& message,
		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time)
		-> void
	{
		write_string(type, message, start_time);
	}

	auto log_collector::write(
		log_types type,
		const std::u32string& message,
		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time)
		-> void
	{
		write_string(type, message, start_time);
	}

	auto log_collector::has_work() const -> bool { return !log_queue_->empty(); }

	auto log_collector::before_start() -> std::tuple<bool, std::optional<std::string>>
	{
		log_job job("START");
		auto [worked, work_error] = job.do_work();
		if (!worked)
		{
			return { false, work_error };
		}

		auto buffer = job.message();

		if (console_log_type_ > log_types::None)
		{
			auto console_queue = console_queue_.lock();
			if (console_queue != nullptr && !buffer.empty())
			{
				auto [enqueued, enqueue_error]
					= console_queue->enqueue(std::make_unique<message_job>(buffer));
				if (!enqueued)
				{
					return { false, enqueue_error };
				}
			}
		}

		if (file_log_type_ > log_types::None)
		{
			auto file_queue = file_queue_.lock();
			if (file_queue != nullptr && !buffer.empty())
			{
				auto [enqueued, enqueue_error]
					= file_queue->enqueue(std::make_unique<message_job>(buffer));
				if (!enqueued)
				{
					return { false, enqueue_error };
				}
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

		auto [job_opt, error] = log_queue_->dequeue();
		if (!job_opt.has_value())
		{
			if (!log_queue_->is_stopped())
			{
				return { false, formatter::format("error dequeue job: {}",
												  error.value_or("unknown error")) };
			}

			return { true, std::nullopt };
		}

		auto current_job = std::move(job_opt.value());
		if (current_job == nullptr)
		{
			return { false, "error executing job: nullptr" };
		}

		auto current_log = std::unique_ptr<log_job>(static_cast<log_job*>(current_job.release()));

		auto [worked, work_error] = current_log->do_work();
		std::string log = (worked) ? current_log->message()
								   : work_error.value_or("Unknown error to convert to log message");

		auto console_queue = console_queue_.lock();
		if (current_log->get_type() <= console_log_type_ && console_queue != nullptr)
		{
			auto [enqueued, enqueue_error]
				= console_queue->enqueue(std::make_unique<message_job>(log));
			if (!enqueued)
			{
				return { false, enqueue_error };
			}
		}

		auto file_queue = file_queue_.lock();
		if (current_log->get_type() <= file_log_type_ && file_queue != nullptr)
		{
			auto [enqueued, enqueue_error]
				= file_queue->enqueue(std::make_unique<message_job>(log));
			if (!enqueued)
			{
				return { false, enqueue_error };
			}
		}

		return { true, std::nullopt };
	}

	auto log_collector::after_stop() -> std::tuple<bool, std::optional<std::string>>
	{
		log_job job("STOP");
		auto [worked, work_error] = job.do_work();
		if (!worked)
		{
			return { false, work_error };
		}

		auto buffer = job.message();

		if (console_log_type_ > log_types::None)
		{
			auto console_queue = console_queue_.lock();
			if (console_queue != nullptr && !buffer.empty())
			{
				auto [enqueued, enqueue_error]
					= console_queue->enqueue(std::make_unique<message_job>(buffer));
				if (!enqueued)
				{
					return { false, enqueue_error };
				}
			}
		}

		if (file_log_type_ > log_types::None)
		{
			auto file_queue = file_queue_.lock();
			if (file_queue != nullptr && !buffer.empty())
			{
				auto [enqueued, enqueue_error]
					= file_queue->enqueue(std::make_unique<message_job>(buffer));
				if (!enqueued)
				{
					return { false, enqueue_error };
				}
			}
		}

		return { true, std::nullopt };
	}

	template <typename StringType>
	void log_collector::write_string(
		log_types type,
		const StringType& message,
		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time)
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
			std::cerr << formatter::format("error enqueuing log job: {}\n",
										   enqueue_error.value_or("unknown error"));
		}
	}
}