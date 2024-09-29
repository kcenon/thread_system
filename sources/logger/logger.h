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
	 * operations.
	 */
	class logger
	{
	public:
		/**
		 * @brief Destructor for the logger class.
		 */
		~logger(void) = default;

		/**
		 * @brief Sets the title for the logger.
		 * @param title The title to set for the logger.
		 */
		auto set_title(const std::string& title) -> void;

		/**
		 * @brief Sets the log types that should be written to a file.
		 * @param type The log types to be written to a file.
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
		 */
		auto set_wake_interval(std::chrono::milliseconds interval) -> void;

		/**
		 * @brief Starts the logger.
		 * @return A tuple containing:
		 *         - bool: Indicates whether the logger was started successfully (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto start(void) -> std::tuple<bool, std::optional<std::string>>;

		/**
		 * @brief Stops the logger.
		 */
		auto stop(void) -> void;

		/**
		 * @brief Gets the current time point.
		 * @return The current time point using high resolution clock.
		 */
		auto time_point(void) -> std::chrono::time_point<std::chrono::high_resolution_clock>;

		/**
		 * @brief Writes a log message.
		 * @param type The type of the log message.
		 * @param message The content of the log message.
		 * @param start_time An optional start time for the log message.
		 */
		auto write(
			log_types type,
			const std::string& message,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt) -> void;

	private:
		/**
		 * @brief Private constructor for the logger class (singleton pattern).
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
		 */
		static auto handle() -> logger&;

		/**
		 * @brief Destroys the singleton instance of the logger.
		 * This method should be called when the logger is no longer needed.
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