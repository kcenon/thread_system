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

#include "logger_implementation.h"
#include "lockfree_log_collector.h"
#include "../detail/forward_declarations.h"
#include "config.h"

namespace log_module
{
	namespace implementation
	{
		/**
		 * @class lockfree_logger
		 * @brief A high-performance lock-free logger implementation.
		 * 
		 * This class provides a lock-free alternative to the standard logger,
		 * using lockfree_job_queue internally for superior performance under
		 * high contention. It maintains the same interface as the standard
		 * logger for easy drop-in replacement.
		 * 
		 * ### Key Features
		 * - **Lock-Free Operations**: Uses lockfree_job_queue for wait-free logging
		 * - **Superior Scalability**: Better performance with multiple threads
		 * - **Lower Latency**: Reduced contention in log message handling
		 * - **Compatible Interface**: Drop-in replacement for logger
		 * 
		 * ### Performance Benefits
		 * - No mutex contention in log message queuing
		 * - Better CPU cache utilization
		 * - Linear scalability with thread count
		 * - Reduced logging overhead in critical paths
		 * 
		 * ### Usage Example
		 * ```cpp
		 * // Replace standard logger with lock-free version
		 * auto& log = log_module::implementation::lockfree_logger::handle();
		 * log.set_title("HighPerformanceApp");
		 * log.start();
		 * 
		 * // Use the same API as standard logger
		 * log.write(log_types::Information, "Application started");
		 * ```
		 */
		class lockfree_logger
		{
		public:
			/**
			 * @brief Virtual destructor for safe resource cleanup.
			 */
			virtual ~lockfree_logger(void) = default;

			/**
			 * @brief Sets a title for the logger.
			 * @param title A string representing the new title.
			 */
			auto set_title(const std::string& title) -> void;

			/**
			 * @brief Configures which log types should be written to the callback writer.
			 * @param type A log_types value indicating the log categories.
			 */
			auto callback_target(const log_types& type) -> void;

			/**
			 * @brief Retrieves the current log types for callback writer.
			 * @return A log_types value.
			 */
			[[nodiscard]] auto callback_target(void) const -> log_types;

			/**
			 * @brief Configures which log types should be written to the file writer.
			 * @param type A log_types value.
			 */
			auto file_target(const log_types& type) -> void;

			/**
			 * @brief Retrieves the current log types for file writer.
			 * @return A log_types value.
			 */
			[[nodiscard]] auto file_target(void) const -> log_types;

			/**
			 * @brief Configures which log types should be written to the console writer.
			 * @param type A log_types value.
			 */
			auto console_target(const log_types& type) -> void;

			/**
			 * @brief Retrieves the current log types for console writer.
			 * @return A log_types value.
			 */
			[[nodiscard]] auto console_target(void) const -> log_types;

			/**
			 * @brief Sets the user-defined callback function.
			 * @param callback A function pointer.
			 */
			auto message_callback(
				const std::function<void(const log_types&, const std::string&, const std::string&)>&
					callback) -> void;

			/**
			 * @brief Sets the maximum number of recent log lines.
			 * @param max_lines The maximum lines to store.
			 */
			auto set_max_lines(uint32_t max_lines) -> void;

			/**
			 * @brief Returns the maximum number of recent log lines.
			 * @return The configured maximum number of lines.
			 */
			[[nodiscard]] auto get_max_lines(void) const -> uint32_t;

			/**
			 * @brief Enables or disables backup log file.
			 * @param use_backup If true, maintains a backup file.
			 */
			auto set_use_backup(bool use_backup) -> void;

			/**
			 * @brief Checks if backup is enabled.
			 * @return true if backup is enabled.
			 */
			[[nodiscard]] auto get_use_backup(void) const -> bool;

			/**
			 * @brief Sets the wake interval.
			 * @param interval A milliseconds value.
			 */
			auto set_wake_interval(std::chrono::milliseconds interval) -> void;

			/**
			 * @brief Starts all underlying lock-free logging operations.
			 * @return std::optional<std::string> containing an error message if startup fails,
			 *         or std::nullopt if the logger starts successfully.
			 */
			auto start(void) -> std::optional<std::string>;

