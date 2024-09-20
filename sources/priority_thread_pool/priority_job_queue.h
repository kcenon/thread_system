#pragma once

#include "job_queue.h"
#include "priority_job.h"

#include <map>

/**
 * @class priority_job_queue
 * @brief A queue for managing priority jobs.
 *
 * This template class implements a queue that manages jobs with different priority levels.
 * It inherits from std::enable_shared_from_this to allow creating shared_ptr from this.
 *
 * @tparam priority_type The type used to represent the priority levels.
 */
template <typename priority_type> class priority_job_queue : public job_queue
{
public:
	/**
	 * @brief Constructs a new priority_job_queue object.
	 */
	priority_job_queue(void);

	/**
	 * @brief Virtual destructor for the priority_job_queue class.
	 */
	virtual ~priority_job_queue(void) override;

public:
	/**
	 * @brief Enqueues a job into the queue.
	 * @param value A unique pointer to the job to be enqueued.
	 * @return std::tuple<bool, std::optional<std::string>> A tuple containing:
	 *         - bool: Indicates whether the enqueue operation was successful (true) or not (false).
	 *         - std::optional<std::string>: An optional string message, typically used for error
	 * descriptions.
	 */
	[[nodiscard]] auto enqueue(std::unique_ptr<job>&& value)
		-> std::tuple<bool, std::optional<std::string>> override;

	/**
	 * @brief Enqueues a priority job into the queue.
	 * @param value A unique pointer to the priority job to be enqueued.
	 * @return std::tuple<bool, std::optional<std::string>> A tuple containing:
	 *         - bool: Indicates whether the enqueue operation was successful (true) or not (false).
	 *         - std::optional<std::string>: An optional string message, typically used for error
	 * descriptions.
	 */
	[[nodiscard]] auto enqueue(std::unique_ptr<priority_job<priority_type>>&& value)
		-> std::tuple<bool, std::optional<std::string>>;

	/**
	 * @brief Dequeues a job from the queue.
	 * @return std::tuple<std::optional<std::unique_ptr<job>>, std::optional<std::string>> A tuple
	 * containing:
	 *         - std::optional<std::unique_ptr<job>>: The dequeued job if available, or nullopt if
	 * the queue is empty.
	 *         - std::optional<std::string>: An optional string message, typically used for error
	 * descriptions.
	 */
	[[nodiscard]] auto dequeue(void)
		-> std::tuple<std::optional<std::unique_ptr<job>>, std::optional<std::string>> override;

	/**
	 * @brief Dequeues a job from the queue based on the given priorities.
	 * @param priorities A vector of priority levels to consider when dequeuing.
	 * @return std::tuple<std::optional<std::unique_ptr<priority_job<priority_type>>>,
	 * std::optional<std::string>> A tuple containing:
	 *         - std::optional<std::unique_ptr<priority_job<priority_type>>>: The dequeued job if
	 * available, or nullopt if no job is found.
	 *         - std::optional<std::string>: An optional string message, typically used for error
	 * descriptions.
	 */
	[[nodiscard]] auto dequeue(const std::vector<priority_type>& priorities)
		-> std::tuple<std::optional<std::unique_ptr<priority_job<priority_type>>>,
					  std::optional<std::string>>;

	/**
	 * @brief Clears all jobs from the queue.
	 */
	auto clear() -> void override;

	/**
	 * @brief Checks if the queue is empty for the given priorities.
	 * @param priorities A vector of priority levels to check.
	 * @return bool True if the queue is empty for all given priorities, false otherwise.
	 */
	[[nodiscard]] auto empty(const std::vector<priority_type>& priorities) const -> bool;

protected:
	/**
	 * @brief Checks if the queue is empty for the given priorities without locking.
	 * @param priorities A vector of priority levels to check.
	 * @return bool True if the queue is empty for all given priorities, false otherwise.
	 */
	[[nodiscard]] auto empty_check_without_lock(const std::vector<priority_type>& priorities) const
		-> bool;

	/**
	 * @brief Attempts to dequeue a job from a specific priority level.
	 * @param priority The priority level to dequeue from.
	 * @return std::optional<std::unique_ptr<priority_job<priority_type>>> The dequeued job if
	 * available, or nullopt if no job is found.
	 */
	[[nodiscard]] auto try_dequeue_from_priority(const priority_type& priority)
		-> std::optional<std::unique_ptr<priority_job<priority_type>>>;

private:
	std::map<priority_type, std::queue<std::unique_ptr<priority_job<priority_type>>>>
		queues_; ///< Map of priority queues
};

#include "priority_job_queue.tpp"