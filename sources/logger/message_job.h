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
	 * @class message_job
	 * @brief Represents a console logging job derived from the base job class.
	 *
	 * This class encapsulates the functionality for creating and executing
	 * console logging operations as jobs within the job system. It handles
	 * writing log messages to the console output.
	 */
	class message_job : public job
	{
	public:
		/**
		 * @brief Constructs a new `message_job` object.
		 * @param log_type The type of log message, indicating the severity or category.
		 * @param datetime The timestamp for when the log message was created.
		 * @param message The actual log message content to be written to the console.
		 */
		explicit message_job(const log_types& log_type,
							 const std::string& datetime,
							 const std::string& message);

		/**
		 * @brief Retrieves the log type for the message.
		 * @return The log type for the message, indicating its severity or category.
		 */
		[[nodiscard]] auto log_type() const -> log_types;

		/**
		 * @brief Retrieves the timestamp associated with the log message.
		 * @return The datetime of the log message.
		 */
		[[nodiscard]] auto datetime(void) const -> std::string;

		/**
		 * @brief Retrieves the log message with optional newline.
		 * @param append_newline If true, a newline character will be appended to the message.
		 * @return The log message as a string, with an optional newline appended.
		 */
		[[nodiscard]] auto message(const bool& append_newline = false) const -> std::string;

		/**
		 * @brief Executes the console logging operation.
		 *
		 * Handles the output of the log message to the console, ensuring
		 * that the message format includes the timestamp and log type for clarity.
		 *
		 * @return A tuple containing:
		 *         - bool: Indicates whether the logging operation was successful.
		 *         - std::optional<std::string>: An optional error description if the operation
		 * failed.
		 */
		[[nodiscard]] auto do_work() -> std::tuple<bool, std::optional<std::string>> override;

	private:
		/** @brief The timestamp for when the log message was created. */
		std::string datetime_;

		/** @brief The log message content to be written to the console. */
		std::string message_;

		/** @brief The type of log message, indicating its category or severity. */
		log_types log_type_;
	};
} // namespace log_module