			/**
			 * @brief Stops all logging operations.
			 */
			auto stop(void) -> void;

			/**
			 * @brief Retrieves the current time point.
			 * @return A time point.
			 */
			auto time_point(void) -> std::chrono::time_point<std::chrono::high_resolution_clock>;

			/**
			 * @brief Writes a formatted log message.
			 */
			template <typename... Args>
			auto write(const log_types& type, const char* formats, const Args&... args) -> void
			{
				if (lockfree_collector_ == nullptr)
				{
					return;
				}

				// Check if the message passes the configured target thresholds
				if (lockfree_collector_->file_target() < type && 
					lockfree_collector_->console_target() < type &&
					lockfree_collector_->callback_target() < type)
				{
					return;
				}

				lockfree_collector_->write(type, utility_module::formatter::format(formats, args...));
			}

			/**
			 * @brief Writes a formatted wide-character log message.
			 */
			template <typename... WideArgs>
			auto write(const log_types& type, const wchar_t* formats, const WideArgs&... args)
				-> void
			{
				if (lockfree_collector_ == nullptr)
				{
					return;
				}

				if (lockfree_collector_->file_target() < type && 
					lockfree_collector_->console_target() < type &&
					lockfree_collector_->callback_target() < type)
				{
					return;
				}

				lockfree_collector_->write(type, utility_module::formatter::format(formats, args...));
			}

			/**
			 * @brief Writes a formatted log message with timestamp.
			 */
			template <typename... Args>
			auto write(
				const log_types& type,
				const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
				const char* formats,
				const Args&... args) -> void
			{
				if (lockfree_collector_ == nullptr)
				{
					return;
				}

				if (lockfree_collector_->file_target() < type && 
					lockfree_collector_->console_target() < type &&
					lockfree_collector_->callback_target() < type)
				{
					return;
				}

				lockfree_collector_->write(type, utility_module::formatter::format(formats, args...), time_point);
			}

			/**
			 * @brief Writes a formatted wide-character log message with timestamp.
			 */
			template <typename... WideArgs>
			auto write(
				const log_types& type,
				const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
				const wchar_t* formats,
				const WideArgs&... args) -> void
			{
				if (lockfree_collector_ == nullptr)
				{
					return;
				}

				if (lockfree_collector_->file_target() < type && 
					lockfree_collector_->console_target() < type &&
					lockfree_collector_->callback_target() < type)
				{
					return;
				}

				lockfree_collector_->write(type, utility_module::formatter::format(formats, args...), time_point);
			}

		private:
			/**
			 * @brief Private constructor enforcing the singleton pattern.
			 * 
			 * Initializes the lock-free log collector and associated writers.
			 */
			lockfree_logger();

			/** @brief Deleted copy constructor to prevent multiple instances. */
			lockfree_logger(const lockfree_logger&) = delete;

			/** @brief Deleted assignment operator enforcing the singleton pattern. */
			lockfree_logger& operator=(const lockfree_logger&) = delete;

		private:
			/** @brief Lock-free log collector for high-performance log processing */
			std::shared_ptr<lockfree_log_collector> lockfree_collector_;

			/** @brief Console writer for console output */
			std::shared_ptr<console_writer> console_writer_;

			/** @brief File writer for file output */
			std::shared_ptr<file_writer> file_writer_;

			/** @brief Callback writer for custom callbacks */
			std::shared_ptr<callback_writer> callback_writer_;

		// Singleton implementation
		#ifdef _MSC_VER
		#pragma region singleton
		#endif
		public:
			/**
			 * @brief Retrieves the singleton instance of the lock-free logger.
			 * @return A reference to the singleton lockfree_logger instance.
			 */
			static auto handle() -> lockfree_logger&;

			/**
			 * @brief Destroys the singleton instance and releases its resources.
			 */
			static auto destroy() -> void;

		private:
			/** @brief Pointer to the singleton instance */
			static std::unique_ptr<lockfree_logger> handle_;

			/** @brief Flag ensuring singleton is initialized only once */
			static std::once_flag once_;
		#ifdef _MSC_VER
		#pragma endregion
		#endif
		};
	} // namespace implementation
} // namespace log_module