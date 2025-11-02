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

#include "typed_thread_pool.h"
#include "typed_thread_worker.h"
#include "typed_job_queue.h"
#include <sstream>

namespace kcenon::thread
{
	template <typename job_type>
	typed_thread_pool_t<job_type>::typed_thread_pool_t(
		const std::string& thread_title,
		const thread_context& context)
		: thread_title_(thread_title)
		, start_pool_(false)
		, job_queue_(std::make_shared<typed_job_queue_t<job_type>>())
		, context_(context)
	{
	}

	template <typename job_type>
	typed_thread_pool_t<job_type>::~typed_thread_pool_t()
	{
		if (start_pool_.load())
		{
			stop(false); // Wait for jobs to complete
		}
	}

	template <typename job_type>
	auto typed_thread_pool_t<job_type>::get_ptr() -> std::shared_ptr<typed_thread_pool_t<job_type>>
	{
		return this->shared_from_this();
	}

	template <typename job_type>
	auto typed_thread_pool_t<job_type>::start() -> result_void
	{
		bool expected = false;
		if (!start_pool_.compare_exchange_strong(expected, true))
		{
			return result_void(error(error_code::thread_already_running, "Thread pool already started"));
		}

		// Check if there are workers to start
		if (workers_.empty())
		{
			start_pool_.store(false); // Reset state since we didn't actually start
			return result_void(error(error_code::invalid_argument, "no workers to start"));
		}

		// Start all workers
		for (auto& worker : workers_)
		{
			if (worker)
			{
				worker->start();
			}
		}

		return {};
	}

	template <typename job_type>
	auto typed_thread_pool_t<job_type>::get_job_queue() -> std::shared_ptr<typed_job_queue_t<job_type>>
	{
		return job_queue_;
	}

	template <typename job_type>
	auto typed_thread_pool_t<job_type>::execute(std::unique_ptr<job>&& work) -> result_void
	{
		if (!start_pool_.load())
		{
			return result_void(error(error_code::thread_not_running, "Thread pool not started"));
		}

		return job_queue_->enqueue(std::move(work));
	}

	template <typename job_type>
	auto typed_thread_pool_t<job_type>::enqueue(std::unique_ptr<typed_job_t<job_type>>&& job)
		-> result_void
	{
		if (!start_pool_.load())
		{
			return result_void(error(error_code::thread_not_running, "Thread pool not started"));
		}

		return job_queue_->enqueue(std::move(job));
	}

	template <typename job_type>
	auto typed_thread_pool_t<job_type>::enqueue_batch(
		std::vector<std::unique_ptr<typed_job_t<job_type>>>&& jobs) -> result_void
	{
		if (!start_pool_.load())
		{
			return result_void(error(error_code::thread_not_running, "Thread pool not started"));
		}

		return job_queue_->enqueue_batch(std::move(jobs));
	}

	template <typename job_type>
	auto typed_thread_pool_t<job_type>::enqueue(
		std::unique_ptr<typed_thread_worker_t<job_type>>&& worker) -> result_void
	{
		if (!worker)
		{
			return result_void(error(error_code::invalid_argument, "Null worker"));
		}

		// Set the job queue for the worker
		worker->set_job_queue(job_queue_);
		worker->set_context(context_);

		// Start the worker if the pool is already started
		if (start_pool_.load())
		{
			worker->start();
		}

		workers_.push_back(std::move(worker));
		return {};
	}

	template <typename job_type>
	auto typed_thread_pool_t<job_type>::enqueue_batch(
		std::vector<std::unique_ptr<typed_thread_worker_t<job_type>>>&& workers) -> result_void
	{
		for (auto& worker : workers)
		{
			auto result = enqueue(std::move(worker));
			if (result.has_error())
			{
				return result;
			}
		}

		return {};
	}

	template <typename job_type>
	auto typed_thread_pool_t<job_type>::stop(bool clear_queue) -> result_void
	{
		// Use compare_exchange_strong to atomically check and set state
		// This prevents race conditions during concurrent stop() calls
		bool expected = true;
		if (!start_pool_.compare_exchange_strong(expected, false,
												  std::memory_order_acq_rel,
												  std::memory_order_acquire))
		{
			return {}; // Already stopped or being stopped by another thread
		}

		// Always stop the queue to prevent new jobs from being enqueued
		// This ensures consistent behavior with thread_pool and prevents
		// race conditions where jobs might be added after stop() is called
		if (job_queue_)
		{
			job_queue_->stop();

			// Optionally clear pending jobs for immediate shutdown
			if (clear_queue)
			{
				job_queue_->clear();
			}
		}

		// Stop all workers
		for (auto& worker : workers_)
		{
			if (worker)
			{
				worker->stop();
				// thread_base doesn't have join(), it manages threads internally
			}
		}

		return {};
	}

	template <typename job_type>
	auto typed_thread_pool_t<job_type>::to_string() const -> std::string
	{
		std::ostringstream oss;
		oss << "typed_thread_pool{"
			<< "title: " << thread_title_
			<< ", started: " << start_pool_.load()
			<< ", workers: " << workers_.size()
			<< "}";
		return oss.str();
	}

	template <typename job_type>
	auto typed_thread_pool_t<job_type>::set_job_queue(
		std::shared_ptr<typed_job_queue_t<job_type>> job_queue) -> void
	{
		job_queue_ = std::move(job_queue);

		// Update all workers with the new queue
		for (auto& worker : workers_)
		{
			if (worker)
			{
				worker->set_job_queue(job_queue_);
			}
		}
	}

	template <typename job_type>
	auto typed_thread_pool_t<job_type>::get_context() const -> const thread_context&
	{
		return context_;
	}

	// Explicit template instantiation for job_types
	template class typed_thread_pool_t<job_types>;

} // namespace kcenon::thread
