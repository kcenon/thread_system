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
	 * @class priority_thread_pool
	 * @brief A thread pool class that manages priority-based jobs and workers.
	 *
	 * This template class implements a thread pool that manages jobs and workers
	 * with different priority levels. It inherits from std::enable_shared_from_this
	 * to allow creating shared_ptr from this.
	 *
	 * @tparam priority_type The type used to represent the priority levels.
	 */
	template <typename priority_type = job_priorities>
	class priority_thread_pool_t
		: public std::enable_shared_from_this<priority_thread_pool_t<priority_type>>
	{
	public:
		/**
		 * @brief Constructs a new priority_thread_pool object.
		 */
		priority_thread_pool_t(const std::string& thread_title = "priority_thread_pool");

		/**
		 * @brief Virtual destructor for the priority_thread_pool class.
		 */
		virtual ~priority_thread_pool_t(void);

		/**
		 * @brief Get a shared pointer to this priority_thread_pool object.
		 * @return std::shared_ptr<priority_thread_pool<priority_type>> A shared pointer to this
		 * priority_thread_pool.
		 */
		[[nodiscard]] auto get_ptr(void) -> std::shared_ptr<priority_thread_pool_t<priority_type>>;

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
		 * @return std::shared_ptr<priority_job_queue_t<priority_type>> A shared pointer to
		 * the priority job queue.
		 */
		[[nodiscard]] auto get_job_queue(void)
			-> std::shared_ptr<priority_job_queue_t<priority_type>>;

		/**
		 * @brief Enqueues a priority job to the thread pool's job queue.
		 * @param job A unique pointer to the priority job to be enqueued.
		 * @return std::optional<std::string> A tuple containing:
		 *         - bool: Indicates whether the enqueue operation was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto enqueue(std::unique_ptr<priority_job_t<priority_type>>&& job)
			-> std::optional<std::string>;

		/**
		 * @brief Enqueues a priority thread worker to the thread pool.
		 * @param worker A unique pointer to the priority thread worker to be enqueued.
		 * @return std::optional<std::string> A tuple containing:
		 *         - bool: Indicates whether the enqueue operation was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto enqueue(std::unique_ptr<priority_thread_worker_t<priority_type>>&& worker)
			-> std::optional<std::string>;

		/**
		 * @brief Stops the thread pool.
		 * @param immediately_stop If true, stops the pool immediately; if false, allows current
		 * jobs to finish (default is false).
		 */
		auto stop(const bool& immediately_stop = false) -> void;

		/**
		 * @brief Get a string representation of the thread pool.
		 * @return std::string A string representation of the thread pool.
		 */
		[[nodiscard]] auto to_string(void) const -> std::string;

	private:
		/** @brief Title for the thread pool */
		std::string thread_title_;

		/** @brief Flag indicating whether the pool is started */
		std::atomic<bool> start_pool_;

		/** @brief The priority job queue for the thread pool */
		std::shared_ptr<priority_job_queue_t<priority_type>> job_queue_;

		/** @brief Collection of priority thread workers */
		std::vector<std::unique_ptr<priority_thread_worker_t<priority_type>>> workers_;
	};

	using priority_thread_pool = priority_thread_pool_t<job_priorities>;
} // namespace priority_thread_pool_module

// Formatter specializations for priority_thread_pool_t<priority_type>
#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for priority_thread_pool_t<priority_type>.
 * Enables formatting of priority_thread_pool_t<priority_type> enum values as strings in the
 * standard library format.
 */
template <typename priority_type>
struct std::formatter<priority_thread_pool_module::priority_thread_pool_t<priority_type>>
	: std::formatter<std::string_view>
{
	/**
	 * @brief Formats a priority_thread_pool_t<priority_type> value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param priority The priority_thread_pool_t<priority_type> enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const priority_thread_pool_module::priority_thread_pool_t<priority_type>& item,
				FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character priority_thread_pool_t<priority_type>.
 * Allows priority_thread_pool_t<priority_type> enum values to be formatted as wide strings in the
 * standard library format.
 */
template <typename priority_type>
struct std::formatter<priority_thread_pool_module::priority_thread_pool_t<priority_type>, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a priority_thread_pool_t<priority_type> value as a wide string.
	 * @tparam FormatContext Type of the format context.
	 * @param priority The priority_thread_pool_t<priority_type> enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
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
 * Enables formatting of priority_thread_pool_t<priority_type> enum values using the fmt library.
 */
template <typename priority_type>
struct fmt::formatter<priority_thread_pool_module::priority_thread_pool_t<priority_type>>
	: fmt::formatter<std::string_view>
{
	/**
	 * @brief Formats a priority_thread_pool_t<priority_type> value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param priority The priority_thread_pool_t<priority_type> enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
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