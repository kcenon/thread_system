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

/**
 * @file thread_pool_fmt.h
 * @brief std::formatter specializations for thread_pool
 * @date 2025-01-11
 *
 * @deprecated This header is deprecated. Include <kcenon/thread/formatters.h> instead
 *             for unified access to all formatter specializations.
 *
 * This file contains std::formatter specializations for the thread_pool class,
 * enabling formatting via C++20 std::format.
 *
 * Separated from thread_pool.h to reduce header size and improve compilation times.
 */

// Deprecation warning is disabled for internal includes.
// Define KCENON_THREAD_INTERNAL_INCLUDE before including to suppress the warning.
#if !defined(KCENON_THREAD_INTERNAL_INCLUDE)
#if defined(__GNUC__) || defined(__clang__)
#pragma message("thread_pool_fmt.h is deprecated. Include <kcenon/thread/formatters.h> instead.")
#elif defined(_MSC_VER)
#pragma message("thread_pool_fmt.h is deprecated. Include <kcenon/thread/formatters.h> instead.")
#endif
#endif

#include <kcenon/thread/utils/convert_string.h>

#include <format>
#include <string>
#include <string_view>

// Forward declaration
namespace kcenon::thread {
class thread_pool;
}

/**
 * @brief Specialization of std::formatter for @c kcenon::thread::thread_pool.
 *
 * Enables formatting of @c thread_pool objects as strings using C++20 std::format.
 *
 * ### Example
 * @code
 * auto pool = std::make_shared<kcenon::thread::thread_pool>("MyPool");
 * std::string output = std::format("Pool Info: {}", *pool); // e.g. "Pool Info: [thread_pool: MyPool]"
 * @endcode
 */
template <>
struct std::formatter<kcenon::thread::thread_pool> : std::formatter<std::string_view>
{
	/**
	 * @brief Formats a @c thread_pool object as a string.
	 * @tparam FormatContext The type of the format context.
	 * @param item The @c thread_pool to format.
	 * @param ctx  The format context for the output.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const kcenon::thread::thread_pool& item, FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character @c kcenon::thread::thread_pool.
 *
 * Allows wide-string formatting of @c thread_pool objects using C++20 std::format.
 */
template <>
struct std::formatter<kcenon::thread::thread_pool, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a @c thread_pool object as a wide string.
	 * @tparam FormatContext The type of the format context.
	 * @param item The @c thread_pool to format.
	 * @param ctx  The wide-character format context.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const kcenon::thread::thread_pool& item, FormatContext& ctx) const
	{
		auto str = item.to_string();
		auto wstr = utility_module::convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
