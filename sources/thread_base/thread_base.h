/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

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
		[[nodiscard]] virtual auto has_work(void) const -> bool { return false; }

		/**
		 * @brief Called before the worker thread starts.
		 * Derived classes can override this to perform initialization.
		 * @return std::tuple<bool, std::optional<std::string>> A tuple containing:
		 *         - bool: Indicates whether the initialization was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		virtual auto before_start(void) -> std::tuple<bool, std::optional<std::string>>
		{
			return { true, std::nullopt };
		}

		/**
		 * @brief Performs the actual work of the thread.
		 * Derived classes should override this to define the thread's behavior.
		 * @return std::tuple<bool, std::optional<std::string>> A tuple containing:
		 *         - bool: Indicates whether the work was successful (true) or not (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		virtual auto do_work(void) -> std::tuple<bool, std::optional<std::string>>
		{
			return { true, std::nullopt };
		}

		/**
		 * @brief Called after the worker thread stops.
		 * Derived classes can override this to perform cleanup.
		 * @return std::tuple<bool, std::optional<std::string>> A tuple containing:
		 *         - bool: Indicates whether the cleanup was successful (true) or not (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		virtual auto after_stop(void) -> std::tuple<bool, std::optional<std::string>>
		{
			return { true, std::nullopt };
		}

	protected:
		/** @brief Interval at which the thread wakes up */
		std::optional<std::chrono::milliseconds> wake_interval_;

	private:
		/** @brief Mutex for protecting shared data and condition variable */
		std::mutex cv_mutex_;

		/** @brief Condition variable for thread synchronization */
		std::condition_variable worker_condition_;

#ifdef USE_STD_JTHREAD
		/** @brief The actual worker thread (jthread version) */
		std::unique_ptr<std::jthread> worker_thread_;

		/** @brief Source for stopping the thread (jthread version) */
		std::optional<std::stop_source> stop_source_;
#else
		/** @brief The actual worker thread */
		std::unique_ptr<std::thread> worker_thread_;

		/** @brief Flag indicating whether the thread should stop */
		std::atomic<bool> stop_requested_;
#endif
	};
} // namespace thread_module