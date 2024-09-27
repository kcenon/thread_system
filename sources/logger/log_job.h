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
	 * @class log_job
	 * @brief Represents a logging job derived from the base job class.
	 *
	 * This class encapsulates the functionality for creating and executing
	 * logging operations as jobs within the job system. It supports different
	 * log types and optional start times for more detailed logging.
	 */
	class log_job : public job
	{
	public:
		/**
		 * @brief Constructs a new log_job object.
		 * @param message The log message to be recorded.
		 * @param type An optional parameter specifying the type of log entry (default is
		 * std::nullopt).
		 * @param start_time An optional parameter specifying the start time of the log entry
		 * (default is std::nullopt).
		 */
		explicit log_job(
			const std::string& message,
			std::optional<log_types> type = std::nullopt,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt);

		/**
		 * @brief Executes the logging operation.
		 * @return A tuple containing:
		 *         - bool: Indicates whether the logging operation was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		[[nodiscard]] auto do_work() -> std::tuple<bool, std::optional<std::string>> override;

		/**
		 * @brief Gets the type of the log entry.
		 * @return The type of the log entry.
		 */
		[[nodiscard]] auto get_type() const -> log_types;

		/**
		 * @brief Gets the formatted log message.
		 * @return The formatted log message as a string.
		 */
		[[nodiscard]] auto log() const -> std::string;

	private:
		/** @brief The original unformatted log message */
		std::string message_;

		/** @brief The formatted log message ready for output */
		std::string log_message_;

		/** @brief The type of the log entry (e.g., info, warning, error) */
		std::optional<log_types> type_;

		/** @brief The timestamp of when the log job was created */
		std::chrono::system_clock::time_point timestamp_;

		/** @brief The optional start time of the log entry for duration calculations */
		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time_;
	};
} // namespace log_module