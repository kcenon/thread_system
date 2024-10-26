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

#pragma once

#include "convert_string.h"

#include <map>
#include <string>
#include <tuple>
#include <vector>
#include <optional>
#include <system_error>

namespace utility_module
{
	/**
	 * @class argument_manager
	 * @brief A class to parse and manage command-line arguments.
	 *
	 * The argument_manager class provides methods to parse command-line arguments
	 * in various formats (string, wstring, char array, wchar_t array) and retrieve
	 * them as different data types.
	 */
	class argument_manager
	{
	public:
		/**
		 * @brief Constructs an argument_manager instance.
		 */
		argument_manager();

		/**
		 * @brief Parses arguments from a single string or wide string.
		 *
		 * This function parses command-line arguments given as a single string or wide string.
		 *
		 * @tparam StringType The type of the argument string (e.g., std::string, std::wstring).
		 * @param arguments The arguments string to parse.
		 * @return A tuple containing a success flag and an optional error message.
		 */
		template <typename StringType>
		auto try_parse(const StringType& arguments) -> std::tuple<bool, std::optional<std::string>>
		{
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
				std::tie(converted, convert_error)
					= convert_string::to_string(std::wstring(arguments));
			}
			else
			{
				return { false, "Unsupported string type" };
			}

			if (!converted.has_value())
			{
				return { false, convert_error };
			}

			auto argument_string = converted.value();

			if (argument_string.empty()
				|| std::all_of(argument_string.begin(), argument_string.end(),
							   [](char c) {
								   return c == '\0' || std::isspace(static_cast<unsigned char>(c));
							   }))
			{
				return { false, "no valid arguments found." };
			}

			auto null_pos = argument_string.find('\0');
			if (null_pos != std::string::npos)
			{
				converted = argument_string.substr(0, null_pos);
			}

			auto [splitted, split_error] = convert_string::split(converted.value(), " ");
			if (split_error.has_value())
			{
				return { false, split_error };
			}

			auto splitted_vector = splitted.value();

			splitted_vector.erase(
				std::remove_if(
					splitted_vector.begin(), splitted_vector.end(), [](const std::string& s)
					{ return s.empty() || s.find_first_not_of(" \t\n\r") == std::string::npos; }),
				splitted_vector.end());

			if (splitted_vector.empty())
			{
				return { false, "no valid arguments found." };
			}

			if (splitted_vector.front().substr(0, 2) != "--")
			{
				if (splitted_vector.size() <= 1)
				{
					return { false, "invalid argument: " + splitted_vector.front() };
				}

				auto second_token = splitted_vector.at(1);
				if (second_token.substr(0, 2) != "--")
				{
					return { false, "invalid argument: " + second_token };
				}
			}

			if (!splitted_vector.empty() && splitted_vector.front().substr(0, 2) != "--")
			{
				splitted_vector.erase(splitted_vector.begin());
			}

			if (!splitted_vector.empty() && splitted_vector.front() == "--help")
			{
				arguments_["--help"] = "display help";
				return { true, std::nullopt };
			}

			auto [parsed, parse_error] = parse(splitted_vector);
			if (!parsed.has_value())
			{
				return { false, parse_error };
			}

			arguments_ = parsed.value();
			return { true, std::nullopt };
		}

		/**
		 * @brief Parses arguments from a char or wchar_t array.
		 *
		 * This function parses command-line arguments provided as argc and argv.
		 *
		 * @tparam CharType The type of the argument array elements (e.g., char, wchar_t).
		 * @param argc The number of arguments.
		 * @param argv The arguments array.
		 * @return A tuple containing a success flag and an optional error message.
		 */
		template <typename CharType>
		auto try_parse(int argc, CharType* argv[]) -> std::tuple<bool, std::optional<std::string>>
		{
			auto [parsed, parse_error] = parse(argc, argv);
			if (!parsed.has_value())
			{
				return { false, parse_error };
			}

			arguments_ = parsed.value();
			return { true, std::nullopt };
		}

		/**
		 * @brief Retrieves the argument value as a string.
		 *
		 * @param key The key of the argument.
		 * @return An optional containing the value if found, or std::nullopt if the key does not
		 * exist.
		 */
		auto to_string(const std::string& key) -> std::optional<std::string>;

		/**
		 * @brief Retrieves the argument value as a boolean.
		 *
		 * @param key The key of the argument.
		 * @return An optional containing the boolean value if found and convertible, otherwise
		 * std::nullopt.
		 */
		auto to_bool(const std::string& key) -> std::optional<bool>;

		/**
		 * @brief Retrieves the argument value as a short integer.
		 *
		 * @param key The key of the argument.
		 * @return An optional containing the short value if found and convertible, otherwise
		 * std::nullopt.
		 */
		auto to_short(const std::string& key) -> std::optional<short>;

		/**
		 * @brief Retrieves the argument value as an unsigned short integer.
		 *
		 * @param key The key of the argument.
		 * @return An optional containing the unsigned short value if found and convertible,
		 * otherwise std::nullopt.
		 */
		auto to_ushort(const std::string& key) -> std::optional<unsigned short>;

		/**
		 * @brief Retrieves the argument value as an integer.
		 *
		 * @param key The key of the argument.
		 * @return An optional containing the integer value if found and convertible, otherwise
		 * std::nullopt.
		 */
		auto to_int(const std::string& key) -> std::optional<int>;

