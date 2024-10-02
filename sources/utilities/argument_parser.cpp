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

	argument_manager::argument_manager(const std::string& arguments)
	{
		auto [splitted, message] = convert_string::split(arguments, " ");

		if (splitted.has_value())
		{
			_arguments = parse(splitted.value());
		}
	}

	argument_manager::argument_manager(const std::wstring& arguments)
	{
		auto [splitted, message] = convert_string::split(convert_string::to_string(arguments), " ");

		if (splitted.has_value())
		{
			_arguments = parse(splitted.value());
		}
	}

	argument_manager::argument_manager(int argc, char* argv[]) { _arguments = parse(argc, argv); }

	argument_manager::argument_manager(int argc, wchar_t* argv[])
	{
		_arguments = parse(argc, argv);
	}

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

#ifdef _WIN32
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

#ifdef _WIN32
		return to_numeric<long long>(target.value());
#else
		return to_numeric<long>(target.value());
#endif
	}

	std::map<std::string, std::string> argument_manager::parse(int argc, char* argv[])
	{
		std::vector<std::string> arguments;
		for (int index = 1; index < argc; ++index)
		{
			arguments.push_back(argv[index]);
		}

		return parse(arguments);
	}

	std::map<std::string, std::string> argument_manager::parse(int argc, wchar_t* argv[])
	{
		std::vector<std::string> arguments;
		for (int index = 1; index < argc; ++index)
		{
			arguments.push_back(convert_string::to_string(argv[index]));
		}

		return parse(arguments);
	}

	std::map<std::string, std::string> argument_manager::parse(
		const std::vector<std::string>& arguments)
	{
		std::map<std::string, std::string> result;

		size_t argc = arguments.size();
		std::string argument_id;
		for (size_t index = 0; index < argc; ++index)
		{
			argument_id = arguments[index];
			size_t offset = argument_id.find("--", 0);
			if (offset != 0)
			{
				continue;
			}

			if (argument_id.compare("--help") == 0)
			{
				result.insert({ argument_id, "display help" });
				continue;
			}

			if (index + 1 >= argc)
			{
				break;
			}

			auto target = result.find(argument_id);
			if (target == result.end())
			{
				result.insert({ argument_id, arguments[index + 1] });
				++index;

				continue;
			}

			target->second = arguments[index + 1];
			++index;
		}

		return result;
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