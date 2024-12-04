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

#include "iconv.h"

#include <tuple>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <errno.h>
#include <optional>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

namespace utility_module
{
	/**
	 * @class convert_string
	 * @brief Comprehensive string conversion utility supporting multiple encodings and character
	 * types
	 *
	 * This class provides a robust set of static methods for:
	 * - Converting between different string types (std::string, std::wstring, std::u16string,
	 * std::u32string)
	 * - Handling system-specific encodings
	 * - Managing UTF-8, UTF-16, and UTF-32 encodings
	 * - Processing Base64 encoding/decoding
	 * - String manipulation (splitting, replacing)
	 *
	 * The class uses iconv for encoding conversions and provides comprehensive error handling
	 * through std::optional and detailed error messages.
	 */
	class convert_string
	{
	public:
		/**
		 * @brief Converts a wide string to a multibyte string
		 *
		 * @param value Input wide string to convert
		 * @return std::tuple<std::optional<std::string>, std::optional<std::string>>
		 *         First element: The converted string if successful, std::nullopt if failed
		 *         Second element: Error message if conversion failed, std::nullopt if successful
		 *
		 * @note Uses system default encoding for conversion
		 * @see get_system_code_page() for determining the system encoding
		 */
		static auto to_string(const std::wstring& value)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

		/**
		 * @brief Converts a wide string view to a multibyte string
		 *
		 * @param value Input wide string view to convert
		 * @return std::tuple<std::optional<std::string>, std::optional<std::string>>
		 *         First element: The converted string if successful, std::nullopt if failed
		 *         Second element: Error message if conversion failed, std::nullopt if successful
		 *
		 * @note More efficient than std::wstring version as it avoids string copying
		 */
		static auto to_string(std::wstring_view value)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

		/**
		 * @brief Converts a multibyte string to a wide string
		 *
		 * @param value Input multibyte string to convert
		 * @return std::tuple<std::optional<std::wstring>, std::optional<std::string>>
		 *         First element: The converted wide string if successful, std::nullopt if failed
		 *         Second element: Error message if conversion failed, std::nullopt if successful
		 *
		 * @note Uses system default encoding for conversion
		 */
		static auto to_wstring(const std::string& value)
			-> std::tuple<std::optional<std::wstring>, std::optional<std::string>>;

		/**
		 * @brief Converts a string view to a wide string
		 *
		 * @param value Input string view to convert
		 * @return std::tuple<std::optional<std::wstring>, std::optional<std::string>>
		 *         First element: The converted wide string if successful, std::nullopt if failed
		 *         Second element: Error message if conversion failed, std::nullopt if successful
		 *
		 * @note More efficient than std::string version as it avoids string copying
		 */
		static auto to_wstring(std::string_view value)
			-> std::tuple<std::optional<std::wstring>, std::optional<std::string>>;

		/**
		 * @brief Retrieves the current system code page
		 *
		 * @return int Current system code page number
		 *
		 * @note Windows-specific implementation returns CP_ACP
		 * @note On non-Windows platforms, typically returns value representing UTF-8
		 */
		static int get_system_code_page();

		/**
		 * @brief Converts a string from system encoding to UTF-8
		 *
		 * @param value Input string in system encoding
		 * @return std::tuple<std::optional<std::string>, std::optional<std::string>>
		 *         First element: UTF-8 encoded string if successful, std::nullopt if failed
		 *         Second element: Error message if conversion failed, std::nullopt if successful
		 */
		static auto system_to_utf8(const std::string& value)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

		/**
		 * @brief Converts a UTF-8 string to system encoding
		 *
		 * @param value Input UTF-8 encoded string
		 * @return std::tuple<std::optional<std::string>, std::optional<std::string>>
		 *         First element: System-encoded string if successful, std::nullopt if failed
		 *         Second element: Error message if conversion failed, std::nullopt if successful
		 */
		static auto utf8_to_system(const std::string& value)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

