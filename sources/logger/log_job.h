/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
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

#include "job.h"
#include "log_types.h"

#include <chrono>
#include <string>
#include <vector>
#include <optional>

using namespace thread_module;

namespace log_module
{
	/**
	 * @class log_job
	 * @brief Represents a logging job derived from the base job class.
	 *
	 * This class encapsulates the functionality for creating and executing
	 * logging operations as jobs within the job system. It supports different
	 * log types and optional start times for more detailed logging.
	 * The class handles various string types (std::string, std::wstring, std::u16string,
	 * std::u32string) to accommodate different character encodings.
	 */
	class log_job : public job
	{
	public:
		/**
		 * @brief Constructs a new log_job object.
		 * @param message The log message to be recorded.
		 * @param type An optional parameter specifying the type of log entry.
		 * @param start_time An optional parameter specifying the start time of the log entry.
		 *        Used for calculating duration of timed operations.
		 */
		explicit log_job(
			const std::string& message,
			std::optional<log_types> type = std::nullopt,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt);

#ifdef _WIN32_BUT_NOT_TESTED
		/**
		 * @brief Constructs a new log_job object with wide string message.
		 * @param message The log message to be recorded as a wide string.
		 * @param type An optional parameter specifying the type of log entry.
		 * @param start_time An optional parameter specifying the start time of the log entry.
		 *        Used for calculating duration of timed operations.
		 */
		explicit log_job(
			const std::wstring& message,
			std::optional<log_types> type = std::nullopt,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt);
#endif

		/**
		 * @brief Constructs a new log_job object with UTF-16 string message.
		 * @param message The log message to be recorded as a UTF-16 string.
		 * @param type An optional parameter specifying the type of log entry.
		 * @param start_time An optional parameter specifying the start time of the log entry.
		 *        Used for calculating duration of timed operations.
		 */
		explicit log_job(
			const std::u16string& message,
			std::optional<log_types> type = std::nullopt,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt);

		/**
		 * @brief Constructs a new log_job object with UTF-32 string message.
		 * @param message The log message to be recorded as a UTF-32 string.
		 * @param type An optional parameter specifying the type of log entry.
		 * @param start_time An optional parameter specifying the start time of the log entry.
		 *        Used for calculating duration of timed operations.
		 */
		explicit log_job(
			const std::u32string& message,
			std::optional<log_types> type = std::nullopt,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt);

		/**
		 * @brief Executes the logging operation.
		 * @return A tuple containing:
		 *         - bool: Indicates whether the logging operation was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 * @note This method overrides the base class's do_work() method.
		 */
		[[nodiscard]] auto do_work() -> std::tuple<bool, std::optional<std::string>> override;

		/**
		 * @brief Gets the type of the log entry.
		 * @return The type of the log entry. If no type was specified during construction,
		 *         a default type may be returned.
		 */
		[[nodiscard]] auto get_type() const -> log_types;

		/**
		 * @brief Gets the formatted log message.
		 * @return The formatted log message as a string, ready for output.
		 */
		[[nodiscard]] auto message() const -> std::string;

	protected:
		/**
		 * @brief Converts the stored message to a std::string.
		 * @return The converted message as a std::string.
		 */
		[[nodiscard]] auto convert_message() const -> std::string;

	private:
		/**
		 * @brief Enumeration of supported message string types.
		 */
		enum class message_types : uint8_t
		{
			String,					 ///< std::string log type
			WString,				 ///< std::wstring log type
			U16String,				 ///< std::u16string log type
			U32String,				 ///< std::u32string log type
		};

		message_types message_type_; ///< The type of the stored message

		std::string message_;		 ///< The original unformatted log message (for std::string)
		std::wstring wmessage_;		 ///< The original unformatted log message (for std::wstring)
		std::u16string u16message_;	 ///< The original unformatted log message (for std::u16string)
		std::u32string u32message_;	 ///< The original unformatted log message (for std::u32string)

		std::string log_message_;	 ///< The formatted log message ready for output

		std::optional<log_types> type_; ///< The type of the log entry (e.g., info, warning, error)

		std::chrono::system_clock::time_point
			timestamp_;					///< The timestamp of when the log job was created

		/** @brief The optional start time of the log entry for duration calculations */
		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time_;
	};
} // namespace log_module