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

#pragma once

#include "../../utilities/core/formatter.h"
#include "../../thread_base/lockfree/queues/lockfree_job_queue.h"
#include "../workers/lockfree_thread_worker.h"
#include "../../utilities/conversion/convert_string.h"
#include "../detail/forward_declarations.h"
#include "config.h"

#include <tuple>
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <optional>
#include <atomic>

using namespace utility_module;
using namespace thread_module;

namespace thread_pool_module
{
	/**
	 * @class lockfree_thread_pool
	 * @brief A high-performance lock-free thread pool for concurrent job execution.
	 *
	 * @ingroup thread_pools
	 *
	 * The @c lockfree_thread_pool class provides a lock-free implementation of a thread pool
	 * that uses lockfree_job_queue internally for superior performance under high contention.
	 * This implementation is designed to be a drop-in replacement for the standard thread_pool
	 * with significantly better scalability.
	 *
	 * ### Key Features
	 * - **Lock-Free Operations**: Uses lockfree_job_queue for wait-free enqueue operations
	 * - **Superior Scalability**: Better performance with increasing thread counts
	 * - **Lower Latency**: Reduced contention and faster job dispatch
	 * - **Compatible Interface**: Drop-in replacement for thread_pool
	 * - **Performance Monitoring**: Built-in statistics collection
	 *
	 * ### Performance Benefits
	 * - No mutex contention in job queue operations
	 * - Better CPU cache utilization
	 * - Reduced context switching
	 * - Linear scalability with thread count
	 *
	 * ### Design Principles
	 * - **Lock-Free Queue**: Uses lockfree_job_queue instead of mutex-based job_queue
	 * - **Specialized Workers**: lockfree_thread_worker optimized for lock-free operations
	 * - **Memory Safety**: Hazard pointers ensure safe memory reclamation
	 * - **Node Pooling**: Reduced allocation overhead through memory pooling
	 *
	 * ### Thread Safety
	 * All public methods are thread-safe and lock-free where possible. The underlying
	 * lockfree_job_queue provides wait-free enqueue and lock-free dequeue operations.
	 *
	 * ### Usage Example
	 * @code
	 * auto pool = std::make_shared<lockfree_thread_pool>("HighPerformancePool");
	 * 
	 * // Add workers
	 * std::vector<std::unique_ptr<lockfree_thread_worker>> workers;
	 * for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
	 *     workers.push_back(std::make_unique<lockfree_thread_worker>());
	 * }
	 * pool->enqueue_batch(std::move(workers));
	 * 
	 * // Start the pool
	 * pool->start();
	 * 
	 * // Submit jobs
	 * for (int i = 0; i < 1000000; ++i) {
	 *     pool->enqueue(std::make_unique<callback_job>([i]() -> result_void {
	 *         // Process work
	 *         return {};
	 *     }));
	 * }
	 * 
	 * // Get statistics
	 * auto stats = pool->get_queue_statistics();
	 * std::cout << "Average enqueue latency: " 
	 *           << stats.get_average_enqueue_latency_ns() << "ns\n";
	 * 
	 * // Shutdown
	 * pool->stop();
	 * @endcode
	 *
	 * @see thread_pool The standard mutex-based implementation
	 * @see lockfree_thread_worker The specialized worker for lock-free operations
	 * @see lockfree_job_queue The underlying lock-free queue implementation
	 */
	class lockfree_thread_pool : public std::enable_shared_from_this<lockfree_thread_pool>
	{
	public:
		/**
		 * @brief Constructs a new lockfree_thread_pool instance.
		 * @param thread_title An optional title for the thread pool (defaults to "lockfree_thread_pool").
		 */
		lockfree_thread_pool(const std::string& thread_title = "lockfree_thread_pool");

		/**
		 * @brief Virtual destructor. Ensures proper cleanup of all resources.
		 */
		virtual ~lockfree_thread_pool(void);

		/**
		 * @brief Retrieves a shared pointer to this lockfree_thread_pool instance.
		 * @return A shared pointer to the current object.
		 */
		[[nodiscard]] auto get_ptr(void) -> std::shared_ptr<lockfree_thread_pool>;

		/**
		 * @brief Starts the thread pool and all associated workers.
		 * @return std::optional<std::string> containing an error message on failure,
		 *         or std::nullopt on success.
		 */
		auto start(void) -> std::optional<std::string>;

		/**
		 * @brief Retrieves the underlying lock-free job queue.
		 * @return A shared pointer to the lockfree_job_queue.
		 */
		[[nodiscard]] auto get_job_queue(void) -> std::shared_ptr<lockfree_job_queue>;

