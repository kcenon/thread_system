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

#include "log_types.h"
#include "../thread_base/jobs/job_queue.h"
#include "../thread_base/core/thread_base.h"

#include <deque>
#include <fstream>

using namespace thread_module;

namespace log_module
{
	/**
	 * @class file_writer
	 * @brief Writes log messages to a file in a dedicated thread.
	 *
	 * This class extends `thread_base` to process and write log messages to a file
	 * asynchronously. It manages file handles, optional backup files, and enforces
	 * a maximum line limit. A log type filter can be used to selectively write only
	 * certain log messages.
	 */
	class file_writer : public thread_base
	{
	public:
		/**
		 * @brief Constructs a new file_writer.
		 *
		 * Initializes the underlying job queue and default file writing settings.
		 */
		file_writer(void);

		/**
		 * @brief Sets the log file title.
		 * @param title The new title for the log file.
		 */
		auto set_title(const std::string& title) -> void;

		/**
		 * @brief Retrieves the current log file title.
		 * @return The current title as a string.
		 */
		[[nodiscard]] auto get_title() const -> std::string;

		/**
		 * @brief Enables or disables the use of a backup log file.
		 * @param use_backup True to enable backup logging, false to disable.
		 */
		auto set_use_backup(const bool& use_backup) -> void;

		/**
		 * @brief Checks if backup logging is enabled.
		 * @return True if backup logging is enabled, false otherwise.
		 */
		[[nodiscard]] auto get_use_backup() const -> bool;

		/**
		 * @brief Specifies the maximum number of lines the log file can hold.
		 * @param max_lines The line count limit.
		 */
		auto set_max_lines(const uint32_t& max_lines) -> void;

		/**
		 * @brief Retrieves the current maximum line limit for the log file.
		 * @return The maximum number of lines allowed.
		 */
		[[nodiscard]] auto get_max_lines() const -> uint32_t;

		/**
		 * @brief Sets the filter for log message types.
		 *
		 * Only messages matching the specified type will be written to the file.
		 *
		 * @param type The log type to filter on.
		 */
		auto file_target(const log_types& type) -> void;

		/**
		 * @brief Returns the current log type filter.
		 * @return The log type used to filter output messages.
		 */
		[[nodiscard]] auto file_target() const -> log_types { return file_target_; }

		/**
		 * @brief Provides access to the job queue.
		 *
		 * The job queue holds tasks that will be written to the log file.
		 *
		 * @return A shared pointer to the job queue.
		 */
		[[nodiscard]] auto get_job_queue() const -> std::shared_ptr<job_queue>
		{
			return job_queue_;
		}

		/**
		 * @brief Determines if there are pending tasks to process.
		 * @return True if the job queue is not empty, false otherwise.
		 */
		[[nodiscard]] auto should_continue_work() const -> bool override;

		/**
		 * @brief Prepares resources before starting the file writer thread.
		 *
		 * Ensures that the necessary file resources and configurations are in place
		 * before logging can begin.
		 *
		 * @return A result_void. If it contains an error, initialization failed;
		 *         otherwise, initialization succeeded.
		 */
		auto before_start() -> result_void override;

		/**
		 * @brief Processes and writes pending log messages to the file.
		 *
		 * Retrieves log tasks from the job queue and writes them to the configured
		 * log file(s).
		 *
		 * @return A result_void. If it contains an error, an error occurred;
		 *         otherwise, the operation succeeded.
		 */
		auto do_work() -> result_void override;

		/**
		 * @brief Cleans up resources after stopping the file writer thread.
		 *
		 * Closes open file handles and releases resources.
		 *
		 * @return A result_void. If it contains an error, an error occurred;
		 *         otherwise, cleanup succeeded.
		 */
		auto after_stop() -> result_void override;

	protected:
		/**
		 * @brief Generates the primary and backup log file names.
		 *
		 * Uses the current title and backup configuration to create both file names.
		 *
		 * @return A tuple: (primary_file_name, backup_file_name).
		 *         The backup file name is meaningful only if backup is enabled.
		 */
		[[nodiscard]] auto generate_file_name() -> std::tuple<std::string, std::string>;

		/**
		 * @brief Ensures the log file handle is open and ready for writing.
		 *
		 * Opens or reopens the file handles if necessary before writing logs.
		 */
		auto check_file_handle(void) -> void;

		/**
		 * @brief Closes the log file handles.
		 *
		 * Ensures that all log file streams are properly closed.
		 */
		auto close_file_handle(void) -> void;

		/**
		 * @brief Writes a collection of messages to the given file stream.
		 *
		 * Processes the specified log messages and appends them to the file.
		 *
		 * @param file_handle Unique pointer to the file stream to write to.
		 * @param messages A deque of log messages to be written.
		 * @return The file stream, which remains valid after writing.
		 */
		[[nodiscard]] auto write_lines(std::unique_ptr<std::fstream> file_handle,
								   const std::deque<std::string>& messages)
			-> std::unique_ptr<std::fstream>;

	private:
		std::string title_;                       ///< Title used in the log file name.
		std::string file_name_;                   ///< Name of the current log file.
		std::string backup_name_;                ///< Name of the backup log file (if enabled).
		std::deque<std::string> log_lines_;       ///< Queue storing log lines to write.

		bool use_backup_;                       ///< Indicates if a backup log file is in use.
		log_types file_target_;                   ///< Log type filter for writing to file.
		uint32_t max_lines_;                    ///< Maximum number of lines to retain in the log.

		std::unique_ptr<std::fstream> log_file_; ///< File handle for the main log file.
		std::unique_ptr<std::fstream> backup_file_; ///< File handle for the backup log file.

		std::shared_ptr<job_queue> job_queue_;      ///< Queue for log tasks to write to the file.
	};
} // namespace log_module