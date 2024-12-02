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

#include <cstdint>
#include <stdexcept>

namespace utility_module
{
	auto convert_string::to_string(const std::wstring& value)
		-> std::tuple<std::optional<std::string>, std::optional<std::string>>
	{
		auto [result, error]
			= convert<std::wstring, std::string>(value, get_wchar_encoding(endian_types::little),
												 get_encoding_name(encoding_types::utf8));

		if (result.has_value())
		{
			return utf8_to_system(result.value());
		}

		return { std::nullopt, error };
	}

	auto convert_string::to_string(std::wstring_view value)
		-> std::tuple<std::optional<std::string>, std::optional<std::string>>
	{
		auto [result, error] = convert<std::wstring_view, std::string>(
			value, get_wchar_encoding(endian_types::little),
			get_encoding_name(encoding_types::utf8));

		if (result.has_value())
		{
			return utf8_to_system(result.value());
		}

		return { std::nullopt, error };
	}

	auto convert_string::to_wstring(const std::string& value)
		-> std::tuple<std::optional<std::wstring>, std::optional<std::string>>
	{
		auto [result, error] = system_to_utf8(value);
		if (!result.has_value())
		{
			return { std::nullopt, error };
		}

		std::string clean_value = remove_utf8_bom(result.value());
		endian_types endian = endian_types::little;
		return convert<std::string, std::wstring>(
			clean_value, get_encoding_name(encoding_types::utf8), get_wchar_encoding(endian));
	}

	auto convert_string::to_wstring(std::string_view value)
		-> std::tuple<std::optional<std::wstring>, std::optional<std::string>>
	{
		auto [result, error] = system_to_utf8(std::string(value));
		if (!result.has_value())
		{
			return { std::nullopt, error };
		}

		std::string clean_value = remove_utf8_bom(result.value());
		endian_types endian = endian_types::little;
		return convert<std::string, std::wstring>(
			clean_value, get_encoding_name(encoding_types::utf8), get_wchar_encoding(endian));
	}

	auto convert_string::get_encoding_name(encoding_types encoding,
										   endian_types endian) -> std::string
	{
		switch (encoding)
		{
		case encoding_types::utf8:
			return "UTF-8";
		case encoding_types::utf16:
			if (endian == endian_types::little)
				return "UTF-16LE";
			else if (endian == endian_types::big)
				return "UTF-16BE";
			else
				return "UTF-16";
		case encoding_types::utf32:
			if (endian == endian_types::little)
				return "UTF-32LE";
			else if (endian == endian_types::big)
				return "UTF-32BE";
			else
				return "UTF-32";
		default:
			throw std::runtime_error("Unknown encoding");
		}
	}

	auto convert_string::get_wchar_encoding(endian_types endian) -> std::string
	{
		if (sizeof(wchar_t) == 2)
		{
			return get_encoding_name(encoding_types::utf16, endian);
		}
		else if (sizeof(wchar_t) == 4)
		{
			return get_encoding_name(encoding_types::utf32, endian);
		}
		else
		{
			throw std::runtime_error("Unsupported wchar_t size");
		}
	}

	auto convert_string::detect_endian(const std::u16string& str) -> endian_types
	{
		if (str.empty())
			return endian_types::unknown;

		if (str[0] == 0xFEFF)
			return endian_types::big;
		if (str[0] == 0xFFFE)
			return endian_types::little;

		size_t sample_size = std::min<size_t>(str.size(), 1000);
		int le_count = 0, be_count = 0;
		for (size_t i = 0; i < sample_size; ++i)
		{
			uint16_t ch = str[i];
			if ((ch & 0xFF00) == 0 && (ch & 0x00FF) != 0)
				++le_count;
			if ((ch & 0x00FF) == 0 && (ch & 0xFF00) != 0)
				++be_count;
		}

		if (le_count > be_count)
			return endian_types::little;
		if (be_count > le_count)
			return endian_types::big;

		return endian_types::unknown;
	}

