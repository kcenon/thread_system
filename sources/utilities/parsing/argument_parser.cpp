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

#include "argument_parser.h"
#include "../conversion/convert_string.h"

#include <algorithm>
#include <cctype>
#include <charconv>

namespace utility_module
{
	auto argument_manager::to_lower(std::string_view str) -> std::string
	{
		std::string lower_str(str);
		std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
					   [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });
		return lower_str;
	}

	auto argument_manager::to_string(std::string_view key) const -> std::optional<std::string>
	{
		auto it = arguments_.find(std::string(key));
		return (it != arguments_.end()) ? std::optional<std::string>(it->second) : std::nullopt;
	}

	auto argument_manager::to_bool(std::string_view key) const -> std::optional<bool>
	{
		auto value = to_string(key);
		if (!value.has_value())
			return std::nullopt;

		auto val_lower = to_lower(value.value());
		if (val_lower == "true" || val_lower == "1")
			return true;
		if (val_lower == "false" || val_lower == "0")
			return false;

		return std::nullopt;
	}

	auto argument_manager::to_short(std::string_view key) const -> std::optional<short>
	{
		return to_numeric<short>(key);
	}

	auto argument_manager::to_ushort(std::string_view key) const -> std::optional<unsigned short>
	{
		return to_numeric<unsigned short>(key);
	}

	auto argument_manager::to_int(std::string_view key) const -> std::optional<int>
	{
		return to_numeric<int>(key);
	}

	auto argument_manager::to_uint(std::string_view key) const -> std::optional<unsigned int>
	{
		return to_numeric<unsigned int>(key);
	}

	auto argument_manager::to_int64(std::string_view key) const -> std::optional<int64_t>
	{
		return to_numeric<int64_t>(key);
	}

	auto argument_manager::to_uint64(std::string_view key) const -> std::optional<uint64_t>
	{
		return to_numeric<uint64_t>(key);
	}

	template <typename NumericType>
	auto argument_manager::to_numeric(std::string_view key) const -> std::optional<NumericType>
	{
		auto val = to_string(key);
		if (!val)
			return std::nullopt;

		const std::string& str_val = val.value();
		NumericType result{};
		auto [ptr, ec] = std::from_chars(str_val.data(), str_val.data() + str_val.size(), result);

		if (ec == std::errc::invalid_argument || ec == std::errc::result_out_of_range)
			return std::nullopt;
		if (ptr != str_val.data() + str_val.size())
			return std::nullopt; // Extra characters remain after conversion

		return result;
	}

	template <typename StringType>
	auto argument_manager::try_parse(const StringType& arguments) -> std::optional<std::string>
	{
		if (arguments.empty())
		{
			return "no valid arguments found.";
		}

		std::optional<std::string> converted;
		std::optional<std::string> convert_error;

		// Handle string conversion
		if constexpr (std::is_same_v<StringType, std::string>
					  || std::is_same_v<StringType, const char*>)
		{
			converted = std::string(arguments);
		}
		else if constexpr (std::is_same_v<StringType, std::wstring>
						   || std::is_same_v<StringType, const wchar_t*>)
		{
			std::tie(converted, convert_error) = convert_string::to_string(std::wstring(arguments));
			if (!converted)
			{
				return convert_error ? convert_error
									 : std::string("Failed to convert wide string to string");
			}
		}
		else
		{
			return "Unsupported string type";
		}

		auto argument_string = converted.value();

		// Check if string is only whitespace
		bool only_whitespace
			= std::all_of(argument_string.begin(), argument_string.end(),
						  [](unsigned char c) { return std::isspace(c) || c == '\0'; });
		if (only_whitespace)
		{
			return "no valid arguments found.";
		}

		// Remove null terminators if any
		auto null_pos = argument_string.find('\0');
		if (null_pos != std::string::npos)
		{
			argument_string = argument_string.substr(0, null_pos);
		}

		auto [splitted, split_error] = convert_string::split(argument_string, " ");
		if (!splitted)
		{
			return split_error;
		}

		auto splitted_vector = splitted.value();
		// Remove empty or whitespace-only strings
		splitted_vector.erase(
			std::remove_if(
				splitted_vector.begin(), splitted_vector.end(), [](const std::string& s)
				{ return s.empty() || s.find_first_not_of(" \t\n\r\0") == std::string::npos; }),
			splitted_vector.end());

		if (splitted_vector.empty())
		{
			return "no valid arguments found.";
		}

		auto [parsed, parse_error] = parse(splitted_vector);
		if (parse_error)
		{
			return parse_error;
		}

		arguments_ = parsed.value();
		return std::nullopt;
	}

	template <typename CharType>
	auto argument_manager::try_parse(int argc, CharType* argv[]) -> std::optional<std::string>
	{
		if (argc < 1)
		{
			return "Invalid argument count";
		}

		std::vector<std::string> args;
		bool found_valid = false;

		for (int i = 0; i < argc; ++i)
		{
			if (!argv[i])
			{
				return "Null argument pointer encountered";
			}

			std::string arg;
			if constexpr (std::is_same_v<CharType, wchar_t>)
			{
				auto [converted, error] = convert_string::to_string(std::wstring(argv[i]));
				if (error)
				{
					return error;
				}
				arg = converted.value();
			}
			else
			{
				arg = std::string(argv[i]);
			}

			if (!arg.empty() && arg.starts_with("--"))
			{
				found_valid = true;
			}
			args.push_back(std::move(arg));
		}

		if (!found_valid)
		{
			return "no valid arguments found.";
		}

		auto [parsed, error] = parse(args);
		if (error)
		{
			return error;
		}

		arguments_ = parsed.value();
		return std::nullopt;
	}

	auto argument_manager::parse(const std::vector<std::string>& arguments)
		-> std::tuple<std::optional<std::map<std::string, std::string>>, std::optional<std::string>>
	{
		if (arguments.empty())
		{
			return { std::nullopt, "no valid arguments found." };
		}

		// Validate first argument or adjust start index
		size_t start_index = 0;
		if (!arguments[0].starts_with("--"))
		{
			if (arguments.size() == 1)
			{
				return { std::nullopt, "invalid argument: " + arguments[0] };
			}
			if (!arguments[1].starts_with("--"))
			{
				return { std::nullopt, "invalid argument: " + arguments[1] };
			}
			start_index = 1;
		}

		std::map<std::string, std::string> result;
		bool found_valid = false;

		for (size_t i = start_index; i < arguments.size(); ++i)
		{
			const auto& arg = arguments[i];

			if (!arg.starts_with("--"))
			{
				return { std::nullopt, "invalid argument: " + arg };
			}

			if (arg == "--help")
			{
				// Help option: no value required
				result[arg] = "display help";
				found_valid = true;
				continue;
			}

			if (i + 1 >= arguments.size() || arguments[i + 1].starts_with("--"))
			{
				return { std::nullopt, "argument '" + arg + "' expects a value." };
			}

			result[arg] = arguments[i + 1];
			found_valid = true;
			++i;
		}

		if (!found_valid)
		{
			return { std::nullopt, "no valid arguments found." };
		}

		return { result, std::nullopt };
	}

	// Explicit template instantiations
	template auto argument_manager::try_parse<std::string>(const std::string&)
		-> std::optional<std::string>;
	template auto argument_manager::try_parse<std::wstring>(const std::wstring&)
		-> std::optional<std::string>;
	template auto argument_manager::try_parse<char>(int, char*[]) -> std::optional<std::string>;
	template auto argument_manager::try_parse<wchar_t>(int,
													   wchar_t*[]) -> std::optional<std::string>;

} // namespace utility_module

