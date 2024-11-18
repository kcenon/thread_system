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
	 * @brief Base class for implementing thread-based workers.
	 *
	 * This class provides a framework for creating worker threads with customizable
	 * behavior. It handles thread lifecycle management and provides mechanisms for
	 * waking the thread at specified intervals and setting custom thread titles.
	 */
	class thread_base
	{
	public:
		thread_base(const thread_base&) = delete;
		thread_base& operator=(const thread_base&) = delete;
		thread_base(thread_base&&) = delete;
		thread_base& operator=(thread_base&&) = delete;

		/**
		 * @brief Constructs a new thread_base object.
		 * @param thread_title The title for the worker thread, used for identification purposes.
		 */
		thread_base(const std::string& thread_title = "thread_base");

		/**
		 * @brief Virtual destructor for the thread_base class.
		 */
		virtual ~thread_base(void);

		/**
		 * @brief Sets the wake interval for the worker thread.
		 * @param wake_interval An optional duration specifying how often the thread should wake up.
		 */
		auto set_wake_interval(const std::optional<std::chrono::milliseconds>& wake_interval)
			-> void;

		/**
		 * @brief Starts the worker thread.
		 * @return std::optional<std::string> A tuple containing:
		 *         - bool: Indicates whether the start operation was successful (true) or not.
		 *         - std::optional<std::string>: An optional error message if the operation fails.
		 */
		auto start(void) -> std::optional<std::string>;

		/**
		 * @brief Stops the worker thread.
		 * @return std::optional<std::string> A tuple containing:
		 *         - bool: Indicates whether the stop operation was successful (true) or not.
		 *         - std::optional<std::string>: An optional error message if the operation fails.
		 */
		auto stop(void) -> std::optional<std::string>;

		/**
		 * @brief Retrieves the title of the worker thread.
		 * @return std::string The title of the worker thread, for identification purposes.
		 */
		[[nodiscard]] auto get_thread_title() const -> std::string { return thread_title_; }

		/**
		 * @brief Checks if the worker thread is running.
		 * @return bool True if the worker thread is running, false otherwise.
		 */
		[[nodiscard]] auto is_running() const -> bool;

		/**
		 * @brief Converts the thread_base object to a string representation.
		 * @return std::string A string representation of the thread_base object.
		 */
		[[nodiscard]]
		virtual auto to_string(void) const -> std::string;

	protected:
		/**
		 * @brief Checks if there is work to be done.
		 * @return bool True if there is work to be done, false otherwise.
		 */
		[[nodiscard]] virtual auto should_continue_work(void) const -> bool { return false; }

		/**
		 * @brief Called before the worker thread starts.
		 *
		 * Derived classes can override this method to perform any setup or initialization
		 * required before the thread begins executing.
		 *
		 * @return std::optional<std::string> A tuple containing:
		 *         - bool: Indicates whether the initialization was successful (true) or not.
		 *         - std::optional<std::string>: An optional error message if initialization fails.
		 */
		virtual auto before_start(void) -> std::optional<std::string> { return std::nullopt; }

		/**
		 * @brief Performs the actual work of the thread.
		 *
		 * This method defines the main behavior of the worker thread. Derived classes should
		 * override this to implement specific functionality.
		 *
		 * @return std::optional<std::string> A tuple containing:
		 *         - bool: Indicates whether the work was successful (true) or not.
		 *         - std::optional<std::string>: An optional error message if the work fails.
		 */
		virtual auto do_work(void) -> std::optional<std::string> { return std::nullopt; }

		/**
		 * @brief Called after the worker thread stops.
		 *
		 * Derived classes can override this method to perform any necessary cleanup after the
		 * thread has finished its work and stopped.
		 *
		 * @return std::optional<std::string> A tuple containing:
		 *         - bool: Indicates whether the cleanup was successful (true) or not.
		 *         - std::optional<std::string>: An optional error message if cleanup fails.
		 */
		virtual auto after_stop(void) -> std::optional<std::string> { return std::nullopt; }

	protected:
		/** @brief Interval at which the thread wakes up */
		std::optional<std::chrono::milliseconds> wake_interval_;

	private:
		/** @brief Mutex for protecting shared data and condition variable */
		std::mutex cv_mutex_;

		/** @brief Condition variable for thread synchronization */
		std::condition_variable worker_condition_;

#ifdef USE_STD_JTHREAD
		/** @brief The actual worker thread (jthread version) */
		std::unique_ptr<std::jthread> worker_thread_;

		/** @brief Source for stopping the thread (jthread version) */
		std::optional<std::stop_source> stop_source_;
#else
		/** @brief The actual worker thread */
		std::unique_ptr<std::thread> worker_thread_;

		/** @brief Flag indicating whether the thread should stop */
		std::atomic<bool> stop_requested_;
#endif

		/** @brief The title of the thread for identification purposes */
		std::string thread_title_;
	};
} // namespace thread_module

// Formatter specializations for thread_base
#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for thread_base.
 * Enables formatting of thread_base enum values as strings in the standard library format.
 */
template <> struct std::formatter<thread_module::thread_base> : std::formatter<std::string_view>
{
	/**
	 * @brief Formats a thread_base value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param priority The thread_base enum value to format.
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
 * @brief Specialization of std::formatter for wide-character thread_base.
 * Allows thread_base enum values to be formatted as wide strings in the standard library format.
 */
template <>
struct std::formatter<thread_module::thread_base, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a thread_base value as a wide string.
	 * @tparam FormatContext Type of the format context.
	 * @param priority The thread_base enum value to format.
	 * @param ctx Format context for the output.
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
#else
/**
 * @brief Specialization of fmt::formatter for thread_base.
 * Enables formatting of thread_base enum values using the fmt library.
 */
template <> struct fmt::formatter<thread_module::thread_base> : fmt::formatter<std::string_view>
{
	/**
	 * @brief Formats a thread_base value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param priority The thread_base enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_module::thread_base& item, FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};
#endif