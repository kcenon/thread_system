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
	 * This class inherits from thread_base and provides functionality for
	 * writing log messages to a file in a separate thread. It manages file handles,
	 * backup files, and limits on the number of log lines.
	 */
	class file_writer : public thread_base
	{
	public:
		/**
		 * @brief Constructor for the file_writer class.
		 * Initializes the job queue and default settings for file writing.
		 */
		file_writer(void);

		/**
		 * @brief Sets the title for the logger.
		 * @param title The title to set.
		 */
		auto set_title(const std::string& title) -> void;

		/**
		 * @brief Gets the current title of the logger.
		 * @return The current title of the logger.
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
		 * @brief Sets the maximum number of lines to keep in the log.
		 * @param max_lines The maximum number of lines to keep in the log.
		 */
		auto set_max_lines(const uint32_t& max_lines) -> void;

		/**
		 * @brief Gets the maximum number of lines to keep in the log.
		 * @return The current maximum number of lines set for the log.
		 */
		[[nodiscard]] auto get_max_lines() const -> uint32_t;

		/**
		 * @brief Gets the job queue used by the file writer.
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
		[[nodiscard]] auto has_work() const -> bool override;

		/**
		 * @brief Performs initialization before starting the file writer thread.
		 * @return A tuple containing:
		 *         - bool: Indicates whether the initialization was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto before_start() -> std::tuple<bool, std::optional<std::string>> override;

		/**
		 * @brief Performs the main work of writing log messages to a file.
		 * @return A tuple containing:
		 *         - bool: Indicates whether the operation was successful (true) or not (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto do_work() -> std::tuple<bool, std::optional<std::string>> override;

		/**
		 * @brief Performs cleanup after stopping the file writer thread.
		 * @return A tuple containing:
		 *         - bool: Indicates whether the cleanup was successful (true) or not (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto after_stop() -> std::tuple<bool, std::optional<std::string>> override;

	protected:
		/**
		 * @brief Generates the file name for the log file.
		 * @return A tuple containing:
		 *         - std::string: The generated file name.
		 *         - std::string: The generated backup file name (if use_backup_ is true).
		 */
		[[nodiscard]] auto generate_file_name() -> std::tuple<std::string, std::string>;

		/**
		 * @brief Checks and manages the file handle for writing.
		 * Ensures that the file handle is open and ready for writing.
		 */
		auto check_file_handle(void) -> void;

		/**
		 * @brief Closes the file handle.
		 * Ensures proper closure of the file handle after writing is complete.
		 */
		auto close_file_handle(void) -> void;

		/**
		 * @brief Writes lines to the specified file.
		 * @param file_handle A unique pointer to the file stream to write to.
		 * @param messages A deque of strings to be written to the file.
		 * @return A unique pointer to the file stream after writing.
		 */
		[[nodiscard]] auto write_lines(std::unique_ptr<std::fstream> file_handle,
									   const std::deque<std::string>& messages)
			-> std::unique_ptr<std::fstream>;

	private:
		/** @brief Title of the logger */
		std::string title_;

		/** @brief Name of the current log file */
		std::string file_name_;

		/** @brief Name of the backup log file */
		std::string backup_name_;

		/** @brief Deque to store log lines before writing to file */
		std::deque<std::string> log_lines_;

		/** @brief Flag indicating whether to use a backup log file */
		bool use_backup_;

		/** @brief Maximum number of lines to keep in the log */
		uint32_t max_lines_;

		/** @brief File handle for the main log file */
		std::unique_ptr<std::fstream> log_file_;

		/** @brief File handle for the backup log file */
		std::unique_ptr<std::fstream> backup_file_;

		/** @brief Queue for log jobs to be written to a file */
		std::shared_ptr<job_queue> job_queue_;
	};
}