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

#include <string>
#include <array>
#include <cstdint>
#include <string_view>

namespace priority_thread_pool_module
{
	/**
	 * @enum job_priorities
	 * @brief Enumeration of job priority levels.
	 *
	 * Defines different levels of job priorities for the thread pool.
	 * This enum class is based on uint8_t for efficient storage.
	 */
	enum class job_priorities : uint8_t
	{
		High,	///< High priority job
		Normal, ///< Normal priority job
		Low		///< Low priority job
	};

	namespace job_detail
	{
		/**
		 * @brief Array of string representations for each job priority level.
		 */
		constexpr std::array job_priority_strings = { "HIGH", "NORMAL", "LOW" };

		/**
		 * @brief Total number of job priority levels defined in job_priorities.
		 */
		constexpr size_t job_priority_count = job_priority_strings.size();

		// Compile-time check to ensure job_priority_strings and job_priorities are in sync
		static_assert(job_priority_count == static_cast<size_t>(job_priorities::Low) + 1,
					  "job_priority_strings and job_priorities enum are out of sync");
	}

	/**
	 * @brief Converts a job_priorities value to its string representation.
	 * @param job_priority The job_priorities value to convert.
	 * @return std::string_view String representation of the job priority.
	 */
	[[nodiscard]] constexpr std::string_view to_string(job_priorities job_priority)
	{
		auto index = static_cast<size_t>(job_priority);
		return (index < job_detail::job_priority_count) ? job_detail::job_priority_strings[index]
														: "UNKNOWN";
	}
} // namespace priority_thread_pool_module

// Formatter specializations for job_priorities
#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for job_priorities.
 * Enables formatting of job_priorities enum values as strings in the standard library format.
 */
template <>
struct std::formatter<priority_thread_pool_module::job_priorities>
	: std::formatter<std::string_view>
{
	/**
	 * @brief Formats a job_priorities value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param job_priority The job_priorities enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const priority_thread_pool_module::job_priorities& job_priority,
				FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(
			priority_thread_pool_module::to_string(job_priority), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character job_priorities.
 * Allows job_priorities enum values to be formatted as wide strings in the standard library format.
 */
template <>
struct std::formatter<priority_thread_pool_module::job_priorities, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a job_priorities value as a wide string.
	 * @tparam FormatContext Type of the format context.
	 * @param job_priority The job_priorities enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const priority_thread_pool_module::job_priorities& job_priority,
				FormatContext& ctx) const
	{
		auto str = priority_thread_pool_module::to_string(job_priority);
		std::wstring wstr(str.begin(), str.end());
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
#else
/**
 * @brief Specialization of fmt::formatter for job_priorities.
 * Enables formatting of job_priorities enum values using the fmt library.
 */
template <>
struct fmt::formatter<priority_thread_pool_module::job_priorities>
	: fmt::formatter<std::string_view>
{
	/**
	 * @brief Formats a job_priorities value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param job_priority The job_priorities enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const priority_thread_pool_module::job_priorities& job_priority,
				FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(
			priority_thread_pool_module::to_string(job_priority), ctx);
	}
};
#endif