#pragma once

#include "thread_base.h"
#include "job_queue.h"

#include <memory>
#include <vector>

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
	 * @param use_time_tag A boolean flag indicating whether to use time tags (default is true).
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
	 */
	auto do_work() -> void override;

private:
	bool use_time_tag_;					   ///< Flag indicating whether to use time tags
	std::shared_ptr<job_queue> job_queue_; ///< The job queue to process
};