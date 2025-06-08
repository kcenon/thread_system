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
		, file_log_type_(log_types::None)
		, console_log_type_(log_types::None)
		, callback_log_type_(log_types::None)
		, log_queue_(std::make_shared<job_queue>())
	{
	}

	auto log_collector::console_target(const log_types& type) -> void 
	{ 
		std::lock_guard<std::mutex> lock(queue_mutex_);
		console_log_type_ = type; 
	}

	auto log_collector::console_target() const -> log_types 
	{ 
		std::lock_guard<std::mutex> lock(queue_mutex_);
		return console_log_type_; 
	}

	auto log_collector::file_target(const log_types& type) -> void 
	{ 
		std::lock_guard<std::mutex> lock(queue_mutex_);
		file_log_type_ = type; 
	}

	auto log_collector::file_target() const -> log_types 
	{ 
		std::lock_guard<std::mutex> lock(queue_mutex_);
		return file_log_type_; 
	}

	auto log_collector::callback_target(const log_types& type) -> void
	{
		std::lock_guard<std::mutex> lock(queue_mutex_);
		callback_log_type_ = type;
	}

	auto log_collector::callback_target() const -> log_types 
	{ 
		std::lock_guard<std::mutex> lock(queue_mutex_);
		return callback_log_type_; 
	}

	auto log_collector::set_console_queue(std::shared_ptr<job_queue> queue) -> void
	{
		std::lock_guard<std::mutex> lock(queue_mutex_);
		console_queue_ = queue;
	}

	auto log_collector::set_file_queue(std::shared_ptr<job_queue> queue) -> void
	{
		std::lock_guard<std::mutex> lock(queue_mutex_);
		file_queue_ = queue;
	}

	auto log_collector::set_callback_queue(std::shared_ptr<job_queue> queue) -> void
	{
		std::lock_guard<std::mutex> lock(queue_mutex_);
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

	auto log_collector::should_continue_work() const -> bool 
	{ 
		std::lock_guard<std::mutex> lock(queue_mutex_);
		// Check if log_queue_ is valid before accessing it
		return log_queue_ && !log_queue_->empty();
	}

	auto log_collector::before_start() -> result_void
	{
		log_job job("START");
		auto work_result = job.do_work();
		if (work_result.has_error())
		{
			return work_result.get_error();
		}

		auto enqueue_result = enqueue_log(console_log_type_, log_types::None, console_queue_,
										 job.datetime(), job.message());
		if (enqueue_result.has_error())
		{
			return enqueue_result.get_error();
		}

		enqueue_result = enqueue_log(file_log_type_, log_types::None, file_queue_, job.datetime(),
									job.message());
		if (enqueue_result.has_error())
		{
			return enqueue_result.get_error();
		}

		return result_void{};
	}

	auto log_collector::do_work() -> result_void
	{
		// First get a local copy of the log queue with proper synchronization
		std::shared_ptr<job_queue> queue_copy;
		{
			std::lock_guard<std::mutex> lock(queue_mutex_);
			if (log_queue_ == nullptr)
			{
				return error{error_code::resource_allocation_failed, "there is no job_queue"};
			}
			queue_copy = log_queue_;
		}

		// Now work with the local copy
		auto dequeue_result = queue_copy->dequeue();
		if (!dequeue_result.has_value())
		{
			if (!queue_copy->is_stopped())
			{
				return error{error_code::queue_empty, 
					formatter::format("error dequeue job: {}", dequeue_result.get_error().to_string())};
			}

			return result_void{};
		}

		auto job_ptr = std::move(dequeue_result.value());
		if (job_ptr == nullptr) 
		{
			return error{error_code::job_invalid, "error executing job: received empty job"};
		}

		auto current_log = std::unique_ptr<log_job>(static_cast<log_job*>(job_ptr.release()));

		auto work_result = current_log->do_work();
		if (work_result.has_error())
		{
			return work_result.get_error();
		}

		// Get thread-safe copies of the targets and queues
		log_types console_target_copy;
		log_types file_target_copy;
		log_types callback_target_copy;
		std::weak_ptr<job_queue> console_queue_copy;
		std::weak_ptr<job_queue> file_queue_copy;
		std::weak_ptr<job_queue> callback_queue_copy;
		
		{
			std::lock_guard<std::mutex> lock(queue_mutex_);
			console_target_copy = console_log_type_;
			file_target_copy = file_log_type_;
			callback_target_copy = callback_log_type_;
			console_queue_copy = console_queue_;
			file_queue_copy = file_queue_;
			callback_queue_copy = callback_queue_;
		}
		
		result_void enqueue_result;

		if (current_log->get_type() <= console_target_copy)
		{
			enqueue_result = enqueue_log(current_log->get_type(), current_log->get_type(), 
										 console_queue_copy, current_log->datetime(), 
										 current_log->message());
			if (enqueue_result.has_error())
			{
				return enqueue_result.get_error();
			}
		}

		if (current_log->get_type() <= file_target_copy)
		{
			enqueue_result = enqueue_log(current_log->get_type(), current_log->get_type(), 
										 file_queue_copy, current_log->datetime(), 
										 current_log->message());
			if (enqueue_result.has_error())
			{
				return enqueue_result.get_error();
			}
		}

		if (current_log->get_type() <= callback_target_copy)
		{
			enqueue_result = enqueue_log(current_log->get_type(), current_log->get_type(), 
										 callback_queue_copy, current_log->datetime(), 
										 current_log->message());
			if (enqueue_result.has_error())
			{
				return enqueue_result.get_error();
			}
		}

		return result_void{};
	}

	auto log_collector::after_stop() -> result_void
	{
		log_job job("STOP");
		auto work_result = job.do_work();
		if (work_result.has_error())
		{
			return work_result.get_error();
		}

		auto enqueue_result = enqueue_log(console_log_type_, log_types::None, console_queue_,
										 job.datetime(), job.message());
		if (enqueue_result.has_error())
		{
			return enqueue_result.get_error();
		}

		enqueue_result = enqueue_log(file_log_type_, log_types::None, file_queue_, job.datetime(),
									job.message());
		if (enqueue_result.has_error())
		{
			return enqueue_result.get_error();
		}

		return result_void{};
	}

	auto log_collector::enqueue_log(const log_types& current_log_type,
									const log_types& target_log_type,
									std::weak_ptr<job_queue> weak_queue,
									const std::string& datetime,
									const std::string& message) -> result_void
	{
		if (current_log_type == log_types::None)
		{
			return result_void{};
		}

		// Safely handle weak pointer - lock and check validity
		std::shared_ptr<job_queue> locked_queue;
		{
			std::lock_guard<std::mutex> lock(queue_mutex_);
			locked_queue = weak_queue.lock();
			if (!locked_queue)
			{
				return error{error_code::resource_allocation_failed, "Queue is no longer available"};
			}
		}
		
		// Now we have a valid shared_ptr to the queue
		if (!message.empty())
		{
			try 
			{
				auto job = std::make_unique<message_job>(target_log_type, datetime, message);
				auto enqueue_result = locked_queue->enqueue(std::move(job));
				if (enqueue_result.has_error())
				{
					return enqueue_result.get_error();
				}
			}
			catch (const std::exception& e)
			{
				return error{error_code::job_creation_failed, 
					formatter::format("Error creating or enqueuing message job: {}", e.what())};
			}
		}

		return result_void{};
	}
} // namespace log_module