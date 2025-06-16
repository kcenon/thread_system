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

#include "../jobs/log_job.h"
#include "../../thread_base/jobs/job_queue.h"
#include "../../thread_base/core/thread_base.h"
#include "../../utilities/core/formatter.h"
#include "log_collector.h"
#include "../writers/console_writer.h"
#include "../writers/file_writer.h"
#include "../writers/callback_writer.h"
#include "../../thread_base/synchronization/error_handling.h"
#include "../detail/forward_declarations.h"
#include "configuration.h"

#include <deque>
#include <mutex>
#include <memory>
#include <string>
#include <chrono>
#include <optional>

using namespace thread_module;
using namespace utility_module;

namespace log_module
{
	namespace implementation
	{
		/**
		 * @class logger
		 * @brief A singleton class providing a unified logging mechanism for console, file, and
		 * callback outputs.
		 *
		 * This class manages logging operations through various writers (e.g., console, file, and
		 * callback). It holds a shared log collector to organize and distribute log messages
		 * according to specified output targets. The singleton pattern ensures consistent logging
		 * behavior throughout an application.
		 *
		 * ### Usage Example
		 * ```
		 * // Obtain the logger instance and configure as needed
		 * auto& log = log_module::implementation::logger::handle();
		 * log.set_title("MyApplication");
		 * log.file_target(log_types::Error); // Only write error logs to file
		 * log.console_target(log_types::Information); // Write info-level (and above) logs to
		 * console
		 *
		 * // Start logging
		 * if (auto error = log.start(); error.has_value()) {
		 *     // Handle error case
		 * }
		 *
		 * // Write logs
		 * log.write(log_types::Information, "Application started with version: {}", version);
		 *
		 * // Stop logging
		 * log.stop();
		 *
		 * // Destroy the logger instance at application shutdown
		 * log_module::implementation::logger::destroy();
		 * ```
		 */
		class logger
		{
		public:
			/**
			 * @brief Virtual destructor for safe resource cleanup.
			 *
			 * Ensures that all underlying resources are properly released.
			 * Derived classes (if any) are also destructed in a safe manner.
			 */
			virtual ~logger(void) = default;

			/**
			 * @brief Sets a title for the logger, potentially used in log file names or console
			 * output.
			 * @param title A string representing the new title.
			 *
			 * Commonly used as a prefix for filenames or as an identifier in console outputs.
			 */
			auto set_title(const std::string& title) -> void;

			/**
			 * @brief Configures which log types should be written to the callback writer.
			 * @param type A `log_types` value (or bitwise combination) indicating the log
			 * categories to be sent to the callback.
			 *
			 * Only messages with a log type that is equal to or greater than this setting
			 * (depending on internal comparison logic) will be dispatched to the callback.
			 */
			auto callback_target(const log_types& type) -> void;

			/**
			 * @brief Retrieves the current log types that are written to the callback writer.
			 * @return A `log_types` value representing the currently configured callback log
			 * target.
			 */
			[[nodiscard]] auto callback_target(void) const -> log_types;

			/**
			 * @brief Configures which log types should be written to the file writer.
			 * @param type A `log_types` value (or bitwise combination) indicating the log
			 * categories to be saved to a file.
			 *
			 * Only messages with a log type that is equal to or greater than this setting
			 * (depending on internal comparison logic) will be dispatched to the file writer.
			 */
			auto file_target(const log_types& type) -> void;

			/**
			 * @brief Retrieves the current log types that are written to the file writer.
			 * @return A `log_types` value representing the currently configured file log target.
			 */
			[[nodiscard]] auto file_target(void) const -> log_types;

			/**
			 * @brief Configures which log types should be written to the console writer.
			 * @param type A `log_types` value (or bitwise combination) indicating the log
			 * categories to be shown in the console.
			 *
			 * Only messages with a log type that is equal to or greater than this setting
			 * (depending on internal comparison logic) will appear in the console output.
			 */
			auto console_target(const log_types& type) -> void;

			/**
			 * @brief Retrieves the current log types that are written to the console writer.
			 * @return A `log_types` value representing the currently configured console log target.
			 */
			[[nodiscard]] auto console_target(void) const -> log_types;