		/**
		 * @brief Enqueues a single job into the thread pool.
		 * @param job A unique pointer to the job to be executed.
		 * @return std::optional<std::string> containing an error message on failure,
		 *         or std::nullopt on success.
		 *
		 * @note This operation is wait-free.
		 */
		auto enqueue(std::unique_ptr<job>&& job) -> std::optional<std::string>;

		/**
		 * @brief Enqueues multiple jobs into the thread pool.
		 * @param jobs A vector of unique pointers to jobs to be executed.
		 * @return std::optional<std::string> containing an error message on failure,
		 *         or std::nullopt on success.
		 *
		 * @note This uses batch operations for improved performance.
		 */
		auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) 
			-> std::optional<std::string>;

		/**
		 * @brief Adds a new worker thread to the pool.
		 * @param worker A unique pointer to the lockfree_thread_worker to add.
		 * @return std::optional<std::string> containing an error message on failure,
		 *         or std::nullopt on success.
		 */
		auto enqueue(std::unique_ptr<lockfree_thread_worker>&& worker) 
			-> std::optional<std::string>;

		/**
		 * @brief Adds multiple worker threads to the pool.
		 * @param workers A vector of unique pointers to lockfree_thread_workers to add.
		 * @return std::optional<std::string> containing an error message on failure,
		 *         or std::nullopt on success.
		 */
		auto enqueue_batch(std::vector<std::unique_ptr<lockfree_thread_worker>>&& workers)
			-> std::optional<std::string>;

		/**
		 * @brief Stops the thread pool and optionally clears pending jobs.
		 * @param immediately If true, stops immediately without processing pending jobs.
		 *                    If false (default), allows workers to finish current jobs.
		 * @return std::optional<std::string> containing an error message on failure,
		 *         or std::nullopt on success.
		 */
		auto stop(bool immediately = false) -> std::optional<std::string>;

		/**
		 * @brief Returns a string representation of the thread pool's state.
		 * @return A string describing the current state of the pool.
		 */
		[[nodiscard]] auto to_string(void) const -> std::string;

		/**
		 * @brief Sets a custom lock-free job queue for the pool.
		 * @param job_queue A shared pointer to the lockfree_job_queue to use.
		 */
		auto set_job_queue(std::shared_ptr<lockfree_job_queue> job_queue) -> void;

		/**
		 * @brief Retrieves performance statistics from the lock-free queue.
		 * @return Queue statistics including throughput and latency metrics.
		 */
		[[nodiscard]] auto get_queue_statistics(void) const 
			-> lockfree_job_queue::queue_statistics;

		/**
		 * @brief Checks if the thread pool has been started.
		 * @return true if the pool is running, false otherwise.
		 */
		[[nodiscard]] auto is_running(void) const -> bool;

		/**
		 * @brief Gets the number of worker threads in the pool.
		 * @return The count of worker threads.
		 */
		[[nodiscard]] auto worker_count(void) const -> size_t;

	private:
		/** @brief Title or identifier for this thread pool. */
		std::string thread_title_;

		/** @brief Indicates whether the pool has been started. */
		std::atomic<bool> start_pool_;

		/** @brief The lock-free job queue for storing pending jobs. */
		std::shared_ptr<lockfree_job_queue> job_queue_;

		/** @brief Collection of worker threads. */
		std::vector<std::unique_ptr<lockfree_thread_worker>> workers_;

		/** @brief Mutex for protecting worker vector modifications. */
		mutable std::mutex workers_mutex_;
	};

} // namespace thread_pool_module

// Formatter specializations for lockfree_thread_pool
#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for lockfree_thread_pool.
 */
template <> 
struct std::formatter<thread_pool_module::lockfree_thread_pool> : std::formatter<std::string_view>
{
	template <typename FormatContext>
	auto format(const thread_pool_module::lockfree_thread_pool& item, FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character lockfree_thread_pool.
 */
template <>
struct std::formatter<thread_pool_module::lockfree_thread_pool, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	template <typename FormatContext>
	auto format(const thread_pool_module::lockfree_thread_pool& item, FormatContext& ctx) const
	{
		auto str = item.to_string();
		auto wstr = convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
#else
/**
 * @brief Specialization of fmt::formatter for lockfree_thread_pool.
 */
template <> 
struct fmt::formatter<thread_pool_module::lockfree_thread_pool> : fmt::formatter<std::string_view>
{
	template <typename FormatContext>
	auto format(const thread_pool_module::lockfree_thread_pool& item, FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};
#endif