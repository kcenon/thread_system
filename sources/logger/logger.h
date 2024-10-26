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

#include "formatter.h"
#include "log_collector.h"
#include "console_writer.h"
#include "file_writer.h"

#include <deque>
#include <mutex>
#include <memory>
#include <string>
#include <optional>

using namespace thread_module;
using namespace utility_module;

namespace log_module
{
	/**
	 * @class logger
	 * @brief A singleton class for managing logging operations.
	 *
	 * Provides a unified logging mechanism with support for both console and file output.
	 * Manages log collectors, console writers, and file writers to handle various logging
	 * tasks, ensuring consistent logging throughout the application.
	 * Uses a singleton pattern for a single, central logging instance.
	 */
	class logger
	{
	public:
		/**
		 * @brief Virtual destructor to ensure proper cleanup of the logger instance.
		 */
		virtual ~logger(void) = default;

		/**
		 * @brief Sets a title for the logger, used in log file names or console output.
		 * @param title A string representing the title for the logger.
		 */
		auto set_title(const std::string& title) -> void;

		/**
		 * @brief Defines the log types that should be written to a file.
		 * @param type A `log_types` value indicating the types of log messages to save to file.
		 */
		auto set_file_target(const log_types& type) -> void;

		/**
		 * @brief Retrieves the current log types that are written to a file.
		 * @return The `log_types` currently set for file output.
		 */
		[[nodiscard]] auto get_file_target(void) const -> log_types;

		/**
		 * @brief Defines the log types that should be written to the console.
		 * @param type A `log_types` value indicating the types of log messages to display in the
		 * console.
		 */
		auto set_console_target(const log_types& type) -> void;

		/**
		 * @brief Retrieves the current log types that are written to the console.
		 * @return The `log_types` currently set for console output.
		 */
		[[nodiscard]] auto get_console_target(void) const -> log_types;

		/**
		 * @brief Sets the maximum number of lines to retain in the log.
		 * @param max_lines Maximum number of lines to retain, helping manage log size.
		 */
		auto set_max_lines(uint32_t max_lines) -> void;

		/**
		 * @brief Retrieves the maximum number of lines configured for the log.
		 * @return The maximum number of lines to retain in the log.
		 */
		[[nodiscard]] auto get_max_lines(void) const -> uint32_t;

		/**
		 * @brief Configures whether to create a backup log file.
		 * @param use_backup A boolean indicating if a backup log file should be used.
		 *
		 * When enabled, the logger creates a backup of the log file for archiving or recovery
		 * purposes.
		 */
		auto set_use_backup(bool use_backup) -> void;

		/**
		 * @brief Checks if backup logging is enabled.
		 * @return `true` if a backup log file is in use, `false` otherwise.
		 */
		[[nodiscard]] auto get_use_backup(void) const -> bool;

		/**
		 * @brief Sets the interval at which the logger checks for new messages to log.
		 * @param interval A `std::chrono::milliseconds` value defining the wake interval.
		 */
		auto set_wake_interval(std::chrono::milliseconds interval) -> void;

		/**
		 * @brief Starts the logger instance.
		 * @return A tuple where:
		 *         - The first element is a boolean indicating whether the logger started
		 * successfully.
		 *         - The second element is an optional string with an error description, if
		 * applicable.
		 *
		 * Call this method to initialize and begin logging. Must be called before any log
		 * operations.
		 */
		auto start(void) -> std::tuple<bool, std::optional<std::string>>;

		/**
		 * @brief Stops the logger instance.
		 *
		 * Stops all logging operations and performs cleanup as needed.
		 */
		auto stop(void) -> void;

		/**
		 * @brief Gets the current time point using a high-resolution clock.
		 * @return The current time point, useful for precise log timing.
		 */
		auto time_point(void) -> std::chrono::time_point<std::chrono::high_resolution_clock>;

		/**
		 * @brief Writes a formatted log message to the log collector.
		 *
		 * Formats and logs a message with a specified log type. Can optionally include a
		 * timestamp representing the start time of the log event.
		 *
		 * @tparam Args Types of arguments used to fill in placeholders in the format string.
		 *
		 * @param type The log type (e.g., info, warning, error) used for categorizing the message.
		 * @param formats Format string defining the log message structure and placeholders.
		 * @param args Additional arguments for each placeholder in `formats`.
		 */
		template <typename... Args>
		auto log(const log_types& type, format_string<Args...> formats, Args&&... args) -> void
		{
			if (collector_ == nullptr)
			{
				return;
			}

			if (collector_->get_file_target() < type && collector_->get_console_target() < type)
			{
				return;
			}

			collector_->write(type,
							  formatter::format(std::move(formats), std::forward<Args>(args)...));
		}

		/**
		 * @brief Writes a formatted wide-character log message to the log collector.
		 *
		 * Formats and logs a message with a specified log type. Can optionally include a
		 * timestamp representing the start time of the log event.
		 *
		 * @tparam Args Types of arguments used to fill in placeholders in the wide format string.
		 *
		 * @param type The log type (e.g., info, warning, error) used for categorizing the message.
		 * @param formats A wide-character format string with placeholders.
		 * @param args Additional arguments for each placeholder in `formats`.
		 */
		template <typename... WideArgs>
		auto wlog(const log_types& type,
				  wformat_string<WideArgs...> formats,
				  WideArgs&&... args) -> void
		{
			if (collector_ == nullptr)
			{
				return;
			}

			if (collector_->get_file_target() < type && collector_->get_console_target() < type)
			{
				return;
			}

			collector_->write(
				type, formatter::format(std::move(formats), std::forward<WideArgs>(args)...));
		}

