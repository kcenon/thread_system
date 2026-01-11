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

#include <kcenon/thread/impl/typed_pool/typed_thread_worker.h>
#include <kcenon/thread/impl/typed_pool/typed_job_queue.h>

namespace kcenon::thread
{
	template <typename job_type>
	typed_thread_worker_t<job_type>::typed_thread_worker_t(
		std::vector<job_type> types,
		const bool& use_time_tag,
		const thread_context& context)
		: thread_base()
		, use_time_tag_(use_time_tag)
		, types_(std::move(types))
		, job_queue_(nullptr)
		, context_(context)
	{
	}

	template <typename job_type>
	typed_thread_worker_t<job_type>::~typed_thread_worker_t()
	{
	}

	template <typename job_type>
	auto typed_thread_worker_t<job_type>::set_job_queue(
		std::shared_ptr<typed_job_queue_t<job_type>> job_queue) -> void
	{
		job_queue_ = std::move(job_queue);
	}

	template <typename job_type>
	auto typed_thread_worker_t<job_type>::set_aging_job_queue(
		std::shared_ptr<aging_typed_job_queue_t<job_type>> job_queue) -> void
	{
		aging_job_queue_ = std::move(job_queue);
	}

	template <typename job_type>
	auto typed_thread_worker_t<job_type>::types() const -> std::vector<job_type>
	{
		return types_;
	}

	template <typename job_type>
	auto typed_thread_worker_t<job_type>::set_context(const thread_context& context) -> void
	{
		context_ = context;
	}

	template <typename job_type>
	auto typed_thread_worker_t<job_type>::get_context() const -> const thread_context&
	{
		return context_;
	}

	template <typename job_type>
	auto typed_thread_worker_t<job_type>::should_continue_work() const -> bool
	{
		// Check aging queue first if available
		if (aging_job_queue_)
		{
			return !aging_job_queue_->empty(types_);
		}

		if (!job_queue_)
		{
			return false;
		}

		return !job_queue_->empty(types_);
	}

	template <typename job_type>
	auto typed_thread_worker_t<job_type>::do_work() -> common::VoidResult
	{
		// Use aging queue if available
		if (aging_job_queue_)
		{
			auto job_result = aging_job_queue_->dequeue(types_);
			if (job_result.is_err())
			{
				return common::ok(); // No job available or error occurred
			}

			auto job = std::move(job_result.value());
			if (!job)
			{
				return common::ok(); // No job to process
			}

			return job->do_work();
		}

		if (!job_queue_)
		{
			return common::ok(); // No queue available
		}

		// Dequeue a job matching the worker's priority levels
		auto job_result = job_queue_->dequeue(types_);
		if (job_result.is_err())
		{
			return common::ok(); // No job available or error occurred
		}

		auto job = std::move(job_result.value());
		if (!job)
		{
			return common::ok(); // No job to process
		}

		// Execute the job
		return job->do_work();
	}

	// Explicit template instantiation for job_types
	template class typed_thread_worker_t<job_types>;

} // namespace kcenon::thread
