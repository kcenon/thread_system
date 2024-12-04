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

#include <tuple>
#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace utility_module
{
	/**
	 * @class file
	 * @brief Provides static utilities for file system operations
	 *
	 * Handles basic file operations including reading, writing,
	 * appending, and deletion with error handling
	 */
	class file
	{
	public:
		/**
		 * @brief Deletes a file from the file system
		 *
		 * @param path Path to the file to delete
		 * @return std::optional<std::string> Error message if deletion failed, std::nullopt if
		 * successful
		 */
		static auto remove(const std::string& path) -> std::optional<std::string>;

		/**
		 * @brief Reads entire file content into memory
		 *
		 * @param path Path to the file to read
		 * @return std::tuple<std::vector<uint8_t>, std::optional<std::string>>
		 *         First element: File contents as byte array
		 *         Second element: Error message if read failed, std::nullopt if successful
		 */
		static auto load(const std::string& path)
			-> std::tuple<std::vector<uint8_t>, std::optional<std::string>>;

		/**
		 * @brief Writes data to a file, creating or overwriting it
		 *
		 * @param path Target file path
		 * @param data Byte array to write
		 * @return std::optional<std::string> Error message if write failed, std::nullopt if
		 * successful
		 */
		static auto save(const std::string& path,
						 const std::vector<uint8_t>& data) -> std::optional<std::string>;

		/**
		 * @brief Appends data to end of existing file
		 *
		 * @param path Target file path
		 * @param data Byte array to append
		 * @return std::optional<std::string> Error message if append failed, std::nullopt if
		 * successful
		 */
		static auto append(const std::string& path,
						   const std::vector<uint8_t>& data) -> std::optional<std::string>;
	};
} // namespace utility_module