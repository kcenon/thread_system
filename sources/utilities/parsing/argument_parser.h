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
#include <cstdint>

namespace utility_module
{
	/**
	 * @class argument_manager
	 * @brief A utility class for parsing and managing command-line arguments.
	 *
	 * The @c argument_manager class provides functions to:
	 * - Parse command-line arguments from either a single string (e.g., "--option value")
	 *   or the traditional @c argc, @c argv approach.
	 * - Store parsed arguments in a key-value map of type @c std::map<std::string, std::string>.
	 * - Retrieve values by key in a type-safe manner (e.g., @c bool, @c int).
	 *
	 * ### Parsing Conventions
	 * - Arguments are expected in the form: `--key value`.
	 * - If an argument has no following value, it may be stored with an empty string (`""`).
	 * - If a key appears multiple times, the last occurrence typically overrides previous ones.
	 *
	 * ### Example Usage
	 * @code
	 * int main(int argc, char* argv[])
	 * {
	 *     utility_module::argument_manager arg_mgr;
	 *     if (auto err = arg_mgr.try_parse(argc, argv)) {
	 *         std::cerr << "Error parsing arguments: " << *err << std::endl;
	 *         return EXIT_FAILURE;
	 *     }
	 *
	 *     // Retrieve a string option
	 *     if (auto username = arg_mgr.to_string("--username")) {
	 *         std::cout << "Username: " << *username << std::endl;
	 *     }
	 *
	 *     // Retrieve a bool option
	 *     if (auto debug_mode = arg_mgr.to_bool("--debug")) {
	 *         std::cout << "Debug mode: " << (*debug_mode ? "on" : "off") << std::endl;
	 *     }
	 *
	 *     return 0;
	 * }
	 * @endcode
	 */
	class argument_manager
	{
	public:
		/**
		 * @brief Default constructor.
		 *
		 * Initializes an empty @c argument_manager with no pre-parsed arguments.
		 */
		argument_manager() = default;

		/**
		 * @brief Parse command-line arguments from a single (possibly wide) string.
		 * @tparam StringType The type of the input string (e.g., @c std::string or @c
		 * std::wstring).
		 * @param arguments The argument string, which may look like `--option1 value1 --flag
		 * --option2 value2`.
		 * @return @c std::optional<std::string> containing an error message on failure,
		 *         or @c std::nullopt on success.
		 *
		 * Internally, this method:
		 * 1. Converts the input string to a UTF-8 or system-encoded narrow string if necessary.
		 * 2. Splits the string into tokens based on whitespace.
		 * 3. Calls @c parse() to populate @c arguments_ with key-value pairs.
		 *
		 * #### Example
		 * @code
		 * argument_manager mgr;
		 * std::string cmd = "--user alice --debug --count 10";
		 * if (auto err = mgr.try_parse(cmd)) {
		 *     std::cerr << "Parse error: " << *err << std::endl;
		 * }
		 * @endcode
		 */
		template <typename StringType>
		auto try_parse(const StringType& arguments) -> std::optional<std::string>;

		/**
		 * @brief Parse command-line arguments using the traditional @c argc and @c argv.
		 * @tparam CharType The character type of the @c argv array (usually @c char or @c wchar_t).
		 * @param argc The number of arguments in @c argv.
		 * @param argv The array of argument strings (e.g., @c argv[0] is the program name).
		 * @return @c std::optional<std::string> containing an error message on failure,
		 *         or @c std::nullopt on success.
		 *
		 * Internally, this method:
		 * 1. Converts each @c argv[i] to a narrow string if necessary.
		 * 2. Calls @c parse() to populate @c arguments_ with key-value pairs.
		 *
		 * This is typically called at the start of @c main().
		 *
		 * #### Example
		 * @code
		 * int main(int argc, char* argv[])
		 * {
		 *     argument_manager mgr;
		 *     if (auto err = mgr.try_parse(argc, argv)) {
		 *         std::cerr << "Error: " << *err << std::endl;
		 *         return EXIT_FAILURE;
		 *     }
		 *     // ...
		 * }
		 * @endcode
		 */
		template <typename CharType>
		auto try_parse(int argc, CharType* argv[]) -> std::optional<std::string>;

		/**
		 * @brief Retrieves the value of an argument as a string.
		 * @param key The argument key, including the leading "`--`" (e.g., "`--username`").
		 * @return @c std::optional<std::string> containing the value if found, or @c std::nullopt
		 * if not found.
		 *
		 * If multiple values were specified for the same @p key, the most recently parsed value
		 * is returned. If no matching key is present, @c std::nullopt is returned.
		 */
		auto to_string(std::string_view key) const -> std::optional<std::string>;

