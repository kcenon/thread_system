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
#include "callback_writer.h"

#include <deque>
#include <mutex>
#include <memory>
#include <string>
#include <optional>

using namespace thread_module;
using namespace utility_module;

namespace log_module
{
	namespace detail
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
			 * @brief Defines the log types that should be written to a callback.
			 * @param type A `log_types` value indicating the types of log messages to save to
			 * callback.
			 */
			auto callback_target(const log_types& type) -> void;

			/**
			 * @brief Retrieves the current log types that are written to a callback.
			 * @return The `log_types` currently set for callback output.
			 */
			[[nodiscard]] auto callback_target(void) const -> log_types;

			/**
			 * @brief Defines the log types that should be written to a file.
			 * @param type A `log_types` value indicating the types of log messages to save to file.
			 */
			auto file_target(const log_types& type) -> void;

			/**
			 * @brief Retrieves the current log types that are written to a file.
			 * @return The `log_types` currently set for file output.
			 */
			[[nodiscard]] auto file_target(void) const -> log_types;

			/**
			 * @brief Defines the log types that should be written to the console.
			 * @param type A `log_types` value indicating the types of log messages to display in
			 * the console.
			 */
			auto console_target(const log_types& type) -> void;

			/**
			 * @brief Retrieves the current log types that are written to the console.
			 * @return The `log_types` currently set for console output.
			 */
			[[nodiscard]] auto console_target(void) const -> log_types;

			/**
			 * @brief Sets the message callback function for handling log messages.
			 * @param callback A function pointer to the message callback function.
			 */
			auto message_callback(
				const std::function<void(const log_types&, const std::string&, const std::string&)>&
					callback) -> void;

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
			auto start(void) -> std::optional<std::string>;

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
			 * @param type The log type (e.g., info, warning, error) used for categorizing the
			 * message.
			 * @param formats Format string defining the log message structure and placeholders.
			 * @param args Additional arguments for each placeholder in `formats`.
			 */
			template <typename... Args>
			auto write(const log_types& type, const char* formats, const Args&... args) -> void
			{
				if (collector_ == nullptr)
				{
					return;
				}

				if (collector_->file_target() < type && collector_->console_target() < type
					&& collector_->callback_target() < type)
				{
					return;
				}

				collector_->write(type, utility_module::formatter::format(formats, args...));
			}

			/**
			 * @brief Writes a formatted wide-character log message to the log collector.
			 *
			 * Formats and logs a message with a specified log type. Can optionally include a
			 * timestamp representing the start time of the log event.
			 *
			 * @tparam Args Types of arguments used to fill in placeholders in the wide format
			 * string.
			 *
			 * @param type The log type (e.g., info, warning, error) used for categorizing the
			 * message.
			 * @param formats A wide-character format string with placeholders.
			 * @param args Additional arguments for each placeholder in `formats`.
			 */
			template <typename... WideArgs>
			auto write(const log_types& type,
					   const wchar_t* formats,
					   const WideArgs&... args) -> void
			{
				if (collector_ == nullptr)
				{
					return;
				}

				if (collector_->file_target() < type && collector_->console_target() < type
					&& collector_->callback_target() < type)
				{
					return;
				}

				collector_->write(type, utility_module::formatter::format(formats, args...));
			}

			/**
			 * @brief Writes a formatted log message to the log collector, with optional timestamp.
			 *
			 * Formats and logs a message with a specified type and optional start timestamp.
			 * Enables accurate timing for each log entry when a timestamp is provided.
			 *
			 * @tparam Args Types of arguments used to fill in placeholders in the format string.
			 *
			 * @param type The log type (e.g., info, warning, error) used for categorizing the
			 * message.
			 * @param start_time An optional timestamp marking the start of the log event.
			 * @param formats Format string defining the log message structure and placeholders.
			 * @param args Additional arguments matching the placeholders in `formats`.
			 */
			template <typename... Args>
			auto write(
				const log_types& type,
				const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
				const char* formats,
				const Args&... args) -> void
			{
				if (collector_ == nullptr)
				{
					return;
				}

				if (collector_->file_target() < type && collector_->console_target() < type
					&& collector_->callback_target() < type)
				{
					return;
				}

				collector_->write(type, utility_module::formatter::format(formats, args...),
								  time_point);
			}

