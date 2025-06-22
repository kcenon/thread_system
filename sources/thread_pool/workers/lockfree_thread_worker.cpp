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

#include "lockfree_thread_worker.h"
#include "../../utilities/core/formatter.h"
#include <thread>
#include <algorithm>

namespace thread_pool_module
{
	lockfree_thread_worker::lockfree_thread_worker(const bool& use_time_tag,
												   const backoff_config& config)
		: thread_base("lockfree_thread_worker")
		, use_time_tag_(use_time_tag)
		, backoff_config_(config)
		, current_backoff_(config.min_backoff)
	{
	}

	lockfree_thread_worker::~lockfree_thread_worker(void)
	{
		if (is_running())
		{
			stop();
		}
	}

	auto lockfree_thread_worker::set_job_queue(std::shared_ptr<lockfree_job_queue> job_queue) -> void
	{
		job_queue_ = job_queue;
	}

	auto lockfree_thread_worker::get_job_queue(void) const -> std::shared_ptr<lockfree_job_queue>
	{
		return job_queue_;
	}

	auto lockfree_thread_worker::get_statistics(void) const -> worker_statistics
	{
		return {
			stats_.jobs_processed.load(),
			stats_.total_processing_time_ns.load(),
			stats_.idle_time_ns.load(),
			stats_.backoff_count.load(),
			stats_.batch_dequeue_count.load()
		};
	}

	auto lockfree_thread_worker::set_batch_processing(bool enable, size_t batch_size) -> void
	{
		batch_processing_enabled_.store(enable);
		batch_size_.store(batch_size);
	}

	auto lockfree_thread_worker::to_string(void) const -> std::string
	{
		auto stats = get_statistics();
		double avg_processing_time = stats.jobs_processed > 0 
			? static_cast<double>(stats.total_processing_time_ns) / stats.jobs_processed 
			: 0.0;

		return formatter::format(
			"lockfree_thread_worker [Title: {}, Running: {}, Jobs Processed: {}, "
			"Avg Processing Time: {:.2f}ns, Idle Time: {}ms, Backoffs: {}, "
			"Batch Processing: {}, Batch Size: {}]",
			get_thread_title(),
			is_running() ? "true" : "false",
			stats.jobs_processed,
			avg_processing_time,
			stats.idle_time_ns / 1'000'000, // Convert to ms
			stats.backoff_count,
			batch_processing_enabled_.load() ? "enabled" : "disabled",
			batch_size_.load()
		);
	}

	auto lockfree_thread_worker::should_continue_work(void) const -> bool
	{
		if (!job_queue_)
		{
			return false;
		}

		// Continue if queue is not empty or if we haven't been asked to stop
		return !job_queue_->empty() || is_running();
	}

