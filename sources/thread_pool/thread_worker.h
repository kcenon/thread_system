#pragma once

#include "thread_base.h"
#include "job_queue.h"

#include <memory>
#include <vector>

using namespace thread_module;

namespace thread_pool_module
{
	/**
	 * @class thread_worker
	 * @brief A worker thread class that processes jobs from a job queue.
	 *
	 * This class extends thread_base to create a worker thread that can
	 * process jobs from a specified job queue. It provides functionality
	 * to set a job queue and override the base class's work-related methods.
	 */
	class thread_worker : public thread_base
	{
	public:
		/**
		 * @brief Constructs a new thread_worker object.
		 * @param use_time_tag A boolean flag indicating whether to use time tags in job processing
		 * (default is true).
		 */
		thread_worker(const bool& use_time_tag = true);

		/**
		 * @brief Virtual destructor for the thread_worker class.
		 */
		virtual ~thread_worker(void);

		/**
		 * @brief Sets the job queue for this worker to process.
		 * @param job_queue A shared pointer to the job queue.
		 */
		auto set_job_queue(std::shared_ptr<job_queue> job_queue) -> void;

	protected:
		/**
		 * @brief Checks if there is work available in the job queue.
		 * @return bool True if there is work to be done, false otherwise.
		 */
		[[nodiscard]] auto has_work() const -> bool override;

		/**
		 * @brief Performs the actual work by processing jobs from the queue.
		 * @return std::tuple<bool, std::optional<std::string>> A tuple containing:
		 *         - bool: Indicates whether the work was successful (true) or not (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto do_work() -> std::tuple<bool, std::optional<std::string>> override;

	private:
		/** @brief Flag indicating whether to use time tags in job processing */
		bool use_time_tag_;

		/** @brief The job queue to process */
		std::shared_ptr<job_queue> job_queue_;
	};
} // namespace thread_pool_module