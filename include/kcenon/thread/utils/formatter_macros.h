// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#pragma once

/**
 * @file formatter_macros.h
 * @brief Provides macros for generating std::formatter specializations.
 *
 * This file contains macros that generate std::formatter specializations for custom types
 * to work with C++20 std::format. The macros eliminate code duplication by generating
 * the necessary boilerplate code for formatter specializations.
 *
 * Usage example:
 * @code
 * // In your header file after class definition:
 * DECLARE_FORMATTER(my_namespace::my_class)
 * @endcode
 *
 * @note This project requires C++20 std::format support. The fmt library fallback
 *       has been removed as of issue #219.
 */

#include "formatter.h"
#include "convert_string.h"

#include <format>

/**
 * @brief Generates std::formatter specializations for narrow and wide characters.
 *
 * This macro creates two formatter specializations:
 * 1. std::formatter<CLASS_NAME> for narrow character formatting
 * 2. std::formatter<CLASS_NAME, wchar_t> for wide character formatting
 *
 * Requirements:
 * - The class must have a to_string() method that returns std::string
 * - The convert_string::to_wstring function must be available
 *
 * @param CLASS_NAME The fully qualified class name (including namespace if any)
 */
#define DECLARE_FORMATTER(CLASS_NAME)                                                              \
template <> struct std::formatter<CLASS_NAME> : std::formatter<std::string_view>                   \
{                                                                                                  \
    template <typename FormatContext>                                                              \
    auto format(const CLASS_NAME& item, FormatContext& ctx) const                                  \
    {                                                                                              \
        return std::formatter<std::string_view>::format(item.to_string(), ctx);                    \
    }                                                                                              \
};                                                                                                 \
                                                                                                   \
template <>                                                                                        \
struct std::formatter<CLASS_NAME, wchar_t> : std::formatter<std::wstring_view, wchar_t>            \
{                                                                                                  \
    template <typename FormatContext>                                                              \
    auto format(const CLASS_NAME& item, FormatContext& ctx) const                                  \
    {                                                                                              \
        auto str = item.to_string();                                                               \
        auto wstr = utility_module::convert_string::to_wstring(str);                               \
        return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);                      \
    }                                                                                              \
};
