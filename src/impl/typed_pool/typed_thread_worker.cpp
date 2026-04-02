// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#include <kcenon/thread/impl/typed_pool/typed_thread_worker.h>

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
	auto typed_thread_worker_t<job_type>::set_aging_job_queue(
		std::shared_ptr<aging_typed_job_queue_t<job_type>> job_queue) -> void
	{
		job_queue_ = std::move(job_queue);
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
		if (!job_queue_)
		{
			return false;
		}

		return !job_queue_->empty(types_);
	}

	template <typename job_type>
	auto typed_thread_worker_t<job_type>::do_work() -> common::VoidResult
	{
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
