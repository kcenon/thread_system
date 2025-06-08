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

#include "thread_worker.h"

#include "logger.h"
#include "formatter.h"

namespace thread_pool_module
{
	thread_worker::thread_worker(const bool& use_time_tag)
		: thread_base("thread_worker"), use_time_tag_(use_time_tag), job_queue_(nullptr)
	{
	}

	thread_worker::~thread_worker(void) {}

	auto thread_worker::set_job_queue(std::shared_ptr<job_queue> job_queue) -> void
	{
		job_queue_ = job_queue;
	}

	auto thread_worker::should_continue_work() const -> bool
	{
		if (job_queue_ == nullptr)
		{
			return false;
		}

		return !job_queue_->empty();
	}

	auto thread_worker::do_work() -> result_void
	{
		if (job_queue_ == nullptr)
		{
			return error{error_code::resource_allocation_failed, "there is no job_queue"};
		}

		auto dequeue_result = job_queue_->dequeue();
		if (!dequeue_result.has_value())
		{
			if (!job_queue_->is_stopped())
			{
				return error{error_code::queue_empty, 
					formatter::format("error dequeue job: {}", dequeue_result.get_error().to_string())};
			}

			return result_void{};
		}

		auto current_job = std::move(dequeue_result.value());
		if (current_job == nullptr)
		{
			return error{error_code::job_invalid, "error executing job: nullptr"};
		}

		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>>
			started_time_point = std::nullopt;
		if (use_time_tag_)
		{
			started_time_point = std::chrono::high_resolution_clock::now();
		}

		current_job->set_job_queue(job_queue_);
		auto work_result = current_job->do_work();
		if (work_result.has_error())
		{
			return error{error_code::job_execution_failed, 
				formatter::format("error executing job: {}", work_result.get_error().to_string())};
		}

		if (!started_time_point.has_value())
		{
			log_module::write_sequence("job executed successfully: {} on thread_worker",
									   current_job->get_name());
		}

		log_module::write_sequence(started_time_point.value(),
								   "job executed successfully: {} on thread_worker",
								   current_job->get_name());

		return result_void{};
	}
} // namespace thread_pool_module