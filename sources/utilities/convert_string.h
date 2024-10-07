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

#include <string>
#include <tuple>
#include <optional>
#include <vector>
#include <stdexcept>
#include <iconv.h>
#include <cstring>
#include <errno.h>
#include <algorithm>

namespace utility_module
{
	/**
	 * @class convert_string
	 * @brief Provides utility functions for string conversion between different encodings.
	 */
	class convert_string
	{
	public:
		/**
		 * @brief Converts a std::wstring to a std::string.
		 * @param value The input std::wstring.
		 * @return A tuple containing the converted std::string or an error message.
		 */
		static auto to_string(const std::wstring& value)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

		/**
		 * @brief Converts a std::u16string to a std::string.
		 * @param value The input std::u16string.
		 * @return A tuple containing the converted std::string or an error message.
		 */
		static auto to_string(const std::u16string& value)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

		/**
		 * @brief Converts a std::u32string to a std::string.
		 * @param value The input std::u32string.
		 * @return A tuple containing the converted std::string or an error message.
		 */
		static auto to_string(const std::u32string& value)
			-> std::tuple<std::optional<std::string>, std::optional<std::string>>;

		/**
		 * @brief Converts a std::string to a std::wstring.
		 * @param value The input std::string.
		 * @return A tuple containing the converted std::wstring or an error message.
		 */
		static auto to_wstring(const std::string& value)
			-> std::tuple<std::optional<std::wstring>, std::optional<std::string>>;

		/**
		 * @brief Converts a std::u16string to a std::wstring.
		 * @param value The input std::u16string.
		 * @return A tuple containing the converted std::wstring or an error message.
		 */
		static auto to_wstring(const std::u16string& value)
			-> std::tuple<std::optional<std::wstring>, std::optional<std::string>>;

		/**
		 * @brief Converts a std::u32string to a std::wstring.
		 * @param value The input std::u32string.
		 * @return A tuple containing the converted std::wstring or an error message.
		 */
		static auto to_wstring(const std::u32string& value)
			-> std::tuple<std::optional<std::wstring>, std::optional<std::string>>;

		/**
		 * @brief Converts a std::string to a std::u16string.
		 * @param value The input std::string.
		 * @return A tuple containing the converted std::u16string or an error message.
		 */
		static auto to_u16string(const std::string& value)
			-> std::tuple<std::optional<std::u16string>, std::optional<std::string>>;

		/**
		 * @brief Converts a std::wstring to a std::u16string.
		 * @param value The input std::wstring.
		 * @return A tuple containing the converted std::u16string or an error message.
		 */
		static auto to_u16string(const std::wstring& value)
			-> std::tuple<std::optional<std::u16string>, std::optional<std::string>>;

		/**
		 * @brief Converts a std::u32string to a std::u16string.
		 * @param value The input std::u32string.
		 * @return A tuple containing the converted std::u16string or an error message.
		 */
		static auto to_u16string(const std::u32string& value)
			-> std::tuple<std::optional<std::u16string>, std::optional<std::string>>;

		/**
		 * @brief Converts a std::string to a std::u32string.
		 * @param value The input std::string.
		 * @return A tuple containing the converted std::u32string or an error message.
		 */
		static auto to_u32string(const std::string& value)
			-> std::tuple<std::optional<std::u32string>, std::optional<std::string>>;

		/**
		 * @brief Converts a std::wstring to a std::u32string.
		 * @param value The input std::wstring.
		 * @return A tuple containing the converted std::u32string or an error message.
		 */
		static auto to_u32string(const std::wstring& value)
			-> std::tuple<std::optional<std::u32string>, std::optional<std::string>>;

		/**
		 * @brief Converts a std::u16string to a std::u32string.
		 * @param value The input std::u16string.
		 * @return A tuple containing the converted std::u32string or an error message.
		 */
		static auto to_u32string(const std::u16string& value)
			-> std::tuple<std::optional<std::u32string>, std::optional<std::string>>;

		/**
		 * @brief Splits a string into a vector of strings based on a delimiter.
		 * @param source The input string to split.
		 * @param token The delimiter string.
		 * @return A tuple containing the vector of split strings or an error message.
		 */
		static auto split(const std::string& source, const std::string& token)
			-> std::tuple<std::optional<std::vector<std::string>>, std::optional<std::string>>;

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
		 * @brief Removes the UTF-16 BOM from a u16string if present.
		 * @param value The input u16string.
		 * @return The u16string without the BOM.
		 */
		static auto remove_utf16_bom(const std::u16string& value) -> std::u16string;

		/**
		 * @brief Adds a UTF-16 BOM to a u16string if not already present.
		 * @param value The input u16string.
		 * @param endian The desired endianness.
		 * @return The u16string with the BOM.
		 */
		static auto add_utf16_bom(const std::u16string& value,
								  endian_types endian = endian_types::little) -> std::u16string;

		/**
		 * @brief Removes the UTF-32 BOM from a u32string if present.
		 * @param value The input u32string.
		 * @return The u32string without the BOM.
		 */
		static auto remove_utf32_bom(const std::u32string& value) -> std::u32string;

		/**
		 * @brief Adds a UTF-32 BOM to a u32string if not already present.
		 * @param value The input u32string.
		 * @param endian The desired endianness.
		 * @return The u32string with the BOM.
		 */
		static auto add_utf32_bom(const std::u32string& value,
								  endian_types endian = endian_types::little) -> std::u32string;
	};
} // namespace utility_module
