/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this
   list of conditions and the following disclaimer in the documentation
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
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <errno.h>
#include <optional>
#include <algorithm>
#include <string_view>

namespace utility_module
{
	/**
	 * @class convert_string
	 * @brief Provides utilities for converting between various string encodings,
	 *        handling Base64, splitting, and replacing substrings.
	 *
	 * The class uses iconv (or Windows APIs on Windows) for encoding conversion,
	 * supports UTF-8, UTF-16, UTF-32, system encodings, and also handles Base64 encoding/decoding.
	 */
	class convert_string
	{
	public:
		/**
		 * @brief Convert a std::wstring to a std::string using system encoding.
		 * @param value The wide string input.
		 * @return A tuple with the converted string or an error message.
		 */
		static auto to_string(const std::wstring& value)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

		/**
		 * @brief Convert a std::wstring_view to a std::string using system encoding.
		 * @param value The wide string view input.
		 * @return A tuple with the converted string or an error message.
		 */
		static auto to_string(std::wstring_view value)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

		/**
		 * @brief Convert a std::string to a std::wstring using system encoding.
		 * @param value The multibyte string input.
		 * @return A tuple with the converted wide string or an error message.
		 */
		static auto to_wstring(const std::string& value)
			-> std::tuple<std::optional<std::wstring>, std::optional<std::string>>;

		/**
		 * @brief Convert a std::string_view to a std::wstring using system encoding.
		 * @param value The multibyte string view input.
		 * @return A tuple with the converted wide string or an error message.
		 */
		static auto to_wstring(std::string_view value)
			-> std::tuple<std::optional<std::wstring>, std::optional<std::string>>;

		/**
		 * @brief Retrieve the system code page.
		 * @return The system code page as an integer.
		 */
		static auto get_system_code_page() -> int;

		/**
		 * @brief Convert a system-encoded string to UTF-8.
		 * @param value The input string in system encoding.
		 * @return A tuple with the UTF-8 string or an error message.
		 */
		static auto system_to_utf8(const std::string& value)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

		/**
		 * @brief Convert a UTF-8 encoded string to system encoding.
		 * @param value The UTF-8 encoded input string.
		 * @return A tuple with the system-encoded string or an error message.
		 */
		static auto utf8_to_system(const std::string& value)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

		/**
		 * @brief Split a string by a delimiter token.
		 * @param source The source string.
		 * @param token The delimiter token.
		 * @return A tuple containing a vector of substrings or an error message.
		 */
		static auto split(const std::string& source, const std::string& token)
			-> std::tuple<std::optional<std::vector<std::string>>, std::optional<std::string>>;

		/**
		 * @brief Convert a system-encoded string to a byte array (UTF-8).
		 * @param value The input string in system encoding.
		 * @return A tuple with a byte array or an error message.
		 */
		static auto to_array(const std::string& value)
			-> std::tuple<std::optional<std::vector<uint8_t>>, std::optional<std::string>>;

		/**
		 * @brief Convert a byte array (UTF-8) to a system-encoded string.
		 * @param value The input byte array.
		 * @return A tuple with the system-encoded string or an error message.
		 */
		static auto to_string(const std::vector<uint8_t>& value)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

		/**
		 * @brief Encode a byte array into a Base64 string.
		 * @param value The input byte array.
		 * @return A tuple with the Base64 encoded string or an error message.
		 */
		static auto to_base64(const std::vector<uint8_t>& value)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

		/**
		 * @brief Decode a Base64 string into a byte array.
		 * @param base64_str The Base64 encoded string.
		 * @return A tuple with the decoded byte array and an optional error message.
		 */
		static auto from_base64(const std::string& base64_str)
			-> std::tuple<std::vector<uint8_t>, std::optional<std::string>>;

		/**
		 * @brief Replace all occurrences of a token within a string with another string, modifying
		 * in place.
		 * @param source The source string to modify.
		 * @param token The substring to find.
		 * @param target The substring to replace token with.
		 * @return std::optional<std::string> An error message if replacement fails, or std::nullopt
		 * on success.
		 */
		static auto replace(std::string& source,
							const std::string& token,
							const std::string& target) -> std::optional<std::string>;

