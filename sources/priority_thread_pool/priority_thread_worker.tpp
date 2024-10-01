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

#include "priority_thread_worker.h"

#include "logger.h"
#include "priority_job_queue.h"

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#endif

using namespace log_module;

namespace priority_thread_pool_module
{
	template <typename priority_type>
	priority_thread_worker<priority_type>::priority_thread_worker(
		std::vector<priority_type> priorities, const bool& use_time_tag)
		: job_queue_(nullptr), priorities_(priorities), use_time_tag_(use_time_tag)
	{
	}

	template <typename priority_type>
	priority_thread_worker<priority_type>::~priority_thread_worker(void)
	{
	}

	template <typename priority_type>
	auto priority_thread_worker<priority_type>::set_job_queue(
		std::shared_ptr<priority_job_queue<priority_type>> job_queue) -> void
	{
		job_queue_ = job_queue;
	}

	template <typename priority_type>
	auto priority_thread_worker<priority_type>::priorities(void) const -> std::vector<priority_type>
	{
		return priorities_;
	}

	template <typename priority_type>
	auto priority_thread_worker<priority_type>::has_work() const -> bool
	{
		if (job_queue_ == nullptr)
		{
			return false;
		}

		return !job_queue_->empty(priorities_);
	}

	template <typename priority_type>
	auto priority_thread_worker<priority_type>::do_work()
		-> std::tuple<bool, std::optional<std::string>>
	{
		if (job_queue_ == nullptr)
		{
			return { false, "there is no job_queue" };
		}

		auto [job_opt, error] = job_queue_->dequeue(priorities_);
		if (!job_opt.has_value())
		{
			if (!job_queue_->is_stopped())
			{
				return { false,
#ifdef USE_STD_FORMAT
						 std::format
#else
						 fmt::format
#endif
						 ("cannot dequeue job: {}", error.value_or("unknown error")) };
			}

			return { true, std::nullopt };
		}

		auto current_job = std::move(job_opt.value());
		if (current_job == nullptr)
		{
			return { false, "error executing job: nullptr" };
		}

		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> time_point
			= std::nullopt;
		if (use_time_tag_)
		{
			time_point = std::chrono::high_resolution_clock::now();
		}

		current_job->set_job_queue(job_queue_);
		auto [worked, work_error] = current_job->do_work();
		if (!worked)
		{
			return { false,
#ifdef USE_STD_FORMAT
					 std::format
#else
					 fmt::format
#endif
					 ("error executing job: {}", work_error.value_or("unknown error")) };
		}

		if (logger::handle().get_file_target() >= log_types::Sequence
			|| logger::handle().get_console_target() >= log_types::Sequence)
		{
			logger::handle().write(log_types::Sequence,
#ifdef USE_STD_FORMAT
								   std::format
#else
								   fmt::format
#endif
								   ("job executed successfully: {}[{}] on priority_thread_worker",
									current_job->get_name(), current_job->priority()),
								   time_point);
		}

		return { true, std::nullopt };
	}
} // namespace priority_thread_pool_module