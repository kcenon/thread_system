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

#include "convert_string.h"

#include <stdexcept>

#include "unicode/unistr.h"
#include "unicode/ustream.h"
#include "unicode/ucnv.h"

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#endif

namespace utility_module
{
	auto convert_string::split(const std::string& source, const std::string& token)
		-> std::tuple<std::optional<std::vector<std::string>>, std::optional<std::string>>
	{
		try
		{
			std::vector<std::string> result;

			size_t start = 0;
			while (true)
			{
				size_t end = source.find(token, start);
				if (end == std::string::npos)
				{
					result.push_back(source.substr(start));
					break;
				}

				result.push_back(source.substr(start, end - start));
				start = end + token.length();
			}

			return { std::move(result), std::nullopt };
		}
		catch (const std::exception& e)
		{
			return { std::nullopt,
#ifdef USE_STD_FORMAT
					 std::format
#else
					 fmt::format
#endif
					 ("Error splitting string: {}", e.what()) };
		}
	}

	auto convert_string::to_string(const std::wstring& wide_string_message)
		-> std::tuple<std::optional<std::string>, std::optional<std::string>>
	{
		try
		{
			icu::UnicodeString unicode_string(
				reinterpret_cast<const UChar*>(wide_string_message.data()),
				static_cast<int32_t>(wide_string_message.length()));

			std::string result;
			unicode_string.toUTF8String(result);

			return { std::move(result), std::nullopt };
		}
		catch (const std::exception& e)
		{
			return { std::nullopt,
#ifdef USE_STD_FORMAT
					 std::format
#else
					 fmt::format
#endif
					 ("Error converting wstring to string: {}", e.what()) };
		}
	}

	auto convert_string::to_string(const std::u16string& utf16_string_message)
		-> std::tuple<std::optional<std::string>, std::optional<std::string>>
	{
		try
		{
			icu::UnicodeString unicode_string(
				reinterpret_cast<const UChar*>(utf16_string_message.data()),
				static_cast<int32_t>(utf16_string_message.length()));

			std::string result;
			unicode_string.toUTF8String(result);

			return { std::move(result), std::nullopt };
		}
		catch (const std::exception& e)
		{
			return { std::nullopt,
#ifdef USE_STD_FORMAT
					 std::format
#else
					 fmt::format
#endif
					 ("Error converting u16string to string: {}", e.what()) };
		}
	}

	auto convert_string::to_string(const std::u32string& utf32_string_message)
		-> std::tuple<std::optional<std::string>, std::optional<std::string>>
	{
		try
		{
			icu::UnicodeString unicode_string = icu::UnicodeString::fromUTF32(
				reinterpret_cast<const UChar32*>(utf32_string_message.data()),
				static_cast<int32_t>(utf32_string_message.length()));

			std::string result;
			unicode_string.toUTF8String(result);

			return { std::move(result), std::nullopt };
		}
		catch (const std::exception& e)
		{
			return { std::nullopt,
#ifdef USE_STD_FORMAT
					 std::format
#else
					 fmt::format
#endif
					 ("Error converting u32string to string: {}", e.what()) };
		}
	}

	auto convert_string::to_wstring(const std::string& utf8_string_message)
		-> std::tuple<std::optional<std::wstring>, std::optional<std::string>>
	{
		try
		{
			icu::UnicodeString unicode_string = icu::UnicodeString::fromUTF8(utf8_string_message);

			std::wstring result(unicode_string.length(), L'\0');
			unicode_string.extract(0, unicode_string.length(),
								   reinterpret_cast<UChar*>(&result[0]));

			return { std::move(result), std::nullopt };
		}
		catch (const std::exception& e)
		{
			return { std::nullopt,
#ifdef USE_STD_FORMAT
					 std::format
#else
					 fmt::format
#endif
					 ("Error converting string to wstring: {}", e.what()) };
		}
	}

	auto convert_string::to_wstring(const std::u16string& utf16_string_message)
		-> std::tuple<std::optional<std::wstring>, std::optional<std::string>>
	{
		try
		{
			icu::UnicodeString unicode_string(
				reinterpret_cast<const UChar*>(utf16_string_message.data()),
				static_cast<int32_t>(utf16_string_message.length()));

			std::wstring result(unicode_string.length(), L'\0');
			unicode_string.extract(0, unicode_string.length(),
								   reinterpret_cast<UChar*>(&result[0]));

			return { std::move(result), std::nullopt };
		}
		catch (const std::exception& e)
		{
			return { std::nullopt,
#ifdef USE_STD_FORMAT
					 std::format
#else
					 fmt::format
#endif
					 ("Error converting u16string to wstring: {}", e.what()) };
		}
	}

