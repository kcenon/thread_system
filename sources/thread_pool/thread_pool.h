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

#include <tuple>
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <optional>

using namespace thread_module;

namespace thread_pool_module
{
	/**
	 * @class thread_pool
	 * @brief A thread pool class that manages a collection of worker threads.
	 *
	 * This class provides functionality to manage a pool of worker threads,
	 * a job queue, and methods to enqueue jobs and workers. It inherits from
	 * std::enable_shared_from_this to allow creating shared_ptr from this.
	 */
	class thread_pool : public std::enable_shared_from_this<thread_pool>
	{
	public:
		/**
		 * @brief Constructs a new thread_pool object.
		 */
		thread_pool(const std::string& thread_title = "thread_pool");

		/**
		 * @brief Virtual destructor for the thread_pool class.
		 */
		virtual ~thread_pool(void);

		/**
		 * @brief Get a shared pointer to this thread_pool object.
		 * @return std::shared_ptr<thread_pool> A shared pointer to this thread_pool.
		 */
		[[nodiscard]] auto get_ptr(void) -> std::shared_ptr<thread_pool>;

		/**
		 * @brief Starts the thread pool.
		 * @return std::optional<std::string> A tuple containing:
		 *         - bool: Indicates whether the start operation was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto start(void) -> std::optional<std::string>;

		/**
		 * @brief Gets the job queue associated with this thread pool.
		 * @return std::shared_ptr<job_queue> A shared pointer to the job queue.
		 */
		[[nodiscard]] auto get_job_queue(void) -> std::shared_ptr<job_queue>;

		/**
		 * @brief Enqueues a job to the thread pool's job queue.
		 * @param job A unique pointer to the job to be enqueued.
		 * @return std::optional<std::string> A tuple containing:
		 *         - bool: Indicates whether the enqueue operation was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto enqueue(std::unique_ptr<job>&& job) -> std::optional<std::string>;

		/**
		 * @brief Enqueues a worker thread to the thread pool.
		 * @param worker A unique pointer to the worker thread to be enqueued.
		 * @return std::optional<std::string> A tuple containing:
		 *         - bool: Indicates whether the enqueue operation was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto enqueue(std::unique_ptr<thread_worker>&& worker) -> std::optional<std::string>;

		/**
		 * @brief Stops the thread pool.
		 * @param immediately_stop If true, stops the pool immediately; if false, allows current
		 * jobs to finish (default is false).
		 */
		auto stop(const bool& immediately_stop = false) -> void;

		/**
		 * @brief Converts the thread_pool object to a string representation.
		 * @return std::string A string representation of the thread_pool object.
		 */
		[[nodiscard]] auto to_string(void) const -> std::string;

	private:
		/** @brief Title for the thread pool */
		std::string thread_title_;

		/** @brief Flag indicating whether the pool is started */
		std::atomic<bool> start_pool_;

		/** @brief The job queue for the thread pool */
		std::shared_ptr<job_queue> job_queue_;

		/** @brief Collection of worker threads */
		std::vector<std::unique_ptr<thread_worker>> workers_;
	};
} // namespace thread_pool_module

// Formatter specializations for thread_pool
#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for thread_pool.
 * Enables formatting of thread_pool enum values as strings in the standard library format.
 */
template <>
struct std::formatter<thread_pool_module::thread_pool> : std::formatter<std::string_view>
{
	/**
	 * @brief Formats a thread_pool value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param priority The thread_pool enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_pool_module::thread_pool& item, FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character thread_pool.
 * Allows thread_pool enum values to be formatted as wide strings in the standard library format.
 */
template <>
struct std::formatter<thread_pool_module::thread_pool, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a thread_pool value as a wide string.
	 * @tparam FormatContext Type of the format context.
	 * @param priority The thread_pool enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_pool_module::thread_pool& item, FormatContext& ctx) const
	{
		auto str = item.to_string();
		std::wstring wstr(str.begin(), str.end());
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
#else
/**
 * @brief Specialization of fmt::formatter for thread_pool.
 * Enables formatting of thread_pool enum values using the fmt library.
 */
template <>
struct fmt::formatter<thread_pool_module::thread_pool> : fmt::formatter<std::string_view>
{
	/**
	 * @brief Formats a thread_pool value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param priority The thread_pool enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_pool_module::thread_pool& item, FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};
#endif