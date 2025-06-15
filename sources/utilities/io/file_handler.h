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
#include "../core/span.h"

namespace utility_module
{
	/**
	 * @class file
	 * @brief A utility class for basic file I/O operations.
	 *
	 * The @c file class provides static methods for:
	 * - Removing files from the filesystem.
	 * - Reading file contents into a byte buffer.
	 * - Saving (overwriting) data to a file.
	 * - Appending data to existing files.
	 *
	 * Under the hood, it uses standard C++ file streams (and possibly @c std::filesystem features,
	 * if available) to manage file paths and I/O operations. Each method returns an optional
	 * error message:
	 * - @c std::nullopt indicates success.
	 * - A non-empty @c std::string indicates the reason why the operation failed.
	 *
	 * ### Example Usage
	 * @code
	 * // Remove a file
	 * if (auto err = file::remove("data.bin")) {
	 *     std::cerr << "Failed to remove file: " << *err << std::endl;
	 * }
	 *
	 * // Load file into memory
	 * auto [contents, load_err] = file::load("image.png");
	 * if (load_err) {
	 *     std::cerr << "Failed to load file: " << *load_err << std::endl;
	 * } else {
	 *     // Use contents...
	 * }
	 *
	 * // Save data
	 * std::vector<uint8_t> data = {0x01, 0x02, 0x03};
	 * if (auto err = file::save("output.bin", data)) {
	 *     std::cerr << "Failed to save file: " << *err << std::endl;
	 * }
	 * @endcode
	 */
	class file
	{
	public:
		/**
		 * @brief Removes a file from the filesystem.
		 * @param path The path to the file to remove.
		 * @return @c std::optional<std::string> containing an error message if the removal
		 *         operation fails, or @c std::nullopt on success.
		 *
		 * This function deletes the file specified by @p path. If the file does not exist,
		 * some platforms may treat it as an error, while others consider it a success. The
		 * behavior depends on the underlying C++ library and filesystem.
		 */
		static auto remove(const std::string& path) -> std::optional<std::string>;

		/**
		 * @brief Loads the entire contents of a file into memory.
		 * @param path The path to the file to be read.
		 * @return A tuple of:
		 *         - @c std::vector<uint8_t>: The file contents as a byte vector. If an error
		 *           occurs, this vector is typically empty.
		 *         - @c std::optional<std::string>: An error message if reading fails, or
		 *           @c std::nullopt on success.
		 *
		 * The file is opened in binary mode. If the file is very large, be mindful of available
		 * memory. In case of an error (e.g., file not found or permission denied), the second
		 * element contains the reason, and the first element will be an empty vector.
		 */
		static auto load(const std::string& path)
			-> std::tuple<std::vector<uint8_t>, std::optional<std::string>>;

		/**
		 * @brief Saves data to a file, overwriting it if it already exists.
		 * @param path The target file path.
		 * @param data A byte buffer containing the data to write.
		 * @return @c std::optional<std::string> with an error message on failure, or
		 *         @c std::nullopt on success.
		 *
		 * The file is opened in binary mode for output. If the file does not exist, it is
		 * created. If it exists, its contents are replaced by @p data. In case of I/O errors
		 * (e.g., insufficient permissions, write failure), an error message is returned.
		 */
		static auto save(const std::string& path, const std::vector<uint8_t>& data)
			-> std::optional<std::string>;
			
		/**
		 * @brief Saves data to a file using a span, overwriting it if it already exists.
		 * @param path The target file path.
		 * @param data A span pointing to the data to write.
		 * @return @c std::optional<std::string> with an error message on failure, or
		 *         @c std::nullopt on success.
		 *
		 * The file is opened in binary mode for output. If the file does not exist, it is
		 * created. If it exists, its contents are replaced by @p data. In case of I/O errors
		 * (e.g., insufficient permissions, write failure), an error message is returned.
		 * 
		 * This overload uses a span to avoid unnecessary copies of the data.
		 */
		static auto save(const std::string& path, span<const uint8_t> data)
			-> std::optional<std::string>;

		/**
		 * @brief Appends data to the end of an existing file.
		 * @param path The target file path.
		 * @param data A byte buffer containing the data to append.
		 * @return @c std::optional<std::string> with an error message on failure, or
		 *         @c std::nullopt on success.
		 *
		 * The file is opened in binary append mode. If the file does not exist, behavior may depend
		 * on your platform or library settings (some may create it, others may fail). If you need
		 * to ensure creation, consider using @c save() first or verifying the file's existence.
		 */
		static auto append(const std::string& path, const std::vector<uint8_t>& data)
			-> std::optional<std::string>;
			
		/**
		 * @brief Appends data to the end of an existing file using a span.
		 * @param path The target file path.
		 * @param data A span pointing to the data to append.
		 * @return @c std::optional<std::string> with an error message on failure, or
		 *         @c std::nullopt on success.
		 *
		 * The file is opened in binary append mode. If the file does not exist, behavior may depend
		 * on your platform or library settings (some may create it, others may fail). If you need
		 * to ensure creation, consider using @c save() first or verifying the file's existence.
		 * 
		 * This overload uses a span to avoid unnecessary copies of the data.
		 */
		static auto append(const std::string& path, span<const uint8_t> data)
			-> std::optional<std::string>;
	};
} // namespace utility_module