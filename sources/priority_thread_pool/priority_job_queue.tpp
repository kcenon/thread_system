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

#include "priority_job_queue.h"

namespace priority_thread_pool_module
{
	template <typename priority_type>
	priority_job_queue_t<priority_type>::priority_job_queue_t(void) : job_queue(), queues_()
	{
	}

	template <typename priority_type>
	priority_job_queue_t<priority_type>::~priority_job_queue_t(void)
	{
	}

	template <typename priority_type>
	auto priority_job_queue_t<priority_type>::enqueue(std::unique_ptr<job>&& value)
		-> std::optional<std::string>
	{
		if (stop_.load())
		{
			return "Job queue is stopped";
		}

		auto priority_job_ptr = dynamic_cast<priority_job_t<priority_type>*>(value.get());

		if (!priority_job_ptr)
		{
			return "Enqueued job is not a priority_job";
		}

		return enqueue(std::unique_ptr<priority_job_t<priority_type>>(priority_job_ptr));
	}

	template <typename priority_type>
	auto priority_job_queue_t<priority_type>::enqueue(
		std::unique_ptr<priority_job_t<priority_type>>&& value) -> std::optional<std::string>
	{
		if (stop_.load())
		{
			return "Job queue is stopped";
		}

		if (value == nullptr)
		{
			return "cannot enqueue null job";
		}

		auto job_priority = value->priority();

		std::scoped_lock<std::mutex> lock(mutex_);

		auto iter = queues_.find(job_priority);
		if (iter != queues_.end())
		{
			iter->second.push_back(std::move(value));

			queue_sizes_[job_priority].fetch_add(1);

			condition_.notify_one();

			return std::nullopt;
		}

		iter = queues_
				   .emplace(job_priority,
							std::deque<std::unique_ptr<priority_job_t<priority_type>>>())
				   .first;
		iter->second.push_back(std::move(value));

		queue_sizes_.emplace(job_priority, 1);

		condition_.notify_one();

		return std::nullopt;
	}

	template <typename priority_type>
	auto priority_job_queue_t<priority_type>::dequeue()
		-> std::tuple<std::optional<std::unique_ptr<job>>, std::optional<std::string>>
	{
		return { std::nullopt, "Dequeue operation without specified priorities is "
							   "not supported in priority_job_queue" };
	}

	template <typename priority_type>
	auto priority_job_queue_t<priority_type>::dequeue(const std::vector<priority_type>& priorities)
		-> std::tuple<std::optional<std::unique_ptr<priority_job_t<priority_type>>>,
					  std::optional<std::string>>
	{
		std::unique_lock<std::mutex> lock(mutex_);

		auto dequeue_job
			= [this, &priorities]() -> std::optional<std::unique_ptr<priority_job_t<priority_type>>>
		{
			for (const auto& priority : priorities)
			{
				if (auto value = std::move(try_dequeue_from_priority(priority)))
				{
					return value;
				}
			}
			return std::nullopt;
		};

		std::optional<std::unique_ptr<priority_job_t<priority_type>>> result;
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
			return { std::move(job), std::nullopt };
		}

		lock.unlock();

		if (stop_.load())
		{
			return { std::nullopt, "Job queue is stopped" };
		}

		return { std::nullopt, "Unexpected error: No job found after waiting" };
	}

	template <typename priority_type> auto priority_job_queue_t<priority_type>::clear() -> void
	{
		std::scoped_lock<std::mutex> lock(mutex_);

		for (auto& pair : queues_)
		{
			std::deque<std::unique_ptr<priority_job_t<priority_type>>> empty;
			std::swap(pair.second, empty);
			queue_sizes_[pair.first].store(0);
		}

		queues_.clear();

		condition_.notify_all();
	}

	template <typename priority_type>
	auto priority_job_queue_t<priority_type>::empty(
		const std::vector<priority_type>& priorities) const -> bool
	{
		std::scoped_lock<std::mutex> lock(mutex_);

		return empty_check_without_lock(priorities);
	}

	template <typename priority_type>
	auto priority_job_queue_t<priority_type>::to_string(void) const -> std::string
	{
		std::string format_string;

		formatter::format_to(std::back_inserter(format_string), "Priority job queue:\n");
		for (const auto& pair : queue_sizes_)
		{
			formatter::format_to(std::back_inserter(format_string), "\tPriority: {} -> {} jobs\n",
								 pair.first, pair.second.load());
		}

		return format_string;
	}

	template <typename priority_type>
	auto priority_job_queue_t<priority_type>::empty_check_without_lock(
		const std::vector<priority_type>& priorities) const -> bool
	{
		for (const auto& priority : priorities)
		{
			auto it = queues_.find(priority);
			if (it != queues_.end() && !it->second.empty())
			{
				return false;
			}
		}

		return true;
	}

	template <typename priority_type>
	auto priority_job_queue_t<priority_type>::try_dequeue_from_priority(
		const priority_type& priority)
		-> std::optional<std::unique_ptr<priority_job_t<priority_type>>>
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
} // namespace priority_thread_pool_module