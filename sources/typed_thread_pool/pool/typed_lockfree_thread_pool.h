/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this
   list of conditions and the following disclaimer in the documentation
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
#include "../../utilities/conversion/convert_string.h"
#include "../scheduling/typed_lockfree_job_queue.h"
#include "../scheduling/typed_lockfree_thread_worker.h"

#include <tuple>
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <optional>

using namespace utility_module;
using namespace thread_module;

namespace typed_thread_pool_module
{
	/**
	 * @class typed_lockfree_thread_pool_t
	 * @brief A lock-free thread pool that schedules and executes jobs based on their priority levels.
	 *
	 * @tparam job_type The type representing job types (e.g., enum or integral type).
	 * @ingroup thread_pools
	 *
	 * The typed_lockfree_thread_pool_t template class provides a lock-free thread pool implementation 
	 * that processes jobs according to their assigned types rather than just submission order.
	 * This implementation uses typed_lockfree_job_queue for high-performance concurrent operations.
	 *
	 * ### Key Features
	 * - **Lock-Free Operations**: Uses typed_lockfree_job_queue for scalable performance
	 * - **Type-Based Scheduling**: Jobs with higher priority are processed first.
	 * - **Customizable Type Types**: Supports custom priority types through templates.
	 * - **Worker Thread Model**: Each worker runs in its own thread, processing jobs.
	 * - **Dynamic Thread Management**: Add/remove workers at runtime.
	 * - **Graceful Shutdown**: Option to complete current jobs before stopping.
	 *
	 * ### Performance Benefits
	 * - No mutex contention in job queue operations
	 * - Better scalability with increasing thread counts
	 * - Lower latency for high-priority job processing
	 * - Reduced context switching overhead
	 */
	template <typename job_type = job_types>
	class typed_lockfree_thread_pool_t
		: public std::enable_shared_from_this<typed_lockfree_thread_pool_t<job_type>>
	{
	public:
		/**
		 * @brief Constructs a new typed_lockfree_thread_pool_t instance.
		 *
		 * @param thread_title A human-readable title or name for the thread pool.
		 * This can help in debugging or logging.
		 */
		typed_lockfree_thread_pool_t(const std::string& thread_title = "typed_lockfree_thread_pool");

		/**
		 * @brief Destroys the typed_lockfree_thread_pool_t object.
		 *
		 * The destructor will invoke stop() if the pool is still running,
		 * ensuring that all threads are properly terminated before destruction.
		 */
		virtual ~typed_lockfree_thread_pool_t(void);

		/**
		 * @brief Returns a shared pointer to the current typed_lockfree_thread_pool_t.
		 *
		 * This is a convenience method when you only have a raw pointer to the
		 * pool but need a std::shared_ptr.
		 *
		 * @return std::shared_ptr<typed_lockfree_thread_pool_t<job_type>>
		 * A shared pointer to this thread pool.
		 */
		[[nodiscard]] auto get_ptr(void) -> std::shared_ptr<typed_lockfree_thread_pool_t<job_type>>;

		/**
		 * @brief Starts the thread pool by creating worker threads and
		 * initializing internal structures.
		 *
		 * @return result_void
		 *         - If an error occurs during start-up, the returned result
		 *           will contain an error object.
		 *         - If no error occurs, the result will be a success value.
		 *
		 * ### Thread Safety
		 * This method is typically called once, before using other methods such as
		 * enqueue(). Calling start() multiple times without stopping is not recommended.
		 */
		auto start(void) -> result_void;

		/**
		 * @brief Retrieves the underlying lock-free priority job queue managed by this thread pool.
		 *
		 * @return std::shared_ptr<typed_lockfree_job_queue_t<job_type>>
		 * A shared pointer to the lock-free priority job queue.
		 *
		 * ### Thread Safety
		 * This queue is shared and used by worker threads, so care should be taken
		 * if you modify or replace the queue. Typically, external code only needs
		 * to enqueue new jobs via enqueue(), rather than directly accessing the queue.
		 */
		[[nodiscard]] auto get_job_queue(void)
			-> std::shared_ptr<typed_lockfree_job_queue_t<job_type>>;

		/**
		 * @brief Enqueues a priority job into the thread pool's job queue.
		 *
		 * @param job A unique pointer to the priority job to be added.
		 *
		 * @return result_void
		 *         - Contains an error if the enqueue operation fails.
		 *         - Otherwise, returns a success value.
		 *
		 * ### Thread Safety
		 * This method is thread-safe and lock-free; multiple threads can safely 
		 * enqueue jobs concurrently without contention.
		 */
		auto enqueue(std::unique_ptr<typed_job_t<job_type>>&& job) -> result_void;

		/**
		 * @brief Enqueues a batch of priority jobs into the thread pool's job queue.
		 *
		 * @param jobs A vector of unique pointers to priority jobs to be added.
		 *
		 * @return result_void
		 *         - Contains an error if the enqueue operation fails.
		 *         - Otherwise, returns a success value.
		 *
		 * ### Thread Safety
		 * This method is thread-safe and lock-free; multiple threads can safely 
		 * enqueue jobs concurrently without contention.
		 */
		auto enqueue_batch(std::vector<std::unique_ptr<typed_job_t<job_type>>>&& jobs)
			-> result_void;

		/**
		 * @brief Enqueues a new worker thread for this thread pool.
		 *
		 * This allows dynamic addition of worker threads while the pool is running.
		 *
		 * @param worker A unique pointer to the lockfree thread worker to be added.
		 *
		 * @return result_void
		 *         - Contains an error if the enqueue operation fails.
		 *         - Otherwise, returns a success value.
		 *
		 * ### Note
		 * Typically, most applications create a fixed number of workers at startup.
		 * However, if your workload changes significantly, adding more workers at
		 * runtime can help handle increased job load.
		 *
		 * ### Thread Safety
		 * This method is thread-safe.
		 */
		auto enqueue(std::unique_ptr<typed_lockfree_thread_worker_t<job_type>>&& worker)
			-> result_void;

		/**
		 * @brief Enqueues a batch of new worker threads for this thread pool.
		 *
		 * This allows dynamic addition of multiple worker threads while the pool is running.
		 *
		 * @param workers A vector of unique pointers to lockfree thread workers to be added.
		 *
		 * @return result_void
		 *         - Contains an error if the enqueue operation fails.
		 *         - Otherwise, returns a success value.
		 *
		 * ### Note
		 * Typically, most applications create a fixed number of workers at startup.
		 * However, if your workload changes significantly, adding more workers at
		 * runtime can help handle increased job load.
		 *
		 * ### Thread Safety
		 * This method is thread-safe.
		 */
		auto enqueue_batch(
			std::vector<std::unique_ptr<typed_lockfree_thread_worker_t<job_type>>>&& workers)
			-> result_void;

		/**
		 * @brief Stops the thread pool and optionally waits for currently running
		 * jobs to finish.
		 *
		 * @param clear_queue If `true`, any queued jobs are removed.
		 *                   If `false` (default), the pool stops accepting new jobs
		 *                   but allows currently running jobs to complete.
		 * @return result_void
		 *         - Contains an error if the stop operation fails.
		 *         - Otherwise, returns a success value.
		 *
		 * ### Thread Safety
		 * Calling stop() from multiple threads simultaneously is safe,
		 * but redundant calls to stop() will have no additional effect after the first.
		 */
		auto stop(bool clear_queue = false) -> result_void;

		/**
		 * @brief Generates a string representation of the thread pool's internal state.
		 *
		 * @return std::string A human-readable string containing pool details,
		 *                     such as whether the pool is running, the thread title,
		 *                     and potentially the number of workers.
		 *
		 * ### Example
		 * @code{.cpp}
		 * std::cout << pool.to_string() << std::endl;
		 * // Output might look like:
		 * // "typed_lockfree_thread_pool [Title: typed_lockfree_thread_pool, Started: true, Workers: 4]"
		 * @endcode
		 */
		[[nodiscard]] auto to_string(void) const -> std::string;

		/**
		 * @brief Sets the job queue for this thread pool and its workers.
		 *
		 * @param job_queue A shared pointer to the lock-free job queue to use.
		 */
		auto set_job_queue(std::shared_ptr<typed_lockfree_job_queue_t<job_type>> job_queue) -> void;

		/**
		 * @brief Gets the performance statistics from the lock-free queue.
		 *
		 * @return typed_queue_statistics_t<job_type> Statistics about queue performance.
		 */
		[[nodiscard]] auto get_queue_statistics(void) const -> typed_queue_statistics_t<job_type>;

	private:
		/** @brief A descriptive name or title for this thread pool, useful for logging. */
		std::string thread_title_;

		/** @brief Indicates whether the thread pool has been started. */
		std::atomic<bool> start_pool_;

		/** @brief The shared lock-free priority job queue from which workers fetch jobs. */
		std::shared_ptr<typed_lockfree_job_queue_t<job_type>> job_queue_;

		/** @brief The collection of lockfree worker threads responsible for processing jobs. */
		std::vector<std::unique_ptr<typed_lockfree_thread_worker_t<job_type>>> workers_;
	};

