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

#pragma once

#include <string>
#include <map>
#include <optional>
#include <tuple>
#include <string_view>
#include <vector>

namespace utility_module
{
	/**
	 * @class argument_manager
	 * @brief Manages command line arguments and provides type-safe retrieval of values.
	 *
	 * This class can parse command line arguments supplied either as a single string or via
	 * argc/argv. It stores arguments in a key-value map (e.g., "--key" : "value") and provides
	 * functions to retrieve these values as various data types (string, bool, int, etc.).
	 */
	class argument_manager
	{
	public:
		/**
		 * @brief Default constructor.
		 */
		argument_manager() = default;

		/**
		 * @brief Parse arguments from a string or wide string.
		 * @tparam StringType The type of the input argument string (e.g., std::string or
		 * std::wstring).
		 * @param arguments The input arguments as a single string.
		 * @return std::optional<std::string> An error message if parsing fails, or std::nullopt on
		 * success.
		 *
		 * This function converts the input string to the system encoding if necessary, splits it
		 * into individual arguments, and stores them as key-value pairs.
		 */
		template <typename StringType>
		auto try_parse(const StringType& arguments) -> std::optional<std::string>;

		/**
		 * @brief Parse arguments from argc/argv.
		 * @tparam CharType The character type of the argv array (char or wchar_t).
		 * @param argc Number of arguments.
		 * @param argv Array of argument strings.
		 * @return std::optional<std::string> An error message if parsing fails, or std::nullopt on
		 * success.
		 *
		 * Similar to the string-based parsing, but directly uses the conventional argc/argv
		 * parameters typically passed to main().
		 */
		template <typename CharType>
		auto try_parse(int argc, CharType* argv[]) -> std::optional<std::string>;

		/**
		 * @brief Retrieve the value of an argument as a string.
		 * @param key The argument key (including the leading "--").
		 * @return std::optional<std::string> The argument value as a string if found, otherwise
		 * std::nullopt.
		 */
		auto to_string(std::string_view key) const -> std::optional<std::string>;

		/**
		 * @brief Retrieve the value of an argument as a boolean.
		 * @param key The argument key (including the leading "--").
		 * @return std::optional<bool> The argument value as a bool if found and convertible,
		 * otherwise std::nullopt.
		 *
		 * Acceptable values for true: "true", "1".
		 * Acceptable values for false: "false", "0".
		 */
		auto to_bool(std::string_view key) const -> std::optional<bool>;

		/**
		 * @brief Retrieve the value of an argument as a short integer.
		 * @param key The argument key (including the leading "--").
		 * @return std::optional<short> The argument value as a short if found and convertible,
		 * otherwise std::nullopt.
		 */
		auto to_short(std::string_view key) const -> std::optional<short>;

		/**
		 * @brief Retrieve the value of an argument as an unsigned short integer.
		 * @param key The argument key (including the leading "--").
		 * @return std::optional<unsigned short> The argument value as an unsigned short if found
		 * and convertible, otherwise std::nullopt.
		 */
		auto to_ushort(std::string_view key) const -> std::optional<unsigned short>;

		/**
		 * @brief Retrieve the value of an argument as an integer.
		 * @param key The argument key (including the leading "--").
		 * @return std::optional<int> The argument value as an int if found and convertible,
		 * otherwise std::nullopt.
		 */
		auto to_int(std::string_view key) const -> std::optional<int>;

		/**
		 * @brief Retrieve the value of an argument as an unsigned integer.
		 * @param key The argument key (including the leading "--").
		 * @return std::optional<unsigned int> The argument value as an unsigned int if found and
		 * convertible, otherwise std::nullopt.
		 */
		auto to_uint(std::string_view key) const -> std::optional<unsigned int>;

#ifdef _WIN32
		/**
		 * @brief Retrieve the value of an argument as a long long (Windows only).
		 * @param key The argument key (including the leading "--").
		 * @return std::optional<long long> The argument value as a long long if found and
		 * convertible, otherwise std::nullopt.
		 */
		auto to_llong(std::string_view key) const -> std::optional<long long>;
#else
		/**
		 * @brief Retrieve the value of an argument as a long (non-Windows).
		 * @param key The argument key (including the leading "--").
		 * @return std::optional<long> The argument value as a long if found and convertible,
		 * otherwise std::nullopt.
		 */
		auto to_long(std::string_view key) const -> std::optional<long>;
#endif

	private:
		/**
		 * @brief Internal storage of parsed arguments.
		 *
		 * This map holds key-value pairs (e.g., "--option" : "value").
		 */
		std::map<std::string, std::string> arguments_;

		/**
		 * @brief Parse a vector of already split argument strings.
		 * @param arguments A vector of argument strings (e.g., {"--option", "value"}).
		 * @return A tuple containing:
		 *         - std::optional<std::map<std::string, std::string>>: The parsed arguments map if
		 * successful, otherwise std::nullopt.
		 *         - std::optional<std::string>: An error message if parsing fails, otherwise
		 * std::nullopt.
		 */
		auto parse(const std::vector<std::string>& arguments)
			-> std::tuple<std::optional<std::map<std::string, std::string>>,
						  std::optional<std::string>>;

		/**
		 * @brief Convert a string argument to a numeric type.
		 * @tparam NumericType The target numeric type (e.g., int, long, short).
		 * @param key The argument key (including the leading "--").
		 * @return std::optional<NumericType> The converted numeric value if successful, otherwise
		 * std::nullopt.
		 */
		template <typename NumericType>
		auto to_numeric(std::string_view key) const -> std::optional<NumericType>;

		/**
		 * @brief Convert a string_view to a lowercase std::string.
		 * @param str The input string_view.
		 * @return A lowercase std::string.
		 */
		static auto to_lower(std::string_view str) -> std::string;
	};
} // namespace utility_module
