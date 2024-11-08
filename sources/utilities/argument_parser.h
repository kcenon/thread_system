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

#include <string>
#include <map>
#include <optional>
#include <tuple>
#include <string_view>
#include <vector>

namespace utility_module
{
	/**
	 * @brief Command line argument management class
	 * @details Provides functionality to parse and manage command line arguments
	 *          with type-safe access and error handling
	 */
	class argument_manager
	{
	public:
		/**
		 * @brief Default constructor
		 */
		argument_manager();

		/**
		 * @brief Parse command line arguments from string types
		 * @tparam StringType The string type (std::string or std::wstring)
		 * @param arguments The arguments to parse
		 * @return Tuple of (success flag, optional error message)
		 */
		template <typename StringType>
		auto try_parse(const StringType& arguments) -> std::tuple<bool, std::optional<std::string>>;

		/**
		 * @brief Parse command line arguments from argc/argv style input
		 * @tparam CharType The character type (char or wchar_t)
		 * @param argc Number of arguments
		 * @param argv Array of argument strings
		 * @return Tuple of (success flag, optional error message)
		 */
		template <typename CharType>
		auto try_parse(int argc, CharType* argv[]) -> std::tuple<bool, std::optional<std::string>>;

		/**
		 * @brief Get argument value as string
		 * @param key The argument key to look up (including --)
		 * @return Optional containing the string value if found
		 */
		auto to_string(const std::string& key) -> std::optional<std::string>;

		/**
		 * @brief Get argument value as boolean
		 * @param key The argument key to look up
		 * @return Optional containing the boolean value if found and convertible
		 */
		auto to_bool(const std::string& key) -> std::optional<bool>;

		/**
		 * @brief Get argument value as short
		 * @param key The argument key to look up
		 * @return Optional containing the short value if found and convertible
		 */
		auto to_short(const std::string& key) -> std::optional<short>;

		/**
		 * @brief Get argument value as unsigned short
		 * @param key The argument key to look up
		 * @return Optional containing the unsigned short value if found and convertible
		 */
		auto to_ushort(const std::string& key) -> std::optional<unsigned short>;

		/**
		 * @brief Get argument value as int
		 * @param key The argument key to look up
		 * @return Optional containing the int value if found and convertible
		 */
		auto to_int(const std::string& key) -> std::optional<int>;

		/**
		 * @brief Get argument value as unsigned int
		 * @param key The argument key to look up
		 * @return Optional containing the unsigned int value if found and convertible
		 */
		auto to_uint(const std::string& key) -> std::optional<unsigned int>;

#ifdef _WIN32
		/**
		 * @brief Get argument value as long long (Windows)
		 * @param key The argument key to look up
		 * @return Optional containing the long long value if found and convertible
		 */
		auto to_llong(const std::string& key) -> std::optional<long long>;
#else
		/**
		 * @brief Get argument value as long (non-Windows)
		 * @param key The argument key to look up
		 * @return Optional containing the long value if found and convertible
		 */
		auto to_long(const std::string& key) -> std::optional<long>;
#endif

	private:
		std::map<std::string, std::string> arguments_;

		/**
		 * @brief Parse a vector of string arguments
		 * @param arguments Vector of argument strings
		 * @return Tuple of (parsed arguments map, optional error message)
		 */
		auto parse(const std::vector<std::string>& arguments)
			-> std::tuple<std::optional<std::map<std::string, std::string>>,
						  std::optional<std::string>>;

		/**
		 * @brief Convert string value to numeric type
		 * @tparam NumericType Target numeric type
		 * @param key Argument key to convert
		 * @return Optional containing the converted value if successful
		 */
		template <typename NumericType>
		auto to_numeric(const std::string& key) -> std::optional<NumericType>;
	};
} // namespace utility_module