			/**
			 * @brief Sets the user-defined callback function to handle log messages.
			 * @param callback A function pointer with the signature `(const log_types&, const
			 * std::string&, const std::string&)`.
			 *
			 * This callback is invoked for every log message that meets the configured callback log
			 * target.
			 */
			auto message_callback(
				const std::function<void(const log_types&, const std::string&, const std::string&)>&
					callback) -> void;

			/**
			 * @brief Sets the maximum number of recent log lines to keep in the log collector.
			 * @param max_lines The maximum lines to store in memory before older logs are
			 * discarded.
			 *
			 * This helps manage in-memory usage when logs are being collected over a long period.
			 */
			auto set_max_lines(uint32_t max_lines) -> void;

			/**
			 * @brief Returns the maximum number of recent log lines retained by the log collector.
			 * @return The configured maximum number of lines to keep in memory.
			 */
			[[nodiscard]] auto get_max_lines(void) const -> uint32_t;

			/**
			 * @brief Enables or disables the creation of a backup log file.
			 * @param use_backup If `true`, the logger will maintain a backup file. Otherwise, no
			 * backup is created.
			 *
			 * Typically used to retain older log data for archiving or diagnostic purposes.
			 */
			auto set_use_backup(bool use_backup) -> void;

			/**
			 * @brief Checks if the logger is configured to create a backup log file.
			 * @return `true` if a backup file is enabled, otherwise `false`.
			 */
			[[nodiscard]] auto get_use_backup(void) const -> bool;

			/**
			 * @brief Sets the interval at which the logger checks for new messages in its queue.
			 * @param interval A `std::chrono::milliseconds` value specifying the sleep or wake-up
			 * interval.
			 *
			 * A smaller interval results in more frequent log processing, but increases CPU usage.
			 * A larger interval can reduce CPU usage but can delay log output.
			 */
			auto set_wake_interval(std::chrono::milliseconds interval) -> void;

			/**
			 * @brief Starts all underlying logging operations, including writers and collectors.
			 * @return std::optional<std::string> containing an error message if startup fails,
			 *         or `std::nullopt` if the logger starts successfully.
			 *
			 * Must be called before issuing any log messages. If this returns an error, logging
			 * should be considered non-functional.
			 */
			auto start(void) -> std::optional<std::string>;

			/**
			 * @brief Stops all logging operations and performs any required cleanup.
			 *
			 * Once stopped, no further log messages will be processed. Call `start()` again
			 * only if the logger supports restarting (implementation dependent).
			 */
			auto stop(void) -> void;

			/**
			 * @brief Retrieves the current time point from a high-resolution clock.
			 * @return A `std::chrono::time_point<std::chrono::high_resolution_clock>` representing
			 * the current time.
			 *
			 * Typically used to timestamp log messages for performance analysis or chronological
			 * ordering.
			 */
			auto time_point(void) -> std::chrono::time_point<std::chrono::high_resolution_clock>;

