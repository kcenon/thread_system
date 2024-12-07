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
#include <charconv>
#include <algorithm>
#include <cctype>

namespace utility_module
{
	/**
	 * @brief Command line argument manager
	 * @details
	 *  - Parses arguments and stores them in a key-value form.
	 *  - Provides type-safe conversion capabilities.
	 */
	class argument_manager
	{
	public:
		argument_manager() = default;

		/**
		 * @brief Parse arguments from a string or wide string
		 * @tparam StringType std::string or std::wstring
		 * @param arguments Input argument string
		 * @return Optional error message, or std::nullopt on success
		 */
		template <typename StringType>
		auto try_parse(const StringType& arguments) -> std::optional<std::string>;

		/**
		 * @brief Parse arguments from argc/argv
		 * @tparam CharType char or wchar_t
		 * @param argc Number of arguments
		 * @param argv Argument array
		 * @return Optional error message, or std::nullopt on success
		 */
		template <typename CharType>
		auto try_parse(int argc, CharType* argv[]) -> std::optional<std::string>;

		/**
		 * @brief Get the argument value as a string by key
		 * @param key The argument key (including "--")
		 * @return Optional string value if found
		 */
		auto to_string(std::string_view key) const -> std::optional<std::string>;

		/**
		 * @brief Get the argument value as a boolean
		 * @param key The argument key
		 * @return Optional boolean if found and convertible
		 */
		auto to_bool(std::string_view key) const -> std::optional<bool>;

		/**
		 * @brief Get the argument value as a short
		 * @param key The argument key
		 * @return Optional short if found and convertible
		 */
		auto to_short(std::string_view key) const -> std::optional<short>;

		/**
		 * @brief Get the argument value as an unsigned short
		 * @param key The argument key
		 * @return Optional unsigned short if found and convertible
		 */
		auto to_ushort(std::string_view key) const -> std::optional<unsigned short>;

		/**
		 * @brief Get the argument value as an int
		 * @param key The argument key
		 * @return Optional int if found and convertible
		 */
		auto to_int(std::string_view key) const -> std::optional<int>;

		/**
		 * @brief Get the argument value as an unsigned int
		 * @param key The argument key
		 * @return Optional unsigned int if found and convertible
		 */
		auto to_uint(std::string_view key) const -> std::optional<unsigned int>;

#ifdef _WIN32
		/**
		 * @brief Get the argument value as a long long (Windows)
		 * @param key The argument key
		 * @return Optional long long if found and convertible
		 */
		auto to_llong(std::string_view key) const -> std::optional<long long>;
#else
		/**
		 * @brief Get the argument value as a long (non-Windows)
		 * @param key The argument key
		 * @return Optional long if found and convertible
		 */
		auto to_long(std::string_view key) const -> std::optional<long>;
#endif

	private:
		std::map<std::string, std::string> arguments_;

		/**
		 * @brief Parse a vector of string arguments
		 * @param arguments Vector of argument strings
		 * @return Tuple of (optional map of arguments, optional error message)
		 */
		auto parse(const std::vector<std::string>& arguments)
			-> std::tuple<std::optional<std::map<std::string, std::string>>,
						  std::optional<std::string>>;

		/**
		 * @brief Convert a string value to a numeric type
		 * @tparam NumericType Target numeric type
		 * @param key The argument key
		 * @return Optional converted value if successful
		 */
		template <typename NumericType>
		auto to_numeric(std::string_view key) const -> std::optional<NumericType>;

		/**
		 * @brief Helper function to convert a string_view to lowercase
		 */
		static auto to_lower(std::string_view str) -> std::string;
	};
} // namespace utility_module