		/**
		 * @brief Splits a string into multiple substrings based on a delimiter
		 *
		 * @param source String to split
		 * @param token Delimiter string
		 * @return std::tuple<std::optional<std::vector<std::string>>, std::optional<std::string>>
		 *         First element: Vector of split strings if successful, std::nullopt if failed
		 *         Second element: Error message if splitting failed, std::nullopt if successful
		 *
		 * @note Empty delimiters result in single-element vector containing the original string
		 * @note Consecutive delimiters produce empty strings in the result
		 */
		static auto split(const std::string& source, const std::string& token)
			-> std::tuple<std::optional<std::vector<std::string>>, std::optional<std::string>>;

		/**
		 * @brief Converts a string to a byte array
		 *
		 * @param value Input string to convert
		 * @return std::tuple<std::optional<std::vector<uint8_t>>, std::optional<std::string>>
		 *         First element: Byte array if successful, std::nullopt if failed
		 *         Second element: Error message if conversion failed, std::nullopt if successful
		 */
		static auto to_array(const std::string& value)
			-> std::tuple<std::optional<std::vector<uint8_t>>, std::optional<std::string>>;

		/**
		 * @brief Converts a byte array to a string
		 *
		 * @param value Input byte array to convert
		 * @return std::tuple<std::optional<std::string>, std::optional<std::string>>
		 *         First element: Converted string if successful, std::nullopt if failed
		 *         Second element: Error message if conversion failed, std::nullopt if successful
		 */
		static auto to_string(const std::vector<uint8_t>& value)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

		/**
		 * @brief Encodes a byte array to Base64 string
		 *
		 * @param value Input byte array to encode
		 * @return std::tuple<std::optional<std::string>, std::optional<std::string>>
		 *         First element: Base64 encoded string if successful, std::nullopt if failed
		 *         Second element: Error message if encoding failed, std::nullopt if successful
		 */
		static auto to_base64(const std::vector<uint8_t>& value)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

		/**
		 * @brief Decodes a Base64 string to byte array
		 *
		 * @param base64_str Input Base64 encoded string
		 * @return std::tuple<std::optional<std::vector<uint8_t>>, std::optional<std::string>>
		 *         First element: Decoded byte array if successful, std::nullopt if failed
		 *         Second element: Error message if decoding failed, std::nullopt if successful
		 *
		 * @note Validates input string format before decoding
		 */
		static auto from_base64(const std::string& base64_str)
			-> std::tuple<std::optional<std::vector<uint8_t>>, std::optional<std::string>>;

		/**
		 * @brief Replaces all occurrences of a substring with another string
		 *
		 * @param source String to perform replacements in (modified in-place)
		 * @param token Substring to replace
		 * @param target Replacement string
		 * @return std::optional<std::string> Error message if replacement failed, std::nullopt if
		 * successful
		 *
		 * @note Modifies the input string directly
		 */
		static auto replace(std::string& source,
							const std::string& token,
							const std::string& target) -> std::optional<std::string>;

		/**
		 * @brief Replaces all occurrences of a substring with another string (non-modifying
		 * version)
		 *
		 * @param source Input string to perform replacements on
		 * @param token Substring to replace
		 * @param target Replacement string
		 * @return std::tuple<std::optional<std::string>, std::optional<std::string>>
		 *         First element: Result string with replacements if successful, std::nullopt if
		 * failed Second element: Error message if replacement failed, std::nullopt if successful
		 *
		 * @note Creates a new string instead of modifying the input
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
		 * @brief Returns the name of the code page.
		 * @param code_page The code page number.
		 * @return The name of the code page.
		 */
		static std::string get_code_page_name(int code_page);

		/**
		 * @brief Gets the encoding name based on the encoding type and endianness.
		 * @param encoding The encoding type.
		 * @param endian The endianness.
		 * @return The encoding name as a string.
		 */
		static auto get_encoding_name(encoding_types encoding,
									  endian_types endian = endian_types::little) -> std::string;

		/**
		 * @brief Determines the encoding of wchar_t based on its size.
		 * @param endian The endianness.
		 * @return The encoding name for wchar_t.
		 */
		static auto get_wchar_encoding(endian_types endian = endian_types::little) -> std::string;

