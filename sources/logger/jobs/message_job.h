/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this
   list of conditions and the following disclaimer in the documentation
   and/or
   other materials provided with the distribution.

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
#include "../../thread_base/sync/error_handling.h"
#include "../detail/forward_declarations.h"

#include <chrono>
#include <string>
#include <vector>
#include <optional>

using namespace thread_module;

namespace log_module
{
	/**
	 * @class message_job
	 * @brief Represents a console logging job derived from the base job class.
	 *
	 * This class encapsulates the functionality for creating and executing
	 * console logging operations as jobs within a job system (e.g., a thread pool).
	 * It primarily handles the process of writing log messages to standard output,
	 * making it easier to manage logging in a multi-threaded environment.
	 *
	 * Example usage:
	 * @code
	 * // Create a message_job instance
	 * auto my_log_job = std::make_shared<message_job>(
	 *     log_types::info,
	 *     "2025-02-14 10:00:00",
	 *     "This is an informational message."
	 * );
	 *
	 * // Submit the job to a job system or thread pool
	 * job_system.submit(my_log_job);
	 * @endcode
	 */
	class message_job : public job
	{
	public:
		/**
		 * @brief Constructs a new `message_job` object.
		 *
		 * Initializes all information needed to process a single log entry.
		 * The provided parameters determine the type of the log (severity/category),
		 * the timestamp, and the actual message to be written.
		 *
		 * @param log_type  The type of log message (e.g., debug, info, warning, error).
		 * @param datetime  A string representing the timestamp at which the log was generated.
		 * @param message   The actual log message content that will be printed to console.
		 *
		 * Example:
		 * @code
		 * message_job my_job(log_types::warning, "2025-02-14 10:00:00", "Background disk space!");
		 * @endcode
		 */
		explicit message_job(const log_types& log_type,
							 const std::string& datetime,
							 const std::string& message);

		/**
		 * @brief Retrieves the log type for the message.
		 *
		 * This method provides information on the severity or category of the log message,
		 * such as debug, info, warning, or error. It can be used to filter or handle log
		 * messages differently based on their importance.
		 *
		 * @return The log type for this particular message.
		 *
		 * @see log_types
		 */
		[[nodiscard]] auto log_type() const -> log_types;

		/**
		 * @brief Retrieves the timestamp associated with the log message.
		 *
		 * This timestamp is generally assigned at the time the log message is generated.
		 * It can be useful for tracking when events occurred in the application.
		 *
		 * @return A string containing the timestamp (e.g., "2025-02-14 10:00:00").
		 */
		[[nodiscard]] auto datetime(void) const -> std::string;

		/**
		 * @brief Retrieves the log message with an optional newline appended.
		 *
		 * Depending on your needs, you may want to keep multiple log messages on a single line,
		 * or separate them for readability. Use the parameter `append_newline` to control
		 * whether or not a newline character is included at the end of the message.
		 *
		 * @param append_newline  If true, a newline character ('\n') is appended to the message.
		 * @return The log message as a string, optionally followed by a newline.
		 *
		 * Example:
		 * @code
		 * auto msg = my_job.message();         // "This is a log message."
		 * auto msg_nl = my_job.message(true);  // "This is a log message.\n"
		 * @endcode
		 */
		[[nodiscard]] auto message(const bool& append_newline = false) const -> std::string;

		/**
		 * @brief Executes the console logging operation.
		 *
		 * This method is called (for example, by a job system or thread pool) to perform
		 * the actual console output. It typically prints the log message to `std::cout` or
		 * another standard output stream. If the operation fails for any reason (e.g., issues
		 * with the output stream), an error will be returned.
		 *
		 * @return A result_void that contains an error if the operation fails, or a success
		 *         result if it succeeds.
		 */
		[[nodiscard]] auto do_work() -> result_void override;

	private:
		/**
		 * @brief The timestamp for when the log message was created.
		 *
		 * This is typically formatted as a human-readable string (e.g., "YYYY-MM-DD HH:MM:SS"),
		 * but the format can vary based on the application's logging requirements.
		 */
		std::string datetime_;

		/**
		 * @brief The log message content to be written to the console.
		 *
		 * This string is usually a short or descriptive text that provides
		 * information about an event, state, or error occurring within the application.
		 */
		std::string message_;

		/**
		 * @brief The type of log message, indicating its category or severity.
		 *
		 * Common types might include `debug`, `info`, `warning`, `error`, etc.
		 * The `log_type` determines how the message might be formatted or filtered.
		 */
		log_types log_type_;
	};
} // namespace log_module