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

#include <array>
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>

namespace utility_module
{
	/**
	 * @brief UTF-8 Byte Order Mark (BOM)
	 *
	 * This constant represents the UTF-8 Byte Order Mark, which is a sequence of bytes
	 * (0xEF, 0xBB, 0xBF) that may appear at the beginning of a UTF-8 encoded file or stream.
	 */
	constexpr std::array<uint8_t, 3> UTF8_BOM = { 0xEF, 0xBB, 0xBF };

	/**
	 * @struct ConversionOptions
	 * @brief Options for string conversion operations.
	 *
	 * This struct allows for configuration of various aspects of the string conversion process.
	 */
	struct ConversionOptions
	{
		bool remove_bom = true; ///< Whether to remove the Byte Order Mark during conversion
		// Add more options as needed
	};

	/**
	 * @class StringConverter
	 * @brief A utility class for converting between different string types.
	 *
	 * This template class provides a mechanism for converting between different string encodings.
	 * It uses the ConversionOptions to customize the conversion process.
	 *
	 * @tparam From The source string type.
	 * @tparam To The target string type.
	 */
	template <typename From, typename To> class StringConverter
	{
	private:
		std::basic_string_view<typename From::value_type> from;
		ConversionOptions options;

	public:
		/**
		 * @brief Constructs a StringConverter object.
		 * @param f The source string to convert.
		 * @param opts The conversion options to use.
		 */
		StringConverter(std::basic_string_view<typename From::value_type> f,
						const ConversionOptions& opts = ConversionOptions());

		/**
		 * @brief Performs the string conversion.
		 * @return The converted string.
		 */
		To convert();
	};

	/**
	 * @class convert_string
	 * @brief A utility class for string conversions between different character encodings.
	 *
	 * This class provides static methods to convert between std::string, std::wstring,
	 * std::u16string, and std::u32string. It also includes methods for converting
	 * between strings and byte arrays.
	 */
	class convert_string
	{
	public:
		/**
		 * @brief Converts a wide string to a UTF-8 string.
		 * @param message The wide string to convert.
		 * @return The converted UTF-8 string.
		 */
		static auto to_string(const std::wstring& message) -> std::string;

		/**
		 * @brief Converts a UTF-16 string to a UTF-8 string.
		 * @param message The UTF-16 string to convert.
		 * @return The converted UTF-8 string.
		 */
		static auto to_string(const std::u16string& message) -> std::string;

		/**
		 * @brief Converts a UTF-32 string to a UTF-8 string.
		 * @param message The UTF-32 string to convert.
		 * @return The converted UTF-8 string.
		 */
		static auto to_string(const std::u32string& message) -> std::string;

		/**
		 * @brief Converts a UTF-8 string to a wide string.
		 * @param message The UTF-8 string to convert.
		 * @return The converted wide string.
		 */
		static auto to_wstring(const std::string& message) -> std::wstring;

		/**
		 * @brief Converts a UTF-16 string to a wide string.
		 * @param message The UTF-16 string to convert.
		 * @return The converted wide string.
		 */
		static auto to_wstring(const std::u16string& message) -> std::wstring;

		/**
		 * @brief Converts a UTF-32 string to a wide string.
		 * @param message The UTF-32 string to convert.
		 * @return The converted wide string.
		 */
		static auto to_wstring(const std::u32string& message) -> std::wstring;

		/**
		 * @brief Converts a UTF-8 string to a UTF-16 string.
		 * @param message The UTF-8 string to convert.
		 * @return The converted UTF-16 string.
		 */
		static auto to_u16string(const std::string& message) -> std::u16string;

		/**
		 * @brief Converts a wide string to a UTF-16 string.
		 * @param message The wide string to convert.
		 * @return The converted UTF-16 string.
		 */
		static auto to_u16string(const std::wstring& message) -> std::u16string;

		/**
		 * @brief Converts a UTF-32 string to a UTF-16 string.
		 * @param message The UTF-32 string to convert.
		 * @return The converted UTF-16 string.
		 */
		static auto to_u16string(const std::u32string& message) -> std::u16string;

		/**
		 * @brief Converts a UTF-8 string to a UTF-32 string.
		 * @param message The UTF-8 string to convert.
		 * @return The converted UTF-32 string.
		 */
		static auto to_u32string(const std::string& message) -> std::u32string;

		/**
		 * @brief Converts a wide string to a UTF-32 string.
		 * @param message The wide string to convert.
		 * @return The converted UTF-32 string.
		 */
		static auto to_u32string(const std::wstring& message) -> std::u32string;

		/**
		 * @brief Converts a UTF-16 string to a UTF-32 string.
		 * @param message The UTF-16 string to convert.
		 * @return The converted UTF-32 string.
		 */
		static auto to_u32string(const std::u16string& message) -> std::u32string;

		/**
		 * @brief Converts a string to a byte array.
		 * @param value The string to convert.
		 * @return A vector of bytes representing the string.
		 */
		static auto to_array(const std::string& value) -> std::vector<uint8_t>;

		/**
		 * @brief Converts a byte array to a string.
		 * @param value The byte array to convert.
		 * @return The converted string.
		 */
		static auto to_string(const std::vector<uint8_t>& value) -> std::string;

	private:
		/**
		 * @brief Checks if a multi-byte sequence is valid UTF-8.
		 * @tparam N The number of bytes in the sequence.
		 * @param view The string view to check.
		 * @param index The starting index of the sequence.
		 * @return True if the sequence is valid, false otherwise.
		 */
		template <size_t N>
		static auto is_valid_multi_byte_sequence(std::string_view view,
												 const size_t& index) -> bool;

		/**
		 * @brief Checks if a string is valid UTF-8.
		 * @param value The string view to check.
		 * @param index The current index in the string.
		 * @return True if the string is valid UTF-8, false otherwise.
		 */
		static auto is_valid_utf8(std::string_view value, size_t& index) -> bool;
	};

	/**
	 * @brief Checks if a string starts with a UTF-8 BOM.
	 * @param value The string to check.
	 * @return True if the string starts with a UTF-8 BOM, false otherwise.
	 */
	bool has_utf8_bom(const std::string& value);

	/**
	 * @brief Gets the starting index after the UTF-8 BOM, if present.
	 * @param view The string view to check.
	 * @return The index after the BOM if present, or 0 if not.
	 */
	size_t get_utf8_start_index(std::string_view view);

} // namespace utility_module