#pragma once

#include "job.h"

#include <mutex>
#include <queue>
#include <tuple>
#include <atomic>
#include <optional>
#include <condition_variable>

/**
 * @class job_queue
 * @brief Represents a thread-safe queue for managing jobs.
 *
 * This class provides a mechanism for enqueueing and dequeueing jobs in a thread-safe manner.
 * It inherits from std::enable_shared_from_this to allow creating shared_ptr from this.
 */
class job_queue : public std::enable_shared_from_this<job_queue>
{
public:
	/**
	 * @brief Constructs a new job_queue object.
	 */
	job_queue();

	/**
	 * @brief Virtual destructor for the job_queue class.
	 */
	virtual ~job_queue(void);

	/**
	 * @brief Get a shared pointer to this job_queue object.
	 * @return std::shared_ptr<job_queue> A shared pointer to this job_queue.
	 */
	std::shared_ptr<job_queue> get_ptr(void);

public:
	/**
	 * @brief Checks if the job queue is stopped.
	 * @return bool True if the job queue is stopped, false otherwise.
	 */
	[[nodiscard]]
	auto is_stopped() const -> bool;

	/**
	 * @brief Sets the notify flag for the job queue.
	 * @param notify The value to set the notify flag to.
	 */
	auto set_notify(bool notify) -> void;

	/**
	 * @brief Enqueues a job into the queue.
	 * @param value A unique pointer to the job to be enqueued.
	 * @return std::tuple<bool, std::optional<std::string>> A tuple containing:
	 *         - bool: Indicates whether the enqueue operation was successful (true) or not (false).
	 *         - std::optional<std::string>: An optional string message, typically used for error
	 * descriptions.
	 */
	[[nodiscard]] virtual auto enqueue(std::unique_ptr<job>&& value)
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
	[[nodiscard]] virtual auto dequeue(void)
		-> std::tuple<std::optional<std::unique_ptr<job>>, std::optional<std::string>>;

	/**
	 * @brief Clears all jobs from the queue.
	 */
	virtual auto clear(void) -> void;

	/**
	 * @brief Checks if the queue is empty.
	 * @return bool True if the queue is empty, false otherwise.
	 */
	[[nodiscard]] auto empty(void) const -> bool;

	/**
	 * @brief Stops the job queue.
	 *
	 */
	auto stop_waiting_dequeue(void) -> void;

	/**
	 * @brief Dequeues all jobs from the queue.
	 * @return std::queue<std::unique_ptr<job>> A queue containing all the jobs that were in the
	 * job_queue.
	 */
	[[nodiscard]] auto dequeue_all(void) -> std::queue<std::unique_ptr<job>>;

protected:
	std::atomic_bool notify_;				 ///< Flag indicating whether to notify when enqueuing
	std::atomic_bool stop_;					 ///< Flag indicating whether the queue is stopped
	mutable std::mutex mutex_;				 ///< Mutex for thread-safe operations
	std::condition_variable condition_;		 ///< Condition variable for signaling between threads
	std::queue<std::unique_ptr<job>> queue_; ///< The underlying queue of jobs
};