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

#include <map>
#include <string>
#include <vector>
#include <optional>

namespace utility_module
{
	/**
	 * @class argument_manager
	 * @brief Manages and parses command-line arguments.
	 *
	 * This class provides functionality to parse and access command-line arguments
	 * in various formats. It supports conversion of argument values to different
	 * data types.
	 */
	class argument_manager
	{
	public:
		/**
		 * @brief Default constructor.
		 */
		argument_manager(void);

		/**
		 * @brief Constructs the argument manager with a string of arguments.
		 * @param arguments A string containing the command-line arguments.
		 */
		argument_manager(const std::string& arguments);

		/**
		 * @brief Constructs the argument manager with a wstring of arguments.
		 * @param arguments A string containing the command-line arguments.
		 */
		argument_manager(const std::wstring& arguments);

		/**
		 * @brief Constructs the argument manager with argc and argv.
		 * @param argc The number of arguments.
		 * @param argv An array of C-style strings containing the arguments.
		 */
		argument_manager(int argc, char* argv[]);

		/**
		 * @brief Constructs the argument manager with argc and argv.
		 * @param argc The number of arguments.
		 * @param argv An array of C-style strings containing the arguments.
		 */
		argument_manager(int argc, wchar_t* argv[]);

		/**
		 * @brief Retrieves the value of an argument as a string.
		 * @param key The key of the argument.
		 * @return An optional containing the string value if found, or nullopt if not found.
		 */
		auto to_string(const std::string& key) -> std::optional<std::string>;

		/**
		 * @brief Retrieves the value of an argument as a boolean.
		 * @param key The key of the argument.
		 * @return An optional containing the boolean value if found and convertible, or nullopt if
		 * not.
		 */
		auto to_bool(const std::string& key) -> std::optional<bool>;

		/**
		 * @brief Retrieves the value of an argument as a short integer.
		 * @param key The key of the argument.
		 * @return An optional containing the short value if found and convertible, or nullopt if
		 * not.
		 */
		auto to_short(const std::string& key) -> std::optional<short>;

		/**
		 * @brief Retrieves the value of an argument as an unsigned short integer.
		 * @param key The key of the argument.
		 * @return An optional containing the unsigned short value if found and convertible, or
		 * nullopt if not.
		 */
		auto to_ushort(const std::string& key) -> std::optional<unsigned short>;

		/**
		 * @brief Retrieves the value of an argument as an integer.
		 * @param key The key of the argument.
		 * @return An optional containing the int value if found and convertible, or nullopt if not.
		 */
		auto to_int(const std::string& key) -> std::optional<int>;

		/**
		 * @brief Retrieves the value of an argument as an unsigned integer.
		 * @param key The key of the argument.
		 * @return An optional containing the unsigned int value if found and convertible, or
		 * nullopt if not.
		 */
		auto to_uint(const std::string& key) -> std::optional<unsigned int>;

#ifdef _WIN32
		/**
		 * @brief Retrieves the value of an argument as a long long integer (Windows-specific).
		 * @param key The key of the argument.
		 * @return An optional containing the long long value if found and convertible, or nullopt
		 * if not.
		 */
		auto to_llong(const std::string& key) -> std::optional<long long>;
#else
		/**
		 * @brief Retrieves the value of an argument as a long integer (non-Windows platforms).
		 * @param key The key of the argument.
		 * @return An optional containing the long value if found and convertible, or nullopt if
		 * not.
		 */
		auto to_long(const std::string& key) -> std::optional<long>;
#endif

	private:
		/**
		 * @brief Parses command-line arguments from argc and argv.
		 * @param argc The number of arguments.
		 * @param argv An array of C-style strings containing the arguments.
		 * @return A map of parsed arguments.
		 */
		auto parse(int argc, char* argv[]) -> std::map<std::string, std::string>;

		/**
		 * @brief Parses command-line arguments from argc and argv.
		 * @param argc The number of arguments.
		 * @param argv An array of C-style strings containing the arguments.
		 * @return A map of parsed arguments.
		 */
		auto parse(int argc, wchar_t* argv[]) -> std::map<std::string, std::string>;

		/**
		 * @brief Parses command-line arguments from a vector of strings.
		 * @param arguments A vector of strings containing the arguments.
		 * @return A map of parsed arguments.
		 */
		auto parse(const std::vector<std::string>& arguments) -> std::map<std::string, std::string>;

		/**
		 * @brief Converts a string value to a numeric type.
		 * @tparam T The numeric type to convert to.
		 * @param value The string value to convert.
		 * @return An optional containing the converted numeric value if successful, or nullopt if
		 * not.
		 */
		template <typename T> auto to_numeric(const std::string& value) const -> std::optional<T>;

	private:
		std::map<std::string, std::string> _arguments; ///< Stores the parsed arguments
	};
} // namespace utility_module
