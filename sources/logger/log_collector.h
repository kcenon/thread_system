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
	 * It manages separate queues for log collection, console output, and file output.
	 * The class supports various string types for log messages including std::string,
	 * std::wstring, std::u16string, and std::u32string.
	 */
	class log_collector : public thread_base
	{
	public:
		/**
		 * @brief Constructor for the log_collector class.
		 * Initializes the log queue and sets up default log types for console and file output.
		 */
		log_collector(void);

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
		 * @brief Writes a log message (std::string version).
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
		 * @brief Writes a log message (std::wstring version).
		 * @param type The type of the log message.
		 * @param message The content of the log message as a wide string.
		 * @param start_time An optional start time for the log message.
		 */
		auto write(
			log_types type,
			const std::wstring& message,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt) -> void;

		/**
		 * @brief Writes a log message (std::u16string version).
		 * @param type The type of the log message.
		 * @param message The content of the log message as a UTF-16 string.
		 * @param start_time An optional start time for the log message.
		 */
		auto write(
			log_types type,
			const std::u16string& message,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt) -> void;

		/**
		 * @brief Writes a log message (std::u32string version).
		 * @param type The type of the log message.
		 * @param message The content of the log message as a UTF-32 string.
		 * @param start_time An optional start time for the log message.
		 */
		auto write(
			log_types type,
			const std::u32string& message,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt) -> void;

		/**
		 * @brief Checks if there are log messages to be processed.
		 * @return True if there are log messages in the queue, false otherwise.
		 */
		[[nodiscard]] auto has_work() const -> bool override;

		/**
		 * @brief Performs initialization before starting the log collector thread.
		 * @return A tuple containing:
		 *         - bool: Indicates whether the initialization was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto before_start() -> std::tuple<bool, std::optional<std::string>> override;

		/**
		 * @brief Processes log messages and distributes them to console and file queues.
		 * @return A tuple containing:
		 *         - bool: Indicates whether the processing was successful (true) or not (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto do_work() -> std::tuple<bool, std::optional<std::string>> override;

		/**
		 * @brief Performs cleanup after stopping the log collector thread.
		 * @return A tuple containing:
		 *         - bool: Indicates whether the cleanup was successful (true) or not (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto after_stop() -> std::tuple<bool, std::optional<std::string>> override;

	protected:
		/**
		 * @brief Template method for writing log messages of various string types.
		 *
		 * @tparam StringType The type of the string (std::string, std::wstring, std::u16string, or
		 * std::u32string).
		 * @param type The type of the log message.
		 * @param message The content of the log message.
		 * @param start_time An optional start time for the log message.
		 *
		 * This method provides a unified implementation for writing log messages,
		 * regardless of the string type used. It's called by the public write methods.
		 */
		template <typename StringType>
		auto write_string(log_types type,
						  const StringType& message,
						  std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>>
							  start_time) -> void;

	private:
		log_types file_log_type_;			   ///< Types of logs to write to file
		log_types console_log_type_;		   ///< Types of logs to write to console

		std::shared_ptr<job_queue> log_queue_; ///< Queue for incoming log messages
		std::weak_ptr<job_queue>
			console_queue_;					  ///< Weak pointer to the queue for console output jobs
		std::weak_ptr<job_queue> file_queue_; ///< Weak pointer to the queue for file output jobs
	};
}