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

#ifdef USE_STD_FORMAT
#include <format>
#else
#include "fmt/format.h"
#endif

#include <string>
#include <array>
#include <cstdint>

/**
 * @enum test_priority
 * @brief Enumeration of test priority levels.
 *
 * This enum class defines various priority levels for tests.
 * It is based on uint8_t for efficient storage.
 */
enum class test_priority : uint8_t
{
	Top = 0,
	Middle,
	Bottom
};

namespace test_detail
{
	constexpr std::array test_priority_strings = { "Top", "Middle", "Bottom" };

	constexpr size_t test_priority_count = test_priority_strings.size();

	// Compile-time check to ensure test_priority_strings and test_priority are in sync
	static_assert(test_priority_count == static_cast<size_t>(test_priority::Bottom) + 1,
				  "test_priority_strings and test_priority enum are out of sync");
}

/**
 * @brief Converts a test_priority value to its string representation.
 * @param priority The test_priority value to convert.
 * @return std::string_view A string representation of the test priority.
 */
constexpr std::string_view to_string(test_priority priority)
{
	auto index = static_cast<size_t>(priority);
	return (index < test_detail::test_priority_count) ? test_detail::test_priority_strings[index]
													  : "Unknown";
}

#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for test_priority.
 *
 * This formatter allows test_priority to be used with std::format.
 * It converts the test_priority enum values to their string representations.
 */
template <> struct std::formatter<test_priority>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const test_priority& priority, FormatContext& ctx) const
	{
		return std::format_to(ctx.out(), "{}", to_string(priority));
	}
};
#else
/**
 * @brief Specialization of fmt::formatter for test_priority.
 *
 * This formatter allows test_priority to be used with fmt::format.
 * It converts the test_priority enum values to their string representations.
 */
template <> struct fmt::formatter<test_priority>
{
	constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const test_priority& priority, FormatContext& ctx) const
	{
		return fmt::format_to(ctx.out(), "{}", to_string(priority));
	}
};
#endif