	auto convert_string::to_wstring(const std::u32string& utf32_string_message)
		-> std::tuple<std::optional<std::wstring>, std::optional<std::string>>
	{
		try
		{
			icu::UnicodeString unicode_string = icu::UnicodeString::fromUTF32(
				reinterpret_cast<const UChar32*>(utf32_string_message.data()),
				static_cast<int32_t>(utf32_string_message.length()));

			std::wstring result(unicode_string.length(), L'\0');
			unicode_string.extract(0, unicode_string.length(),
								   reinterpret_cast<UChar*>(&result[0]));

			return { std::move(result), std::nullopt };
		}
		catch (const std::exception& e)
		{
			return { std::nullopt,
#ifdef USE_STD_FORMAT
					 std::format
#else
					 fmt::format
#endif
					 ("Error converting u32string to wstring: {}", e.what()) };
		}
	}

	auto convert_string::to_u16string(const std::string& utf8_string_message)
		-> std::tuple<std::optional<std::u16string>, std::optional<std::string>>
	{
		try
		{
			icu::UnicodeString unicode_string = icu::UnicodeString::fromUTF8(utf8_string_message);

			std::u16string result(unicode_string.length(), u'\0');
			unicode_string.extract(0, unicode_string.length(),
								   reinterpret_cast<UChar*>(&result[0]));

			return { std::move(result), std::nullopt };
		}
		catch (const std::exception& e)
		{
			return { std::nullopt,
#ifdef USE_STD_FORMAT
					 std::format
#else
					 fmt::format
#endif
					 ("Error converting string to u16string: {}", e.what()) };
		}
	}

	auto convert_string::to_u16string(const std::wstring& wide_string_message)
		-> std::tuple<std::optional<std::u16string>, std::optional<std::string>>
	{
		try
		{
			icu::UnicodeString unicode_string(
				reinterpret_cast<const UChar*>(wide_string_message.data()),
				static_cast<int32_t>(wide_string_message.length()));

			std::u16string result(unicode_string.length(), u'\0');
			unicode_string.extract(0, unicode_string.length(),
								   reinterpret_cast<UChar*>(&result[0]));

			return { std::move(result), std::nullopt };
		}
		catch (const std::exception& e)
		{
			return { std::nullopt,
#ifdef USE_STD_FORMAT
					 std::format
#else
					 fmt::format
#endif
					 ("Error converting wstring to u16string: {}", e.what()) };
		}
	}

	auto convert_string::to_u16string(const std::u32string& utf32_string_message)
		-> std::tuple<std::optional<std::u16string>, std::optional<std::string>>
	{
		try
		{
			icu::UnicodeString unicode_string = icu::UnicodeString::fromUTF32(
				reinterpret_cast<const UChar32*>(utf32_string_message.data()),
				static_cast<int32_t>(utf32_string_message.length()));

			std::u16string result(unicode_string.length(), u'\0');
			unicode_string.extract(0, unicode_string.length(),
								   reinterpret_cast<UChar*>(&result[0]));

			return { std::move(result), std::nullopt };
		}
		catch (const std::exception& e)
		{
			return { std::nullopt,
#ifdef USE_STD_FORMAT
					 std::format
#else
					 fmt::format
#endif
					 ("Error converting u32string to u16string: {}", e.what()) };
		}
	}

	auto convert_string::to_u32string(const std::string& utf8_string_message)
		-> std::tuple<std::optional<std::u32string>, std::optional<std::string>>
	{
		try
		{
			icu::UnicodeString unicode_string = icu::UnicodeString::fromUTF8(utf8_string_message);

			std::u32string result(unicode_string.length(), U'\0');
			UErrorCode error_code = U_ZERO_ERROR;
			unicode_string.toUTF32(reinterpret_cast<UChar32*>(&result[0]),
								   static_cast<int32_t>(result.capacity()), error_code);

			if (U_FAILURE(error_code))
			{
				return { std::nullopt,
#ifdef USE_STD_FORMAT
						 std::format
#else
						 fmt::format
#endif
						 ("Error converting string to u32string: {}", u_errorName(error_code)) };
			}
			return { std::move(result), std::nullopt };
		}
		catch (const std::exception& e)
		{
			return { std::nullopt,
#ifdef USE_STD_FORMAT
					 std::format
#else
					 fmt::format
#endif
					 ("Error converting string to u32string: {}", e.what()) };
		}
	}