		/**
		 * @brief Writes a formatted wide-character log message to the log collector.
		 *
		 * Formats and logs a message with a specified log type. Can optionally include a
		 * timestamp representing the start time of the log event.
		 *
		 * @tparam Args Types of arguments used to fill in placeholders in the wide format string.
		 *
		 * @param type The log type (e.g., info, warning, error) used for categorizing the message.
		 * @param formats A wide character string literal containing the format string with
		 * placeholders.
		 * @param args Additional arguments for each placeholder in `formats`.
		 */
		template <typename... WideArgs>
		auto wlog(const log_types& type, const wchar_t* formats, WideArgs&&... args) -> void
		{
			if (collector_ == nullptr)
			{
				return;
			}

			if (collector_->get_file_target() < type && collector_->get_console_target() < type)
			{
				return;
			}

			collector_->write(type, formatter::format(formats, std::forward<WideArgs>(args)...));
		}

		/**
		 * @brief Writes a formatted log message to the log collector, with optional timestamp.
		 *
		 * Formats and logs a message with a specified type and optional start timestamp.
		 * Enables accurate timing for each log entry when a timestamp is provided.
		 *
		 * @tparam Args Types of arguments used to fill in placeholders in the format string.
		 *
		 * @param type The log type (e.g., info, warning, error) used for categorizing the message.
		 * @param start_time An optional timestamp marking the start of the log event.
		 * @param formats Format string defining the log message structure and placeholders.
		 * @param args Additional arguments matching the placeholders in `formats`.
		 */
		template <typename... Args>
		auto log_timestamp(const log_types& type,
						   std::chrono::time_point<std::chrono::high_resolution_clock> start_time,
						   format_string<Args...> formats,
						   Args&&... args) -> void
		{
			if (collector_ == nullptr)
			{
				return;
			}

			if (collector_->get_file_target() < type && collector_->get_console_target() < type)
			{
				return;
			}

			collector_->write(type,
							  formatter::format(std::move(formats), std::forward<Args>(args)...),
							  start_time);
		}

		/**
		 * @brief Writes a formatted wide-character log message with an optional timestamp.
		 *
		 * Formats and logs a message with a specified type and optional start timestamp.
		 * Enables accurate timing for each log entry when a timestamp is provided.
		 *
		 * @tparam Args Types of arguments used to fill in placeholders in the wide format string.
		 *
		 * @param type The log type (e.g., info, warning, error) used for categorizing the message.
		 * @param start_time An optional timestamp marking the start of the log event.
		 * @param formats A wide-character format string with placeholders.
		 * @param args Additional arguments matching the placeholders in `formats`.
		 */
		template <typename... WideArgs>
		auto wlog_timestamp(const log_types& type,
							std::chrono::time_point<std::chrono::high_resolution_clock> start_time,
							wformat_string<WideArgs...> formats,
							WideArgs&&... args) -> void
		{
			if (collector_ == nullptr)
			{
				return;
			}

			if (collector_->get_file_target() < type && collector_->get_console_target() < type)
			{
				return;
			}

			collector_->write(
				type, formatter::format(std::move(formats), std::forward<WideArgs>(args)...),
				start_time);
		}

		/**
		 * @brief Writes a formatted wide-character log message with an optional timestamp.
		 *
		 * Formats and logs a message with a specified type and optional start timestamp.
		 * Enables accurate timing for each log entry when a timestamp is provided.
		 *
		 * @tparam Args Types of arguments used to fill in placeholders in the wide format string.
		 *
		 * @param type The log type (e.g., info, warning, error) used for categorizing the message.
		 * @param start_time An optional timestamp marking the start of the log event.
		 * @param formats A wide character string literal containing the format string with
		 * placeholders.
		 * @param args Additional arguments for each placeholder in `formats`.
		 */
		template <typename... WideArgs>
		auto wlog_timestamp(const log_types& type,
							std::chrono::time_point<std::chrono::high_resolution_clock> start_time,
							const wchar_t* formats,
							WideArgs&&... args) -> void
		{
			if (collector_ == nullptr)
			{
				return;
			}

			if (collector_->get_file_target() < type && collector_->get_console_target() < type)
			{
				return;
			}

			collector_->write(type, formatter::format(formats, std::forward<WideArgs>(args)...),
							  start_time);
		}

	private:
		/**
		 * @brief Private constructor enforcing singleton pattern.
		 *
		 * Initializes log collectors, console writer, and file writer.
		 */
		logger();

		/** @brief Deleted copy constructor to prevent multiple instances. */
		logger(const logger&) = delete;

		/** @brief Deleted assignment operator to enforce singleton pattern. */
		logger& operator=(const logger&) = delete;

	private:
		/** @brief Shared pointer to the main log collector handling log storage and management. */
		std::shared_ptr<log_collector> collector_;

		/** @brief Shared pointer to the console writer handling console log output. */
		std::shared_ptr<console_writer> console_writer_;

		/** @brief Shared pointer to the file writer handling log file output. */
		std::shared_ptr<file_writer> file_writer_;

#pragma region singleton
	public:
		/**
		 * @brief Gets the singleton instance of the logger.
		 * @return A reference to the singleton logger instance.
		 */
		static auto handle() -> logger&;

		/**
		 * @brief Destroys the singleton instance of the logger.
		 *
		 * This method should be called when logging is no longer needed, typically at
		 * application shutdown.
		 */
		static auto destroy() -> void;

	private:
		/** @brief Singleton instance of the logger. */
		static std::unique_ptr<logger> handle_;

		/** @brief Flag to ensure the singleton is initialized only once. */
		static std::once_flag once_;
#pragma endregion
	};
} // namespace log_module
