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

#include "log_job.h"
#include "job_queue.h"
#include "thread_base.h"

#include "log_collector.h"
#include "console_writer.h"
#include "file_writer.h"

#include <deque>
#include <mutex>
#include <memory>
#include <string>
#include <optional>

using namespace thread_module;

namespace log_module
{
	/**
	 * @class logger
	 * @brief A singleton class for managing logging operations.
	 *
	 * This class provides comprehensive logging functionality with support for both console and
	 * file output. It manages log collectors, console writers, and file writers to handle various
	 * logging tasks. The singleton pattern ensures a single point of control for all logging
	 * operations throughout the application.
	 */
	class logger
	{
	public:
		/**
		 * @brief Virtual destructor for the logger class.
		 *
		 * Ensures proper cleanup of derived classes if any.
		 */
		virtual ~logger(void) = default;

		/**
		 * @brief Sets the title for the logger.
		 * @param title The title to set for the logger.
		 *
		 * This title may be used in log file names or console output headers.
		 */
		auto set_title(const std::string& title) -> void;

		/**
		 * @brief Sets the log types that should be written to a file.
		 * @param type The log types to be written to a file.
		 *
		 * This method allows you to specify which types of log messages should be saved to a file.
		 */
		auto set_file_target(const log_types& type) -> void;

		/**
		 * @brief Gets the log types that are written to a file.
		 * @return The log types that are currently set to be written to a file.
		 */
		[[nodiscard]] auto get_file_target(void) const -> log_types;

		/**
		 * @brief Sets the log types that should be written to the console.
		 * @param type The log types to be written to the console.
		 *
		 * This method allows you to specify which types of log messages should be displayed in the
		 * console.
		 */
		auto set_console_target(const log_types& type) -> void;

		/**
		 * @brief Gets the log types that are written to the console.
		 * @return The log types that are currently set to be written to the console.
		 */
		[[nodiscard]] auto get_console_target(void) const -> log_types;

		/**
		 * @brief Sets the maximum number of lines to keep in the log.
		 * @param max_lines The maximum number of lines to keep in the log.
		 *
		 * This helps in managing the size of log files and prevents them from growing indefinitely.
		 */
		auto set_max_lines(uint32_t max_lines) -> void;

		/**
		 * @brief Gets the maximum number of lines to keep in the log.
		 * @return The current maximum number of lines set for the log.
		 */
		[[nodiscard]] auto get_max_lines(void) const -> uint32_t;

		/**
		 * @brief Sets whether to use a backup log file.
		 * @param use_backup Flag indicating whether to use a backup log file.
		 *
		 * When enabled, this feature creates a backup of the log file, which can be useful for
		 * archiving or recovery purposes.
		 */
		auto set_use_backup(bool use_backup) -> void;

		/**
		 * @brief Gets whether a backup log file is being used.
		 * @return True if a backup log file is being used, false otherwise.
		 */
		[[nodiscard]] auto get_use_backup(void) const -> bool;

		/**
		 * @brief Sets the wake interval for the logger.
		 * @param interval The wake interval to set for the logger thread.
		 *
		 * This interval determines how often the logger thread wakes up to process queued log
		 * messages.
		 */
		auto set_wake_interval(std::chrono::milliseconds interval) -> void;

		/**
		 * @brief Starts the logger.
		 * @return A tuple containing:
		 *         - bool: Indicates whether the logger was started successfully (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 *
		 * This method initializes and starts the logging process. It should be called before any
		 * logging operations.
		 */
		auto start(void) -> std::tuple<bool, std::optional<std::string>>;

		/**
		 * @brief Stops the logger.
		 *
		 * This method halts all logging operations and performs necessary cleanup.
		 */
		auto stop(void) -> void;

		/**
		 * @brief Gets the current time point.
		 * @return The current time point using high resolution clock.
		 *
		 * This method is useful for precise timing of log events.
		 */
		auto time_point(void) -> std::chrono::time_point<std::chrono::high_resolution_clock>;

		/**
		 * @brief Writes a log message.
		 * @param type The type of the log message.
		 * @param message The content of the log message.
		 * @param start_time An optional start time for the log message.
		 *
		 * This overload handles std::string messages.
		 */
		auto write(
			log_types type,
			const std::string& message,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt) -> void;

		/**
		 * @brief Writes a log message.
		 * @param type The type of the log message.
		 * @param message The content of the log message.
		 * @param start_time An optional start time for the log message.
		 *
		 * This overload handles std::wstring messages.
		 */
		auto write(
			log_types type,
			const std::wstring& message,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt) -> void;

		/**
		 * @brief Writes a log message.
		 * @param type The type of the log message.
		 * @param message The content of the log message.
		 * @param start_time An optional start time for the log message.
		 *
		 * This overload handles std::u16string messages.
		 */
		auto write(
			log_types type,
			const std::u16string& message,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt) -> void;

		/**
		 * @brief Writes a log message.
		 * @param type The type of the log message.
		 * @param message The content of the log message.
		 * @param start_time An optional start time for the log message.
		 *
		 * This overload handles std::u32string messages.
		 */
		auto write(
			log_types type,
			const std::u32string& message,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt) -> void;

	private:
		/**
		 * @brief Private constructor for the logger class (singleton pattern).
		 *
		 * Initializes the log collector, console writer, and file writer.
		 */
		logger();

		/**
		 * @brief Deleted copy constructor to enforce singleton pattern.
		 */
		logger(const logger&) = delete;

		/**
		 * @brief Deleted assignment operator to enforce singleton pattern.
		 */
		logger& operator=(const logger&) = delete;

	private:
		/** @brief Shared pointer to the log collector */
		std::shared_ptr<log_collector> collector_;

		/** @brief Shared pointer to the console writer */
		std::shared_ptr<console_writer> console_writer_;

		/** @brief Shared pointer to the file writer */
		std::shared_ptr<file_writer> file_writer_;

#pragma region singleton
	public:
		/**
		 * @brief Gets the singleton instance of the logger.
		 * @return Reference to the singleton logger instance.
		 *
		 * This method ensures that only one instance of the logger exists throughout the
		 * application.
		 */
		static auto handle() -> logger&;

		/**
		 * @brief Destroys the singleton instance of the logger.
		 *
		 * This method should be called when the logger is no longer needed, typically at
		 * application shutdown.
		 */
		static auto destroy() -> void;

	private:
		/** @brief Singleton instance of the logger */
		static std::unique_ptr<logger> handle_;

		/** @brief Flag to ensure singleton is initialized only once */
		static std::once_flag once_;
#pragma endregion
	};
} // namespace log_module