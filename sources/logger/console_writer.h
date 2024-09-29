#pragma once

#include "job_queue.h"
#include "thread_base.h"

using namespace thread_module;

namespace log_module
{
	/**
	 * @class console_writer
	 * @brief A class for writing log messages to the console.
	 *
	 * This class inherits from thread_base and provides functionality for
	 * writing log messages to the console in a separate thread. It manages
	 * its own job queue for console writing tasks.
	 */
	class console_writer : public thread_base
	{
	public:
		/**
		 * @brief Constructor for the console_writer class.
		 * Initializes the job queue for console writing tasks.
		 */
		console_writer(void);

		/**
		 * @brief Gets the job queue used by the console writer.
		 * @return A shared pointer to the job queue containing console writing tasks.
		 */
		[[nodiscard]] auto get_job_queue() const -> std::shared_ptr<job_queue>
		{
			return job_queue_;
		}

		/**
		 * @brief Checks if there is work to be done in the job queue.
		 * @return True if there are jobs in the queue, false otherwise.
		 */
		[[nodiscard]] auto has_work() const -> bool override;

		/**
		 * @brief Performs initialization before starting the console writer thread.
		 * @return A tuple containing:
		 *         - bool: Indicates whether the initialization was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto before_start() -> std::tuple<bool, std::optional<std::string>> override;

		/**
		 * @brief Performs the main work of writing log messages to the console.
		 * @return A tuple containing:
		 *         - bool: Indicates whether the operation was successful (true) or not (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto do_work() -> std::tuple<bool, std::optional<std::string>> override;

	private:
		/** @brief Queue for log jobs to be written to the console */
		std::shared_ptr<job_queue> job_queue_;
	};
}