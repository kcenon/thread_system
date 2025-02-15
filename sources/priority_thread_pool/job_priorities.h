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

namespace priority_thread_pool_module
{
	/**
	 * @enum job_priorities
	 * @brief Defines different levels of job priorities for a priority-based thread pool.
	 *
	 * Each job in a priority thread pool can be assigned one of these levels (High, Normal, Low)
	 * to influence the order in which tasks are executed. This enum uses @c uint8_t as its
	 * underlying type to minimize storage overhead.
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
		 * @brief String representations corresponding to each @c job_priorities value.
		 *
		 * Indexed by casting a @c job_priorities value to @c size_t. E.g.,
		 * @code
		 * job_detail::job_priority_strings[static_cast<size_t>(job_priorities::High)] // "HIGH"
		 * @endcode
		 */
		constexpr std::array job_priority_strings = { "HIGH", "NORMAL", "LOW" };

		/**
		 * @brief The number of priority levels defined in @c job_priorities.
		 *
		 * Used in boundary checks to prevent out-of-range access into @c job_priority_strings.
		 */
		constexpr size_t job_priority_count = job_priority_strings.size();

		/**
		 * @brief Compile-time check ensuring @c job_priority_strings matches the @c job_priorities
		 * enum.
		 *
		 * If this static_assert fails, it usually means a new priority value was added
		 * to @c job_priorities without updating @c job_priority_strings.
		 */
		static_assert(job_priority_count == static_cast<size_t>(job_priorities::Low) + 1,
					  "job_priority_strings and job_priorities enum are out of sync");
	}

	/**
	 * @brief Converts a @c job_priorities value to its corresponding string representation.
	 * @param job_priority The @c job_priorities value to convert.
	 * @return A @c std::string_view containing one of "HIGH", "NORMAL", "LOW",
	 *         or "UNKNOWN" if @p job_priority is out of expected range.
	 *
	 * ### Example
	 * @code
	 * auto p = job_priorities::High;
	 * std::string_view sv = to_string(p); // "HIGH"
	 * @endcode
	 */
	[[nodiscard]] constexpr std::string_view to_string(job_priorities job_priority)
	{
		auto index = static_cast<size_t>(job_priority);
		return (index < job_detail::job_priority_count) ? job_detail::job_priority_strings[index]
														: "UNKNOWN";
	}

	/**
	 * @brief Returns a vector containing all possible @c job_priorities values.
	 * @return A @c std::vector<job_priorities> with [High, Normal, Low].
	 *
	 * This function is useful when iterating over all defined priorities (e.g. for logging,
	 * UI selection, or testing).
	 *
	 * ### Example
	 * @code
	 * for (auto priority : all_priorities()) {
	 *     std::cout << to_string(priority) << std::endl;
	 * }
	 * // Output:
	 * // HIGH
	 * // NORMAL
	 * // LOW
	 * @endcode
	 */
	[[nodiscard]] inline auto all_priorities(void) -> std::vector<job_priorities>
	{
		return { job_priorities::High, job_priorities::Normal, job_priorities::Low };
	}
} // namespace priority_thread_pool_module

// ----------------------------------------------------------------------------
// Formatter specializations for job_priorities
// ----------------------------------------------------------------------------

#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of @c std::formatter for @c job_priorities using narrow strings.
 *
 * Allows code such as:
 * @code
 * std::string s = std::format("Priority is {}", job_priorities::High); // "Priority is HIGH"
 * @endcode
 */
template <>
struct std::formatter<priority_thread_pool_module::job_priorities>
	: std::formatter<std::string_view>
{
	/**
	 * @brief Formats a @c job_priorities value as a narrow string.
	 * @tparam FormatContext The type of the format context.
	 * @param job_priority The @c job_priorities enum value to format.
	 * @param ctx The format context for the output.
	 * @return An iterator to the end of the formatted output.
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
 * @brief Specialization of @c std::formatter for @c job_priorities using wide strings.
 *
 * Allows code such as:
 * @code
 * std::wstring ws = std::format(L"Priority is {}", job_priorities::Normal); // L"Priority is
 * NORMAL"
 * @endcode
 */
template <>
struct std::formatter<priority_thread_pool_module::job_priorities, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a @c job_priorities value as a wide string.
	 * @tparam FormatContext The type of the format context.
	 * @param job_priority The @c job_priorities enum value to format.
	 * @param ctx The wide-character format context for the output.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const priority_thread_pool_module::job_priorities& job_priority,
				FormatContext& ctx) const
	{
		auto str = priority_thread_pool_module::to_string(job_priority);
		auto wstr = convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};

#else // USE_STD_FORMAT

/**
 * @brief Specialization of fmt::formatter for @c job_priorities using narrow strings.
 *
 * Allows code such as:
 * @code
 * std::string s = fmt::format("Priority is {}", job_priorities::High); // "Priority is HIGH"
 * @endcode
 */
template <>
struct fmt::formatter<priority_thread_pool_module::job_priorities>
	: fmt::formatter<std::string_view>
{
	/**
	 * @brief Formats a @c job_priorities value as a narrow string using {fmt}.
	 * @tparam FormatContext The type of the format context.
	 * @param job_priority The @c job_priorities enum value to format.
	 * @param ctx The format context for the output.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const priority_thread_pool_module::job_priorities& job_priority,
				FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(
			priority_thread_pool_module::to_string(job_priority), ctx);
	}
};

/**
 * @brief Specialization of fmt::formatter for @c job_priorities using wide strings.
 *
 * Allows code such as:
 * @code
 * std::wstring ws = fmt::format(L"Priority is {}", job_priorities::Low); // L"Priority is LOW"
 * @endcode
 */
template <>
struct fmt::formatter<priority_thread_pool_module::job_priorities, wchar_t>
	: fmt::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a @c job_priorities value as a wide string using {fmt}.
	 * @tparam FormatContext The type of the format context.
	 * @param job_priority The @c job_priorities enum value to format.
	 * @param ctx The wide-character format context for the output.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const priority_thread_pool_module::job_priorities& job_priority,
				FormatContext& ctx) const
	{
		auto str = priority_thread_pool_module::to_string(job_priority);
		auto wstr = convert_string::to_wstring(str);
		return fmt::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
#endif