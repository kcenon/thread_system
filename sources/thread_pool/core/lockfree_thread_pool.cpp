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

#include "lockfree_thread_pool.h"
#include "../../utilities/core/formatter.h"

namespace thread_pool_module
{
	lockfree_thread_pool::lockfree_thread_pool(const std::string& thread_title)
		: thread_title_(thread_title)
		, start_pool_(false)
		, job_queue_(std::make_shared<lockfree_job_queue>())
	{
	}

	lockfree_thread_pool::~lockfree_thread_pool(void)
	{
		if (start_pool_.load())
		{
			stop();
		}
	}

	auto lockfree_thread_pool::get_ptr(void) -> std::shared_ptr<lockfree_thread_pool>
	{
		return shared_from_this();
	}

	auto lockfree_thread_pool::start(void) -> std::optional<std::string>
	{
		bool expected = false;
		if (!start_pool_.compare_exchange_strong(expected, true))
		{
			return formatter::format("Lockfree thread pool '{}' is already started", 
									thread_title_);
		}

		// Start all workers
		std::lock_guard<std::mutex> lock(workers_mutex_);
		for (auto& worker : workers_)
		{
			if (!worker)
			{
				continue;
			}

			worker->set_job_queue(job_queue_);
			auto result = worker->start();
			if (result.has_error())
			{
				// If any worker fails to start, stop all previously started workers
				stop(true);
				return formatter::format("Failed to start worker in pool '{}': {}", 
										thread_title_, result.get_error().message());
			}
		}

		return std::nullopt;
	}

	auto lockfree_thread_pool::get_job_queue(void) -> std::shared_ptr<lockfree_job_queue>
	{
		return job_queue_;
	}

	auto lockfree_thread_pool::enqueue(std::unique_ptr<job>&& job) -> std::optional<std::string>
	{
		if (!job)
		{
			return std::string("Cannot enqueue null job");
		}

		if (!start_pool_.load())
		{
			return formatter::format("Lockfree thread pool '{}' is not started", 
									thread_title_);
		}

		auto result = job_queue_->enqueue(std::move(job));
		if (result.has_error())
		{
			return result.get_error().message();
		}

		return std::nullopt;
	}

	auto lockfree_thread_pool::enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) 
		-> std::optional<std::string>
	{
		if (jobs.empty())
		{
			return std::nullopt;
		}

		if (!start_pool_.load())
		{
			return formatter::format("Lockfree thread pool '{}' is not started", 
									thread_title_);
		}

		auto result = job_queue_->enqueue_batch(std::move(jobs));
		if (result.has_error())
		{
			return result.get_error().message();
		}

		return std::nullopt;
	}

	auto lockfree_thread_pool::enqueue(std::unique_ptr<lockfree_thread_worker>&& worker) 
		-> std::optional<std::string>
	{
		if (!worker)
		{
			return std::string("Cannot enqueue null worker");
		}

		std::lock_guard<std::mutex> lock(workers_mutex_);
		
		// If pool is already started, start the new worker immediately
		if (start_pool_.load())
		{
			worker->set_job_queue(job_queue_);
			auto result = worker->start();
			if (result.has_error())
			{
				return formatter::format("Failed to start worker: {}", 
										result.get_error().message());
			}
		}

		workers_.push_back(std::move(worker));
		return std::nullopt;
	}

	auto lockfree_thread_pool::enqueue_batch(
		std::vector<std::unique_ptr<lockfree_thread_worker>>&& workers)
		-> std::optional<std::string>
	{
		if (workers.empty())
		{
			return std::nullopt;
		}

		std::lock_guard<std::mutex> lock(workers_mutex_);

		// If pool is already started, start all new workers
		if (start_pool_.load())
		{
			for (auto& worker : workers)
			{
				if (!worker)
				{
					continue;
				}

				worker->set_job_queue(job_queue_);
				auto result = worker->start();
				if (result.has_error())
				{
					return formatter::format("Failed to start worker: {}", 
											result.get_error().message());
				}
			}
		}

		// Move all workers to the pool
		for (auto& worker : workers)
		{
			if (worker)
			{
				workers_.push_back(std::move(worker));
			}
		}

		return std::nullopt;
	}

	auto lockfree_thread_pool::stop(bool immediately) -> std::optional<std::string>
	{
		bool expected = true;
		if (!start_pool_.compare_exchange_strong(expected, false))
		{
			return formatter::format("Lockfree thread pool '{}' is not started", 
									thread_title_);
		}

		// Clear the queue if immediate stop is requested
		if (immediately && job_queue_)
		{
			job_queue_->clear();
		}

		// Stop all workers
		std::lock_guard<std::mutex> lock(workers_mutex_);
		std::vector<std::string> errors;

		for (auto& worker : workers_)
		{
			if (!worker)
			{
				continue;
			}

			auto result = worker->stop();
			if (result.has_error())
			{
				errors.push_back(result.get_error().message());
			}
		}

		if (!errors.empty())
		{
			std::string error_msg = "Errors occurred while stopping workers: ";
			for (size_t i = 0; i < errors.size(); ++i)
			{
				if (i > 0) error_msg += "; ";
				error_msg += errors[i];
			}
			return error_msg;
		}

		return std::nullopt;
	}

	auto lockfree_thread_pool::to_string(void) const -> std::string
	{
		std::lock_guard<std::mutex> lock(workers_mutex_);
		
		auto stats = get_queue_statistics();
		
		return formatter::format(
			"lockfree_thread_pool [Title: {}, Running: {}, Workers: {}, "
			"Queue Size: {}, Total Enqueued: {}, Total Dequeued: {}, "
			"Avg Enqueue Latency: {:.2f}ns, Avg Dequeue Latency: {:.2f}ns]",
			thread_title_,
			start_pool_.load() ? "true" : "false",
			workers_.size(),
			stats.current_size,
			stats.enqueue_count,
			stats.dequeue_count,
			stats.get_average_enqueue_latency_ns(),
			stats.get_average_dequeue_latency_ns()
		);
	}

	auto lockfree_thread_pool::set_job_queue(std::shared_ptr<lockfree_job_queue> job_queue) -> void
	{
		if (!job_queue)
		{
			return;
		}

		// Don't allow changing queue while pool is running
		if (start_pool_.load())
		{
			return;
		}

		job_queue_ = job_queue;
	}

	auto lockfree_thread_pool::get_queue_statistics(void) const 
		-> lockfree_job_queue::queue_statistics
	{
		if (job_queue_)
		{
			return job_queue_->get_statistics();
		}
		return {};
	}

	auto lockfree_thread_pool::is_running(void) const -> bool
	{
		return start_pool_.load();
	}

	auto lockfree_thread_pool::worker_count(void) const -> size_t
	{
		std::lock_guard<std::mutex> lock(workers_mutex_);
		return workers_.size();
	}

} // namespace thread_pool_module