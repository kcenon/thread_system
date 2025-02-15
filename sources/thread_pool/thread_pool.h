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

#include "job_queue.h"
#include "thread_worker.h"
#include "convert_string.h"

#include <tuple>
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <optional>

using namespace utility_module;
using namespace thread_module;

namespace thread_pool_module
{
	/**
	 * @class thread_pool
	 * @brief Manages a group of @c thread_worker instances and a shared @c job_queue for concurrent
	 * job processing.
	 *
	 * The @c thread_pool class provides an interface to:
	 * - Maintain a shared @c job_queue
	 * - Maintain multiple @c thread_worker objects
	 * - Enqueue jobs (@c job) into the shared queue
	 * - Start/stop all worker threads as a group
	 *
	 * This class inherits from @c std::enable_shared_from_this, allowing you to safely retrieve
	 * a @c std::shared_ptr<thread_pool> from member functions via @c get_ptr().
	 *
	 * ### Typical Usage
	 * 1. Create a @c thread_pool (usually with @c std::make_shared).
	 * 2. Optionally create and enqueue additional @c thread_worker objects (if you want specialized
	 * behaviors).
	 * 3. Call @c start() to launch all workers, enabling them to process jobs in the @c job_queue.
	 * 4. Enqueue @c job objects into the shared queue as needed.
	 * 5. Eventually call @c stop() to allow all threads to finish current tasks (or stop
	 * immediately).
	 */
	class thread_pool : public std::enable_shared_from_this<thread_pool>
	{
	public:
		/**
		 * @brief Constructs a new @c thread_pool instance.
		 * @param thread_title An optional title or identifier for the thread pool (defaults to
		 * "thread_pool").
		 *
		 * This title can be used for logging or debugging purposes.
		 */
		thread_pool(const std::string& thread_title = "thread_pool");

		/**
		 * @brief Virtual destructor. Cleans up resources used by the thread pool.
		 *
		 * If the pool is still running, this typically calls @c stop() internally
		 * to ensure all worker threads are properly shut down.
		 */
		virtual ~thread_pool(void);

		/**
		 * @brief Retrieves a @c std::shared_ptr to this @c thread_pool instance.
		 * @return A shared pointer to the current @c thread_pool object.
		 *
		 * By inheriting from @c std::enable_shared_from_this, you can call @c get_ptr()
		 * within member functions to avoid storing a separate shared pointer.
		 */
		[[nodiscard]] auto get_ptr(void) -> std::shared_ptr<thread_pool>;

		/**
		 * @brief Starts the thread pool and all associated workers.
		 * @return @c std::optional<std::string> containing an error message if the start
		 *         operation fails, or @c std::nullopt on success.
		 *
		 * If the pool is already running, a subsequent call to @c start() may return an error.
		 * On success, each @c thread_worker in @c workers_ is started, enabling them to process
		 * jobs from the @c job_queue_.
		 */
		auto start(void) -> std::optional<std::string>;

		/**
		 * @brief Returns the shared @c job_queue used by this thread pool.
		 * @return A @c std::shared_ptr<job_queue> that stores the queued jobs.
		 *
		 * The returned queue is shared among all worker threads in the pool, which
		 * can concurrently dequeue and process jobs.
		 */
		[[nodiscard]] auto get_job_queue(void) -> std::shared_ptr<job_queue>;

		/**
		 * @brief Enqueues a new job into the shared @c job_queue.
		 * @param job A @c std::unique_ptr<job> representing the work to be done.
		 * @return @c std::optional<std::string> containing an error message if the enqueue
		 *         operation fails, or @c std::nullopt on success.
		 *
		 * Example:
		 * @code
		 * auto pool = std::make_shared<thread_pool_module::thread_pool>();
		 * pool->start();
		 *
		 * auto my_job = std::make_unique<callback_job>(
		 * 	   // some callback...
		 * );
		 * if (auto err = pool->enqueue(std::move(my_job))) {
		 *     // handle error
		 * }
		 * @endcode
		 */
		auto enqueue(std::unique_ptr<job>&& job) -> std::optional<std::string>;

		/**
		 * @brief Adds a @c thread_worker to the thread pool for specialized or additional
		 * processing.
		 * @param worker A @c std::unique_ptr<thread_worker> object.
		 * @return @c std::optional<std::string> containing an error message if the operation
		 *         fails, or @c std::nullopt on success.
		 *
		 * Each worker is stored in @c workers_. When @c start() is called, these workers
		 * begin running and can process jobs from the @c job_queue.
		 */
		auto enqueue(std::unique_ptr<thread_worker>&& worker) -> std::optional<std::string>;

