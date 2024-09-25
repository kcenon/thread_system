#pragma once

#include <mutex>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>
#include <optional>
#include <condition_variable>

#ifdef USE_STD_JTHREAD
#include <stop_token>
#endif

namespace thread_module
{
	/**
	 * @class thread_base
	 * @brief Base class for implementing thread-based workers.
	 *
	 * This class provides a framework for creating worker threads with customizable
	 * behavior. It handles thread lifecycle management and provides mechanisms for
	 * waking the thread and setting wake intervals.
	 */
	class thread_base : public std::enable_shared_from_this<thread_base>
	{
	public:
		/**
		 * @brief Constructs a new thread_base object.
		 */
		thread_base(void);

		/**
		 * @brief Virtual destructor for the thread_base class.
		 */
		virtual ~thread_base(void);

		/**
		 * @brief Get a shared pointer to this thread_base object.
		 * @return std::shared_ptr<thread_base> A shared pointer to this thread_base.
		 */
		[[nodiscard]] auto get_ptr(void) -> std::shared_ptr<thread_base>;

	public:
		/**
		 * @brief Sets the wake interval for the worker thread.
		 * @param wake_interval An optional duration specifying how often the thread should wake up.
		 */
		auto set_wake_interval(const std::optional<std::chrono::milliseconds>& wake_interval)
			-> void;

		/**
		 * @brief Starts the worker thread.
		 * @return std::tuple<bool, std::optional<std::string>> A tuple containing:
		 *         - bool: Indicates whether the start operation was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto start(void) -> std::tuple<bool, std::optional<std::string>>;

		/**
		 * @brief Stops the worker thread.
		 * @return std::tuple<bool, std::optional<std::string>> A tuple containing:
		 *         - bool: Indicates whether the stop operation was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto stop(void) -> std::tuple<bool, std::optional<std::string>>;

	protected:
		/**
		 * @brief Checks if there is work to be done.
		 * @return bool True if there is work to be done, false otherwise.
		 */
		[[nodiscard]] virtual auto has_work(void) const -> bool { return false; };

		/**
		 * @brief Called before the worker thread starts.
		 * Derived classes can override this to perform initialization.
		 */
		virtual auto before_start(void) -> void {};

		/**
		 * @brief Performs the actual work of the thread.
		 * Derived classes should override this to define the thread's behavior.
		 */
		virtual auto do_work(void) -> void {};

		/**
		 * @brief Called after the worker thread stops.
		 * Derived classes can override this to perform cleanup.
		 */
		virtual auto after_stop(void) -> void {};

	protected:
		std::optional<std::chrono::milliseconds>
			wake_interval_;	   ///< Interval at which the thread wakes up

	private:
		std::mutex cv_mutex_;  ///< Mutex for protecting shared data and condition variable
		std::condition_variable
			worker_condition_; ///< Condition variable for thread synchronization

#ifdef USE_STD_JTHREAD
		std::unique_ptr<std::jthread> worker_thread_; ///< The actual worker thread
		std::optional<std::stop_source> stop_source_; ///< Source for stopping the thread
#else
		std::unique_ptr<std::thread> worker_thread_; ///< The actual worker thread
		std::atomic<bool> stop_requested_; ///< Flag indicating whether the thread should stop
#endif
	};
} // namespace thread_module