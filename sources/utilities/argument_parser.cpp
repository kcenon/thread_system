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

#include <charconv>
#include <algorithm>

namespace utility_module
{
	argument_manager::argument_manager(void) {}

	auto argument_manager::to_string(const std::string& key) -> std::optional<std::string>
	{
		auto target = arguments_.find(key);
		return target != arguments_.end() ? std::optional(target->second) : std::nullopt;
	}

	auto argument_manager::to_bool(const std::string& key) -> std::optional<bool>
	{
		auto target = to_string(key);
		if (!target.has_value())
		{
			return std::nullopt;
		}

		auto temp = target.value();
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

	auto argument_manager::parse(const std::vector<std::string>& arguments)
		-> std::tuple<std::optional<std::map<std::string, std::string>>, std::optional<std::string>>
	{
		if (arguments.empty())
		{
			return { std::nullopt, "no valid arguments found." };
		}

		std::map<std::string, std::string> result;
		bool found_valid_argument = false;

		for (size_t i = 0; i < arguments.size(); ++i)
		{
			const auto& arg = arguments[i];

			if (arg.substr(0, 2) != "--")
			{
				return { std::nullopt, "invalid argument: " + arg };
			}

			if (arg == "--help")
			{
				result.insert({ arg, "display help" });
				found_valid_argument = true;
				continue;
			}

			if (i + 1 >= arguments.size())
			{
				return { std::nullopt, "argument '" + arg + "' expects a value." };
			}

			if (arguments[i + 1].substr(0, 2) == "--")
			{
				return { std::nullopt, "argument '" + arg + "' expects a value." };
			}

			result[arg] = arguments[++i];
			found_valid_argument = true;
		}

		if (!found_valid_argument)
		{
			return { std::nullopt, "no valid arguments found." };
		}

		return { result, std::nullopt };
	}
} // namespace argument_parser