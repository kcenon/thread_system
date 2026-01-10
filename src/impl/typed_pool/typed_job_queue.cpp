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

#include <kcenon/thread/impl/typed_pool/typed_job_queue.h>
#include <kcenon/thread/core/job_queue.h>
#include <sstream>

namespace kcenon::thread
{
	template <typename job_type>
	typed_job_queue_t<job_type>::typed_job_queue_t()
		: job_queue()
	{
	}

	template <typename job_type>
	typed_job_queue_t<job_type>::~typed_job_queue_t()
	{
		clear();
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::get_or_create_queue(const job_type& type) -> job_queue*
	{
		auto it = job_queues_.find(type);
		if (it != job_queues_.end())
		{
			return it->second.get();
		}

		// Create a new queue for this priority level (with max size)
		auto new_queue = std::make_unique<job_queue>(std::make_optional<std::size_t>(10000)); // Default capacity
		auto* queue_ptr = new_queue.get();
		job_queues_[type] = std::move(new_queue);
		return queue_ptr;
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::enqueue(std::unique_ptr<job>&& value) -> common::VoidResult
	{
		// Try to cast to typed_job
		auto* typed_job_ptr = dynamic_cast<typed_job_t<job_type>*>(value.get());
		if (!typed_job_ptr)
		{
			return common::error_info{static_cast<int>(error_code::invalid_argument), "Job is not a typed job", "thread_system"};
		}

		value.release(); // Release ownership before casting
		return enqueue(std::unique_ptr<typed_job_t<job_type>>(typed_job_ptr));
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::enqueue_batch_ref(std::vector<std::unique_ptr<job>>& jobs)
		-> common::VoidResult
	{
		std::unique_lock lock(queues_mutex_);

		for (auto& job : jobs)
		{
			auto* typed_job_ptr = dynamic_cast<typed_job_t<job_type>*>(job.get());
			if (!typed_job_ptr)
			{
				continue; // Skip non-typed jobs
			}

			auto priority = typed_job_ptr->priority();
			auto* queue = get_or_create_queue(priority);

			job.release(); // Release ownership
			auto result = queue->enqueue(std::unique_ptr<kcenon::thread::job>(typed_job_ptr));
			if (result.is_err())
			{
				return result;
			}
		}

		return common::ok();
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs)
		-> common::VoidResult
	{
		return enqueue_batch_ref(jobs);
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::enqueue(std::unique_ptr<typed_job_t<job_type>>&& value)
		-> common::VoidResult
	{
		if (!value)
		{
			return common::error_info{static_cast<int>(error_code::invalid_argument), "Null job", "thread_system"};
		}

		std::unique_lock lock(queues_mutex_);

		auto priority = value->priority();
		auto* queue = get_or_create_queue(priority);

		return queue->enqueue(std::move(value));
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::enqueue_batch(
		std::vector<std::unique_ptr<typed_job_t<job_type>>>&& jobs) -> common::VoidResult
	{
		std::unique_lock lock(queues_mutex_);

		for (auto& job : jobs)
		{
			if (!job)
			{
				continue;
			}

			auto priority = job->priority();
			auto* queue = get_or_create_queue(priority);

			auto result = queue->enqueue(std::move(job));
			if (result.is_err())
			{
				return result;
			}
		}

		return common::ok();
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::dequeue() -> common::Result<std::unique_ptr<job>>
	{
		std::shared_lock lock(queues_mutex_);

		// Try to dequeue from any available queue
		for (auto& [priority, queue] : job_queues_)
		{
			auto result = queue->dequeue();
			if (result.is_ok())
			{
				return std::move(result.value());
			}
		}

		return common::error_info{static_cast<int>(error_code::queue_empty), "No jobs available", "thread_system"};
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::dequeue(const std::vector<job_type>& types)
		-> common::Result<std::unique_ptr<typed_job_t<job_type>>>
	{
		return dequeue(utility_module::span<const job_type>(types));
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::dequeue(utility_module::span<const job_type> types)
		-> common::Result<std::unique_ptr<typed_job_t<job_type>>>
	{
		std::shared_lock lock(queues_mutex_);

		// Try to dequeue from queues matching the specified types
		for (const auto& type : types)
		{
			auto it = job_queues_.find(type);
			if (it != job_queues_.end())
			{
				auto result = it->second->dequeue();
				if (result.is_ok())
				{
					auto job = std::move(result.value());
					auto* typed_job_ptr = static_cast<typed_job_t<job_type>*>(job.release());
					return std::unique_ptr<typed_job_t<job_type>>(typed_job_ptr);
				}
			}
		}

		return common::error_info{static_cast<int>(error_code::queue_empty), "No jobs available for specified types", "thread_system"};
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::clear() -> void
	{
		std::unique_lock lock(queues_mutex_);

		for (auto& [priority, queue] : job_queues_)
		{
			queue->clear();
		}
		job_queues_.clear();
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::empty(const std::vector<job_type>& types) const -> bool
	{
		return empty(utility_module::span<const job_type>(types));
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::empty(utility_module::span<const job_type> types) const -> bool
	{
		std::shared_lock lock(queues_mutex_);
		return empty_check_without_lock(types);
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::to_string() const -> std::string
	{
		std::shared_lock lock(queues_mutex_);

		std::ostringstream oss;
		oss << "typed_job_queue{queues: " << job_queues_.size() << "}";
		return oss.str();
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::stop() -> void
	{
		std::unique_lock lock(queues_mutex_);
		// Note: job_queue base class doesn't have stop(), so we just clear
		// Derived classes can override if needed
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::empty_check_without_lock(
		const std::vector<job_type>& types) const -> bool
	{
		return empty_check_without_lock(utility_module::span<const job_type>(types));
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::empty_check_without_lock(
		utility_module::span<const job_type> types) const -> bool
	{
		for (const auto& type : types)
		{
			auto it = job_queues_.find(type);
			if (it != job_queues_.end() && !it->second->empty())
			{
				return false;
			}
		}
		return true;
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::try_dequeue_from_priority(const job_type& priority)
		-> std::optional<std::unique_ptr<typed_job_t<job_type>>>
	{
		auto it = job_queues_.find(priority);
		if (it == job_queues_.end())
		{
			return std::nullopt;
		}

		auto result = it->second->dequeue();
		if (result.is_err())
		{
			return std::nullopt;
		}

		auto job = std::move(result.value());
		auto* typed_job_ptr = static_cast<typed_job_t<job_type>*>(job.release());
		return std::unique_ptr<typed_job_t<job_type>>(typed_job_ptr);
	}

	// Explicit template instantiation for job_types
	template class typed_job_queue_t<job_types>;

} // namespace kcenon::thread
