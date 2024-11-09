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

#include "priority_thread_pool.h"

#include "logger.h"
#include "formatter.h"

using namespace log_module;
using namespace utility_module;

namespace priority_thread_pool_module
{
	template <typename priority_type>
	priority_thread_pool<priority_type>::priority_thread_pool(void)
		: job_queue_(std::make_shared<priority_job_queue<priority_type>>()), start_pool_(false)
	{
	}

	template <typename priority_type> priority_thread_pool<priority_type>::~priority_thread_pool()
	{
		stop();
	}

	template <typename priority_type>
	auto priority_thread_pool<priority_type>::get_ptr(void)
		-> std::shared_ptr<priority_thread_pool<priority_type>>
	{
		return this->shared_from_this();
	}

	template <typename priority_type>
	auto priority_thread_pool<priority_type>::start(void)
		-> std::tuple<bool, std::optional<std::string>>
	{
		if (workers_.empty())
		{
			return { false, "no workers to start" };
		}

		for (auto& worker : workers_)
		{
			auto [started, start_error] = worker->start();
			if (!started)
			{
				stop();
				return { false, start_error };
			}
		}

		start_pool_.store(true);

		return { true, std::nullopt };
	}

	template <typename priority_type>
	auto priority_thread_pool<priority_type>::get_job_queue(void)
		-> std::shared_ptr<priority_job_queue<priority_type>>
	{
		return job_queue_;
	}

	template <typename priority_type>
	auto priority_thread_pool<priority_type>::enqueue(
		std::unique_ptr<priority_job<priority_type>>&& job)
		-> std::tuple<bool, std::optional<std::string>>
	{
		if (job == nullptr)
		{
			return { false, "cannot enqueue null job" };
		}

		if (job_queue_ == nullptr)
		{
			return { false, "cannot enqueue job to null job queue" };
		}

		auto [enqueued, enqueue_error] = job_queue_->enqueue(std::move(job));
		if (!enqueued)
		{
			return { false, enqueue_error };
		}

		return { true, std::nullopt };
	}

	template <typename priority_type>
	auto priority_thread_pool<priority_type>::enqueue(
		std::unique_ptr<priority_thread_worker<priority_type>>&& worker)
		-> std::tuple<bool, std::optional<std::string>>
	{
		if (worker == nullptr)
		{
			return { false, "cannot enqueue null worker" };
		}

		if (job_queue_ == nullptr)
		{
			return { false, "cannot enqueue worker due to null job queue" };
		}

		worker->set_job_queue(job_queue_);

		if (start_pool_.load())
		{
			auto [started, start_error] = worker->start();
			if (!started)
			{
				stop();
				return { false, start_error };
			}
		}

		workers_.emplace_back(std::move(worker));

		return { true, std::nullopt };
	}

	template <typename priority_type>
	auto priority_thread_pool<priority_type>::stop(const bool& immediately_stop) -> void
	{
		if (!start_pool_.load())
		{
			return;
		}

		if (job_queue_ != nullptr)
		{
			job_queue_->stop_waiting_dequeue();

			if (immediately_stop)
			{
				job_queue_->clear();
			}
		}

		for (auto& worker : workers_)
		{
			auto [stopped, stop_error] = worker->stop();
			if (!stopped)
			{
				log_module::write(log_types::Error, "error stopping worker: {}",
								  stop_error.value_or("unknown error"));
			}
		}

		start_pool_.store(false);
	}
} // namespace priority_thread_pool_module