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
	 * @class message_job
	 * @brief Represents a console logging job derived from the base job class.
	 *
	 * This class encapsulates the functionality for creating and executing
	 * console logging operations as jobs within the job system. It handles
	 * writing log messages to the console output.
	 */
	class message_job : public job
	{
	public:
		/**
		 * @brief Constructs a new message_job object.
		 * @param message The log message to be written to the console.
		 */
		explicit message_job(const std::string& message);

		/**
		 * @brief Retrieves the log message, optionally appending a newline.
		 * @param append_newline If true, a newline character will be appended to the message.
		 * @return The log message as a string, with an optional newline appended.
		 */
		[[nodiscard]] auto message(const bool& append_newline = false) const -> std::string;

		/**
		 * @brief Executes the console logging operation.
		 * @return A tuple containing:
		 *         - bool: Indicates whether the logging operation was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		[[nodiscard]] auto do_work() -> std::tuple<bool, std::optional<std::string>> override;

	private:
		/** @brief The log message to be written to the console */
		std::string message_;
	};
} // namespace log_module