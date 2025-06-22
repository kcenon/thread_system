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

#include "log_collector.h"
#include "../../thread_base/lockfree/queues/lockfree_job_queue.h"
#include <atomic>

namespace log_module
{
	/**
	 * @class lockfree_log_collector
	 * @brief A high-performance lock-free log collector for concurrent logging.
	 * 
	 * This class extends log_collector to use lockfree_job_queue instead of
	 * the standard mutex-based job_queue, providing superior performance under
	 * high contention from multiple logging threads.
	 * 
	 * ### Key Features
	 * - **Lock-Free Queue**: Uses lockfree_job_queue for wait-free enqueue
	 * - **Atomic Operations**: Minimizes synchronization overhead
	 * - **Batch Processing**: Supports efficient batch dequeue operations
	 * - **Compatible Interface**: Drop-in replacement for log_collector
	 * 
	 * ### Performance Benefits
	 * - Eliminates mutex contention in log message submission
	 * - Linear scalability with increasing thread count
	 * - Reduced latency for log operations
	 * - Better CPU cache utilization
	 * 
	 * ### Design Principles
	 * - Uses lockfree_job_queue for the main log queue
	 * - Maintains compatibility with existing writer queues
	 * - Provides thread-safe operations without traditional locking
	 */
	class lockfree_log_collector : public log_collector
	{
	public:
		/**
		 * @brief Constructor for the lockfree_log_collector class.
		 * 
		 * Initializes the lock-free log queue and sets up default log types
		 * for console, file, and callback outputs.
		 */
		lockfree_log_collector(void);

		/**
		 * @brief Virtual destructor for proper cleanup.
		 */
		virtual ~lockfree_log_collector(void) = default;

		/**
		 * @brief Writes a log message using lock-free operations (std::string version).
		 * @param type The type of the log message.
		 * @param message The content of the log message.
		 * @param start_time An optional start time for the log message.
		 * 
		 * This method provides wait-free enqueue operations for maximum performance.
		 */
		auto write(
			const log_types& type,
			const std::string& message,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt) -> void;

		/**
		 * @brief Writes a log message using lock-free operations (std::wstring version).
		 * @param type The type of the log message.
		 * @param message The content of the log message as a wide string.
		 * @param start_time An optional start time for the log message.
		 * 
		 * This method provides wait-free enqueue operations for maximum performance.
		 */
		auto write(
			const log_types& type,
			const std::wstring& message,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt) -> void;

		/**
		 * @brief Checks if there are log messages to be processed.
		 * @return True if there are log messages in the queue, false otherwise.
		 * 
		 * Uses lock-free empty() check for minimal overhead.
		 */
		[[nodiscard]] auto should_continue_work() const -> bool override;

		/**
		 * @brief Processes log messages using lock-free dequeue operations.
		 * @return A result_void indicating success or an error.
		 * 
		 * Efficiently dequeues messages from the lock-free queue and
		 * distributes them to the appropriate output queues.
		 */
		auto do_work() -> result_void override;

		/**
		 * @brief Performs initialization before starting the log collector thread.
		 * @return A result_void indicating success or an error.
		 * 
		 * Sets up the lock-free queue and prepares for high-performance logging.
		 */
		auto before_start() -> result_void override;

		/**
		 * @brief Gets performance statistics from the lock-free queue.
		 * @return Queue statistics including throughput and latency metrics.
		 */
		[[nodiscard]] auto get_queue_statistics() const 
			-> lockfree_job_queue::queue_statistics;

	protected:
		/**
		 * @brief Template method for writing log messages using lock-free operations.
		 * 
		 * @tparam StringType The type of the string (std::string, std::wstring, etc.)
		 * @param type The type of the log message.
		 * @param message The content of the log message.
		 * @param start_time An optional start time for the log message.
		 * 
		 * This method provides the core lock-free implementation for log writes.
		 */
		template <typename StringType>
		auto write_string_lockfree(
			const log_types& type,
			const StringType& message,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time) 
			-> void
		{
			std::unique_ptr<log_job> new_log_job;

			try
			{
				new_log_job = std::make_unique<log_job>(message, type, start_time);
			}
			catch (const std::bad_alloc& e)
			{
				std::cerr << "error allocating log job: " << e.what() << std::endl;
				return;
			}

			// Direct access to lock-free queue without mutex
			if (!lockfree_log_queue_)
			{
				std::cerr << "Cannot enqueue log: lockfree queue is null\n";
				return;
			}

			// Wait-free enqueue operation
			auto enqueue_result = lockfree_log_queue_->enqueue(std::move(new_log_job));
			if (enqueue_result.has_error())
			{
				std::cerr << formatter::format("error enqueuing log job: {}\n",
											   enqueue_result.get_error().to_string());
			}
		}

	private:
		/** @brief Lock-free queue for high-performance log message handling */
		std::shared_ptr<lockfree_job_queue> lockfree_log_queue_;

		/** @brief Atomic flag for efficient empty checks */
		mutable std::atomic<bool> has_messages_{false};
	};
} // namespace log_module