		/**
		 * @brief Retrieves the argument value as an unsigned integer.
		 *
		 * @param key The key of the argument.
		 * @return An optional containing the unsigned integer value if found and convertible,
		 * otherwise std::nullopt.
		 */
		auto to_uint(const std::string& key) -> std::optional<unsigned int>;

#ifdef _WIN32
		/**
		 * @brief Retrieves the argument value as a long long integer (Windows).
		 *
		 * @param key The key of the argument.
		 * @return An optional containing the long long value if found and convertible, otherwise
		 * std::nullopt.
		 */
		auto to_llong(const std::string& key) -> std::optional<long long>;
#else
		/**
		 * @brief Retrieves the argument value as a long integer (non-Windows).
		 *
		 * @param key The key of the argument.
		 * @return An optional containing the long value if found and convertible, otherwise
		 * std::nullopt.
		 */
		auto to_long(const std::string& key) -> std::optional<long>;
#endif

	private:
		std::map<std::string, std::string>
			arguments_; ///< Stores parsed arguments as key-value pairs.

		/**
		 * @brief Converts an argument value to a numeric type.
		 *
		 * @tparam NumericType The target numeric type (e.g., int, short).
		 * @param key The key of the argument.
		 * @return An optional containing the converted numeric value if successful, otherwise
		 * std::nullopt.
		 */
		template <typename NumericType>
		auto to_numeric(const std::string& key) -> std::optional<NumericType>
		{
			auto target = to_string(key);
			if (!target.has_value())
			{
				return std::nullopt;
			}

			try
			{
				if constexpr (std::is_same_v<NumericType, int>)
				{
					return std::stoi(target.value());
				}
				else if constexpr (std::is_same_v<NumericType, long>)
				{
					return std::stol(target.value());
				}
				else if constexpr (std::is_same_v<NumericType, long long>)
				{
					return std::stoll(target.value());
				}
				else if constexpr (std::is_same_v<NumericType, unsigned long>)
				{
					return std::stoul(target.value());
				}
				else if constexpr (std::is_same_v<NumericType, unsigned long long>)
				{
					return std::stoull(target.value());
				}
				else if constexpr (std::is_same_v<NumericType, short>)
				{
					return static_cast<short>(std::stoi(target.value()));
				}
				else if constexpr (std::is_same_v<NumericType, unsigned short>)
				{
					return static_cast<unsigned short>(std::stoul(target.value()));
				}
				else if constexpr (std::is_same_v<NumericType, unsigned int>)
				{
					return static_cast<unsigned int>(std::stoul(target.value()));
				}
				return std::nullopt;
			}
			catch (...)
			{
				return std::nullopt;
			}
		}

		/**
		 * @brief Parses arguments from an array of characters.
		 *
		 * @tparam CharType The type of characters in the array (e.g., char, wchar_t).
		 * @param argc The number of arguments.
		 * @param argv The argument array.
		 * @return A tuple containing a map of arguments and an optional error message.
		 */
		template <typename CharType>
		auto parse(int argc, CharType* argv[])
			-> std::tuple<std::optional<std::map<std::string, std::string>>,
						  std::optional<std::string>>
		{
			std::map<std::string, std::string> result;
			bool found_valid_argument = false;

			// 시작 인덱스 결정 (프로그램 이름 무시)
			int start_index = 0;
			if (argc > 0)
			{
				std::string first_arg;
				if constexpr (std::is_same_v<CharType, wchar_t>)
				{
					auto [converted_str, error] = convert_string::to_string(std::wstring(argv[0]));
					if (!converted_str.has_value())
					{
						return { std::nullopt, error };
					}
					first_arg = converted_str.value();
				}
				else
				{
					first_arg = std::string(argv[0]);
				}

				if (first_arg.substr(0, 2) != "--")
				{
					start_index = 1;
				}
			}

			for (int index = start_index; index < argc; ++index)
			{
				std::string argument_id;
				if constexpr (std::is_same_v<CharType, wchar_t>)
				{
					auto [converted_str, error]
						= convert_string::to_string(std::wstring(argv[index]));
					if (!converted_str.has_value())
					{
						return { std::nullopt, error };
					}
					argument_id = converted_str.value();
				}
				else
				{
					argument_id = std::string(argv[index]);
				}

				if (argument_id.empty() || argument_id.substr(0, 2) != "--")
				{
					return { std::nullopt, "invalid argument: " + argument_id };
				}

				if (argument_id == "--help")
				{
					result.insert({ argument_id, "display help" });
					found_valid_argument = true;
					continue;
				}

				if (index + 1 >= argc)
				{
					return { std::nullopt, "argument '" + argument_id + "' expects a value." };
				}

				std::string value;
				if constexpr (std::is_same_v<CharType, wchar_t>)
				{
					auto [converted_str, error]
						= convert_string::to_string(std::wstring(argv[index + 1]));
					if (!converted_str.has_value())
					{
						return { std::nullopt, error };
					}
					value = converted_str.value();
				}
				else
				{
					value = std::string(argv[index + 1]);
				}

				result[argument_id] = value;
				found_valid_argument = true;
				++index;
			}

			if (!found_valid_argument)
			{
				return { std::nullopt, "no valid arguments found." };
			}

			return { result, std::nullopt };
		}

		/**
		 * @brief Parses a vector of string arguments into key-value pairs.
		 *
		 * This function processes arguments in the format "--key value" and stores them in a map.
		 *
		 * @param arguments A vector of arguments to parse.
		 * @return A tuple containing a map of parsed arguments and an optional error message.
		 */
		auto parse(const std::vector<std::string>& arguments)
			-> std::tuple<std::optional<std::map<std::string, std::string>>,
						  std::optional<std::string>>;
	};

} // namespace utility_module
