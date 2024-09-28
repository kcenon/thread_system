#pragma once

#include "job.h"
#include "log_types.h"

#include <chrono>
#include <string>
#include <vector>
#include <optional>

using namespace thread_module;

namespace log_module
{
	/**
	 * @class file_job
	 * @brief Represents a file logging job derived from the base job class.
	 *
	 * This class encapsulates the functionality for creating and executing
	 * logging operations as jobs within the job system. It handles writing
	 * log messages to a file with options for maximum line count and backup.
	 */
	class file_job : public job
	{
	public:
		/**
		 * @brief Constructs a new file_job object.
		 * @param title The title of the log entry.
		 * @param message The log message to be recorded.
		 * @param max_lines The maximum number of lines to keep in the log file (0 for unlimited).
		 * @param use_backup Flag indicating whether to use a backup log file.
		 */
		file_job(const std::string& title,
				 const std::string& message,
				 const uint32_t& max_lines = 0,
				 const bool& use_backup = false);

		/**
		 * @brief Executes the file logging operation.
		 * @return A tuple containing:
		 *         - bool: Indicates whether the logging operation was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		[[nodiscard]] auto do_work() -> std::tuple<bool, std::optional<std::string>> override;

	protected:
		/**
		 * @brief Generates the file name for the log file.
		 * @return A tuple containing:
		 *         - std::string: The generated file name.
		 *         - std::string: The generated backup file name (if use_backup_ is true).
		 */
		[[nodiscard]] auto generate_file_name() -> std::tuple<std::string, std::string>;

		/**
		 * @brief Reads lines from the specified file.
		 * @param file_name The name of the file to read from.
		 * @return A vector of strings, each representing a line from the file.
		 */
		[[nodiscard]] auto read_lines(const std::string& file_name) -> std::vector<std::string>;

		/**
		 * @brief Appends lines to the specified file.
		 * @param file_name The name of the file to append to.
		 * @param messages A vector of strings to be appended to the file.
		 * @return True if the append operation was successful, false otherwise.
		 */
		auto append_lines(const std::string& file_name,
						  const std::vector<std::string>& messages) -> bool;

	private:
		/** @brief The title of the log entry */
		std::string title_;

		/** @brief The log message to be written to the file */
		std::string message_;

		/** @brief The maximum number of lines to keep in the log file (0 for unlimited) */
		uint32_t max_lines_;

		/** @brief Flag indicating whether to use a backup log file */
		bool use_backup_;
	};
} // namespace log_module