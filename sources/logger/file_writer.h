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
#include "job_queue.h"
#include "thread_base.h"

#include <deque>
#include <fstream>

using namespace thread_module;

namespace log_module
{
	/**
	 * @class file_writer
	 * @brief A class for writing log messages to a file.
	 *
	 * This class inherits from `thread_base` and provides functionality for
	 * writing log messages to a file in a separate thread. It manages file handles,
	 * backup files, and limits on the number of log lines, and allows users to set a specific
	 * log type for filtering messages written to the file.
	 */
	class file_writer : public thread_base
	{
	public:
		/**
		 * @brief Constructor for the `file_writer` class.
		 *
		 * Initializes the job queue and default settings for file writing.
		 */
		file_writer(void);

		/**
		 * @brief Sets the title for the log file.
		 * @param title The title to set.
		 */
		auto set_title(const std::string& title) -> void;

		/**
		 * @brief Gets the current title of the log file.
		 * @return The current title of the log file.
		 */
		[[nodiscard]] auto get_title() const -> std::string;

		/**
		 * @brief Sets whether to use a backup log file.
		 * @param use_backup Flag indicating whether to use a backup log file.
		 */
		auto set_use_backup(const bool& use_backup) -> void;

		/**
		 * @brief Gets whether a backup log file is being used.
		 * @return True if a backup log file is being used, false otherwise.
		 */
		[[nodiscard]] auto get_use_backup() const -> bool;

		/**
		 * @brief Sets the maximum number of lines to retain in the log.
		 * @param max_lines The maximum number of lines to keep in the log.
		 */
		auto set_max_lines(const uint32_t& max_lines) -> void;

		/**
		 * @brief Gets the maximum number of lines to keep in the log.
		 * @return The current maximum number of lines set for the log.
		 */
		[[nodiscard]] auto get_max_lines() const -> uint32_t;

		/**
		 * @brief Sets the log type filter for the file writer.
		 *
		 * Defines the type of logs that should be written to the file, allowing for
		 * selective logging based on log severity or category.
		 *
		 * @param type The log type to set for filtering log messages.
		 */
		auto file_target(const log_types& type) -> void;

		/**
		 * @brief Gets the current log type filter for the file writer.
		 * @return The log type currently set for file output filtering.
		 */
		[[nodiscard]] auto file_target() const -> log_types { return file_target_; }

		/**
		 * @brief Retrieves the job queue used by the file writer.
		 *
		 * Provides access to the job queue containing tasks for writing log messages to a file.
		 *
		 * @return A shared pointer to the job queue.
		 */
		[[nodiscard]] auto get_job_queue() const -> std::shared_ptr<job_queue>
		{
			return job_queue_;
		}

		/**
		 * @brief Checks if there is work to be done in the job queue.
		 * @return True if there are jobs in the queue, false otherwise.
		 */
		[[nodiscard]] auto should_continue_work() const -> bool override;

		/**
		 * @brief Performs initialization before starting the file writer thread.
		 *
		 * Ensures that file resources and configurations are properly set up before
		 * beginning to write logs.
		 *
		 * @return A tuple containing:
		 *         - bool: Indicates whether the initialization was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional error message if initialization fails.
		 */
		auto before_start() -> std::optional<std::string> override;

		/**
		 * @brief Writes log messages to the file.
		 *
		 * Processes the log messages in the job queue and writes them to the designated log file.
		 *
		 * @return A tuple containing:
		 *         - bool: Indicates whether the writing operation was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional error message if writing fails.
		 */
		auto do_work() -> std::optional<std::string> override;

		/**
		 * @brief Performs cleanup after stopping the file writer thread.
		 *
		 * Closes file handles and releases resources associated with the file writer.
		 *
		 * @return A tuple containing:
		 *         - bool: Indicates whether the cleanup was successful (true) or not (false).
		 *         - std::optional<std::string>: An optional error message if cleanup fails.
		 */
		auto after_stop() -> std::optional<std::string> override;

	protected:
		/**
		 * @brief Generates the primary and backup file names for the log file.
		 *
		 * Creates the file names based on the title and backup configuration.
		 *
		 * @return A tuple containing:
		 *         - std::string: The generated primary file name.
		 *         - std::string: The generated backup file name (if `use_backup_` is true).
		 */
		[[nodiscard]] auto generate_file_name() -> std::tuple<std::string, std::string>;

		/**
		 * @brief Ensures the file handle is ready for writing.
		 *
		 * Opens or reopens the file handle as needed to prepare for log writing.
		 */
		auto check_file_handle(void) -> void;

		/**
		 * @brief Closes the file handle.
		 *
		 * Ensures proper closure of the file handle after writing is complete.
		 */
		auto close_file_handle(void) -> void;

		/**
		 * @brief Writes lines to the specified file.
		 *
		 * Processes a set of log messages and writes them to the provided file stream.
		 *
		 * @param file_handle A unique pointer to the file stream to write to.
		 * @param messages A deque of strings to be written to the file.
		 * @return A unique pointer to the file stream after writing.
		 */
		[[nodiscard]] auto write_lines(std::unique_ptr<std::fstream> file_handle,
									   const std::deque<std::string>& messages)
			-> std::unique_ptr<std::fstream>;

	private:
		std::string title_;						 ///< Title used in the log file name
		std::string file_name_;					 ///< Name of the current log file
		std::string backup_name_;				 ///< Name of the backup log file (if enabled)
		std::deque<std::string> log_lines_;		 ///< Queue storing log lines to write

		bool use_backup_;						 ///< Indicates if a backup log file is in use
		log_types file_target_;					 ///< Log type filter for writing to file
		uint32_t max_lines_;					 ///< Maximum number of lines to retain in the log

		std::unique_ptr<std::fstream> log_file_; ///< File handle for the main log file
		std::unique_ptr<std::fstream> backup_file_; ///< File handle for the backup log file

		std::shared_ptr<job_queue> job_queue_;		///< Queue for log tasks to write to the file
	};
} // namespace log_module