		/**
		 * @brief Converts a basic_string to a vector of bytes.
		 * @tparam T The character type of the input string.
		 * @param value The input string.
		 * @return A vector of bytes representing the input string.
		 */
		template <typename T> static auto string_to_vector(const T& value) -> std::vector<char>
		{
			return std::vector<char>(reinterpret_cast<const char*>(value.data()),
									 reinterpret_cast<const char*>(value.data() + value.size()));
		}

		/**
		 * @brief Converts a string from one encoding to another.
		 * @tparam FromType The input string type.
		 * @tparam ToType The output string type.
		 * @param value The input string.
		 * @param from_encoding The encoding of the input string.
		 * @param to_encoding The desired encoding of the output string.
		 * @return A tuple containing the converted string or an error message.
		 */
		template <typename FromType, typename ToType>
		static auto convert(const FromType& value,
							const std::string& from_encoding,
							const std::string& to_encoding)
			-> std::tuple<std::optional<ToType>, std::optional<std::string>>
		{
			iconv_t cd = iconv_open(to_encoding.c_str(), from_encoding.c_str());
			if (cd == (iconv_t)-1)
			{
				return { std::nullopt, "iconv_open failed: " + std::string(strerror(errno)) };
			}

			std::vector<char> in_buf = string_to_vector(value);
			char* in_ptr = in_buf.data();
			size_t in_bytes_left = in_buf.size();

			size_t out_buf_size = in_bytes_left * 2;
			std::vector<char> out_buf(out_buf_size);
			char* out_ptr = out_buf.data();
			size_t out_bytes_left = out_buf.size();

			size_t result = iconv(cd, &in_ptr, &in_bytes_left, &out_ptr, &out_bytes_left);
			while (result == (size_t)-1 && errno == E2BIG)
			{
				size_t used = out_buf_size - out_bytes_left;
				out_buf_size *= 2;
				out_buf.resize(out_buf_size);
				out_ptr = out_buf.data() + used;
				out_bytes_left = out_buf_size - used;
				result = iconv(cd, &in_ptr, &in_bytes_left, &out_ptr, &out_bytes_left);
			}

			if (result == (size_t)-1)
			{
				std::string error_msg = "iconv failed: " + std::string(strerror(errno));
				iconv_close(cd);
				return { std::nullopt, error_msg };
			}

			iconv_close(cd);

			size_t converted_size = out_buf_size - out_bytes_left;
			ToType converted_string(reinterpret_cast<typename ToType::value_type*>(out_buf.data()),
									converted_size / sizeof(typename ToType::value_type));

			return { converted_string, std::nullopt };
		}

		/**
		 * @brief Detects the endianness of a u16string.
		 * @param value The input u16string.
		 * @return The detected endianness.
		 */
		static auto detect_endian(const std::u16string& value) -> endian_types;

		/**
		 * @brief Detects the endianness of a u32string.
		 * @param value The input u32string.
		 * @return The detected endianness.
		 */
		static auto detect_endian(const std::u32string& value) -> endian_types;

		/**
		 * @brief Checks if a string has a UTF-8 BOM.
		 * @param value The input string.
		 * @return True if the string has a UTF-8 BOM, false otherwise.
		 */
		static auto has_utf8_bom(const std::string& value) -> bool;

		/**
		 * @brief Removes the UTF-8 BOM from a string if present.
		 * @param value The input string.
		 * @return The string without the UTF-8 BOM.
		 */
		static auto remove_utf8_bom(const std::string& value) -> std::string;

		/**
		 * @brief Adds a UTF-8 BOM to a string if not already present.
		 * @param value The input string.
		 * @return The string with the UTF-8 BOM.
		 */
		static auto add_utf8_bom(const std::string& value) -> std::string;

		/**
		 * @brief Encodes a vector of bytes to a base64 string.
		 * @param data The input vector of bytes.
		 * @return The base64-encoded string.
		 */
		static auto base64_encode(const std::vector<uint8_t>& data) -> std::string;

		/**
		 * @brief Decodes a base64 string to a vector of bytes.
		 * @param base64_str The input base64 string.
		 * @return A tuple containing the decoded vector of bytes or an error message.
		 */
		static auto base64_decode(const std::string& base64_str)
			-> std::tuple<std::optional<std::vector<uint8_t>>, std::optional<std::string>>;
	};
} // namespace utility_module