	auto lockfree_thread_worker::do_work(void) -> result_void
	{
		if (!job_queue_)
		{
			return result_void(error(error_code::invalid_argument, 
									"Job queue not set for lockfree worker"));
		}

		[[maybe_unused]] auto start_time = use_time_tag_ ? std::chrono::steady_clock::now() 
										: std::chrono::steady_clock::time_point{};

		// Try batch processing if enabled
		if (batch_processing_enabled_.load())
		{
			size_t processed = process_batch();
			if (processed > 0)
			{
				current_backoff_ = backoff_config_.min_backoff;
				return {};
			}
		}

		// Try single job dequeue
		auto dequeue_result = job_queue_->dequeue();
		
		if (dequeue_result.has_value())
		{
			auto job = std::move(dequeue_result.value());
			
			if (use_time_tag_)
			{
				auto process_start = std::chrono::steady_clock::now();
				auto idle_duration = process_start - last_job_time_;
				stats_.idle_time_ns.fetch_add(
					static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(idle_duration).count()));
				
				auto result = process_job(std::move(job));
				
				auto process_end = std::chrono::steady_clock::now();
				auto process_duration = process_end - process_start;
				stats_.total_processing_time_ns.fetch_add(
					static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(process_duration).count()));
				
				last_job_time_ = process_end;
				return result;
			}
			else
			{
				return process_job(std::move(job));
			}
		}
		else if (!dequeue_result.has_value())
		{
			// Implement backoff strategy for contention
			static size_t backoff_attempt = 0;
			backoff(backoff_attempt++);
			
			// Reset backoff counter after successful dequeue
			if (backoff_attempt > backoff_config_.spin_count)
			{
				backoff_attempt = 0;
			}
		}

		return {};
	}

	auto lockfree_thread_worker::before_start(void) -> result_void
	{
		if (!job_queue_)
		{
			return result_void(error(error_code::invalid_argument, 
									"Job queue not set before starting lockfree worker"));
		}

		// Initialize timing
		last_job_time_ = std::chrono::steady_clock::now();
		
		// Reset statistics
		stats_.jobs_processed.store(0);
		stats_.total_processing_time_ns.store(0);
		stats_.idle_time_ns.store(0);
		stats_.backoff_count.store(0);
		stats_.batch_dequeue_count.store(0);
		
		return {};
	}

	auto lockfree_thread_worker::after_stop(void) -> result_void
	{
		// Process any remaining jobs
		while (job_queue_ && !job_queue_->empty())
		{
			auto dequeue_result = job_queue_->dequeue();
			if (dequeue_result.has_value())
			{
				process_job(std::move(dequeue_result.value()));
			}
		}

		return {};
	}

	auto lockfree_thread_worker::backoff(size_t attempt) -> void
	{
		if (attempt < backoff_config_.spin_count)
		{
			// Spin wait for initial attempts
			size_t spin_count = 1ULL << attempt;
			for (size_t i = 0; i < spin_count; ++i)
			{
				// Busy wait - prevent compiler optimization
				std::atomic_thread_fence(std::memory_order_relaxed);
			}
		}
		else
		{
			// Exponential backoff with sleep
			stats_.backoff_count.fetch_add(1);
			
			current_backoff_ = std::chrono::nanoseconds(
				static_cast<int64_t>(current_backoff_.count() * backoff_config_.backoff_multiplier));
			
			// Cap at maximum backoff
			if (current_backoff_ > backoff_config_.max_backoff)
			{
				current_backoff_ = backoff_config_.max_backoff;
			}
			
			std::this_thread::sleep_for(current_backoff_);
		}
	}

	auto lockfree_thread_worker::process_job(std::unique_ptr<job> job) -> result_void
	{
		if (!job)
		{
			return result_void(error(error_code::invalid_argument, "Null job in process_job"));
		}

		stats_.jobs_processed.fetch_add(1);
		
		try
		{
			return job->do_work();
		}
		catch (const std::exception& e)
		{
			return result_void(error(error_code::job_execution_failed, 
								   formatter::format("Job execution failed: {}", e.what())));
		}
		catch (...)
		{
			return result_void(error(error_code::job_execution_failed, "Unknown error in job execution"));
		}
	}

	auto lockfree_thread_worker::process_batch() -> size_t
	{
		size_t processed = 0;
		size_t batch_size = batch_size_.load();
		
		std::vector<std::unique_ptr<job>> jobs;
		jobs.reserve(batch_size);
		
		// Try to dequeue multiple jobs
		for (size_t i = 0; i < batch_size; ++i)
		{
			auto dequeue_result = job_queue_->dequeue();
			if (dequeue_result.has_value())
			{
				jobs.push_back(std::move(dequeue_result.value()));
			}
			else
			{
				break;
			}
		}
		
		if (!jobs.empty())
		{
			stats_.batch_dequeue_count.fetch_add(1);
			
			// Process all dequeued jobs
			for (auto& job : jobs)
			{
				process_job(std::move(job));
				++processed;
			}
		}
		
		return processed;
	}

} // namespace thread_pool_module