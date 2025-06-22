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

#include "lockfree_log_collector.h"
#include "../../utilities/core/formatter.h"
#include "../jobs/message_job.h"
#include <iomanip>
#include <sstream>

namespace log_module
{
	lockfree_log_collector::lockfree_log_collector(void)
		: log_collector()
	{
		// Create lock-free queue instead of standard queue
		lockfree_log_queue_ = std::make_shared<lockfree_job_queue>();
		
		// Note: We don't set log_queue_ from parent class as we'll use lockfree_log_queue_
	}

	auto lockfree_log_collector::write(
		const log_types& type,
		const std::string& message,
		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time)
		-> void
	{
		write_string_lockfree(type, message, start_time);
		has_messages_.store(true, std::memory_order_release);
	}

	auto lockfree_log_collector::write(
		const log_types& type,
		const std::wstring& message,
		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time)
		-> void
	{
		write_string_lockfree(type, message, start_time);
		has_messages_.store(true, std::memory_order_release);
	}

	auto lockfree_log_collector::should_continue_work() const -> bool
	{
		// Use atomic flag for quick check first
		if (has_messages_.load(std::memory_order_acquire))
		{
			return true;
		}

		// Double-check with actual queue
		return lockfree_log_queue_ && !lockfree_log_queue_->empty();
	}

	auto lockfree_log_collector::before_start() -> result_void
	{
		if (!lockfree_log_queue_)
		{
			return result_void(error(error_code::invalid_argument, 
									"Lock-free log queue not initialized"));
		}

		// Lock-free queue doesn't need explicit start

		return {};
	}

	auto lockfree_log_collector::do_work() -> result_void
	{
		if (!lockfree_log_queue_)
		{
			return result_void(error(error_code::invalid_argument, 
									"Lock-free log queue is null"));
		}

		// Process multiple messages in batch for better performance
		constexpr size_t batch_size = 32;
		size_t processed = 0;

		while (processed < batch_size)
		{
			auto dequeue_result = lockfree_log_queue_->dequeue();
			if (!dequeue_result.has_value())
			{
				// No more messages
				break;
			}

			auto job = std::move(dequeue_result.value());
			if (auto log_job_ptr = dynamic_cast<class log_job*>(job.get()))
			{
				// Format timestamp
				auto now = std::chrono::system_clock::now();
				auto time_t_now = std::chrono::system_clock::to_time_t(now);
				
				std::tm tm_now;
				#ifdef _WIN32
					localtime_s(&tm_now, &time_t_now);
				#else
					localtime_r(&time_t_now, &tm_now);
				#endif

				std::ostringstream datetime_stream;
				datetime_stream << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
				auto datetime = datetime_stream.str();

				// Get message details
				auto log_type = log_job_ptr->get_type();
				auto message = log_job_ptr->message();

				// Use parent class method to enqueue to writer queues
				// We need to access these through getter methods since they're private
				log_collector::enqueue_log(log_type, console_target(), 
										  get_console_queue(), datetime, message);
				
				log_collector::enqueue_log(log_type, file_target(), 
										  get_file_queue(), datetime, message);
				
				log_collector::enqueue_log(log_type, callback_target(), 
										  get_callback_queue(), datetime, message);
			}

			++processed;
		}

		// Update atomic flag
		if (lockfree_log_queue_->empty())
		{
			has_messages_.store(false, std::memory_order_release);
		}

		return {};
	}

	auto lockfree_log_collector::get_queue_statistics() const 
		-> lockfree_job_queue::queue_statistics
	{
		if (lockfree_log_queue_)
		{
			return lockfree_log_queue_->get_statistics();
		}
		
		return {};
	}

} // namespace log_module