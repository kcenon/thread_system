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

#include "priority_job_queue.h"
#include "priority_thread_worker.h"

#include <tuple>
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <optional>

using namespace thread_module;

namespace priority_thread_pool_module
{
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

		/**
		 * @brief Starts the thread pool.
		 * @return std::optional<std::string> A tuple containing:
		 *         - bool: Indicates whether the start operation was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto start(void) -> std::optional<std::string>;

		/**
		 * @brief Gets the job queue associated with this thread pool.
		 * @return std::shared_ptr<priority_job_queue<priority_type>> A shared pointer to the
		 * priority job queue.
		 */
		[[nodiscard]] auto get_job_queue(void)
			-> std::shared_ptr<priority_job_queue<priority_type>>;

		/**
		 * @brief Enqueues a priority job to the thread pool's job queue.
		 * @param job A unique pointer to the priority job to be enqueued.
		 * @return std::optional<std::string> A tuple containing:
		 *         - bool: Indicates whether the enqueue operation was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto enqueue(std::unique_ptr<priority_job<priority_type>>&& job)
			-> std::optional<std::string>;

		/**
		 * @brief Enqueues a priority thread worker to the thread pool.
		 * @param worker A unique pointer to the priority thread worker to be enqueued.
		 * @return std::optional<std::string> A tuple containing:
		 *         - bool: Indicates whether the enqueue operation was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto enqueue(std::unique_ptr<priority_thread_worker<priority_type>>&& worker)
			-> std::optional<std::string>;

		/**
		 * @brief Stops the thread pool.
		 * @param immediately_stop If true, stops the pool immediately; if false, allows current
		 * jobs to finish (default is false).
		 */
		auto stop(const bool& immediately_stop = false) -> void;

	private:
		/** @brief Flag indicating whether the pool is started */
		std::atomic<bool> start_pool_;

		/** @brief The priority job queue for the thread pool */
		std::shared_ptr<priority_job_queue<priority_type>> job_queue_;

		/** @brief Collection of priority thread workers */
		std::vector<std::unique_ptr<priority_thread_worker<priority_type>>> workers_;
	};
} // namespace priority_thread_pool_module

#include "priority_thread_pool.tpp"