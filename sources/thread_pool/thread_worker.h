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

#include "thread_base.h"

#include "job_queue.h"
#include "convert_string.h"

#include <memory>
#include <vector>

using namespace utility_module;
using namespace thread_module;

namespace thread_pool_module
{
	/**
	 * @class thread_worker
	 * @brief A worker thread class that processes jobs from a job queue.
	 *
	 * This class extends thread_base to create a worker thread that can
	 * process jobs from a specified job queue. It provides functionality
	 * to set a job queue and override the base class's work-related methods.
	 */
	class thread_worker : public thread_base
	{
	public:
		/**
		 * @brief Constructs a new thread_worker object.
		 * @param use_time_tag A boolean flag indicating whether to use time tags in job processing
		 * (default is true).
		 */
		thread_worker(const bool& use_time_tag = true);

		/**
		 * @brief Virtual destructor for the thread_worker class.
		 */
		virtual ~thread_worker(void);

		/**
		 * @brief Sets the job queue for this worker to process.
		 * @param job_queue A shared pointer to the job queue.
		 */
		auto set_job_queue(std::shared_ptr<job_queue> job_queue) -> void;

	protected:
		/**
		 * @brief Checks if there is work available in the job queue.
		 * @return bool True if there is work to be done, false otherwise.
		 */
		[[nodiscard]] auto should_continue_work() const -> bool override;

		/**
		 * @brief Performs the actual work by processing jobs from the queue.
		 * @return std::optional<std::string> A tuple containing:
		 *         - bool: Indicates whether the work was successful (true) or not (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto do_work() -> std::optional<std::string> override;

	private:
		/** @brief Flag indicating whether to use time tags in job processing */
		bool use_time_tag_;

		/** @brief The job queue to process */
		std::shared_ptr<job_queue> job_queue_;
	};
} // namespace thread_pool_module

// Formatter specializations for thread_worker
#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for thread_worker.
 * Enables formatting of thread_worker enum values as strings in the standard library format.
 */
template <>
struct std::formatter<thread_pool_module::thread_worker> : std::formatter<std::string_view>
{
	/**
	 * @brief Formats a thread_worker value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param priority The thread_worker enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_pool_module::thread_worker& item, FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character thread_worker.
 * Allows thread_worker enum values to be formatted as wide strings in the standard library format.
 */
template <>
struct std::formatter<thread_pool_module::thread_worker, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a thread_worker value as a wide string.
	 * @tparam FormatContext Type of the format context.
	 * @param priority The thread_worker enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_pool_module::thread_worker& item, FormatContext& ctx) const
	{
		auto str = item.to_string();
		auto wstr = convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
#else
/**
 * @brief Specialization of fmt::formatter for thread_worker.
 * Enables formatting of thread_worker enum values using the fmt library.
 */
template <>
struct fmt::formatter<thread_pool_module::thread_worker> : fmt::formatter<std::string_view>
{
	/**
	 * @brief Formats a thread_worker value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param priority The thread_worker enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_pool_module::thread_worker& item, FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};
#endif