	auto convert_string::detect_endian(const std::u32string& str) -> endian_types
	{
		if (str.empty())
			return endian_types::unknown;

		if (str[0] == 0x0000FEFF)
			return endian_types::big;
		if (str[0] == 0xFFFE0000)
			return endian_types::little;

		size_t sample_size = std::min<size_t>(str.size(), 1000);
		int le_count = 0, be_count = 0;
		for (size_t i = 0; i < sample_size; ++i)
		{
			uint32_t ch = str[i];
			if ((ch & 0xFFFFFF00) == 0 && (ch & 0x000000FF) != 0)
				++le_count;
			if ((ch & 0x00FFFFFF) == 0 && (ch & 0xFF000000) != 0)
				++be_count;
		}

		if (le_count > be_count)
			return endian_types::little;
		if (be_count > le_count)
			return endian_types::big;

		return endian_types::unknown;
	}

	auto convert_string::has_utf8_bom(const std::string& str) -> bool
	{
		return str.length() >= 3 && static_cast<unsigned char>(str[0]) == 0xEF
			   && static_cast<unsigned char>(str[1]) == 0xBB
			   && static_cast<unsigned char>(str[2]) == 0xBF;
	}

	auto convert_string::remove_utf8_bom(const std::string& str) -> std::string
	{
		return has_utf8_bom(str) ? str.substr(3) : str;
	}

	auto convert_string::add_utf8_bom(const std::string& str) -> std::string
	{
		return has_utf8_bom(str) ? str : std::string("\xEF\xBB\xBF") + str;
	}

	int convert_string::get_system_code_page()
	{
#ifdef _WIN32
		return GetACP();
#else
		return 65001;
#endif
	}

	std::string convert_string::get_code_page_name(int code_page)
	{
		switch (code_page)
		{
		case 65001:
			return "UTF-8";
		default:
			return "CP" + std::to_string(code_page);
		}
	}

	auto convert_string::system_to_utf8(const std::string& value)
		-> std::tuple<std::optional<std::string>, std::optional<std::string>>
	{
		int code_page = get_system_code_page();
		if (code_page == 65001)
		{
			return { value, std::nullopt };
		}
		return convert<std::string, std::string>(value, get_code_page_name(code_page),
												 get_encoding_name(encoding_types::utf8));
	}

	auto convert_string::utf8_to_system(const std::string& value)
		-> std::tuple<std::optional<std::string>, std::optional<std::string>>
	{
		int code_page = get_system_code_page();
		if (code_page == 65001)
		{
			return { value, std::nullopt };
		}
		return convert<std::string, std::string>(value, get_encoding_name(encoding_types::utf8),
												 get_code_page_name(code_page));
	}

	auto convert_string::split(const std::string& source, const std::string& token)
		-> std::tuple<std::optional<std::vector<std::string>>, std::optional<std::string>>
	{
		if (token.empty())
		{
			return { std::vector{ source }, std::nullopt };
		}

		std::vector<std::string> result;
		size_t start_pos = 0;
		size_t end_pos = source.find(token);

		while (end_pos != std::string::npos)
		{
			result.emplace_back(source.substr(start_pos, end_pos - start_pos));
			start_pos = end_pos + token.length();
			end_pos = source.find(token, start_pos);
		}

		result.emplace_back(source.substr(start_pos));

		return { result, std::nullopt };
	}

	auto convert_string::to_array(const std::string& value)
		-> std::tuple<std::optional<std::vector<uint8_t>>, std::optional<std::string>>
	{
		auto [utf8, convert_error] = system_to_utf8(value);
		if (convert_error.has_value())
		{
			return { std::nullopt, convert_error };
		}

		auto utf8_no_bom = remove_utf8_bom(utf8.value());

		return { std::vector<uint8_t>(utf8_no_bom.begin(), utf8_no_bom.end()), std::nullopt };
	}

	auto convert_string::to_string(const std::vector<uint8_t>& value)
		-> std::tuple<std::optional<std::string>, std::optional<std::string>>
	{
		std::string utf8(value.begin(), value.end());
		auto utf8_no_bom = remove_utf8_bom(utf8);

		return utf8_to_system(utf8);
	}
} // namespace utility_module