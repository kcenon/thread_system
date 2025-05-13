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

#include "convert_string.h"
#include "priority_job_queue.h"
#include "priority_thread_worker.h"

#include <tuple>
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <optional>

using namespace utility_module;
using namespace thread_module;

namespace priority_thread_pool_module
{
	/**
	 * @class priority_thread_pool_t
	 * @brief Manages a pool of threads to execute jobs based on priority levels.
	 *
	 * This template class provides a thread pool that schedules and executes jobs
	 * according to their assigned priorities. The class:
	 *   - Maintains a thread-safe priority job queue.
	 *   - Spawns worker threads (priority_thread_worker_t) that continuously
	 *     process jobs from the queue.
	 *   - Allows users to enqueue new jobs or new workers dynamically.
	 *
	 * @tparam priority_type The type that represents job priority (e.g., an enum or integral type).
	 *
	 * ### Example Usage
	 * @code{.cpp}
	 * using my_thread_pool = priority_thread_pool_t<my_priority_enum>;
	 *
	 * auto pool = std::make_shared<my_thread_pool>("My Thread Pool");
	 * auto start_result = pool->start();
	 * if (start_result.has_error())
	 * {
	 *     // Handle error starting the pool
	 *     std::cerr << start_result.get_error().message() << std::endl;
	 * }
	 *
	 * // Enqueue a job
	 * auto job = std::make_unique<priority_job_t<my_priority_enum>>(
	 *     my_priority_enum::HIGH,
	 *     [](){ std::cout << "High priority job executed\n"; }
	 * );
	 * pool->enqueue(std::move(job));
	 *
	 * // Stop the pool
	 * pool->stop();
	 * @endcode
	 */
	template <typename priority_type = job_priorities>
	class priority_thread_pool_t
		: public std::enable_shared_from_this<priority_thread_pool_t<priority_type>>
	{
	public:
		/**
		 * @brief Constructs a new priority_thread_pool_t instance.
		 *
		 * @param thread_title A human-readable title or name for the thread pool.
		 * This can help in debugging or logging.
		 */
		priority_thread_pool_t(const std::string& thread_title = "priority_thread_pool");

		/**
		 * @brief Destroys the priority_thread_pool_t object.
		 *
		 * The destructor will invoke stop() if the pool is still running,
		 * ensuring that all threads are properly terminated before destruction.
		 */
		virtual ~priority_thread_pool_t(void);

		/**
		 * @brief Returns a shared pointer to the current priority_thread_pool_t.
		 *
		 * This is a convenience method when you only have a raw pointer to the
		 * pool but need a std::shared_ptr.
		 *
		 * @return std::shared_ptr<priority_thread_pool_t<priority_type>>
		 * A shared pointer to this thread pool.
		 */
		[[nodiscard]] auto get_ptr(void) -> std::shared_ptr<priority_thread_pool_t<priority_type>>;

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
		 * @brief Retrieves the underlying priority job queue managed by this thread pool.
		 *
		 * @return std::shared_ptr<priority_job_queue_t<priority_type>>
		 * A shared pointer to the thread-safe priority job queue.
		 *
		 * ### Thread Safety
		 * This queue is shared and used by worker threads, so care should be taken
		 * if you modify or replace the queue. Typically, external code only needs
		 * to enqueue new jobs via enqueue(), rather than directly accessing the queue.
		 */
		[[nodiscard]] auto get_job_queue(void)
			-> std::shared_ptr<priority_job_queue_t<priority_type>>;

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
		 * This method is thread-safe; multiple threads can safely enqueue jobs
		 * concurrently.
		 */
		auto enqueue(std::unique_ptr<priority_job_t<priority_type>>&& job)
			-> result_void;

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
		 * This method is thread-safe; multiple threads can safely enqueue jobs
		 * concurrently.
		 */
		auto enqueue_batch(std::vector<std::unique_ptr<priority_job_t<priority_type>>>&& jobs)
			-> result_void;

		/**
		 * @brief Enqueues a new worker thread for this thread pool.
		 *
		 * This allows dynamic addition of worker threads while the pool is running.
		 *
		 * @param worker A unique pointer to the priority thread worker to be added.
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
		auto enqueue(std::unique_ptr<priority_thread_worker_t<priority_type>>&& worker)
			-> result_void;

		/**
		 * @brief Enqueues a batch of new worker threads for this thread pool.
		 *
		 * This allows dynamic addition of multiple worker threads while the pool is running.
		 *
		 * @param workers A vector of unique pointers to priority thread workers to be added.
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
			std::vector<std::unique_ptr<priority_thread_worker_t<priority_type>>>&& workers)
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
		 * // "priority_thread_pool [Title: priority_thread_pool, Started: true, Workers: 4]"
		 * @endcode
		 */
		[[nodiscard]] auto to_string(void) const -> std::string;

		/**
		 * @brief Sets the job queue for this thread pool and its workers.
		 * 
		 * @param job_queue A shared pointer to the job queue to use.
		 */
		auto set_job_queue(std::shared_ptr<priority_job_queue_t<priority_type>> job_queue) -> void;

	private:
		/** @brief A descriptive name or title for this thread pool, useful for logging. */
		std::string thread_title_;

		/** @brief Indicates whether the thread pool has been started. */
		std::atomic<bool> start_pool_;

		/** @brief The shared priority job queue from which workers fetch jobs. */
		std::shared_ptr<priority_job_queue_t<priority_type>> job_queue_;

		/** @brief The collection of worker threads responsible for processing jobs. */
		std::vector<std::unique_ptr<priority_thread_worker_t<priority_type>>> workers_;
	};