		/**
		 * @brief Stops the thread pool and all worker threads.
		 * @param immediately_stop If @c true, any ongoing jobs may be interrupted; if @c false
		 *        (default), each worker attempts to finish its current job before stopping.
		 *
		 * Once stopped, the pool's @c start_pool_ flag is set to false, and no further
		 * job processing occurs. Behavior of re-starting a stopped pool depends on the
		 * implementation and may require re-initialization.
		 */
		auto stop(const bool& immediately_stop = false) -> void;

		/**
		 * @brief Provides a string representation of this @c thread_pool.
		 * @return A string describing the pool, including its title and other optional details.
		 *
		 * Derived classes may override this to include more diagnostic or state-related info.
		 */
		[[nodiscard]] auto to_string(void) const -> std::string;

	private:
		/**
		 * @brief A title or name for this thread pool, useful for identification and logging.
		 */
		std::string thread_title_;

		/**
		 * @brief Indicates whether the pool is currently running.
		 *
		 * Set to @c true after a successful call to @c start(), and reset to @c false after @c
		 * stop(). Used internally to prevent multiple active starts or erroneous state transitions.
		 */
		std::atomic<bool> start_pool_;

		/**
		 * @brief The shared job queue where jobs (@c job objects) are enqueued.
		 *
		 * Worker threads dequeue jobs from this queue to perform tasks. The queue persists
		 * for the lifetime of the pool or until no more references exist.
		 */
		std::shared_ptr<job_queue> job_queue_;

		/**
		 * @brief A collection of worker threads associated with this pool.
		 *
		 * Each @c thread_worker typically runs in its own thread context, processing jobs
		 * from @c job_queue_ or performing specialized logic. They are started together
		 * when @c thread_pool::start() is called.
		 */
		std::vector<std::unique_ptr<thread_worker>> workers_;
	};
} // namespace thread_pool_module

// ----------------------------------------------------------------------------
// Formatter specializations for thread_pool
// ----------------------------------------------------------------------------

#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for @c thread_pool_module::thread_pool.
 *
 * Enables formatting of @c thread_pool objects as strings using the C++20 <format> library
 * (when @c USE_STD_FORMAT is defined).
 *
 * ### Example
 * @code
 * auto pool = std::make_shared<thread_pool_module::thread_pool>("MyPool");
 * std::string output = std::format("Pool Info: {}", *pool); // e.g. "Pool Info: [thread_pool:
 * MyPool]"
 * @endcode
 */
template <>
struct std::formatter<thread_pool_module::thread_pool> : std::formatter<std::string_view>
{
	/**
	 * @brief Formats a @c thread_pool object as a string.
	 * @tparam FormatContext The type of the format context.
	 * @param item The @c thread_pool to format.
	 * @param ctx  The format context for the output.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_pool_module::thread_pool& item, FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character @c thread_pool_module::thread_pool.
 *
 * Allows wide-string formatting of @c thread_pool objects using the C++20 <format> library.
 */
template <>
struct std::formatter<thread_pool_module::thread_pool, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a @c thread_pool object as a wide string.
	 * @tparam FormatContext The type of the format context.
	 * @param item The @c thread_pool to format.
	 * @param ctx  The wide-character format context.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_pool_module::thread_pool& item, FormatContext& ctx) const
	{
		auto str = item.to_string();
		auto wstr = convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};

#else // USE_STD_FORMAT

/**
 * @brief Specialization of fmt::formatter for @c thread_pool_module::thread_pool.
 *
 * Allows @c thread_pool objects to be formatted as strings using the {fmt} library.
 *
 * ### Example
 * @code
 * auto pool = std::make_shared<thread_pool_module::thread_pool>("MyPool");
 * pool->start();
 * std::string output = fmt::format("Pool Info: {}", *pool);
 * @endcode
 */
template <>
struct fmt::formatter<thread_pool_module::thread_pool> : fmt::formatter<std::string_view>
{
	/**
	 * @brief Formats a @c thread_pool object as a string using {fmt}.
	 * @tparam FormatContext The type of the format context.
	 * @param item The @c thread_pool to format.
	 * @param ctx  The format context for output.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_pool_module::thread_pool& item, FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};
#endif