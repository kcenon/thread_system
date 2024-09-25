#pragma once

#include "job.h"

using namespace thread_module;

namespace priority_thread_pool_module
{
	template <typename priority_type> class priority_job_queue;

	/**
	 * @class priority_job
	 * @brief Represents a job with a priority level.
	 *
	 * This template class encapsulates a job with a callback function, a priority level,
	 * and a name. It inherits from std::enable_shared_from_this to allow creating shared_ptr from
	 * this.
	 *
	 * @tparam priority_type The type used to represent the priority level.
	 */
	template <typename priority_type> class priority_job : public job
	{
	public:
		/**
		 * @brief Constructs a new priority_job object.
		 * @param callback The function to be executed when the job is processed.
		 * @param priority The priority level of the job.
		 * @param name The name of the job (default is "priority_job").
		 */
		priority_job(
			const std::function<std::tuple<bool, std::optional<std::string>>(void)>& callback,
			priority_type priority,
			const std::string& name = "priority_job");

		/**
		 * @brief Virtual destructor for the priority_job class.
		 */
		virtual ~priority_job(void) override;

	public:
		/**
		 * @brief Get the priority level of the job.
		 * @return priority_type The priority level of the job.
		 */
		[[nodiscard]] auto priority() const -> priority_type;

		/**
		 * @brief Set the job queue for this job.
		 * @param job_queue A shared pointer to the priority job queue.
		 */
		auto set_job_queue(const std::shared_ptr<job_queue>& job_queue) -> void override;

		/**
		 * @brief Get the job queue associated with this job.
		 * @return std::shared_ptr<priority_job_queue<priority_type>> A shared pointer to the
		 * associated priority job queue.
		 */
		auto get_job_queue(void) const -> std::shared_ptr<job_queue> override;

	private:
		priority_type priority_; ///< The priority level of the job.
		std::weak_ptr<priority_job_queue<priority_type>>
			job_queue_;			 ///< A weak pointer to the associated priority job queue.
	};
} // namespace priority_thread_pool_module

#include "priority_job.tpp"