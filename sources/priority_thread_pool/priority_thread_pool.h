#pragma once

#include "priority_job_queue.h"
#include "priority_thread_worker.h"

#include <tuple>
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <optional>

/**
 * @class priority_thread_pool
 * @brief A thread pool class that manages priority-based jobs and workers.
 *
 * This template class implements a thread pool that manages jobs and workers
 * with different priority levels. It inherits from std::enable_shared_from_this
 * to allow creating shared_ptr from this.
 *
 * @tparam priority_type The type used to represent the priority levels.
 */
template <typename priority_type>
class priority_thread_pool
	: public std::enable_shared_from_this<priority_thread_pool<priority_type>>
{
public:
	/**
	 * @brief Constructs a new priority_thread_pool object.
	 */
	priority_thread_pool(void);

	/**
	 * @brief Virtual destructor for the priority_thread_pool class.
	 */
	virtual ~priority_thread_pool(void);

	/**
	 * @brief Get a shared pointer to this priority_thread_pool object.
	 * @return std::shared_ptr<priority_thread_pool<priority_type>> A shared pointer to this
	 * priority_thread_pool.
	 */
	[[nodiscard]] auto get_ptr(void) -> std::shared_ptr<priority_thread_pool<priority_type>>;

public:
	/**
	 * @brief Starts the thread pool.
	 * @return std::tuple<bool, std::optional<std::string>> A tuple containing:
	 *         - bool: Indicates whether the start operation was successful (true) or not (false).
	 *         - std::optional<std::string>: An optional string message, typically used for error
	 * descriptions.
	 */
	auto start(void) -> std::tuple<bool, std::optional<std::string>>;

	/**
	 * @brief Gets the job queue associated with this thread pool.
	 * @return std::shared_ptr<priority_job_queue<priority_type>> A shared pointer to the priority
	 * job queue.
	 */
	[[nodiscard]] auto get_job_queue(void) -> std::shared_ptr<priority_job_queue<priority_type>>;

	/**
	 * @brief Enqueues a priority job to the thread pool's job queue.
	 * @param job A unique pointer to the priority job to be enqueued.
	 * @return std::tuple<bool, std::optional<std::string>> A tuple containing:
	 *         - bool: Indicates whether the enqueue operation was successful (true) or not (false).
	 *         - std::optional<std::string>: An optional string message, typically used for error
	 * descriptions.
	 */
	auto enqueue(std::unique_ptr<priority_job<priority_type>>&& job)
		-> std::tuple<bool, std::optional<std::string>>;

	/**
	 * @brief Enqueues a priority thread worker to the thread pool.
	 * @param worker A unique pointer to the priority thread worker to be enqueued.
	 * @return std::tuple<bool, std::optional<std::string>> A tuple containing:
	 *         - bool: Indicates whether the enqueue operation was successful (true) or not (false).
	 *         - std::optional<std::string>: An optional string message, typically used for error
	 * descriptions.
	 */
	auto enqueue(std::unique_ptr<priority_thread_worker<priority_type>>&& worker)
		-> std::tuple<bool, std::optional<std::string>>;

	/**
	 * @brief Stops the thread pool.
	 * @param immediately_stop If true, stops the pool immediately; if false, allows current jobs to
	 * finish (default is false).
	 */
	auto stop(const bool& immediately_stop = false) -> void;

private:
	std::atomic<bool> start_pool_; ///< Flag indicating whether the pool is started
	std::shared_ptr<priority_job_queue<priority_type>>
		job_queue_;				   ///< The priority job queue for the thread pool
	std::vector<std::unique_ptr<priority_thread_worker<priority_type>>>
		workers_;				   ///< Collection of priority thread workers
};

#include "priority_thread_pool.tpp"