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

#include "console_writer.h"

#include "formatter.h"
#include "message_job.h"

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#endif

using namespace utility_module;
using namespace thread_module;

namespace log_module
{
	console_writer::console_writer(void)
		: thread_base("console_writer"), job_queue_(std::make_shared<job_queue>())
	{
	}

	auto console_writer::should_continue_work() const -> bool { return !job_queue_->empty(); }

	auto console_writer::before_start() -> result_void
	{
		if (job_queue_ == nullptr)
		{
			return result_void{error{error_code::resource_allocation_failed, "error creating job_queue"}};
		}

		job_queue_->set_notify(!wake_interval_.has_value());

		return {};
	}

	auto console_writer::do_work() -> result_void
	{
		if (job_queue_ == nullptr)
		{
			return result_void{error{error_code::resource_allocation_failed, "there is no job_queue"}};
		}

		std::string console_buffer = "";
		auto remaining_logs = job_queue_->dequeue_batch();
		while (!remaining_logs.empty())
		{
			auto current_job = std::move(remaining_logs.front());
			remaining_logs.pop_front();

			auto current_log
				= std::unique_ptr<message_job>(static_cast<message_job*>(current_job.release()));

			auto work_result = current_log->do_work();
			if (work_result.has_error())
			{
				continue;
			}

			if (current_log->log_type() == log_types::None)
			{
				formatter::format_to(std::back_inserter(console_buffer), "[{}]{}",
										 current_log->datetime(), current_log->message(true));

				continue;
			}

			formatter::format_to(std::back_inserter(console_buffer), "[{}][{}] {}",
									 current_log->datetime(), current_log->log_type(),
									 current_log->message(true));
		}

#ifdef USE_STD_FORMAT
		std::cout << console_buffer;
#else
		fmt::print("{}", console_buffer);
#endif

		return {};
	}
}