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

#include "formatter.h"
#include "convert_string.h"
#include "thread_conditions.h"

#include <mutex>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>
#include <optional>
#include <condition_variable>

#ifdef USE_STD_JTHREAD
#include <stop_token>
#endif

using namespace utility_module;

namespace thread_module
{
	/**
	 * @class thread_base
	 * @brief A base class for implementing custom worker threads.
	 *
	 * The @c thread_base class provides a framework for managing a single worker thread,
	 * offering lifecycle methods (start, stop), optional wake intervals, and hooks
	 * (@c before_start, @c do_work, @c after_stop) for derived classes to customize behavior.
	 *
	 * ### Typical Usage
	 * 1. Inherit from @c thread_base and override @c do_work(), @c before_start(), and/or
	 *    @c after_stop() as needed.
	 * 2. Instantiate your derived class, set a wake interval if desired, then call @c start()
	 *    to launch the thread.
	 * 3. When shutting down or no longer needing the thread's work, call @c stop().
	 * 4. The thread can periodically check @c should_continue_work() or internal conditions
	 *    to determine if it should continue running.
	 */
	class thread_base
	{
	public:
		thread_base(const thread_base&) = delete;
		thread_base& operator=(const thread_base&) = delete;
		thread_base(thread_base&&) = delete;
		thread_base& operator=(thread_base&&) = delete;

		/**
		 * @brief Constructs a new @c thread_base object.
		 * @param thread_title A human-readable title for this worker thread (default:
		 * "thread_base").
		 *
		 * The @p thread_title can be useful for logging, debugging, or thread naming
		 * (where the platform supports it).
		 */
		thread_base(const std::string& thread_title = "thread_base");

		/**
		 * @brief Virtual destructor. Ensures proper cleanup of derived classes.
		 *
		 * If the thread is still running when the destructor is called, @c stop() is
		 * typically called internally to join the thread before destruction.
		 */
		virtual ~thread_base(void);

		/**
		 * @brief Sets the interval at which the worker thread should wake up (if any).
		 * @param wake_interval Duration in milliseconds for periodic wake-ups, or @c std::nullopt
		 *                      to disable periodic wake-ups.
		 *
		 * If a wake interval is set, the worker thread can periodically perform some action (e.g.,
		 * housekeeping tasks) even if there's no immediate external signal.
		 */
		auto set_wake_interval(const std::optional<std::chrono::milliseconds>& wake_interval)
			-> void;

		/**
		 * @brief Starts the worker thread.
		 * @return @c std::optional<std::string> containing an error message if the start operation
		 *         fails, or @c std::nullopt on success.
		 *
		 * Internally, this method:
		 * 1. Calls @c before_start() to allow derived classes to perform setup.
		 * 2. Spawns a new thread (using either @c std::jthread or @c std::thread).
		 * 3. Repeatedly calls @c do_work() until the thread is signaled to stop.
		 */
		auto start(void) -> std::optional<std::string>;

		/**
		 * @brief Requests the worker thread to stop and waits for it to finish.
		 * @return @c std::optional<std::string> containing an error message if the stop operation
		 *         fails, or @c std::nullopt on success.
		 *
		 * Internally, this method:
		 * 1. Signals the thread to stop (via @c stop_source_ or @c stop_requested_).
		 * 2. Joins the thread, ensuring it has fully exited.
		 * 3. Calls @c after_stop() for post-shutdown cleanup in derived classes.
		 */
		auto stop(void) -> std::optional<std::string>;

		/**
		 * @brief Returns the worker thread's title.
		 * @return A string representing the thread's title.
		 */
		[[nodiscard]] auto get_thread_title() const -> std::string { return thread_title_; }

		/**
		 * @brief Checks whether the worker thread is currently running.
		 * @return @c true if the thread is running, @c false otherwise.
		 *
		 * This depends on the thread's internal condition state (e.g., @c
		 * thread_conditions::running).
		 */
		[[nodiscard]] auto is_running() const -> bool;

		/**
		 * @brief Returns a string representation of this @c thread_base object.
		 * @return A string containing descriptive or diagnostic information about the thread.
		 *
		 * Derived classes may override this to include additional details (e.g., current status,
		 * counters, or other state).
		 */
		[[nodiscard]] virtual auto to_string(void) const -> std::string;

	protected:
		/**
		 * @brief Determines whether the thread should continue doing work.
		 * @return @c true if there is work to do, @c false otherwise.
		 *
		 * The default implementation always returns @c false (indicating no ongoing work).
		 * Override this in derived classes if you wish the thread to perform repeated tasks
		 * until some condition changes.
		 */
		[[nodiscard]] virtual auto should_continue_work(void) const -> bool { return false; }

		/**
		 * @brief Called just before the worker thread starts running.
		 * @return @c std::optional<std::string> containing an error message on failure,
		 *         or @c std::nullopt on success.
		 *
		 * Override this method in derived classes to perform any initialization or setup
		 * required before the worker thread begins its main loop.
		 */
		virtual auto before_start(void) -> std::optional<std::string> { return std::nullopt; }