		/**
		 * @brief Replace all occurrences of a token in a string with another string, returning a
		 * new string.
		 * @param source The source string.
		 * @param token The substring to find.
		 * @param target The substring to replace token with.
		 * @return A tuple with the replaced string or an error message.
		 */
		static auto replace2(const std::string& source,
							 const std::string& token,
							 const std::string& target)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

	private:
		enum class endian_types
		{
			little,
			big,
			unknown
		};

		enum class encoding_types
		{
			utf8,
			utf16,
			utf32
		};

		/**
		 * @brief Get the code page name string.
		 * @param code_page The code page number.
		 * @return A string representing the code page name.
		 */
		static auto get_code_page_name(int code_page) -> std::string;

		/**
		 * @brief Get the encoding name string based on encoding type and endianness.
		 * @param encoding The encoding type (e.g., utf8, utf16).
		 * @param endian The endianness.
		 * @return A string representing the encoding name.
		 */
		static auto get_encoding_name(encoding_types encoding,
									  endian_types endian = endian_types::little) -> std::string;

		/**
		 * @brief Determine wchar_t encoding name (UTF-16 or UTF-32) based on wchar_t size and
		 * endianness.
		 * @param endian The endianness.
		 * @return A string representing the wchar_t encoding.
		 */
		static auto get_wchar_encoding(endian_types endian = endian_types::little) -> std::string;

		/**
		 * @brief Convert a basic_string to a vector<char> for processing.
		 * @tparam T The character type of the input string.
		 * @param value The input string.
		 * @return A vector<char> containing the raw bytes of the string.
		 */
		template <typename T> static auto string_to_vector(const T& value) -> std::vector<char>
		{
			return std::vector<char>(reinterpret_cast<const char*>(value.data()),
									 reinterpret_cast<const char*>(value.data() + value.size()));
		}

		/**
		 * @brief Convert from one encoding to another using iconv.
		 * @tparam FromType The input string type.
		 * @tparam ToType The output string type.
		 * @param value The input string.
		 * @param from_encoding Source encoding name.
		 * @param to_encoding Target encoding name.
		 * @return A tuple with the converted string or an error message.
		 */
		template <typename FromType, typename ToType>
		static auto convert(const FromType& value,
							const std::string& from_encoding,
							const std::string& to_encoding)
			-> std::tuple<std::optional<ToType>, std::optional<std::string>>;

		/**
		 * @brief Detect endianness in a UTF-16 string.
		 * @param value The UTF-16 string.
		 * @return The detected endianness.
		 */
		static auto detect_endian(const std::u16string& value) -> endian_types;

		/**
		 * @brief Detect endianness in a UTF-32 string.
		 * @param value The UTF-32 string.
		 * @return The detected endianness.
		 */
		static auto detect_endian(const std::u32string& value) -> endian_types;

		/**
		 * @brief Check if a string has a UTF-8 BOM.
		 * @param value The input string.
		 * @return True if it has a UTF-8 BOM, otherwise false.
		 */
		static auto has_utf8_bom(const std::string& value) -> bool;

		/**
		 * @brief Remove a UTF-8 BOM from a string if present.
		 * @param value The input string.
		 * @return The string without the BOM.
		 */
		static auto remove_utf8_bom(const std::string& value) -> std::string;

		/**
		 * @brief Add a UTF-8 BOM to a string if not present.
		 * @param value The input string.
		 * @return The string with a UTF-8 BOM.
		 */
		static auto add_utf8_bom(const std::string& value) -> std::string;

		/**
		 * @brief Encode a byte array into a Base64 string.
		 * @param data The input byte array.
		 * @return The Base64-encoded string.
		 */
		static auto base64_encode(const std::vector<uint8_t>& data) -> std::string;

		/**
		 * @brief Decode a Base64 string into a byte array.
		 * @param base64_str The Base64 string.
		 * @return A tuple with the decoded byte array or an error message.
		 */
		static auto base64_decode(const std::string& base64_str)
			-> std::tuple<std::vector<uint8_t>, std::optional<std::string>>;
	};
} // namespace utility_module