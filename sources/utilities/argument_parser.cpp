/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2021, 🍀☀🌕🌥 🌊
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

#include "argument_parser.h"

#include "convert_string.h"

#include <algorithm>

namespace utility_module
{

	argument_manager::argument_manager() {}

	auto argument_manager::to_string(const std::string& key) -> std::optional<std::string>
	{
		auto it = arguments_.find(key);
		return it != arguments_.end() ? std::optional(it->second) : std::nullopt;
	}

	auto argument_manager::to_bool(const std::string& key) -> std::optional<bool>
	{
		auto value = to_string(key);
		if (!value.has_value())
		{
			return std::nullopt;
		}

		auto temp = value.value();
		std::transform(temp.begin(), temp.end(), temp.begin(), ::tolower);
		if (temp == "true" || temp == "1")
		{
			return true;
		}
		if (temp == "false" || temp == "0")
		{
			return false;
		}
		return std::nullopt;
	}

	auto argument_manager::to_short(const std::string& key) -> std::optional<short>
	{
		return to_numeric<short>(key);
	}

	auto argument_manager::to_ushort(const std::string& key) -> std::optional<unsigned short>
	{
		return to_numeric<unsigned short>(key);
	}

	auto argument_manager::to_int(const std::string& key) -> std::optional<int>
	{
		return to_numeric<int>(key);
	}

	auto argument_manager::to_uint(const std::string& key) -> std::optional<unsigned int>
	{
		return to_numeric<unsigned int>(key);
	}

#ifdef _WIN32
	auto argument_manager::to_llong(const std::string& key) -> std::optional<long long>
	{
		return to_numeric<long long>(key);
	}
#else
	auto argument_manager::to_long(const std::string& key) -> std::optional<long>
	{
		return to_numeric<long>(key);
	}
#endif

	template <typename NumericType>
	auto argument_manager::to_numeric(const std::string& key) -> std::optional<NumericType>
	{
		auto value = to_string(key);
		if (!value.has_value())
		{
			return std::nullopt;
		}

		try
		{
			if constexpr (std::is_same_v<NumericType, int>)
			{
				return std::stoi(value.value());
			}
			else if constexpr (std::is_same_v<NumericType, long>)
			{
				return std::stol(value.value());
			}
			else if constexpr (std::is_same_v<NumericType, long long>)
			{
				return std::stoll(value.value());
			}
			else if constexpr (std::is_same_v<NumericType, unsigned long>)
			{
				return std::stoul(value.value());
			}
			else if constexpr (std::is_same_v<NumericType, unsigned long long>)
			{
				return std::stoull(value.value());
			}
			else if constexpr (std::is_same_v<NumericType, short>)
			{
				return static_cast<short>(std::stoi(value.value()));
			}
			else if constexpr (std::is_same_v<NumericType, unsigned short>)
			{
				return static_cast<unsigned short>(std::stoul(value.value()));
			}
			else if constexpr (std::is_same_v<NumericType, unsigned int>)
			{
				return static_cast<unsigned int>(std::stoul(value.value()));
			}
		}
		catch (...)
		{
			return std::nullopt;
		}
		return std::nullopt;
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

		if constexpr (std::is_same_v<StringType, std::string>
					  || std::is_same_v<StringType, const char*>)
		{
			converted = std::string(arguments);
		}
		else if constexpr (std::is_same_v<StringType, std::wstring>
						   || std::is_same_v<StringType, const wchar_t*>)
		{
			std::tie(converted, convert_error) = convert_string::to_string(std::wstring(arguments));
		}
		else
		{
			return "Unsupported string type";
		}

		if (!converted.has_value())
		{
			return convert_error;
		}

		auto argument_string = converted.value();

		bool only_whitespace
			= std::all_of(argument_string.begin(), argument_string.end(),
						  [](unsigned char c) { return std::isspace(c) || c == '\0'; });
		if (only_whitespace)
		{
			return "no valid arguments found.";
		}

		auto null_pos = argument_string.find('\0');
		if (null_pos != std::string::npos)
		{
			argument_string = argument_string.substr(0, null_pos);
		}

		auto [splitted, split_error] = convert_string::split(argument_string, " ");
		if (split_error.has_value())
		{
			return split_error;
		}

		auto splitted_vector = splitted.value();
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
		if (parse_error.has_value())
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
				if (error.has_value())
				{
					return error;
				}
				arg = converted.value();
			}
			else
			{
				arg = std::string(argv[i]);
			}

			if (!arg.empty() && arg.substr(0, 2) == "--")
			{
				found_valid = true;
			}
			args.push_back(arg);
		}

		if (!found_valid)
		{
			return "no valid arguments found.";
		}

		auto [parsed, error] = parse(args);
		if (error.has_value())
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

		if (arguments[0].substr(0, 2) != "--")
		{
			if (arguments.size() == 1)
			{
				return { std::nullopt, "invalid argument: " + arguments[0] };
			}

			if (arguments[1].substr(0, 2) != "--")
			{
				return { std::nullopt, "invalid argument: " + arguments[1] };
			}
		}

		std::map<std::string, std::string> result;
		bool found_valid = false;

		size_t i = (arguments[0].substr(0, 2) != "--") ? 1 : 0;

		for (; i < arguments.size(); ++i)
		{
			const auto& arg = arguments[i];

			if (arg.substr(0, 2) != "--")
			{
				return { std::nullopt, "invalid argument: " + arg };
			}

			if (arg == "--help")
			{
				result[arg] = "display help";
				found_valid = true;
				continue;
			}

			if (i + 1 >= arguments.size())
			{
				return { std::nullopt, "argument '" + arg + "' expects a value." };
			}

			const auto& next_arg = arguments[i + 1];
			if (next_arg.substr(0, 2) == "--")
			{
				return { std::nullopt, "argument '" + arg + "' expects a value." };
			}

			result[arg] = next_arg;
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