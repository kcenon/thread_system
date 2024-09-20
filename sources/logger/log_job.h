#pragma once

#include "job.h"
#include "log_types.h"

#include <chrono>
#include <string>
#include <vector>
#include <optional>

/**
 * @class log_job
 * @brief Represents a logging job derived from the base job class.
 *
 * This class encapsulates the functionality for creating and executing
 * logging operations as jobs within the job system.
 */
class log_job : public job
{
public:
	/**
	 * @brief Constructs a new log_job object.
	 * @param message The log message to be recorded.
	 * @param type An optional parameter specifying the type of log entry (default is std::nullopt).
	 * @param start_time An optional parameter specifying the start time of the log entry (default
	 * is std::nullopt).
	 */
	log_job(const std::string& message,
			std::optional<log_types> type = std::nullopt,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt);

	/**
	 * @brief Executes the logging operation.
	 * @return std::tuple<bool, std::optional<std::string>> A tuple containing:
	 *         - bool: Indicates whether the logging operation was successful (true) or not (false).
	 *         - std::optional<std::string>: An optional string message, typically used for error
	 * descriptions.
	 */
	[[nodiscard]] auto do_work() -> std::tuple<bool, std::optional<std::string>> override;

	/**
	 * @brief Gets the type of the log entry.
	 * @return log_types The type of the log entry.
	 */
	[[nodiscard]] auto get_type() const -> log_types;

	/**
	 * @brief Gets the formatted log message.
	 * @return std::string The formatted log message.
	 */
	[[nodiscard]] auto log() const -> std::string;

private:
	std::string message_;			///< The original log message
	std::string log_message_;		///< The formatted log message
	std::optional<log_types> type_; ///< The type of the log entry
	std::chrono::system_clock::time_point
		timestamp_;					///< The timestamp of when the log job was created
	std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>>
		start_time_;				///< The optional start time of the log entry
};