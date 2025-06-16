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

#include "../../thread_base/jobs/job.h"
#include "../types/log_types.h"
#include "../../thread_base/synchronization/error_handling.h"
#include "../detail/forward_declarations.h"

#include <chrono>
#include <string>
#include <vector>
#include <optional>

using namespace thread_module;

namespace log_module
{
	/**
	 * @class log_job
	 * @brief Represents a discrete logging task that can be scheduled and executed within
	 *        a concurrent job system.
	 *
	 * The @c log_job class extends the base @c job class to provide specialized handling
	 * for logging operations. Each @c log_job stores a message in one of several string
	 * encodings, along with optional metadata such as log type (e.g., info, warning, error)
	 * and timing information. When executed, it converts the message into a standardized
	 * format (including timestamps) and makes it available for output.
	 *
	 * This design enables asynchronous or concurrent logging, allowing the main application
	 * threads to enqueue log messages for later processing, thus avoiding potential
	 * performance bottlenecks.
	 */
	class log_job : public job
	{
	public:
		/**
		 * @brief Constructs a @c log_job using an @c std::string.
		 *
		 * @param message
		 *   The primary log message to be recorded.
		 * @param type
		 *   An optional log type indicating the message category or severity (e.g., info,
		 *   warning, error). If not provided, a default or neutral type may be assumed.
		 * @param start_time
		 *   An optional start time used to compute elapsed durations if the log entry
		 *   records a measured operation (e.g., function execution time).
		 */
		explicit log_job(
			const std::string& message,
			std::optional<log_types> type = std::nullopt,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt);

		/**
		 * @brief Constructs a @c log_job using an @c std::wstring.
		 *
		 * @param message
		 *   The primary log message to be recorded in wide-character form.
		 * @param type
		 *   An optional log type indicating the message category or severity.
		 * @param start_time
		 *   An optional start time used to compute elapsed durations if the log entry
		 *   records a measured operation.
		 */
		explicit log_job(
			const std::wstring& message,
			std::optional<log_types> type = std::nullopt,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt);

		/**
		 * @brief Executes the logging operation within the job system.
		 *
		 * In this method, the stored message is converted to a standardized format and
		 * timestamped. This final, formatted output is typically passed to a logging
		 * backend (e.g., console, file, or network). The method returns a result_void
		 * indicating success or failure.
		 *
		 * @return
		 *   - A success result if the operation completes successfully,
		 *   - An error result containing the error description if it fails.
		 *
		 * @note
		 *   This overrides the base class method @c do_work() from @c job.
		 */
		[[nodiscard]] auto do_work() -> result_void override;

		/**
		 * @brief Retrieves the log type of this entry.
		 *
		 * @return
		 *   The @c log_types enumeration value representing the entry's severity or
		 *   category (e.g., info, warning, error). If no log type was specified upon
		 *   construction, this may return a default or generic type.
		 */
		[[nodiscard]] auto get_type() const -> log_types;

		/**
		 * @brief Obtains the date and time string associated with this log entry.
		 *
		 * The date and time are typically generated when the @c log_job is created,
		 * formatted into a human-readable form. This is useful for correlating logs
		 * in chronological order.
		 *
		 * @return
		 *   A string containing the formatted local date and time of the log entry.
		 */
		[[nodiscard]] auto datetime() const -> std::string;

		/**
		 * @brief Retrieves the fully formatted log message.
		 *
		 * This includes the original message text and any additional prefix or suffix
		 * such as timestamps, log type indicators, or runtime durations (if applicable).
		 *
		 * @return
		 *   A formatted string representation of the log message, ready for output.
		 */
		[[nodiscard]] auto message() const -> std::string;

	protected:
		/**
		 * @brief Converts the stored message data (in various encodings) to @c std::string.
		 *
		 * Internal helper function used to ensure that all character types
		 * (@c std::wstring, @c std::u16string, @c std::u32string) can be normalized into
		 * a standard string for uniform handling. Depending on the underlying system, this
		 * may involve character transcoding.
		 *
		 * @return
		 *   A UTF-8 (or system-default) @c std::string version of the message.
		 */
		[[nodiscard]] auto convert_message() const -> std::string;

	private:
		/**
		 * @enum message_types
		 * @brief Identifies which string variant is in use for the stored log message.
		 *
		 * Each @c log_job can store exactly one of the following string encodings, as
		 * specified during construction.
		 */
		enum class message_types : uint8_t
		{
			String,						///< Message is stored as @c std::string
			WString,					///< Message is stored as @c std::wstring
			U16String,					///< Message is stored as @c std::u16string
			U32String,					///< Message is stored as @c std::u32string
		};

		message_types message_type_;	///< Indicates which encoding is used by the stored message.

		std::string message_;			///< Raw message for the @c std::string variant.
		std::wstring wmessage_;			///< Raw message for the @c std::wstring variant.
		std::u16string u16message_;		///< Raw message for the @c std::u16string variant.
		std::u32string u32message_;		///< Raw message for the @c std::u32string variant.

		std::string datetime_;			///< Formatted date and time of log creation.
		std::string log_message_;		///< Final log message string, prepared by @c do_work().

		std::optional<log_types> type_; ///< Optional severity/category type for this log entry.

		/**
		 * @brief Timestamp indicating when the log job was instantiated.
		 *
		 * This is set to the current system time at construction, helping track when
		 * the log message was created, even if the job is executed later.
		 */
		std::chrono::system_clock::time_point timestamp_;

		/**
		 * @brief Optional start time for measuring durations.
		 *
		 * If provided, the difference between @c start_time_ and @c timestamp_ can be
		 * calculated to log how long a particular operation has taken up to this point.
		 */
		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time_;
	};
} // namespace log_module