			/**
			 * @brief Writes a formatted log message to the log collector.
			 *
			 * @tparam Args Variadic template parameter pack for any format placeholders.
			 * @param type The log level (e.g., `log_types::Information`, `log_types::Error`) of
			 * this message.
			 * @param formats A printf-style format string containing placeholders for the
			 * arguments.
			 * @param args The values to substitute into the `formats` string.
			 *
			 * Logs are dispatched only if they meet or exceed the configured log target thresholds.
			 */
			template <typename... Args>
			auto write(const log_types& type, const char* formats, const Args&... args) -> void
			{
				std::lock_guard<std::mutex> lock(collector_mutex_);
				if (collector_ == nullptr)
				{
					return;
				}

				// Check if the message passes the configured target thresholds
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
			 * @tparam WideArgs Variadic template parameter pack for any wide-character format
			 * placeholders.
			 * @param type The log level (e.g., `log_types::Information`, `log_types::Error`) of
			 * this message.
			 * @param formats A wide-character format string containing placeholders.
			 * @param args The values to substitute into the `formats` string.
			 *
			 * Logs are dispatched only if they meet or exceed the configured log target thresholds.
			 */
			template <typename... WideArgs>
			auto write(const log_types& type, const wchar_t* formats, const WideArgs&... args)
				-> void
			{
				std::lock_guard<std::mutex> lock(collector_mutex_);
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
			 * @brief Writes a formatted log message with an optional high-resolution timestamp.
			 *
			 * @tparam Args Variadic template parameter pack for any format placeholders.
			 * @param type The log level (e.g., `log_types::Information`, `log_types::Error`) of
			 * this message.
			 * @param time_point A timestamp from `std::chrono::high_resolution_clock` for precise
			 * log timing.
			 * @param formats A printf-style format string containing placeholders.
			 * @param args The values to substitute into the `formats` string.
			 *
			 * This method enables more precise timing of logs by attaching a user-provided time
			 * point. Logs are dispatched only if they meet or exceed the configured log target
			 * thresholds.
			 */
			template <typename... Args>
			auto write(
				const log_types& type,
				const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
				const char* formats,
				const Args&... args) -> void
			{
				std::lock_guard<std::mutex> lock(collector_mutex_);
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
			 * @brief Writes a formatted wide-character log message with an optional high-resolution
			 * timestamp.
			 *
			 * @tparam WideArgs Variadic template parameter pack for any wide-character format
			 * placeholders.
			 * @param type The log level (e.g., `log_types::Information`, `log_types::Error`) of
			 * this message.
			 * @param time_point A timestamp from `std::chrono::high_resolution_clock` for precise
			 * log timing.
			 * @param formats A wide-character format string containing placeholders.
			 * @param args The values to substitute into the `formats` string.
			 *
			 * This method enables more precise timing of logs by attaching a user-provided time
			 * point. Logs are dispatched only if they meet or exceed the configured log target
			 * thresholds.
			 */
			template <typename... WideArgs>
			auto write(
				const log_types& type,
				const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
				const wchar_t* formats,
				const WideArgs&... args) -> void
			{
				std::lock_guard<std::mutex> lock(collector_mutex_);
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
			 * @brief Private constructor enforcing the singleton pattern.
			 *
			 * Initializes core components such as the log collector, console writer, file writer,
			 * and callback writer. Use `logger::handle()` to access the singleton instance.
			 */
			logger();

			/** @brief Deleted copy constructor to prevent multiple `logger` instances. */
			logger(const logger&) = delete;

			/** @brief Deleted assignment operator enforcing the singleton pattern. */
			logger& operator=(const logger&) = delete;

		private:
			/** @brief Shared pointer to the main log collector, responsible for storing and
			 * distributing logs. */
			std::shared_ptr<log_collector> collector_;

			/** @brief Shared pointer to the console writer, managing console-based output. */
			std::shared_ptr<console_writer> console_writer_;

			/** @brief Shared pointer to the file writer, managing file-based output. */
			std::shared_ptr<file_writer> file_writer_;

			/** @brief Shared pointer to the callback writer, managing user-defined callback output.
			 */
			std::shared_ptr<callback_writer> callback_writer_;
			
			/** @brief Mutex to protect access to the collector and other shared resources */
			mutable std::mutex collector_mutex_;

	// Singleton implementation
	#ifdef _MSC_VER
	#pragma region singleton
	#endif
		public:
			/**
			 * @brief Retrieves the singleton instance of the logger.
			 * @return A reference to the singleton `logger` instance.
			 *
			 * Use this method to obtain the global logger and configure it before calling
			 * `start()`.
			 */
			static auto handle() -> logger&;

			/**
			 * @brief Destroys the singleton instance of the logger and releases its resources.
			 *
			 * This method should generally be called at application shutdown to ensure a clean
			 * teardown of all logging resources, including any active writers.
			 */
			static auto destroy() -> void;

		private:
			/** @brief Pointer to the singleton instance of the logger. */
			static std::unique_ptr<logger> handle_;

			/** @brief Flag ensuring the singleton is lazily and atomically initialized only once.
			 */
			static std::once_flag once_;
	#ifdef _MSC_VER
	#pragma endregion
	#endif
		};
	} // namespace implementation
} // namespace log_module