			/**
			 * @brief Writes a formatted wide-character log message with an optional timestamp.
			 *
			 * Formats and logs a message with a specified type and optional start timestamp.
			 * Enables accurate timing for each log entry when a timestamp is provided.
			 *
			 * @tparam Args Types of arguments used to fill in placeholders in the wide format
			 * string.
			 *
			 * @param type The log type (e.g., info, warning, error) used for categorizing the
			 * message.
			 * @param start_time An optional timestamp marking the start of the log event.
			 * @param formats A wide-character format string with placeholders.
			 * @param args Additional arguments matching the placeholders in `formats`.
			 */
			template <typename... WideArgs>
			auto write(
				const log_types& type,
				const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
				const wchar_t* formats,
				const WideArgs&... args) -> void
			{
				if (collector_ == nullptr)
				{
					return;
				}

				if (collector_->file_target() < type && collector_->console_target() < type
					&& collector_->callback_target() < type)
				{
					return;
				}

				collector_->write(type, utility_module::formatter::format(formats, args...),
								  time_point);
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
			/** @brief Shared pointer to the main log collector handling log storage and management.
			 */
			std::shared_ptr<log_collector> collector_;

			/** @brief Shared pointer to the console writer handling console log output. */
			std::shared_ptr<console_writer> console_writer_;

			/** @brief Shared pointer to the file writer handling log file output. */
			std::shared_ptr<file_writer> file_writer_;

			/** @brief Shared pointer to the message handling log callback output. */
			std::shared_ptr<callback_writer> callback_writer_;

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
	} // namespace detail

	inline auto set_title(const std::string& title) -> void
	{
		detail::logger::handle().set_title(title);
	}

	inline auto callback_target(const log_types& type) -> void
	{
		detail::logger::handle().callback_target(type);
	}

	inline auto callback_target() -> log_types
	{
		return detail::logger::handle().callback_target();
	}

	inline auto file_target(const log_types& type) -> void
	{
		detail::logger::handle().file_target(type);
	}

	inline auto file_target() -> log_types { return detail::logger::handle().file_target(); }

	inline auto console_target(const log_types& type) -> void
	{
		detail::logger::handle().console_target(type);
	}

	inline auto console_target() -> log_types { return detail::logger::handle().console_target(); }

	inline auto message_callback(
		const std::function<void(const log_types&, const std::string&, const std::string&)>&
			callback) -> void
	{
		detail::logger::handle().message_callback(callback);
	}

	inline auto set_max_lines(uint32_t max_lines) -> void
	{
		detail::logger::handle().set_max_lines(max_lines);
	}

	inline auto get_max_lines() -> uint32_t { return detail::logger::handle().get_max_lines(); }

	inline auto set_use_backup(bool use_backup) -> void
	{
		detail::logger::handle().set_use_backup(use_backup);
	}

	inline auto get_use_backup() -> bool { return detail::logger::handle().get_use_backup(); }

	inline auto set_wake_interval(std::chrono::milliseconds interval) -> void
	{
		detail::logger::handle().set_wake_interval(interval);
	}

	inline auto start() -> std::optional<std::string> { return detail::logger::handle().start(); }

	inline auto stop() -> void { detail::logger::handle().stop(); }

	inline auto time_point() -> std::chrono::time_point<std::chrono::high_resolution_clock>
	{
		return detail::logger::handle().time_point();
	}

	template <typename... Args>
	inline auto write(const log_types& type, const char* formats, const Args&... args) -> void
	{
		detail::logger::handle().write(type, formats, args...);
	}

	template <typename... WideArgs>
	inline auto write(const log_types& type,
					  const wchar_t* formats,
					  const WideArgs&... args) -> void
	{
		detail::logger::handle().write(type, formats, args...);
	}

	template <typename... Args>
	inline auto write(const log_types& type,
					  const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
					  const char* formats,
					  const Args&... args) -> void
	{
		detail::logger::handle().write(type, time_point, formats, args...);
	}

	template <typename... WideArgs>
	inline auto write(const log_types& type,
					  const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
					  const wchar_t* formats,
					  const WideArgs&... args) -> void
	{
		detail::logger::handle().write(type, time_point, formats, args...);
	}

	inline auto destroy() -> void { detail::logger::destroy(); }
} // namespace log_module
