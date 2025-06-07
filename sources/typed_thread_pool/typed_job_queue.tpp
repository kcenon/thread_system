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

#include "typed_job_queue.h"

namespace typed_thread_pool_module
{
	template <typename job_type>
	typed_job_queue_t<job_type>::typed_job_queue_t(void) : job_queue(), queues_()
	{
	}

	template <typename job_type>
	typed_job_queue_t<job_type>::~typed_job_queue_t(void)
	{
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::enqueue(std::unique_ptr<job>&& value)
		-> result_void
	{
		if (stop_.load())
		{
			return error{error_code::queue_stopped, "Job queue is stopped"};
		}

		auto typed_job_ptr = dynamic_cast<typed_job_t<job_type>*>(value.get());

		if (!typed_job_ptr)
		{
			return error{error_code::job_invalid, "Enqueued job is not a typed_job"};
		}

		return enqueue(std::unique_ptr<typed_job_t<job_type>>(typed_job_ptr));
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::enqueue_batch_ref(std::vector<std::unique_ptr<job>>& jobs)
		-> result_void
	{
		if (stop_.load())
		{
			return error{error_code::queue_stopped, "Job queue is stopped"};
		}

		std::vector<std::unique_ptr<typed_job_t<job_type>>> typed_jobs;
		typed_jobs.reserve(jobs.size());

		for (auto& job : jobs)
		{
			auto typed_job_ptr = dynamic_cast<typed_job_t<job_type>*>(job.get());
			if (!typed_job_ptr)
			{
				return error{error_code::job_invalid, "Enqueued job is not a typed_job"};
			}

			typed_jobs.push_back(
				std::unique_ptr<typed_job_t<job_type>>(typed_job_ptr));
		}

		return enqueue_batch(std::move(typed_jobs));
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs)
		-> result_void
	{
		if (stop_.load())
		{
			return error{error_code::queue_stopped, "Job queue is stopped"};
		}

		std::vector<std::unique_ptr<typed_job_t<job_type>>> typed_jobs;
		typed_jobs.reserve(jobs.size());

		for (auto& job : jobs)
		{
			auto typed_job_ptr = dynamic_cast<typed_job_t<job_type>*>(job.release());
			if (!typed_job_ptr)
			{
				return error{error_code::job_invalid, "Enqueued job is not a typed_job"};
			}

			typed_jobs.push_back(
				std::unique_ptr<typed_job_t<job_type>>(typed_job_ptr));
		}

		return enqueue_batch(std::move(typed_jobs));
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::enqueue(
		std::unique_ptr<typed_job_t<job_type>>&& value) -> result_void
	{
		if (stop_.load())
		{
			return error{error_code::queue_stopped, "Job queue is stopped"};
		}

		if (value == nullptr)
		{
			return error{error_code::job_invalid, "Cannot enqueue null job"};
		}

		auto job_priority = value->priority();

		std::scoped_lock<std::mutex> lock(mutex_);

		auto iter = queues_.find(job_priority);
		if (iter != queues_.end())
		{
			iter->second.push_back(std::move(value));

			queue_sizes_[job_priority].fetch_add(1);

			condition_.notify_one();

			return result_void{};
		}

		iter = queues_
				   .emplace(job_priority,
							std::deque<std::unique_ptr<typed_job_t<job_type>>>())
				   .first;
		iter->second.push_back(std::move(value));

		queue_sizes_.emplace(job_priority, 1);

		condition_.notify_one();

		return result_void{};
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::enqueue_batch(
		std::vector<std::unique_ptr<typed_job_t<job_type>>>&& jobs)
		-> result_void
	{
		if (stop_.load())
		{
			return error{error_code::queue_stopped, "Job queue is stopped"};
		}

		if (jobs.empty())
		{
			return error{error_code::job_invalid, "Cannot enqueue empty batch"};
		}

		std::scoped_lock<std::mutex> lock(mutex_);

		for (auto& job : jobs)
		{
			auto job_priority = job->priority();

			auto iter = queues_.find(job_priority);
			if (iter != queues_.end())
			{
				iter->second.push_back(std::move(job));

				queue_sizes_[job_priority].fetch_add(1);
			}
			else
			{
				iter = queues_
						   .emplace(job_priority,
									std::deque<std::unique_ptr<typed_job_t<job_type>>>())
						   .first;
				iter->second.push_back(std::move(job));

				queue_sizes_.emplace(job_priority, 1);
			}
		}

		condition_.notify_all();

		return result_void{};
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::dequeue()
		-> result<std::unique_ptr<job>>
	{
		return error{error_code::queue_empty, "Dequeue operation without specified types is "
								   "not supported in typed_job_queue"};
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::dequeue(const std::vector<job_type>& types)
		-> result<std::unique_ptr<typed_job_t<job_type>>>
	{
		std::unique_lock<std::mutex> lock(mutex_);

		auto dequeue_job
			= [this, &types]() -> std::optional<std::unique_ptr<typed_job_t<job_type>>>
		{
			for (const auto& priority : types)
			{
				if (auto value = std::move(try_dequeue_from_priority(priority)))
				{
					return value;
				}
			}
			return std::nullopt;
		};

		std::optional<std::unique_ptr<typed_job_t<job_type>>> result;
		condition_.wait(lock,
						[&]()
						{
							result = dequeue_job();
							return result.has_value() || stop_.load();
						});

		if (result.has_value())
		{
			auto job = std::move(result.value());
			lock.unlock();
			return job;
		}

		lock.unlock();

		if (stop_.load())
		{
			return error{error_code::queue_stopped, "Job queue is stopped"};
		}

		return error{error_code::queue_empty, "Unexpected error: No job found after waiting"};
	}
	
	template <typename job_type>
	auto typed_job_queue_t<job_type>::dequeue(utility_module::span<const job_type> types)
		-> result<std::unique_ptr<typed_job_t<job_type>>>
	{
		std::unique_lock<std::mutex> lock(mutex_);

		auto dequeue_job
			= [this, &types]() -> std::optional<std::unique_ptr<typed_job_t<job_type>>>
		{
			for (const auto& priority : types)
			{
				if (auto value = std::move(try_dequeue_from_priority(priority)))
				{
					return value;
				}
			}
			return std::nullopt;
		};

		std::optional<std::unique_ptr<typed_job_t<job_type>>> result;
		condition_.wait(lock,
						[&]()
						{
							result = dequeue_job();
							return result.has_value() || stop_.load();
						});

		if (result.has_value())
		{
			auto job = std::move(result.value());
			lock.unlock();
			return job;
		}

		lock.unlock();

		if (stop_.load())
		{
			return error{error_code::queue_stopped, "Job queue is stopped"};
		}

		return error{error_code::queue_empty, "Unexpected error: No job found after waiting"};
	}

	template <typename job_type> auto typed_job_queue_t<job_type>::clear() -> void
	{
		std::scoped_lock<std::mutex> lock(mutex_);

		for (auto& pair : queues_)
		{
			std::deque<std::unique_ptr<typed_job_t<job_type>>> empty;
			std::swap(pair.second, empty);
			queue_sizes_[pair.first].store(0);
		}

		queues_.clear();

		condition_.notify_all();
	}
    
	template <typename job_type>
	auto typed_job_queue_t<job_type>::stop() -> void
	{
		stop_.store(true);
		condition_.notify_all();
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::empty(
		const std::vector<job_type>& types) const -> bool
	{
		std::scoped_lock<std::mutex> lock(mutex_);

		return empty_check_without_lock(types);
	}
	
	template <typename job_type>
	auto typed_job_queue_t<job_type>::empty(
		utility_module::span<const job_type> types) const -> bool
	{
		std::scoped_lock<std::mutex> lock(mutex_);

		return empty_check_without_lock(types);
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::to_string(void) const -> std::string
	{
		std::string format_string;

		formatter::format_to(std::back_inserter(format_string), "Type job queue:\n");
		for (const auto& pair : queue_sizes_)
		{
			formatter::format_to(std::back_inserter(format_string), "\tType: {} -> {} jobs\n",
								 pair.first, pair.second.load());
		}

		return format_string;
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::empty_check_without_lock(
		const std::vector<job_type>& types) const -> bool
	{
		for (const auto& priority : types)
		{
			auto it = queues_.find(priority);
			if (it != queues_.end() && !it->second.empty())
			{
				return false;
			}
		}

		return true;
	}
	
	template <typename job_type>
	auto typed_job_queue_t<job_type>::empty_check_without_lock(
		utility_module::span<const job_type> types) const -> bool
	{
		for (const auto& priority : types)
		{
			auto it = queues_.find(priority);
			if (it != queues_.end() && !it->second.empty())
			{
				return false;
			}
		}

		return true;
	}

	template <typename job_type>
	auto typed_job_queue_t<job_type>::try_dequeue_from_priority(
		const job_type& priority)
		-> std::optional<std::unique_ptr<typed_job_t<job_type>>>
	{
		auto it = queues_.find(priority);
		if (it == queues_.end() || it->second.empty())
		{
			return std::nullopt;
		}

		auto value = std::move(it->second.front());
		it->second.pop_front();

		queue_sizes_[priority].fetch_sub(1);

		return value;
	}
} // namespace typed_thread_pool_module