	/// Alias for a priority_thread_pool with the default job_priorities type.
	using priority_thread_pool = priority_thread_pool_t<job_priorities>;

} // namespace priority_thread_pool_module

// Formatter specializations for priority_thread_pool_t<priority_type>
#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for priority_thread_pool_t<priority_type>.
 *
 * Allows formatting of priority_thread_pool_t<priority_type> objects as strings
 * using the standard library's format facilities (C++20 or later).
 */
template <typename priority_type>
struct std::formatter<priority_thread_pool_module::priority_thread_pool_t<priority_type>>
	: std::formatter<std::string_view>
{
	/**
	 * @brief Formats a priority_thread_pool_t<priority_type> object as a string.
	 *
	 * @tparam FormatContext Type of the format context.
	 * @param item The priority_thread_pool_t<priority_type> object to format.
	 * @param ctx The format context for the output.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const priority_thread_pool_module::priority_thread_pool_t<priority_type>& item,
				FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character
 * priority_thread_pool_t<priority_type>.
 *
 * This enables formatting of priority_thread_pool_t<priority_type> objects as
 * wide strings using the standard library's format facilities (C++20 or later).
 */
template <typename priority_type>
struct std::formatter<priority_thread_pool_module::priority_thread_pool_t<priority_type>, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a priority_thread_pool_t<priority_type> object as a wide string.
	 *
	 * @tparam FormatContext Type of the format context.
	 * @param item The priority_thread_pool_t<priority_type> object to format.
	 * @param ctx The format context for the output.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const priority_thread_pool_module::priority_thread_pool_t<priority_type>& item,
				FormatContext& ctx) const
	{
		auto str = item.to_string();
		auto wstr = convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
#else
/**
 * @brief Specialization of fmt::formatter for priority_thread_pool_t<priority_type>.
 *
 * Allows formatting of priority_thread_pool_t<priority_type> objects as strings
 * using the {fmt} library (https://github.com/fmtlib/fmt).
 */
template <typename priority_type>
struct fmt::formatter<priority_thread_pool_module::priority_thread_pool_t<priority_type>>
	: fmt::formatter<std::string_view>
{
	/**
	 * @brief Formats a priority_thread_pool_t<priority_type> object as a string.
	 *
	 * @tparam FormatContext Type of the format context.
	 * @param item The priority_thread_pool_t<priority_type> object to format.
	 * @param ctx The format context for the output.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const priority_thread_pool_module::priority_thread_pool_t<priority_type>& item,
				FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};
#endif

#include "priority_thread_pool.tpp"