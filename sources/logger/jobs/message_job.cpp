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

#include "message_job.h"

#include "../../utilities/core/formatter.h"

/**
 * @file message_job.cpp
 * @brief Implementation of formatted message job for log output writers.
 *
 * This file contains the implementation of the message_job class, which represents
 * a fully formatted log message ready for output. Message jobs are created after
 * log_job processing and contain the final formatted message with timestamp and
 * log level information.
 * 
 * Key Features:
 * - Stores pre-formatted log messages for output writers
 * - Maintains log level classification for filtering
 * - Provides flexible message retrieval with optional newline
 * - Lightweight validation during job execution
 * - Minimal processing overhead for output operations
 * 
 * Usage in Logging Pipeline:
 * 1. log_job formats raw message with timestamp and timing
 * 2. message_job created with formatted result
 * 3. message_job dispatched to appropriate output writers
 * 4. Writers retrieve formatted message and write to destination
 * 
 * Design Rationale:
 * - Separates message formatting from output writing
 * - Enables parallel processing of formatted messages
 * - Supports multiple output writers for same message
 * - Maintains clean separation of concerns
 */

using namespace utility_module;

namespace log_module
{
	/**
	 * @brief Constructs a message job with formatted log data.
	 * 
	 * Implementation details:
	 * - Stores pre-formatted message and timestamp from log_job processing
	 * - Maintains log level for filtering and routing decisions
	 * - No additional formatting or processing required during construction
	 * - Designed for efficient creation and dispatch to output writers
	 * 
	 * Message Data:
	 * - datetime: Formatted timestamp string with full precision
	 * - message: Complete formatted message including brackets and timing
	 * - log_type: Classification for filtering and output routing
	 * 
	 * @param log_type Log level classification (info, warning, error, etc.)
	 * @param datetime Pre-formatted timestamp string
	 * @param message Complete formatted log message
	 */
	message_job::message_job(const log_types& log_type,
							 const std::string& datetime,
							 const std::string& message)
		: job("message_job"), datetime_(datetime), message_(message), log_type_(log_type)
	{
	}

	auto message_job::log_type() const -> log_types { return log_type_; }

	auto message_job::datetime() const -> std::string { return datetime_; }

	/**
	 * @brief Retrieves the formatted message with optional newline.
	 * 
	 * Implementation details:
	 * - Returns stored formatted message as-is when newline not requested
	 * - Appends newline character when requested for output formatting
	 * - Uses efficient formatter for newline appending
	 * - Thread-safe operation (read-only access to immutable data)
	 * 
	 * Output Formatting:
	 * - Without newline: Direct message content for custom formatting
	 * - With newline: Ready for direct output to files or console
	 * - Consistent formatting across different output writers
	 * 
	 * @param append_newline If true, appends newline character to message
	 * @return Formatted message string with or without newline
	 */
	auto message_job::message(const bool& append_newline) const -> std::string
	{
		if (!append_newline)
		{
			return message_;
		}

		return formatter::format("{}\n", message_);
	}

	/**
	 * @brief Validates the message job before output processing.
	 * 
	 * Implementation details:
	 * - Performs minimal validation on stored message content
	 * - Ensures message is not empty before output processing
	 * - Lightweight operation suitable for high-frequency execution
	 * - No actual message processing or formatting performed
	 * 
	 * Validation Logic:
	 * - Empty message check prevents invalid output
	 * - Returns success for valid messages
	 * - Returns error for invalid/empty messages
	 * 
	 * Design Rationale:
	 * - Catches formatting errors from previous processing stages
	 * - Provides consistent error handling in logging pipeline
	 * - Minimal overhead for normal operation paths
	 * 
	 * @return result_void indicating validation success or failure
	 */
	auto message_job::do_work() -> result_void
	{
		if (message_.empty())
		{
			return error{error_code::job_execution_failed, "empty message"};
		}

		return result_void{};
	}
} // namespace log_module