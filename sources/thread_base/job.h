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

#include <tuple>
#include <memory>
#include <string>
#include <optional>
#include <functional>
#include <string_view>

using namespace utility_module;

namespace thread_module
{
	class job_queue;

	/**
	 * @class job
	 * @brief Represents a job that can be executed by a job queue.
	 *
	 * This class is a base class for all jobs that can be executed by a job queue.
	 * It provides a callback function that is executed when the job is executed.
	 * The callback function must return a tuple with a boolean value indicating
	 * whether the job was executed successfully and an optional string with an
	 * error message in case the job failed.
	 */
	class job
	{
	public:
		/**
		 * @brief Constructs a new job object.
		 * @param callback The function to be executed when the job is processed.
		 *        It should return a tuple containing a boolean indicating success and an optional
		 * string message.
		 * @param name The name of the job (default is "job").
		 */
		job(const std::function<std::optional<std::string>(void)>& callback,
			const std::string& name = "job");

		/**
		 * @brief Virtual destructor for the job class.
		 */
		virtual ~job(void);

		/**
		 * @brief Get the name of the job.
		 * @return std::string The name of the job.
		 */
		[[nodiscard]] auto get_name(void) const -> std::string;

		/**
		 * @brief Execute the job's work.
		 * @return std::optional<std::string> A tuple containing:
		 *         - bool: Indicates whether the job was successful (true) or not (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions or additional information.
		 */
		[[nodiscard]] virtual auto do_work(void) -> std::optional<std::string>;

		/**
		 * @brief Set the job queue for this job.
		 * @param job_queue A shared pointer to the job queue to be associated with this job.
		 */
		virtual auto set_job_queue(const std::shared_ptr<job_queue>& job_queue) -> void;

		/**
		 * @brief Get the job queue associated with this job.
		 * @return std::shared_ptr<job_queue> A shared pointer to the associated job queue.
		 *         If no job queue is associated, this may return a null shared_ptr.
		 */
		[[nodiscard]] virtual auto get_job_queue(void) const -> std::shared_ptr<job_queue>;

		/**
		 * @brief Get a string representation of the job.
		 * @return std::string A string representation of the job.
		 */
		[[nodiscard]] virtual auto to_string(void) const -> std::string;

	protected:
		/** @brief The name of the job */
		std::string name_;

		/** @brief Weak pointer to the associated job queue */
		std::weak_ptr<job_queue> job_queue_;

		/** @brief The callback function to be executed when the job is processed */
		std::function<std::optional<std::string>(void)> callback_;
	};
} // namespace thread_module

// Formatter specializations for job
#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for job.
 * Enables formatting of job enum values as strings in the standard library format.
 */
template <> struct std::formatter<thread_module::job> : std::formatter<std::string_view>
{
	/**
	 * @brief Formats a job value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param item The job enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_module::job& item, FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character job.
 * Allows job enum values to be formatted as wide strings in the standard library format.
 */
template <>
struct std::formatter<thread_module::job, wchar_t> : std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a job value as a wide string.
	 * @tparam FormatContext Type of the format context.
	 * @param item The job enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_module::job& item, FormatContext& ctx) const
	{
		auto str = item.to_string();
		auto wstr = convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
#else
/**
 * @brief Specialization of fmt::formatter for job.
 * Enables formatting of job enum values using the fmt library.
 */
template <> struct fmt::formatter<thread_module::job> : fmt::formatter<std::string_view>
{
	/**
	 * @brief Formats a job value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param item The job enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_module::job& item, FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};
#endif