		/**
		 * @brief The main work routine for the worker thread.
		 * @return @c std::optional<std::string> containing an error message on failure,
		 *         or @c std::nullopt on success.
		 *
		 * Derived classes should override this method to implement the actual work the thread
		 * needs to perform. This method is called repeatedly (in an internal loop) until the
		 * thread is signaled to stop or @c should_continue_work() returns @c false.
		 */
		virtual auto do_work(void) -> std::optional<std::string> { return std::nullopt; }

		/**
		 * @brief Called immediately after the worker thread has stopped.
		 * @return @c std::optional<std::string> containing an error message on failure,
		 *         or @c std::nullopt on success.
		 *
		 * Override this method in derived classes to perform any cleanup or finalization tasks
		 * once the worker thread has fully exited.
		 */
		virtual auto after_stop(void) -> std::optional<std::string> { return std::nullopt; }

	protected:
		/**
		 * @brief Interval at which the thread is optionally awakened.
		 *
		 * If set, the worker thread can wake periodically (in addition to any other wake
		 * conditions) to perform tasks at regular intervals.
		 */
		std::optional<std::chrono::milliseconds> wake_interval_;

	private:
		/**
		 * @brief Mutex for synchronizing access to internal state and condition variables.
		 *
		 * @c worker_condition_ is typically waited on by the worker thread, and the main thread
		 * may notify it to trigger wake-ups or shutdowns.
		 */
		std::mutex cv_mutex_;

		/**
		 * @brief Condition variable used to block or wake the worker thread.
		 *
		 * Combined with @c cv_mutex_, this enables the worker thread to sleep until
		 * a specific event (e.g., a stop request or a timed interval) occurs.
		 */
		std::condition_variable worker_condition_;

#ifdef USE_STD_JTHREAD
		/**
		 * @brief A @c std::jthread (if available) for managing the worker thread's lifecycle.
		 *
		 * When using jthread, stopping is signaled via @c stop_source_ and @c stop_token.
		 */
		std::unique_ptr<std::jthread> worker_thread_;

		/**
		 * @brief The optional stop source (for jthread) to request stopping the thread.
		 */
		std::optional<std::stop_source> stop_source_;
#else
		/**
		 * @brief A @c std::thread for managing the worker thread's lifecycle (legacy mode).
		 *
		 * If @c USE_STD_JTHREAD is not defined, we fall back to a standard thread and an atomic
		 * stop flag.
		 */
		std::unique_ptr<std::thread> worker_thread_;

		/**
		 * @brief An atomic flag to indicate that the thread should stop.
		 *
		 * Set to @c true when @c stop() is called, signaling the worker thread to exit its loop.
		 */
		std::atomic<bool> stop_requested_;
#endif

		/**
		 * @brief A string title for identifying or naming the worker thread.
		 */
		std::string thread_title_;

		/**
		 * @brief The current condition/state of the thread (e.g., running, stopped).
		 *
		 * Used internally to track whether the thread is active or if it has been requested to
		 * stop.
		 */
		std::atomic<thread_conditions> thread_condition_;
	};
} // namespace thread_module

// ----------------------------------------------------------------------------
// Formatter specializations for thread_base
// ----------------------------------------------------------------------------

#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for @c thread_module::thread_base.
 *
 * Allows @c thread_base objects to be formatted as strings using the C++20 <format> library
 * (when @c USE_STD_FORMAT is defined).
 *
 * ### Example
 * @code
 * auto worker = std::make_shared<my_thread>();
 * std::string output = std::format("Thread info: {}", *worker);
 * @endcode
 */
template <> struct std::formatter<thread_module::thread_base> : std::formatter<std::string_view>
{
	/**
	 * @brief Formats a @c thread_base object as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param item The @c thread_base to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_module::thread_base& item, FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character @c thread_module::thread_base.
 *
 * Allows @c thread_base objects to be formatted as wide strings using the C++20 <format> library.
 */
template <>
struct std::formatter<thread_module::thread_base, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a @c thread_base object as a wide string.
	 * @tparam FormatContext Type of the format context.
	 * @param item The @c thread_base to format.
	 * @param ctx The wide-character format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_module::thread_base& item, FormatContext& ctx) const
	{
		auto str = item.to_string();
		auto wstr = convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
#else // USE_STD_FORMAT

/**
 * @brief Specialization of fmt::formatter for @c thread_module::thread_base.
 *
 * Enables formatting of @c thread_base objects as strings using the {fmt} library.
 *
 * ### Example
 * @code
 * auto worker = std::make_shared<my_thread>();
 * std::string output = fmt::format("Thread info: {}", *worker);
 * @endcode
 */
template <> struct fmt::formatter<thread_module::thread_base> : fmt::formatter<std::string_view>
{
	/**
	 * @brief Formats a @c thread_base object as a string using {fmt}.
	 * @tparam FormatContext Type of the format context.
	 * @param item The @c thread_base to format.
	 * @param ctx  The format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_module::thread_base& item, FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};
#endif
