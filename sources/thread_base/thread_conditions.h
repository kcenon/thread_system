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

#include <string>
#include <array>
#include <cstdint>
#include <string_view>

using namespace utility_module;

namespace thread_module
{
	/**
	 * @enum thread_conditions
	 * @brief Enumeration of job priority levels.
	 *
	 * Defines different levels of job priorities for the thread pool.
	 * This enum class is based on uint8_t for efficient storage.
	 */
	enum class thread_conditions : uint8_t
	{
		Created,  ///< Thread created but not started
		Waiting,  ///< Thread waiting for a job
		Working,  ///< Thread working a job
		Stopping, ///< Thread stopping
		Stopped,  ///< Thread stopped
	};

	namespace job_detail
	{
		/**
		 * @brief Array of string representations for each job priority level.
		 */
		constexpr std::array thread_conditions_strings
			= { "created", "waiting", "working", "stopping", "stopped" };

		/**
		 * @brief Total number of job priority levels defined in thread_conditions.
		 */
		constexpr size_t thread_conditions_count = thread_conditions_strings.size();

		// Compile-time check to ensure thread_conditions_strings and thread_conditions are in sync
		static_assert(thread_conditions_count
						  == static_cast<size_t>(thread_conditions::Stopped) + 1,
					  "thread_conditions_strings and thread_conditions enum are out of sync");
	}

	/**
	 * @brief Converts a thread_conditions value to its string representation.
	 * @param job_priority The thread_conditions value to convert.
	 * @return std::string_view String representation of the job priority.
	 */
	[[nodiscard]] constexpr std::string_view to_string(thread_conditions job_priority)
	{
		auto index = static_cast<size_t>(job_priority);
		return (index < job_detail::thread_conditions_count)
				   ? job_detail::thread_conditions_strings[index]
				   : "UNKNOWN";
	}

	/**
	 * @brief Converts a string to a thread_conditions value.
	 * @return std::vector<thread_conditions>
	 */
	[[nodiscard]] inline auto all_priorities(void) -> std::vector<thread_conditions>
	{
		return { thread_conditions::Created, thread_conditions::Waiting, thread_conditions::Working,
				 thread_conditions::Stopping, thread_conditions::Stopped };
	}
} // namespace thread_module

// Formatter specializations for thread_conditions
#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for thread_conditions.
 * Enables formatting of thread_conditions enum values as strings in the standard library format.
 */
template <>
struct std::formatter<thread_module::thread_conditions> : std::formatter<std::string_view>
{
	/**
	 * @brief Formats a thread_conditions value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param thread_condition The thread_conditions enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_module::thread_conditions& thread_condition, FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(thread_module::to_string(thread_condition),
														ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character thread_conditions.
 * Allows thread_conditions enum values to be formatted as wide strings in the standard library
 * format.
 */
template <>
struct std::formatter<thread_module::thread_conditions, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a thread_conditions value as a wide string.
	 * @tparam FormatContext Type of the format context.
	 * @param thread_condition The thread_conditions enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_module::thread_conditions& thread_condition, FormatContext& ctx) const
	{
		auto str = thread_module::to_string(thread_condition);
		auto wstr = convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
#else
/**
 * @brief Specialization of fmt::formatter for thread_conditions.
 * Enables formatting of thread_conditions enum values using the fmt library.
 */
template <>
struct fmt::formatter<thread_module::thread_conditions> : fmt::formatter<std::string_view>
{
	/**
	 * @brief Formats a thread_conditions value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param thread_condition The thread_conditions enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_module::thread_conditions& thread_condition, FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(thread_module::to_string(thread_condition),
														ctx);
	}
};
#endif