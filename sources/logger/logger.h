#pragma once

#include "log_job.h"
#include "job_queue.h"
#include "thread_base.h"

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
	 * @brief A singleton class for logging operations.
	 *
	 * This class provides logging functionality with support for both console and file output.
	 * It inherits from thread_base to run logging operations in a separate thread.
	 */
	class logger : public thread_base
	{
	public:
		/**
		 * @brief Destructor for the logger class.
		 */
		~logger(void) = default;

		/**
		 * @brief Sets the title for the logger.
		 * @param title The title to set.
		 */
		auto set_title(const std::string& title) -> void;

		/**
		 * @brief Sets the log types that should be written to a file.
		 * @param type The log types to be written to a file.
		 */
		auto set_file_target(const log_types& type) -> void;

		/**
		 * @brief Gets the log types that are written to a file.
		 * @return log_types The log types that are written to a file.
		 */
		[[nodiscard]] auto get_file_target(void) const -> log_types;

		/**
		 * @brief Sets the log types that should be written to the console.
		 * @param type The log types to be written to the console.
		 */
		auto set_console_target(const log_types& type) -> void;

		/**
		 * @brief Gets the log types that are written to the console.
		 * @return log_types The log types that are written to the console.
		 */
		[[nodiscard]] auto get_console_target(void) const -> log_types;

		/**
		 * @brief Sets the maximum number of lines to keep in the log.
		 * @param max_lines The maximum number of lines to keep in the log.
		 */
		auto set_max_lines(uint32_t max_lines) -> void;

		/**
		 * @brief Gets the maximum number of lines to keep in the log.
		 * @return uint32_t The maximum number of lines to keep in the log.
		 */
		[[nodiscard]] auto get_max_lines(void) const -> uint32_t;

		/**
		 * @brief Sets whether to use a backup log file.
		 * @param use_backup Flag indicating whether to use a backup log file.
		 */
		auto set_use_backup(bool use_backup) -> void;

		/**
		 * @brief Gets whether to use a backup log file.
		 * @return bool True if using a backup log file, false otherwise.
		 */
		[[nodiscard]] auto get_use_backup(void) const -> bool;

		/**
		 * @brief Gets the current time point.
		 * @return std::chrono::time_point<std::chrono::high_resolution_clock> The current time
		 * point.
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

	protected:
		/**
		 * @brief Checks if there is work to be done.
		 * @return bool True if there is work to be done, false otherwise.
		 */
		[[nodiscard]] auto has_work() const -> bool override;

		/**
		 * @brief Called before the logger thread starts.
		 * @return std::tuple<bool, std::optional<std::string>> A tuple containing:
		 *         - bool: Indicates whether the initialization was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto before_start() -> std::tuple<bool, std::optional<std::string>> override;

		/**
		 * @brief Performs the actual work of the logger thread.
		 * @return std::tuple<bool, std::optional<std::string>> A tuple containing:
		 *         - bool: Indicates whether the work was successful (true) or not (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto do_work() -> std::tuple<bool, std::optional<std::string>> override;

		/**
		 * @brief Called after the logger thread stops.
		 * @return std::tuple<bool, std::optional<std::string>> A tuple containing:
		 *         - bool: Indicates whether the cleanup was successful (true) or not (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto after_stop() -> std::tuple<bool, std::optional<std::string>> override;

		/**
		 * @brief Writes a message to the console.
		 * @param message The message to write to the console.
		 */
		auto write_to_console(const std::string& message) -> void;

		/**
		 * @brief Writes a message to a file.
		 * @param message The message to write to the file.
		 */
		auto write_to_file(const std::string& message) -> void;

	private:
		/**
		 * @brief Private constructor for the logger class (singleton pattern).
		 */
		logger();

		/**
		 * @brief Deleted copy constructor.
		 */
		logger(const logger&) = delete;

		/**
		 * @brief Deleted assignment operator.
		 */
		logger& operator=(const logger&) = delete;

	private:
		/** @brief Flag indicating whether to use a backup log file */
		bool use_backup_;

		/** @brief Maximum number of lines to keep in the log */
		uint32_t max_lines_;

		/** @brief Title of the logger */
		std::string title_;

		/** @brief Types of logs to write to file */
		log_types file_log_type_;

		/** @brief Types of logs to write to console */
		log_types console_log_type_;

		/** @brief Buffer for log dequeuing related max lines */
		std::deque<std::string> log_buffer_;

		/** @brief Queue for log jobs */
		std::shared_ptr<job_queue> log_queue_;

#pragma region singleton
	public:
		/**
		 * @brief Gets the singleton instance of the logger.
		 * @return logger& Reference to the singleton logger instance.
		 */
		static auto handle() -> logger&;

		/**
		 * @brief Destroys the singleton instance of the logger.
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