	auto convert_string::to_u32string(const std::wstring& wide_string_message)
		-> std::tuple<std::optional<std::u32string>, std::optional<std::string>>
	{
		try
		{
			icu::UnicodeString unicode_string(
				reinterpret_cast<const UChar*>(wide_string_message.data()),
				static_cast<int32_t>(wide_string_message.length()));

			std::u32string result(unicode_string.length(), U'\0');
			UErrorCode error_code = U_ZERO_ERROR;
			unicode_string.toUTF32(reinterpret_cast<UChar32*>(&result[0]),
								   static_cast<int32_t>(result.capacity()), error_code);

			if (U_FAILURE(error_code))
			{
				return { std::nullopt,
#ifdef USE_STD_FORMAT
						 std::format
#else
						 fmt::format
#endif
						 ("Error converting wstring to u32string: {}", u_errorName(error_code)) };
			}

			return { std::move(result), std::nullopt };
		}
		catch (const std::exception& e)
		{
			return { std::nullopt,
#ifdef USE_STD_FORMAT
					 std::format
#else
					 fmt::format
#endif
					 ("Error converting wstring to u32string: {}", e.what()) };
		}
	}

	auto convert_string::to_u32string(const std::u16string& utf16_string_message)
		-> std::tuple<std::optional<std::u32string>, std::optional<std::string>>
	{
		try
		{
			icu::UnicodeString unicode_string(
				reinterpret_cast<const UChar*>(utf16_string_message.data()),
				static_cast<int32_t>(utf16_string_message.length()));

			std::u32string result(unicode_string.length(), U'\0');
			UErrorCode error_code = U_ZERO_ERROR;
			unicode_string.toUTF32(reinterpret_cast<UChar32*>(&result[0]),
								   static_cast<int32_t>(result.capacity()), error_code);

			if (U_FAILURE(error_code))
			{
				return { std::nullopt,
#ifdef USE_STD_FORMAT
						 std::format
#else
						 fmt::format
#endif
						 ("Error converting u16string to u32string: {}", u_errorName(error_code)) };
			}

			return { std::move(result), std::nullopt };
		}
		catch (const std::exception& e)
		{
			return { std::nullopt,
#ifdef USE_STD_FORMAT
					 std::format
#else
					 fmt::format
#endif
					 ("Error converting u16string to u32string: {}", e.what()) };
		}
	}

	auto convert_string::to_array(const std::string& utf8_string_value)
		-> std::tuple<std::optional<std::vector<uint8_t>>, std::optional<std::string>>
	{
		try
		{
			std::vector<uint8_t> result;
			if (has_utf8_bom(utf8_string_value))
			{
				result.insert(result.end(), utf8_string_value.begin(),
							  utf8_string_value.begin() + 3);
				result.insert(result.end(), utf8_string_value.begin() + 3, utf8_string_value.end());
			}
			else
			{
				result.insert(result.end(), utf8_string_value.begin(), utf8_string_value.end());
			}

			return { std::move(result), std::nullopt };
		}
		catch (const std::exception& e)
		{
			return { std::nullopt,
#ifdef USE_STD_FORMAT
					 std::format
#else
					 fmt::format
#endif
					 ("Error converting string to byte array: {}", e.what()) };
		}
	}

	auto convert_string::to_string(const std::vector<uint8_t>& byte_array_value)
		-> std::tuple<std::optional<std::string>, std::optional<std::string>>
	{
		try
		{
			std::string result;
			auto start = has_utf8_bom(byte_array_value) ? byte_array_value.begin() + 3
														: byte_array_value.begin();
			result.insert(result.end(), start, byte_array_value.end());

			return { std::move(result), std::nullopt };
		}
		catch (const std::exception& e)
		{
			return { std::nullopt,
#ifdef USE_STD_FORMAT
					 std::format
#else
					 fmt::format
#endif
					 ("Error converting byte array to string: {}", e.what()) };
		}
	}

	auto convert_string::has_utf8_bom(const std::vector<uint8_t>& value) -> bool
	{
		constexpr std::array<uint8_t, 3> utf8_bom = { 0xEF, 0xBB, 0xBF };
		return value.size() >= 3 && std::equal(utf8_bom.begin(), utf8_bom.end(), value.begin());
	}

	auto convert_string::has_utf8_bom(const std::string& value) -> bool
	{
		constexpr std::array<char, 3> utf8_bom = { '\xEF', '\xBB', '\xBF' };
		return value.size() >= 3 && std::equal(utf8_bom.begin(), utf8_bom.end(), value.begin());
	}
} // namespace utility_module