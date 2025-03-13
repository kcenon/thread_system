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

#include "job.h"
#include "formatter.h"
#include "callback_job.h"
#include "convert_string.h"

#include <mutex>
#include <deque>
#include <tuple>
#include <atomic>
#include <optional>
#include <string_view>
#include <condition_variable>

using namespace utility_module;

namespace thread_module
{
	/**
	 * @class job_queue
	 * @brief A thread-safe job queue for managing and dispatching work items.
	 *
	 * The @c job_queue class provides a synchronized queue for storing and retrieving
	 * @c job objects (or derived classes). Multiple threads can safely enqueue and
	 * dequeue jobs, ensuring proper synchronization and preventing data races.
	 *
	 * This class inherits from @c std::enable_shared_from_this, which allows it to
	 * create @c std::shared_ptr instances referring to itself via @c get_ptr(). This
	 * is often useful when passing a @c job_queue pointer to jobs themselves or other
	 * components that need a safe, shared reference to the queue.
	 *
	 * ### Typical Usage
	 * 1. Create a @c job_queue instance, typically via @c std::make_shared.
	 * 2. Enqueue @c job objects (or derived types) using @c enqueue().
	 * 3. One or more worker threads repeatedly call @c dequeue() to retrieve jobs
	 *    and process them.
	 * 4. Call @c stop_waiting_dequeue() and possibly @c clear() to shut down the queue
	 *    gracefully when all jobs are done or when the system is stopping.
	 */
	class job_queue : public std::enable_shared_from_this<job_queue>
	{
	public:
		/**
		 * @brief Constructs a new, empty @c job_queue.
		 *
		 * Initializes internal synchronization primitives and sets all flags to
		 * their default states (e.g., @c notify_ = false, @c stop_ = false).
		 */
		job_queue();

		/**
		 * @brief Virtual destructor. Cleans up resources used by the @c job_queue.
		 */
		virtual ~job_queue(void);

		/**
		 * @brief Obtains a @c std::shared_ptr that points to this queue instance.
		 * @return A shared pointer to the current @c job_queue object.
		 *
		 * Because @c job_queue inherits from @c std::enable_shared_from_this, calling
		 * @c get_ptr() allows retrieving a @c shared_ptr<job_queue> from within
		 * member functions of @c job_queue.
		 */
		[[nodiscard]] auto get_ptr(void) -> std::shared_ptr<job_queue>;

		/**
		 * @brief Checks if the queue is in a "stopped" state.
		 * @return @c true if the queue is stopped, @c false otherwise.
		 *
		 * When stopped, worker threads are typically notified to cease waiting for
		 * new jobs. New jobs may still be enqueued, but it is up to the system design
		 * how they are handled in a stopped state.
		 */
		[[nodiscard]] auto is_stopped() const -> bool;

		/**
		 * @brief Sets the 'notify' flag for this queue.
		 * @param notify If @c true, signals that enqueue should notify waiting threads.
		 *               If @c false, jobs can still be enqueued, but waiting threads
		 *               won't be automatically notified.
		 */
		auto set_notify(bool notify) -> void;

		/**
		 * @brief Enqueues a new job into the queue.
		 * @param value A unique pointer to the job being added.
		 * @return @c std::optional<std::string> containing an error message if the
		 *         enqueue operation failed; otherwise, @c std::nullopt.
		 *
		 * This method is thread-safe. If @c notify_ is set to @c true, a waiting
		 * thread (if any) will be notified upon successful enqueue.
		 */
		[[nodiscard]] virtual auto enqueue(std::unique_ptr<job>&& value)
			-> std::optional<std::string>;

		/**
		 * @brief Enqueues a batch of jobs into the queue.
		 * @param jobs A vector of unique pointers to the jobs being added.
		 * @return @c std::optional<std::string> containing an error message if the
		 *         enqueue operation failed; otherwise, @c std::nullopt.
		 */
		[[nodiscard]] virtual auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs)
			-> std::optional<std::string>;

		/**
		 * @brief Dequeues a job from the queue in FIFO order.
		 * @return A tuple where:
		 *         - The first element (@c std::optional<std::unique_ptr<job>>) holds
		 *           a valid job if one is available, or @c std::nullopt if the queue
		 *           is empty.
		 *         - The second element (@c std::optional<std::string>) may contain
		 *           an error message or be @c nullopt if no error occurred.
		 *
		 * If the queue is empty, the caller may block depending on the internal
		 * concurrency model (unless @c stop_ is set, in which case it may return
		 * immediately).
		 */
		[[nodiscard]] virtual auto dequeue(void)
			-> std::tuple<std::optional<std::unique_ptr<job>>, std::optional<std::string>>;

		/**
		 * @brief Dequeues all remaining jobs from the queue without processing them.
		 * @return A @c std::deque of unique_ptr<job> containing all jobs that were
		 *         in the queue at the time of the call.
		 *
		 * Similar to @c clear(), but returns the dequeued jobs to the caller for
		 * potential inspection or manual processing.
		 */
		[[nodiscard]] virtual auto dequeue_batch(void) -> std::deque<std::unique_ptr<job>>;

