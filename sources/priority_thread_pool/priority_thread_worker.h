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

#include "thread_base.h"
#include "convert_string.h"
#include "priority_job_queue.h"

#include <memory>
#include <vector>

using namespace utility_module;
using namespace thread_module;

namespace priority_thread_pool_module
{
	/**
	 * @class priority_thread_worker
	 * @brief A worker thread class that processes jobs from a priority job queue.
	 *
	 * This template class extends thread_base to create a worker thread that can
	 * process jobs from a specified priority job queue. It provides functionality
	 * to set a job queue and override the base class's work-related methods.
	 *
	 * @tparam priority_type The type used to represent the priority levels.
	 */
	template <typename priority_type = job_priorities>
	class priority_thread_worker_t : public thread_base
	{
	public:
		/**
		 * @brief Constructs a new priority_thread_worker object.
		 * @param priorities A vector of priority levels this worker can process.
		 * @param use_time_tag A boolean flag indicating whether to use time tags in job processing
		 * (default is true).
		 */
		priority_thread_worker_t(std::vector<priority_type> priorities = all_priorities(),
								 const bool& use_time_tag = true);

		/**
		 * @brief Virtual destructor for the priority_thread_worker class.
		 */
		virtual ~priority_thread_worker_t(void);

		/**
		 * @brief Sets the job queue for this worker to process.
		 * @param job_queue A shared pointer to the priority job queue.
		 */
		auto set_job_queue(std::shared_ptr<priority_job_queue_t<priority_type>> job_queue) -> void;

		/**
		 * @brief Gets the priority levels this worker can process.
		 * @return std::vector<priority_type> A vector of priority levels.
		 */
		[[nodiscard]] auto priorities(void) const -> std::vector<priority_type>;

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

		/** @brief The priority levels this worker can process */
		std::vector<priority_type> priorities_;

		/** @brief The priority job queue to process */
		std::shared_ptr<priority_job_queue_t<priority_type>> job_queue_;
	};

	using priority_thread_worker = priority_thread_worker_t<job_priorities>;
} // namespace priority_thread_pool_module

// Formatter specializations for priority_thread_worker_t<priority_type>
#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for priority_thread_worker_t<priority_type>.
 * Enables formatting of priority_thread_worker_t<priority_type> enum values as strings in the
 * standard library format.
 */
template <typename priority_type>
struct std::formatter<priority_thread_pool_module::priority_thread_worker_t<priority_type>>
	: std::formatter<std::string_view>
{
	/**
	 * @brief Formats a priority_thread_worker_t<priority_type> value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param priority The priority_thread_worker_t<priority_type> enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const priority_thread_pool_module::priority_thread_worker_t<priority_type>& item,
				FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character
 * priority_thread_worker_t<priority_type>. Allows priority_thread_worker_t<priority_type> enum
 * values to be formatted as wide strings in the standard library format.
 */
template <typename priority_type>
struct std::formatter<priority_thread_pool_module::priority_thread_worker_t<priority_type>, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a priority_thread_worker_t<priority_type> value as a wide string.
	 * @tparam FormatContext Type of the format context.
	 * @param priority The priority_thread_worker_t<priority_type> enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const priority_thread_pool_module::priority_thread_worker_t<priority_type>& item,
				FormatContext& ctx) const
	{
		auto str = item.to_string();
		auto wstr = convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
#else
/**
 * @brief Specialization of fmt::formatter for priority_thread_worker_t<priority_type>.
 * Enables formatting of priority_thread_worker_t<priority_type> enum values using the fmt library.
 */
template <typename priority_type>
struct fmt::formatter<priority_thread_pool_module::priority_thread_worker_t<priority_type>>
	: fmt::formatter<std::string_view>
{
	/**
	 * @brief Formats a priority_thread_worker_t<priority_type> value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param priority The priority_thread_worker_t<priority_type> enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const priority_thread_pool_module::priority_thread_worker_t<priority_type>& item,
				FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};
#endif

#include "priority_thread_worker.tpp"