#pragma once

#include "log_types.h"
#include "job_queue.h"
#include "thread_base.h"

using namespace thread_module;

namespace log_module
{
	/**
	 * @class log_collector
	 * @brief A class for collecting and managing log messages.
	 *
	 * This class inherits from thread_base and provides functionality for collecting,
	 * processing, and distributing log messages to console and file outputs.
	 */
	class log_collector : public thread_base
	{
	public:
		/**
		 * @brief Constructor for the log_collector class.
		 */
		log_collector(void);

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
		 * @brief Sets the log types that should be written to the console.
		 * @param type The log types to be written to the console.
		 */
		auto set_console_target(const log_types& type) -> void;

		/**
		 * @brief Gets the log types that are written to the console.
		 * @return The log types that are currently set to be written to the console.
		 */
		[[nodiscard]] auto get_console_target() const -> log_types;

		/**
		 * @brief Sets the log types that should be written to a file.
		 * @param type The log types to be written to a file.
		 */
		auto set_file_target(const log_types& type) -> void;

		/**
		 * @brief Gets the log types that are written to a file.
		 * @return The log types that are currently set to be written to a file.
		 */
		[[nodiscard]] auto get_file_target() const -> log_types;

		/**
		 * @brief Sets the queue for console output jobs.
		 * @param queue Shared pointer to the job queue for console output.
		 */
		auto set_console_queue(std::shared_ptr<job_queue> queue) -> void;

		/**
		 * @brief Sets the queue for file output jobs.
		 * @param queue Shared pointer to the job queue for file output.
		 */
		auto set_file_queue(std::shared_ptr<job_queue> queue) -> void;

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

		/**
		 * @brief Checks if there is work to be done.
		 * @return True if there is work to be done, false otherwise.
		 */
		[[nodiscard]] bool has_work() const override;

		/**
		 * @brief Performs initialization before starting the thread.
		 * @return A tuple containing a boolean indicating success and an optional error message.
		 */
		auto before_start() -> std::tuple<bool, std::optional<std::string>> override;

		/**
		 * @brief Performs the main work of the thread.
		 * @return A tuple containing a boolean indicating success and an optional error message.
		 */
		auto do_work() -> std::tuple<bool, std::optional<std::string>> override;

		/**
		 * @brief Performs cleanup after stopping the thread.
		 * @return A tuple containing a boolean indicating success and an optional error message.
		 */
		auto after_stop() -> std::tuple<bool, std::optional<std::string>> override;

	private:
		/** @brief Title of the logger */
		std::string title_;

		/** @brief Flag indicating whether to use a backup log file */
		bool use_backup_;

		/** @brief Maximum number of lines to keep in the log */
		uint32_t max_lines_;

		/** @brief Types of logs to write to file */
		log_types file_log_type_;

		/** @brief Types of logs to write to console */
		log_types console_log_type_;

		/** @brief Queue for log jobs */
		std::shared_ptr<job_queue> log_queue_;

		/** @brief Queue for console jobs */
		std::weak_ptr<job_queue> console_queue_;

		/** @brief Queue for file jobs */
		std::weak_ptr<job_queue> file_queue_;
	};
}