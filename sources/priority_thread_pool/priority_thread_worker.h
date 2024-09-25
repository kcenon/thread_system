#pragma once

#include "thread_base.h"
#include "priority_job_queue.h"

#include <memory>
#include <vector>

using namespace thread_module;

namespace priority_thread_pool_module
{
	/**
	 * @class priority_thread_worker
	 * @brief A worker thread class that processes jobs from a priority job queue.
	 *
	 * This template class extends thread_base to create a worker thread that can
	 * process jobs from a specified priority job queue. It provides functionality
	 * to set a job queue and override the base class's work-related methods.
	 *
	 * @tparam priority_type The type used to represent the priority levels.
	 */
	template <typename priority_type> class priority_thread_worker : public thread_base
	{
	public:
		/**
		 * @brief Constructs a new priority_thread_worker object.
		 * @param priorities A vector of priority levels this worker can process.
		 * @param use_time_tag A boolean flag indicating whether to use time tags (default is true).
		 */
		priority_thread_worker(std::vector<priority_type> priorities,
							   const bool& use_time_tag = true);

		/**
		 * @brief Virtual destructor for the priority_thread_worker class.
		 */
		virtual ~priority_thread_worker(void);

		/**
		 * @brief Sets the job queue for this worker to process.
		 * @param job_queue A shared pointer to the priority job queue.
		 */
		auto set_job_queue(std::shared_ptr<priority_job_queue<priority_type>> job_queue) -> void;

		/**
		 * @brief Gets the priority levels this worker can process.
		 * @return std::vector<priority_type> A vector of priority levels.
		 */
		[[nodiscard]] auto priorities(void) const -> std::vector<priority_type>;

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
		bool use_time_tag_;						///< Flag indicating whether to use time tags
		std::vector<priority_type> priorities_; ///< The priority levels this worker can process
		std::shared_ptr<priority_job_queue<priority_type>>
			job_queue_;							///< The priority job queue to process
	};
} // namespace priority_thread_pool_module

#include "priority_thread_worker.tpp"