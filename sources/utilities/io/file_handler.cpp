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

#include "file_handler.h"

#include <filesystem>
#include <fstream>

/**
 * @file file_handler.cpp
 * @brief Implementation of cross-platform file I/O operations.
 *
 * This file contains the implementation of the file utility class, providing
 * safe and efficient file operations with proper error handling. The implementation
 * uses modern C++ filesystem library for cross-platform compatibility and
 * standard I/O streams for reliable file access.
 * 
 * Key Features:
 * - Cross-platform file operations using std::filesystem
 * - Binary file I/O with efficient buffer management
 * - Automatic directory creation for output operations
 * - Comprehensive error handling with descriptive messages
 * - Support for both vector and span data types
 * - Safe file removal with validation checks
 * 
 * Error Handling Philosophy:
 * - Returns std::optional<std::string> for error messages
 * - std::nullopt indicates successful operation
 * - Descriptive error messages for debugging and logging
 * - Graceful handling of file system errors
 * 
 * Performance Considerations:
 * - Uses efficient iterator-based loading for large files
 * - Binary I/O mode for optimal performance
 * - Automatic buffer sizing based on file content
 * - Minimal memory allocations during operations
 */

namespace utility_module
{
	/**
	 * @brief Safely removes a file from the filesystem.
	 * 
	 * Implementation details:
	 * - Validates file existence before attempting removal
	 * - Ensures path points to a regular file (not directory or special file)
	 * - Uses error_code version to avoid exceptions
	 * - Provides descriptive error messages for failure cases
	 * 
	 * Safety Checks:
	 * - File existence validation prevents unnecessary operations
	 * - Regular file check prevents accidental directory removal
	 * - Error code handling ensures graceful failure
	 * 
	 * @param path Filesystem path to the file to remove
	 * @return std::nullopt on success, error message on failure
	 */
	auto file::remove(const std::string& path) -> std::optional<std::string>
	{
		if (!std::filesystem::exists(path))
		{
			return "File does not exist";
		}

		if (!std::filesystem::is_regular_file(path))
		{
			return "Path is not a regular file";
		}

		std::error_code error;
		if (!std::filesystem::remove(path, error))
		{
			return error.message();
		}

		return std::nullopt;
	}

	/**
	 * @brief Loads entire file contents into memory as binary data.
	 * 
	 * Implementation details:
	 * - Validates file existence before attempting to open
	 * - Opens file in binary mode for accurate data reading
	 * - Uses iterator-based loading for efficient memory usage
	 * - Automatically sizes buffer based on file content
	 * - Explicit stream closure for resource management
	 * 
	 * Memory Efficiency:
	 * - Single allocation sized to file content
	 * - Iterator-based reading minimizes intermediate buffers
	 * - Automatic buffer resizing handled by vector
	 * 
	 * Error Handling:
	 * - File existence check prevents open failures
	 * - Stream validation ensures successful file access
	 * - Returns empty vector with error message on failure
	 * 
	 * @param path Filesystem path to the file to load
	 * @return Tuple of (file_data, error_message), error is nullopt on success
	 */
	auto file::load(const std::string& path)
		-> std::tuple<std::vector<uint8_t>, std::optional<std::string>>
	{
		if (!std::filesystem::exists(path))
		{
			return { std::vector<uint8_t>{}, "File does not exist" };
		}

		std::ifstream stream(path, std::ios::binary);
		if (!stream.is_open())
		{
			return { std::vector<uint8_t>{}, "Failed to open file" };
		}

		// Efficient iterator-based loading
		std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(stream)),
									std::istreambuf_iterator<char>());
		stream.close();

		return { buffer, std::nullopt };
	}

	/**
	 * @brief Saves binary data to file, creating directories as needed.
	 * 
	 * Implementation details:
	 * - Automatically creates parent directories if they don't exist
	 * - Opens file in binary mode with truncation for clean writes
	 * - Writes entire data buffer in single operation
	 * - Uses reinterpret_cast for safe binary data conversion
	 * - Explicit stream closure for reliable resource management
	 * 
	 * Directory Creation:
	 * - Extracts parent path from target file path
	 * - Creates entire directory hierarchy if needed
	 * - Uses error_code version to avoid exceptions
	 * - Fails gracefully if directory creation fails
	 * 
	 * File Writing:
	 * - Binary mode preserves data integrity
	 * - Truncation mode ensures clean file state
	 * - Single write operation for efficiency
	 * - Proper type casting for binary compatibility
	 * 
	 * @param path Target file path for saving data
	 * @param data Binary data to write to file
	 * @return std::nullopt on success, error message on failure
	 */
	auto file::save(const std::string& path,
					const std::vector<uint8_t>& data) -> std::optional<std::string>
	{
		std::filesystem::path target_path(path);
		if (!target_path.parent_path().empty())
		{
			std::error_code ec;
			std::filesystem::create_directories(target_path.parent_path(), ec);
			if (ec)
			{
				return "Failed to create directories: " + ec.message();
			}
		}

		std::ofstream stream(path, std::ios::binary | std::ios::trunc);
		if (!stream.is_open())
		{
			return "Failed to open file for writing";
		}

		stream.write(reinterpret_cast<const char*>(data.data()),
					 static_cast<std::streamsize>(data.size()));
		stream.close();

		return std::nullopt;
	}
	
	auto file::save(const std::string& path,
					span<const uint8_t> data) -> std::optional<std::string>
	{
		std::filesystem::path target_path(path);
		if (!target_path.parent_path().empty())
		{
			std::error_code ec;
			std::filesystem::create_directories(target_path.parent_path(), ec);
			if (ec)
			{
				return "Failed to create directories: " + ec.message();
			}
		}

		std::ofstream stream(path, std::ios::binary | std::ios::trunc);
		if (!stream.is_open())
		{
			return "Failed to open file for writing";
		}

		stream.write(reinterpret_cast<const char*>(data.data()),
					 static_cast<std::streamsize>(data.size()));
		stream.close();

		return std::nullopt;
	}

	auto file::append(const std::string& path,
					  const std::vector<uint8_t>& data) -> std::optional<std::string>
	{
		std::fstream stream(path, std::ios::out | std::ios::binary | std::ios::app);
		if (!stream.is_open())
		{
			return "Failed to open file for appending";
		}

		stream.write(reinterpret_cast<const char*>(data.data()),
					 static_cast<std::streamsize>(data.size()));
		stream.close();

		return std::nullopt;
	}
	
	auto file::append(const std::string& path,
					  span<const uint8_t> data) -> std::optional<std::string>
	{
		std::fstream stream(path, std::ios::out | std::ios::binary | std::ios::app);
		if (!stream.is_open())
		{
			return "Failed to open file for appending";
		}

		stream.write(reinterpret_cast<const char*>(data.data()),
					 static_cast<std::streamsize>(data.size()));
		stream.close();

		return std::nullopt;
	}
} // namespace utility_module