	/// Alias for a typed_lockfree_thread_pool with the default job_types type.
	using typed_lockfree_thread_pool = typed_lockfree_thread_pool_t<job_types>;

} // namespace typed_thread_pool_module

// Formatter specializations for typed_lockfree_thread_pool_t<job_type>
#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for typed_lockfree_thread_pool_t<job_type>.
 *
 * Allows formatting of typed_lockfree_thread_pool_t<job_type> objects as strings
 * using the standard library's format facilities (C++20 or later).
 */
template <typename job_type>
struct std::formatter<typed_thread_pool_module::typed_lockfree_thread_pool_t<job_type>>
	: std::formatter<std::string_view>
{
	/**
	 * @brief Formats a typed_lockfree_thread_pool_t<job_type> object as a string.
	 *
	 * @tparam FormatContext Type of the format context.
	 * @param item The typed_lockfree_thread_pool_t<job_type> object to format.
	 * @param ctx The format context for the output.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const typed_thread_pool_module::typed_lockfree_thread_pool_t<job_type>& item,
				FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character
 * typed_lockfree_thread_pool_t<job_type>.
 *
 * This enables formatting of typed_lockfree_thread_pool_t<job_type> objects as
 * wide strings using the standard library's format facilities (C++20 or later).
 */
template <typename job_type>
struct std::formatter<typed_thread_pool_module::typed_lockfree_thread_pool_t<job_type>, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a typed_lockfree_thread_pool_t<job_type> object as a wide string.
	 *
	 * @tparam FormatContext Type of the format context.
	 * @param item The typed_lockfree_thread_pool_t<job_type> object to format.
	 * @param ctx The format context for the output.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const typed_thread_pool_module::typed_lockfree_thread_pool_t<job_type>& item,
				FormatContext& ctx) const
	{
		auto str = item.to_string();
		auto wstr = convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
#else
/**
 * @brief Specialization of fmt::formatter for typed_lockfree_thread_pool_t<job_type>.
 *
 * Allows formatting of typed_lockfree_thread_pool_t<job_type> objects as strings
 * using the {fmt} library (https://github.com/fmtlib/fmt).
 */
template <typename job_type>
struct fmt::formatter<typed_thread_pool_module::typed_lockfree_thread_pool_t<job_type>>
	: fmt::formatter<std::string_view>
{
	/**
	 * @brief Formats a typed_lockfree_thread_pool_t<job_type> object as a string.
	 *
	 * @tparam FormatContext Type of the format context.
	 * @param item The typed_lockfree_thread_pool_t<job_type> object to format.
	 * @param ctx The format context for the output.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const typed_thread_pool_module::typed_lockfree_thread_pool_t<job_type>& item,
				FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};
#endif

#include "typed_lockfree_thread_pool.tpp"