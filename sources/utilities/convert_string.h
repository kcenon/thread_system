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

#include <tuple>
#include <array>
#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <string_view>

namespace utility_module
{
	/**
	 * @class convert_string
	 * @brief A utility class for string conversion and manipulation.
	 *
	 * This class provides static methods for converting between different string types
	 * (std::string, std::wstring, std::u16string, std::u32string) and for splitting strings.
	 */
	class convert_string
	{
	public:
		/**
		 * @brief Splits a string into substrings based on a delimiter token.
		 * @param source The source string to split.
		 * @param token The delimiter token to split by.
		 * @return A tuple containing:
		 *         - An optional vector of strings representing the split substrings, if successful.
		 *         - An optional string containing an error message, if an error occurred.
		 */
		static auto split(const std::string& source, const std::string& token)
			-> std::tuple<std::optional<std::vector<std::string>>, std::optional<std::string>>;

		/**
		 * @brief Converts a wide string to a UTF-8 string.
		 * @param wide_string_message The wide string to convert.
		 * @return A tuple containing:
		 *         - An optional string representing the converted UTF-8 string, if successful.
		 *         - An optional string containing an error message, if an error occurred.
		 */
		static auto to_string(const std::wstring& wide_string_message)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

		/**
		 * @brief Converts a UTF-16 string to a UTF-8 string.
		 * @param utf16_string_message The UTF-16 string to convert.
		 * @return A tuple containing:
		 *         - An optional string representing the converted UTF-8 string, if successful.
		 *         - An optional string containing an error message, if an error occurred.
		 */
		static auto to_string(const std::u16string& utf16_string_message)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

		/**
		 * @brief Converts a UTF-32 string to a UTF-8 string.
		 * @param utf32_string_message The UTF-32 string to convert.
		 * @return A tuple containing:
		 *         - An optional string representing the converted UTF-8 string, if successful.
		 *         - An optional string containing an error message, if an error occurred.
		 */
		static auto to_string(const std::u32string& utf32_string_message)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

		/**
		 * @brief Converts a UTF-8 string to a wide string.
		 * @param utf8_string_message The UTF-8 string to convert.
		 * @return A tuple containing:
		 *         - An optional wide string representing the converted wide string, if successful.
		 *         - An optional string containing an error message, if an error occurred.
		 */
		static auto to_wstring(const std::string& utf8_string_message)
			-> std::tuple<std::optional<std::wstring>, std::optional<std::string>>;

		/**
		 * @brief Converts a UTF-16 string to a wide string.
		 * @param utf16_string_message The UTF-16 string to convert.
		 * @return A tuple containing:
		 *         - An optional wide string representing the converted wide string, if successful.
		 *         - An optional string containing an error message, if an error occurred.
		 */
		static auto to_wstring(const std::u16string& utf16_string_message)
			-> std::tuple<std::optional<std::wstring>, std::optional<std::string>>;

		/**
		 * @brief Converts a UTF-32 string to a wide string.
		 * @param utf32_string_message The UTF-32 string to convert.
		 * @return A tuple containing:
		 *         - An optional wide string representing the converted wide string, if successful.
		 *         - An optional string containing an error message, if an error occurred.
		 */
		static auto to_wstring(const std::u32string& utf32_string_message)
			-> std::tuple<std::optional<std::wstring>, std::optional<std::string>>;

		/**
		 * @brief Converts a UTF-8 string to a UTF-16 string.
		 * @param utf8_string_message The UTF-8 string to convert.
		 * @return A tuple containing:
		 *         - An optional UTF-16 string representing the converted UTF-16 string, if
		 * successful.
		 *         - An optional string containing an error message, if an error occurred.
		 */
		static auto to_u16string(const std::string& utf8_string_message)
			-> std::tuple<std::optional<std::u16string>, std::optional<std::string>>;

		/**
		 * @brief Converts a wide string to a UTF-16 string.
		 * @param wide_string_message The wide string to convert.
		 * @return A tuple containing:
		 *         - An optional UTF-16 string representing the converted UTF-16 string, if
		 * successful.
		 *         - An optional string containing an error message, if an error occurred.
		 */
		static auto to_u16string(const std::wstring& wide_string_message)
			-> std::tuple<std::optional<std::u16string>, std::optional<std::string>>;

		/**
		 * @brief Converts a UTF-32 string to a UTF-16 string.
		 * @param utf32_string_message The UTF-32 string to convert.
		 * @return A tuple containing:
		 *         - An optional UTF-16 string representing the converted UTF-16 string, if
		 * successful.
		 *         - An optional string containing an error message, if an error occurred.
		 */
		static auto to_u16string(const std::u32string& utf32_string_message)
			-> std::tuple<std::optional<std::u16string>, std::optional<std::string>>;

		/**
		 * @brief Converts a UTF-8 string to a UTF-32 string.
		 * @param utf8_string_message The UTF-8 string to convert.
		 * @return A tuple containing:
		 *         - An optional UTF-32 string representing the converted UTF-32 string, if
		 * successful.
		 *         - An optional string containing an error message, if an error occurred.
		 */
		static auto to_u32string(const std::string& utf8_string_message)
			-> std::tuple<std::optional<std::u32string>, std::optional<std::string>>;

		/**
		 * @brief Converts a wide string to a UTF-32 string.
		 * @param wide_string_message The wide string to convert.
		 * @return A tuple containing:
		 *         - An optional UTF-32 string representing the converted UTF-32 string, if
		 * successful.
		 *         - An optional string containing an error message, if an error occurred.
		 */
		static auto to_u32string(const std::wstring& wide_string_message)
			-> std::tuple<std::optional<std::u32string>, std::optional<std::string>>;

		/**
		 * @brief Converts a UTF-16 string to a UTF-32 string.
		 * @param utf16_string_message The UTF-16 string to convert.
		 * @return A tuple containing:
		 *         - An optional UTF-32 string representing the converted UTF-32 string, if
		 * successful.
		 *         - An optional string containing an error message, if an error occurred.
		 */
		static auto to_u32string(const std::u16string& utf16_string_message)
			-> std::tuple<std::optional<std::u32string>, std::optional<std::string>>;

		/**
		 * @brief Converts a string to a byte array, handling UTF-8 BOM if present.
		 * @param utf8_string_value The string to convert.
		 * @return A tuple containing:
		 *         - An optional vector of bytes representing the string, including BOM if it was
		 * present in the input.
		 *         - An optional string containing an error message, if an error occurred.
		 */
		static auto to_array(const std::string& utf8_string_value)
			-> std::tuple<std::optional<std::vector<uint8_t>>, std::optional<std::string>>;

		/**
		 * @brief Converts a byte array to a string, handling UTF-8 BOM if present.
		 * @param byte_array_value The byte array to convert.
		 * @return A tuple containing:
		 *         - An optional string representing the converted string, with BOM removed if it
		 * was present in the input.
		 *         - An optional string containing an error message, if an error occurred.
		 */
		static auto to_string(const std::vector<uint8_t>& byte_array_value)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

	private:
		/**
		 * @brief Checks if the given byte array starts with a UTF-8 BOM.
		 * @param value The byte array to check.
		 * @return true if the array starts with a UTF-8 BOM, false otherwise.
		 */
		static auto has_utf8_bom(const std::vector<uint8_t>& value) -> bool;

		/**
		 * @brief Checks if the given string starts with a UTF-8 BOM.
		 * @param value The string to check.
		 * @return true if the string starts with a UTF-8 BOM, false otherwise.
		 */
		static auto has_utf8_bom(const std::string& value) -> bool;
	};
} // namespace utility_module