#pragma once

#include "job.h"

#include <mutex>
#include <queue>
#include <tuple>
#include <atomic>
#include <optional>
#include <condition_variable>

/**
 * @class queue_base
 * @brief Base class for job queue implementations.
 *
 * This abstract class provides a thread-safe interface for managing job queues.
 * It defines the public interface and provides synchronization mechanisms,
 * while leaving the actual queue operations to be implemented by derived classes.
 */
class queue_base : public std::enable_shared_from_this<queue_base>
{
public:
	/**
	 * @brief Constructs a new queue_base object.
	 */
	queue_base();

	/**
	 * @brief Virtual destructor for the queue_base class.
	 */
	virtual ~queue_base(void);

	/**
	 * @brief Get a shared pointer to this queue_base object.
	 * @return std::shared_ptr<queue_base> A shared pointer to this queue_base.
	 */
	[[nodiscard]] auto get_ptr(void) -> std::shared_ptr<queue_base>;

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
		-> std::tuple<std::optional<std::unique_ptr<job>>, std::optional<std::string>>;

	/**
	 * @brief Clears all jobs from the queue.
	 */
	auto clear(void) -> void;

	/**
	 * @brief Checks if the queue is empty.
	 * @return bool True if the queue is empty, false otherwise.
	 */
	[[nodiscard]] auto empty(void) const -> bool;

	/**
	 * @brief Dequeues all jobs from the queue.
	 * @return std::queue<std::unique_ptr<job>> A queue containing all the jobs that were in the
	 * queue.
	 */
	[[nodiscard]] auto dequeue_all(void) -> std::queue<std::unique_ptr<job>>;

protected:
	/**
	 * @brief Pure virtual function to implement the enqueue operation.
	 * @param value A unique pointer to the job to be enqueued.
	 * @return std::tuple<bool, std::optional<std::string>> A tuple containing the result of the
	 * operation.
	 */
	virtual auto do_enqueue(std::unique_ptr<job>&& value)
		-> std::tuple<bool, std::optional<std::string>> = 0;

	/**
	 * @brief Pure virtual function to implement the dequeue operation.
	 * @return std::tuple<std::optional<std::unique_ptr<job>>, std::optional<std::string>> A tuple
	 * containing the dequeued job and operation result.
	 */
	virtual auto do_dequeue(void)
		-> std::tuple<std::optional<std::unique_ptr<job>>, std::optional<std::string>> = 0;

	/**
	 * @brief Pure virtual function to implement the clear operation.
	 */
	virtual auto do_clear(void) -> void = 0;

	/**
	 * @brief Pure virtual function to implement the empty check operation.
	 * @return bool True if the queue is empty, false otherwise.
	 */
	virtual auto do_empty(void) const -> bool = 0;

	/**
	 * @brief Pure virtual function to implement the dequeue all operation.
	 * @return std::queue<std::unique_ptr<job>> A queue containing all dequeued jobs.
	 */
	virtual auto do_dequeue_all(void) -> std::queue<std::unique_ptr<job>> = 0;

private:
	mutable std::mutex mutex_;			///< Mutex for thread-safe operations
	std::condition_variable condition_; ///< Condition variable for signaling between threads
};