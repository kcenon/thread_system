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

#include "typed_lockfree_thread_worker.h"

namespace typed_thread_pool_module
{
	template <typename job_type>
	typed_lockfree_thread_worker_t<job_type>::typed_lockfree_thread_worker_t(
		std::vector<job_type> types,
		const std::string& worker_name)
		: thread_base(worker_name)
		, types_(std::move(types))
		, job_queue_(nullptr)
	{
	}

	template <typename job_type>
	typed_lockfree_thread_worker_t<job_type>::~typed_lockfree_thread_worker_t(void)
	{
		stop();
	}

	template <typename job_type>
	auto typed_lockfree_thread_worker_t<job_type>::set_job_queue(
		std::shared_ptr<typed_lockfree_job_queue_t<job_type>> job_queue) -> void
	{
		job_queue_ = job_queue;
	}

	template <typename job_type>
	auto typed_lockfree_thread_worker_t<job_type>::types(void) const -> std::vector<job_type>
	{
		return types_;
	}

	template <typename job_type>
	auto typed_lockfree_thread_worker_t<job_type>::should_continue_work() const -> bool
	{
		if (job_queue_ == nullptr)
		{
			return false;
		}

		if (types_.empty())
		{
			// If no specific types configured, check if queue has any jobs
			return !job_queue_->empty();
		}

		// Check if any of our configured types have jobs
		return !job_queue_->empty(types_);
	}

	template <typename job_type>
	auto typed_lockfree_thread_worker_t<job_type>::do_work() -> result_void
	{
		if (job_queue_ == nullptr)
		{
			return error{error_code::resource_allocation_failed, "job queue is null"};
		}

		// Try to dequeue a job based on our configured types
		std::optional<std::unique_ptr<thread_module::job>> job_opt;
		
		if (types_.empty())
		{
			// If no specific types, dequeue any job
			auto result = job_queue_->dequeue();
			if (result.has_value())
			{
				job_opt = std::move(result.value());
			}
		}
		else
		{
			// Try to dequeue jobs for our configured types
			for (const auto& type : types_)
			{
				auto result = job_queue_->dequeue(type);
				if (result.has_value())
				{
					job_opt = std::move(result.value());
					break;
				}
			}
		}

		if (!job_opt.has_value())
		{
			// No job available - this is normal, not an error
			std::this_thread::sleep_for(std::chrono::microseconds(100));
			return {};
		}

		auto& job = job_opt.value();
		if (job == nullptr)
		{
			return error{error_code::job_invalid, "dequeued null job"};
		}

		// Execute the job
		auto result = job->do_work();
		if (result.has_error())
		{
			return result.get_error();
		}

		return {};
	}
} // namespace typed_thread_pool_module