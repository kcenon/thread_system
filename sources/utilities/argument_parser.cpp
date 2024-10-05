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

#include <charconv>
#include <algorithm>

namespace utility_module
{
	argument_manager::argument_manager(void) {}

	auto argument_manager::try_parse(const std::string& arguments)
		-> std::tuple<bool, std::optional<std::string>>
	{
		auto [splitted, message] = convert_string::split(arguments, " ");
		if (!splitted.has_value())
		{
			return { false, message };
		}

		auto [parsed, parse_error] = parse(splitted.value());

		if (!parsed.has_value())
		{
			return { false, parse_error };
		}

		_arguments = parsed.value();

		return { true, std::nullopt };
	}

#ifdef _WIN32_BUT_NOT_TESTED
	auto argument_manager::try_parse(const std::wstring& arguments)
		-> std::tuple<bool, std::optional<std::string>>
	{
		auto [converted, convert_error] = convert_string::to_string(arguments);
		if (!converted.has_value())
		{
			return { false, convert_error };
		}

		auto [splitted, message] = convert_string::split(converted.value(), " ");
		if (!splitted.has_value())
		{
			return { false, message };
		}

		auto [parsed, parse_error] = parse(splitted.value());

		if (!parsed.has_value())
		{
			return { false, parse_error };
		}

		_arguments = parsed.value();

		return { true, std::nullopt };
	}
#endif

	auto argument_manager::try_parse(int argc,
									 char* argv[]) -> std::tuple<bool, std::optional<std::string>>
	{
		auto [parsed, parse_error] = parse(argc, argv);

		if (!parsed.has_value())
		{
			return { false, parse_error };
		}

		_arguments = parsed.value();

		return { true, std::nullopt };
	}

#ifdef _WIN32_BUT_NOT_TESTED
	auto argument_manager::try_parse(int argc, wchar_t* argv[])
		-> std::tuple<bool, std::optional<std::string>>
	{
		auto [parsed, parse_error] = parse(argc, argv);

		if (!parsed.has_value())
		{
			return { false, parse_error };
		}

		_arguments = parsed.value();

		return { true, std::nullopt };
	}
#endif

	auto argument_manager::to_string(const std::string& key) -> std::optional<std::string>
	{
		auto target = _arguments.find(key);
		if (target == _arguments.end())
		{
			return std::nullopt;
		}

		return target->second;
	}

	auto argument_manager::to_bool(const std::string& key) -> std::optional<bool>
	{
		auto target = to_string(key);
		if (!target.has_value())
		{
			return std::nullopt;
		}

		auto temp = target.value();
		transform(temp.begin(), temp.end(), temp.begin(), ::tolower);

		return temp.compare("true") == 0;
	}

	auto argument_manager::to_short(const std::string& key) -> std::optional<short>
	{
		auto target = to_string(key);
		if (!target.has_value())
		{
			return std::nullopt;
		}

		return to_numeric<short>(target.value());
	}

	auto argument_manager::to_ushort(const std::string& key) -> std::optional<unsigned short>
	{
		auto target = to_string(key);
		if (!target.has_value())
		{
			return std::nullopt;
		}

		return to_numeric<unsigned short>(target.value());
	}

	auto argument_manager::to_int(const std::string& key) -> std::optional<int>
	{
		auto target = to_string(key);
		if (target == std::nullopt)
		{
			return std::nullopt;
		}

		return to_numeric<int>(target.value());
	}

	auto argument_manager::to_uint(const std::string& key) -> std::optional<unsigned int>
	{
		auto target = to_string(key);
		if (!target.has_value())
		{
			return std::nullopt;
		}

		return to_numeric<unsigned int>(target.value());
	}

#ifdef _WIN32_BUT_NOT_TESTED
	auto argument_manager::to_llong(const std::string& key) -> std::optional<long long>
#else
	auto argument_manager::to_long(const std::string& key) -> std::optional<long>
#endif
	{
		auto target = to_string(key);
		if (!target.has_value())
		{
			return std::nullopt;
		}

#ifdef _WIN32_BUT_NOT_TESTED
		return to_numeric<long long>(target.value());
#else
		return to_numeric<long>(target.value());
#endif
	}

	auto argument_manager::parse(int argc, char* argv[])
		-> std::tuple<std::optional<std::map<std::string, std::string>>, std::optional<std::string>>
	{
		std::vector<std::string> arguments;
		for (int index = 1; index < argc; ++index)
		{
			arguments.push_back(argv[index]);
		}

		return parse(arguments);
	}

#ifdef _WIN32_BUT_NOT_TESTED
	auto argument_manager::parse(int argc, wchar_t* argv[])
		-> std::tuple<std::optional<std::map<std::string, std::string>>, std::optional<std::string>>
	{
		std::vector<std::string> arguments;
		for (int index = 1; index < argc; ++index)
		{
			auto [converted, convert_error] = convert_string::to_string(std::wstring(argv[index]));
			if (!converted.has_value())
			{
				continue;
			}

			arguments.push_back(converted.value());
		}

		return parse(arguments);
	}
#endif

	auto argument_manager::parse(const std::vector<std::string>& arguments)
		-> std::tuple<std::optional<std::map<std::string, std::string>>, std::optional<std::string>>
	{
		std::map<std::string, std::string> result;

		size_t argc = arguments.size();
		std::string argument_id;
		bool found_valid_argument = false;

		for (size_t index = 0; index < argc; ++index)
		{
			argument_id = arguments[index];
			if (argument_id.empty())
			{
				return { std::nullopt, "empty argument" };
			}

			if (argument_id.compare("--help") == 0)
			{
				result.insert({ argument_id, "display help" });
				found_valid_argument = true;
				continue;
			}

			size_t offset = argument_id.find("--", 0);
			if (offset != 0)
			{
				return { std::nullopt, "invalid argument: " + argument_id };
			}

			if (index + 1 >= argc)
			{
				return { std::nullopt, "argument '" + argument_id + "' expects a value." };
			}

			result.insert({ argument_id, arguments[index + 1] });
			found_valid_argument = true;
			++index;
		}

		if (!found_valid_argument)
		{
			return { std::nullopt, "no valid arguments found." };
		}

		return { result, std::nullopt };
	}

	template <typename T>
	auto argument_manager::to_numeric(const std::string& value) const -> std::optional<T>
	{
		T result;
		auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result);

		if (ec == std::errc() && ptr == value.data() + value.size())
			return result;

		return std::nullopt;
	}
} // namespace argument_parser