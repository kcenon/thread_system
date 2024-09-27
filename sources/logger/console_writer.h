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
	 * writing log messages to the console in a separate thread.
	 */
	class console_writer : public thread_base
	{
	public:
		/**
		 * @brief Constructor for the console_writer class.
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
		 * @brief Checks if there is work to be done.
		 * @return True if there are jobs in the queue, false otherwise.
		 */
		[[nodiscard]] bool has_work() const override;

		/**
		 * @brief Performs the main work of writing log messages to the console.
		 * @return A tuple containing:
		 *         - bool: Indicates whether the operation was successful (true) or not (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		std::tuple<bool, std::optional<std::string>> do_work() override;

	private:
		/** @brief Queue for log jobs to be written to the console */
		std::shared_ptr<job_queue> job_queue_;
	};
}