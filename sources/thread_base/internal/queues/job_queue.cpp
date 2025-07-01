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

#include "job_queue.h"

namespace thread_module
{
	job_queue::job_queue() : notify_(true), stop_(false), mutex_(), condition_(), queue_() {}

	job_queue::~job_queue(void) {}

	auto job_queue::get_ptr(void) -> std::shared_ptr<job_queue> { return shared_from_this(); }

	auto job_queue::is_stopped() const -> bool { return stop_.load(); }

	auto job_queue::set_notify(bool notify) -> void { notify_.store(notify); }

	auto job_queue::enqueue(std::unique_ptr<job>&& value) -> result_void
	{
		if (stop_.load())
		{
			return error{error_code::queue_stopped, "Job queue is stopped"};
		}

		if (value == nullptr)
		{
			return error{error_code::invalid_argument, "cannot enqueue null job"};
		}

		std::scoped_lock<std::mutex> lock(mutex_);

		queue_.push_back(std::move(value));

		if (notify_)
		{
			condition_.notify_one();
		}

		return {};
	}

	auto job_queue::enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> result_void
	{
		if (stop_.load())
		{
			return error{error_code::queue_stopped, "Job queue is stopped"};
		}

		if (jobs.empty())
		{
			return error{error_code::invalid_argument, "cannot enqueue empty batch"};
		}

		for (auto& job : jobs)
		{
			if (job == nullptr)
			{
				return error{error_code::invalid_argument, "cannot enqueue null job in batch"};
			}
		}

		std::scoped_lock<std::mutex> lock(mutex_);

		for (auto& job : jobs)
		{
			queue_.push_back(std::move(job));
		}

		if (notify_)
		{
			condition_.notify_one();
		}

		return {};
	}

	auto job_queue::dequeue() -> result<std::unique_ptr<job>>
	{
		std::unique_lock<std::mutex> lock(mutex_);
		condition_.wait(lock, [this]() { return !queue_.empty() || stop_.load(); });

		if (queue_.empty())
		{
			return error{error_code::queue_empty, "there are no jobs to dequeue"};
		}

		auto value = std::move(queue_.front());
		queue_.pop_front();

		return value;
	}

	auto job_queue::dequeue_batch(void) -> std::deque<std::unique_ptr<job>>
	{
		std::deque<std::unique_ptr<job>> all_items;
		{
			std::scoped_lock<std::mutex> lock(mutex_);

			std::swap(queue_, all_items);

			condition_.notify_all();
		}

		return all_items;
	}

	auto job_queue::clear(void) -> void
	{
		std::scoped_lock<std::mutex> lock(mutex_);

		queue_.clear();

		condition_.notify_all();
	}

	auto job_queue::empty(void) const -> bool
	{
		std::scoped_lock<std::mutex> lock(mutex_);

		return queue_.empty();
	}

	auto job_queue::stop_waiting_dequeue(void) -> void
	{
		std::scoped_lock<std::mutex> lock(mutex_);

		stop_.store(true);

		condition_.notify_all();
	}

	auto job_queue::size(void) const -> std::size_t
	{
		std::scoped_lock<std::mutex> lock(mutex_);
		return queue_.size();
	}

	auto job_queue::to_string(void) const -> std::string
	{
		return formatter::format("contained {} jobs", size());
	}
} // namespace thread_module