#pragma once

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

#include "../../thread_base/core/thread_base.h"
#include "../../utilities/core/formatter.h"
#include "../../thread_base/lockfree/queues/lockfree_job_queue.h"
#include "../../utilities/conversion/convert_string.h"
#include "../detail/forward_declarations.h"
#include "worker_policy.h"

#include <memory>
#include <vector>
#include <atomic>
#include <chrono>

using namespace utility_module;
using namespace thread_module;

namespace thread_pool_module
{
	/**
	 * @class lockfree_thread_worker
	 * @brief A specialized worker thread optimized for lock-free job processing.
	 *
	 * The lockfree_thread_worker class is designed to work with lockfree_job_queue
	 * for maximum performance under high contention. It implements optimizations
	 * specific to lock-free operations including:
	 * - Backoff strategies to reduce contention
	 * - Batch dequeue operations when possible
	 * - Minimal synchronization overhead
	 *
	 * ### Key Differences from thread_worker
	 * - Uses lockfree_job_queue instead of standard job_queue
	 * - Implements exponential backoff for failed dequeue attempts
	 * - Supports batch processing for improved throughput
	 * - Optimized for NUMA architectures
	 *
	 * ### Performance Characteristics
	 * - Zero contention on enqueue operations
	 * - Minimal contention on dequeue operations
	 * - Better cache locality through node pooling
	 * - Reduced false sharing through careful memory layout
	 *
	 * @see lockfree_job_queue The queue implementation used by this worker
	 * @see lockfree_thread_pool The pool that manages these workers
	 */
	class lockfree_thread_worker : public thread_base
	{
	public:
		/**
		 * @brief Configuration for backoff strategy
		 */
		struct backoff_config
		{
			std::chrono::nanoseconds min_backoff;      // Minimum backoff time
			std::chrono::nanoseconds max_backoff;      // Maximum backoff time
			double backoff_multiplier;                  // Exponential growth factor
			size_t spin_count;                          // Spins before backing off
			
			backoff_config()
				: min_backoff(100)
				, max_backoff(10000)
				, backoff_multiplier(2.0)
				, spin_count(10)
			{}
		};

		/**
		 * @brief Constructs a new lockfree_thread_worker.
		 * @param use_time_tag If true, enables time-based metrics collection.
		 * @param config Backoff configuration for contention management.
		 */
		lockfree_thread_worker(const bool& use_time_tag = true,
							   const backoff_config& config = backoff_config());

		/**
		 * @brief Virtual destructor. Ensures proper cleanup.
		 */
		virtual ~lockfree_thread_worker(void);

		/**
		 * @brief Sets the lockfree_job_queue for this worker.
		 * @param job_queue A shared pointer to the lock-free queue.
		 */
		auto set_job_queue(std::shared_ptr<lockfree_job_queue> job_queue) -> void;

		/**
		 * @brief Gets the current job queue.
		 * @return A shared pointer to the lockfree_job_queue.
		 */
		[[nodiscard]] auto get_job_queue(void) const -> std::shared_ptr<lockfree_job_queue>;

		/**
		 * @brief Gets worker-specific performance statistics.
		 * @return Worker statistics including jobs processed and idle time.
		 */
		struct worker_statistics
		{
			uint64_t jobs_processed{0};
			uint64_t total_processing_time_ns{0};
			uint64_t idle_time_ns{0};
			uint64_t backoff_count{0};
			uint64_t batch_dequeue_count{0};
		};

		[[nodiscard]] auto get_statistics(void) const -> worker_statistics;

		/**
		 * @brief Enables or disables batch processing.
		 * @param enable If true, the worker will attempt to process multiple jobs per iteration.
		 * @param batch_size Maximum number of jobs to process in a batch.
		 */
		auto set_batch_processing(bool enable, size_t batch_size = 10) -> void;

		/**
		 * @brief Returns a string representation of the worker's state.
		 * @return A string describing the current state and statistics.
		 */
		[[nodiscard]] auto to_string(void) const -> std::string override;

	protected:
		/**
		 * @brief Determines if the worker should continue processing.
		 * @return true if there are jobs in the queue or the worker hasn't been stopped.
		 */
		[[nodiscard]] auto should_continue_work(void) const -> bool override;

		/**
		 * @brief Main work loop that processes jobs from the lock-free queue.
		 * @return result_void indicating success or failure.
		 */
		auto do_work(void) -> result_void override;

		/**
		 * @brief Called before the worker starts.
		 * @return result_void indicating success or failure.
		 */
		auto before_start(void) -> result_void override;

		/**
		 * @brief Called after the worker stops.
		 * @return result_void indicating success or failure.
		 */
		auto after_stop(void) -> result_void override;

	private:
		/**
		 * @brief Implements exponential backoff strategy.
		 * @param attempt The current attempt number.
		 */
		auto backoff(size_t attempt) -> void;

		/**
		 * @brief Processes a single job.
		 * @param job The job to process.
		 * @return result_void from job execution.
		 */
		auto process_job(std::unique_ptr<job> job) -> result_void;

		/**
		 * @brief Attempts to process multiple jobs in a batch.
		 * @return Number of jobs processed.
		 */
		auto process_batch() -> size_t;

	private:
		/** @brief The lock-free job queue to process jobs from. */
		std::shared_ptr<lockfree_job_queue> job_queue_;

		/** @brief Indicates whether time-based metrics should be collected. */
		bool use_time_tag_;

		/** @brief Backoff configuration for contention management. */
		backoff_config backoff_config_;

		/** @brief Current backoff duration. */
		std::chrono::nanoseconds current_backoff_;

		/** @brief Performance statistics for this worker. */
		mutable struct {
			std::atomic<uint64_t> jobs_processed{0};
			std::atomic<uint64_t> total_processing_time_ns{0};
			std::atomic<uint64_t> idle_time_ns{0};
			std::atomic<uint64_t> backoff_count{0};
			std::atomic<uint64_t> batch_dequeue_count{0};
		} stats_;

		/** @brief Batch processing settings. */
		std::atomic<bool> batch_processing_enabled_{false};
		std::atomic<size_t> batch_size_{10};

		/** @brief High-resolution timer for performance measurements. */
		std::chrono::steady_clock::time_point last_job_time_;
	};

} // namespace thread_pool_module

// Formatter specializations for lockfree_thread_worker
#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for lockfree_thread_worker.
 */
template <> 
struct std::formatter<thread_pool_module::lockfree_thread_worker> : std::formatter<std::string_view>
{
	template <typename FormatContext>
	auto format(const thread_pool_module::lockfree_thread_worker& item, FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character lockfree_thread_worker.
 */
template <>
struct std::formatter<thread_pool_module::lockfree_thread_worker, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	template <typename FormatContext>
	auto format(const thread_pool_module::lockfree_thread_worker& item, FormatContext& ctx) const
	{
		auto str = item.to_string();
		auto wstr = convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
#else
/**
 * @brief Specialization of fmt::formatter for lockfree_thread_worker.
 */
template <> 
struct fmt::formatter<thread_pool_module::lockfree_thread_worker> : fmt::formatter<std::string_view>
{
	template <typename FormatContext>
	auto format(const thread_pool_module::lockfree_thread_worker& item, FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};
#endif