		/**
		 * @brief Retrieves the value of an argument as a boolean.
		 * @param key The argument key, including the leading "`--`" (e.g., "`--debug`").
		 * @return @c std::optional<bool> indicating @c true or @c false, or @c std::nullopt if
		 *         the key is not found or the value is not recognized.
		 *
		 * Recognized true values: "`true`" or "`1`" (case-insensitive).
		 * Recognized false values: "`false`" or "`0`" (case-insensitive).
		 */
		auto to_bool(std::string_view key) const -> std::optional<bool>;

		/**
		 * @brief Retrieves the value of an argument as a @c short integer.
		 * @param key The argument key (e.g., "`--port`").
		 * @return @c std::optional<short> if successfully converted, otherwise @c std::nullopt.
		 *
		 * Conversion errors or out-of-range values result in @c std::nullopt.
		 */
		auto to_short(std::string_view key) const -> std::optional<short>;

		/**
		 * @brief Retrieves the value of an argument as an unsigned @c short integer.
		 * @param key The argument key (e.g., "`--threads`").
		 * @return @c std::optional<unsigned short> if successfully converted, otherwise @c
		 * std::nullopt.
		 */
		auto to_ushort(std::string_view key) const -> std::optional<unsigned short>;

		/**
		 * @brief Retrieves the value of an argument as an @c int.
		 * @param key The argument key (e.g., "`--count`").
		 * @return @c std::optional<int> if successfully converted, otherwise @c std::nullopt.
		 */
		auto to_int(std::string_view key) const -> std::optional<int>;

		/**
		 * @brief Retrieves the value of an argument as an unsigned @c int.
		 * @param key The argument key (e.g., "`--size`").
		 * @return @c std::optional<unsigned int> if successfully converted, otherwise @c
		 * std::nullopt.
		 */
		auto to_uint(std::string_view key) const -> std::optional<unsigned int>;

		/**
		 * @brief Retrieves the value of an argument as a 64-bit signed integer.
		 * @param key The argument key (e.g., "`--large-value`").
		 * @return @c std::optional<int64_t> if successfully converted, otherwise @c std::nullopt.
		 * @note This provides consistent behavior across all platforms.
		 */
		auto to_int64(std::string_view key) const -> std::optional<int64_t>;

		/**
		 * @brief Retrieves the value of an argument as a 64-bit unsigned integer.
		 * @param key The argument key (e.g., "`--large-value`").
		 * @return @c std::optional<uint64_t> if successfully converted, otherwise @c std::nullopt.
		 * @note This provides consistent behavior across all platforms.
		 */
		auto to_uint64(std::string_view key) const -> std::optional<uint64_t>;

	private:
		/**
		 * @brief Internal map storing parsed argument key-value pairs.
		 *
		 * Keys include their leading "`--`" prefix. If an argument is provided without
		 * an accompanying value (e.g., "`--flag`"), the value may be an empty string (`""`).
		 */
		std::map<std::string, std::string> arguments_;

		/**
		 * @brief Parses a pre-split list of argument tokens and populates @c arguments_.
		 * @param arguments A vector of tokens (e.g., {"--key", "value", "--flag"}).
		 * @return A tuple of:
		 *         - @c std::optional<std::map<std::string, std::string>>: The parsed arguments
		 *           map on success, or @c std::nullopt on failure.
		 *         - @c std::optional<std::string>: An error message if parsing fails, otherwise
		 *           @c std::nullopt.
		 *
		 * The parser expects tokens in pairs of "`--key`" followed by "value". If a key does
		 * not have a subsequent value token (or if the format is otherwise invalid), an error
		 * message is produced. The last occurrence of a key overrides previous ones.
		 */
		auto parse(const std::vector<std::string>& arguments)
			-> std::tuple<std::optional<std::map<std::string, std::string>>,
						  std::optional<std::string>>;

		/**
		 * @brief Converts the string value for a given key to a numeric type.
		 * @tparam NumericType The desired numeric type (e.g., @c int, @c long, @c short).
		 * @param key The argument key.
		 * @return @c std::optional<NumericType> if conversion succeeds, otherwise @c std::nullopt.
		 *
		 * If the key is missing or the value is not a valid decimal number (or out of range for
		 * @p NumericType), @c std::nullopt is returned.
		 */
		template <typename NumericType>
		auto to_numeric(std::string_view key) const -> std::optional<NumericType>;

		/**
		 * @brief Converts the input string_view to a lowercase std::string.
		 * @param str The input string_view.
		 * @return A new std::string with all characters converted to lowercase.
		 *
		 * Used internally for case-insensitive comparisons (e.g., checking "TRUE"/"true"/"True").
		 */
		static auto to_lower(std::string_view str) -> std::string;
	};
} // namespace utility_module
