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

#include "message_job.h"

namespace log_module
{
	log_collector::log_collector(void)
		: thread_base("log_collector")
		, log_queue_(std::make_shared<job_queue>())
		, file_log_type_(log_types::None)
		, console_log_type_(log_types::None)
		, callback_log_type_(log_types::None)
	{
	}

	auto log_collector::console_target(const log_types& type) -> void { console_log_type_ = type; }

	auto log_collector::console_target() const -> log_types { return console_log_type_; }

	auto log_collector::file_target(const log_types& type) -> void { file_log_type_ = type; }

	auto log_collector::file_target() const -> log_types { return file_log_type_; }

	auto log_collector::callback_target(const log_types& type) -> void
	{
		callback_log_type_ = type;
	}

	auto log_collector::callback_target() const -> log_types { return callback_log_type_; }

	auto log_collector::set_console_queue(std::shared_ptr<job_queue> queue) -> void
	{
		console_queue_ = queue;
	}

	auto log_collector::set_file_queue(std::shared_ptr<job_queue> queue) -> void
	{
		file_queue_ = queue;
	}

	auto log_collector::set_callback_queue(std::shared_ptr<job_queue> queue) -> void
	{
		callback_queue_ = queue;
	}

	auto log_collector::write(
		const log_types& type,
		const std::string& message,
		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time)
		-> void
	{
		write_string(type, message, start_time);
	}

	auto log_collector::write(
		const log_types& type,
		const std::wstring& message,
		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time)
		-> void
	{
		write_string(type, message, start_time);
	}

	auto log_collector::should_continue_work() const -> bool { return !log_queue_->empty(); }

	auto log_collector::before_start() -> std::optional<std::string>
	{
		log_job job("START");
		auto work_error = job.do_work();
		if (work_error.has_value())
		{
			return work_error;
		}

		auto enqueue_error = enqueue_log(console_log_type_, log_types::None, console_queue_,
										 job.datetime(), job.message());
		if (enqueue_error.has_value())
		{
			return enqueue_error;
		}

		enqueue_error = enqueue_log(file_log_type_, log_types::None, file_queue_, job.datetime(),
									job.message());
		if (enqueue_error.has_value())
		{
			return enqueue_error;
		}

		return std::nullopt;
	}

	auto log_collector::do_work() -> std::optional<std::string>
	{
		if (log_queue_ == nullptr)
		{
			return "there is no job_queue";
		}

		auto [job_opt, error] = log_queue_->dequeue();
		if (error.has_value())
		{
			if (!log_queue_->is_stopped())
			{
				return formatter::format("error dequeue job: {}", error.value_or("unknown error"));
			}

			return std::nullopt;
		}

		auto current_job = std::move(job_opt.value());
		if (current_job == nullptr)
		{
			return "error executing job: nullptr";
		}

		auto current_log = std::unique_ptr<log_job>(static_cast<log_job*>(current_job.release()));

		auto work_error = current_log->do_work();
		if (work_error.has_value())
		{
			return std::nullopt;
		}

		auto enqueue_error
			= enqueue_log(current_log->get_type(), current_log->get_type(), console_queue_,
						  current_log->datetime(), current_log->message());
		if (enqueue_error.has_value())
		{
			return enqueue_error;
		}

		enqueue_error = enqueue_log(current_log->get_type(), current_log->get_type(), file_queue_,
									current_log->datetime(), current_log->message());
		if (enqueue_error.has_value())
		{
			return enqueue_error;
		}

		enqueue_error
			= enqueue_log(current_log->get_type(), current_log->get_type(), callback_queue_,
						  current_log->datetime(), current_log->message());
		if (enqueue_error.has_value())
		{
			return enqueue_error;
		}

		return std::nullopt;
	}

	auto log_collector::after_stop() -> std::optional<std::string>
	{
		log_job job("STOP");
		auto work_error = job.do_work();
		if (work_error.has_value())
		{
			return work_error;
		}

		auto enqueue_error = enqueue_log(console_log_type_, log_types::None, console_queue_,
										 job.datetime(), job.message());
		if (enqueue_error.has_value())
		{
			return enqueue_error;
		}

		enqueue_error = enqueue_log(file_log_type_, log_types::None, file_queue_, job.datetime(),
									job.message());
		if (enqueue_error.has_value())
		{
			return enqueue_error;
		}

		return std::nullopt;
	}

	auto log_collector::enqueue_log(const log_types& current_log_type,
									const log_types& target_log_type,
									std::weak_ptr<job_queue> weak_queue,
									const std::string& datetime,
									const std::string& message) -> std::optional<std::string>
	{
		if (current_log_type == log_types::None)
		{
			return std::nullopt;
		}

		auto locked_queue = weak_queue.lock();
		if (locked_queue != nullptr && !message.empty())
		{
			auto enqueue_error = locked_queue->enqueue(
				std::make_unique<message_job>(target_log_type, datetime, message));
			if (enqueue_error.has_value())
			{
				return enqueue_error;
			}
		}

		return std::nullopt;
	}
} // namespace log_module