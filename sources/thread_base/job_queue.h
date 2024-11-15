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

#include "job.h"

#include <mutex>
#include <deque>
#include <tuple>
#include <atomic>
#include <optional>
#include <condition_variable>

namespace thread_module
{
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
		[[nodiscard]] std::shared_ptr<job_queue> get_ptr(void);

		/**
		 * @brief Checks if the job queue is stopped.
		 * @return bool True if the job queue is stopped, false otherwise.
		 */
		[[nodiscard]] auto is_stopped() const -> bool;

		/**
		 * @brief Sets the notify flag for the job queue.
		 * @param notify The value to set the notify flag to.
		 */
		auto set_notify(bool notify) -> void;

		/**
		 * @brief Enqueues a job into the queue.
		 * @param value A unique pointer to the job to be enqueued.
		 * @return std::optional<std::string> A tuple containing:
		 *         - bool: Indicates whether the enqueue operation was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		[[nodiscard]] virtual auto enqueue(std::unique_ptr<job>&& value)
			-> std::optional<std::string>;

		/**
		 * @brief Dequeues a job from the queue.
		 * @return std::tuple<std::optional<std::unique_ptr<job>>, std::optional<std::string>> A
		 * tuple containing:
		 *         - std::optional<std::unique_ptr<job>>: The dequeued job if available, or nullopt
		 * if the queue is empty.
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
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
		 * @brief Stops the job queue from waiting for new jobs.
		 */
		auto stop_waiting_dequeue(void) -> void;

		/**
		 * @brief Dequeues all jobs from the queue.
		 * @return std::queue<std::unique_ptr<job>> A queue containing all the jobs that were in the
		 * job_queue.
		 */
		[[nodiscard]] auto dequeue_all(void) -> std::deque<std::unique_ptr<job>>;

	protected:
		/** @brief Flag indicating whether to notify when enqueuing */
		std::atomic_bool notify_;

		/** @brief Flag indicating whether the queue is stopped */
		std::atomic_bool stop_;

		/** @brief Mutex for thread-safe operations */
		mutable std::mutex mutex_;

		/** @brief Condition variable for signaling between threads */
		std::condition_variable condition_;

		/** @brief The underlying deque of jobs */
		std::deque<std::unique_ptr<job>> queue_;
	};
}