		/**
		 * @brief Removes all jobs currently in the queue without processing them.
		 *
		 * This operation is thread-safe and can be used to discard pending jobs,
		 * typically during shutdown or error recovery. It does not affect the
		 * @c stop_ or @c notify_ flags.
		 */
		virtual auto clear(void) -> void;

		/**
		 * @brief Checks if the queue is currently empty.
		 * @return @c true if the queue has no pending jobs, @c false otherwise.
		 */
		[[nodiscard]] auto empty(void) const -> bool;

		/**
		 * @brief Signals the queue to stop waiting for new jobs (e.g., during shutdown).
		 *
		 * Sets the @c stop_ flag to @c true and notifies any threads that might be
		 * blocked in @c dequeue(). This allows worker threads to exit gracefully
		 * rather than remain blocked indefinitely.
		 */
		auto stop_waiting_dequeue(void) -> void;

		/**
		 * @brief Returns a string representation of this job_queue.
		 * @return A std::string describing the state of the queue (e.g., size, flags).
		 *
		 * Primarily used for logging and debugging. Derived classes may override
		 * this to include additional diagnostic information.
		 */
		[[nodiscard]] virtual auto to_string(void) const -> std::string;

	protected:
		/**
		 * @brief If @c true, threads waiting for new jobs are notified when a new job
		 *        is enqueued. If @c false, enqueuing does not automatically trigger
		 *        a notification.
		 */
		std::atomic_bool notify_;

		/**
		 * @brief Indicates whether the queue has been signaled to stop.
		 *
		 * Setting @c stop_ to @c true typically causes waiting threads to
		 * unblock and exit their waiting loop.
		 */
		std::atomic_bool stop_;

		/**
		 * @brief Mutex to protect access to the underlying @c queue_ container and related state.
		 *
		 * Any operation that modifies or reads the queue should lock this mutex to
		 * ensure thread safety.
		 */
		mutable std::mutex mutex_;

		/**
		 * @brief Condition variable used to signal worker threads.
		 *
		 * Used in combination with @c mutex_ to block or notify threads waiting
		 * for new jobs.
		 */
		std::condition_variable condition_;

	private:
		/**
		 * @brief The underlying container storing the jobs in FIFO order.
		 *
		 * @note This container is guarded by @c mutex_ and should only be modified
		 *       while holding the lock.
		 */
		std::deque<std::unique_ptr<job>> queue_;

		/**
		 * @brief Tracks the current number of jobs in the queue.
		 *
		 * Though @c queue_.size() could be used, maintaining an atomic size counter
		 * can improve performance in certain scenarios.
		 */
		std::atomic_size_t queue_size_;
	};
}

// ----------------------------------------------------------------------------
// Formatter specializations for job_queue
// ----------------------------------------------------------------------------

#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for @c thread_module::job_queue.
 *
 * Enables formatting of @c job_queue objects as strings using the C++20 <format> library,
 * when @c USE_STD_FORMAT is defined.
 *
 * ### Example
 * @code
 * auto queue_ptr = std::make_shared<thread_module::job_queue>();
 * std::string output = std::format("Queue state: {}", *queue_ptr);
 * @endcode
 */
template <> struct std::formatter<thread_module::job_queue> : std::formatter<std::string_view>
{
	/**
	 * @brief Formats a @c job_queue object as a string.
	 * @tparam FormatContext The type of the format context.
	 * @param item The @c job_queue to format.
	 * @param ctx  The format context used for output.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_module::job_queue& item, FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character @c thread_module::job_queue.
 *
 * Allows @c job_queue objects to be formatted as wide strings using the C++20 <format> library.
 */
template <>
struct std::formatter<thread_module::job_queue, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a @c job_queue object as a wide string.
	 * @tparam FormatContext The type of the format context.
	 * @param item The @c job_queue to format.
	 * @param ctx  The wide-character format context.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_module::job_queue& item, FormatContext& ctx) const
	{
		auto str = item.to_string();
		auto wstr = convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
#else  // USE_STD_FORMAT

/**
 * @brief Specialization of fmt::formatter for @c thread_module::job_queue.
 *
 * Enables formatting of @c job_queue objects as strings using the {fmt} library.
 *
 * ### Example
 * @code
 * auto queue_ptr = std::make_shared<thread_module::job_queue>();
 * std::string output = fmt::format("Queue state: {}", *queue_ptr);
 * @endcode
 */
template <> struct fmt::formatter<thread_module::job_queue> : fmt::formatter<std::string_view>
{
	/**
	 * @brief Formats a @c job_queue object as a string using {fmt}.
	 * @tparam FormatContext The type of the format context.
	 * @param item The @c job_queue to format.
	 * @param ctx  The format context used for output.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_module::job_queue& item, FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};
#endif // USE_STD_FORMAT