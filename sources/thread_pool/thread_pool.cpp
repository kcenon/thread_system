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

#include "thread_pool.h"

#include "logger.h"
#include "formatter.h"

using namespace log_module;
using namespace utility_module;

namespace thread_pool_module
{
	thread_pool::thread_pool(void) : job_queue_(std::make_shared<job_queue>()), start_pool_(false)
	{
	}

	thread_pool::~thread_pool() { stop(); }

	auto thread_pool::get_ptr(void) -> std::shared_ptr<thread_pool>
	{
		return this->shared_from_this();
	}

	auto thread_pool::start(void) -> std::optional<std::string>
	{
		if (workers_.empty())
		{
			return "No workers to start";
		}

		for (auto& worker : workers_)
		{
			auto start_error = worker->start();
			if (start_error.has_value())
			{
				stop();
				return start_error;
			}
		}

		start_pool_.store(true);

		return std::nullopt;
	}

	auto thread_pool::get_job_queue(void) -> std::shared_ptr<job_queue> { return job_queue_; }

	auto thread_pool::enqueue(std::unique_ptr<job>&& job) -> std::optional<std::string>
	{
		if (job == nullptr)
		{
			return "Job is null";
		}

		if (job_queue_ == nullptr)
		{
			return "Job queue is null";
		}

		auto enqueue_error = job_queue_->enqueue(std::move(job));
		if (enqueue_error.has_value())
		{
			return enqueue_error;
		}

		return std::nullopt;
	}

	auto thread_pool::enqueue(std::unique_ptr<thread_worker>&& worker) -> std::optional<std::string>
	{
		if (worker == nullptr)
		{
			return "Worker is null";
		}

		if (job_queue_ == nullptr)
		{
			return "Job queue is null";
		}

		worker->set_job_queue(job_queue_);

		if (start_pool_.load())
		{
			auto start_error = worker->start();
			if (start_error.has_value())
			{
				stop();

				return start_error;
			}
		}

		workers_.emplace_back(std::move(worker));

		return std::nullopt;
	}

	auto thread_pool::stop(const bool& immediately_stop) -> void
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
			auto stop_error = worker->stop();
			if (stop_error.has_value())
			{
				log_module::write(log_types::Error, "error stopping worker: {}",
								  stop_error.value_or("unknown error"));
			}
		}

		start_pool_.store(false);
	}
} // namespace thread_pool_module