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

#include "log_job.h"
#include "log_types.h"
#include "../utilities/formatter.h"
#include "../thread_base/job_queue.h"
#include "../thread_base/thread_base.h"
#include "message_job.h"

using namespace thread_module;
using namespace utility_module;

namespace log_module
{
	/**
	 * @class log_collector
	 * @brief A class for collecting and managing log messages.
	 *
	 * This class inherits from `thread_base` and provides functionality for collecting,
	 * processing, and distributing log messages to console, file, and callback outputs.
	 * It manages separate queues for log collection and output handling. The class supports
	 * various string types for log messages including `std::string` and `std::wstring`.
	 */
	class log_collector : public thread_base
	{
	public:
		/**
		 * @brief Constructor for the `log_collector` class.
		 *
		 * Initializes the log queue and sets up default log types for console, file, and callback
		 * outputs.
		 */
		log_collector(void);

		/**
		 * @brief Sets the log types that should be written to the console.
		 * @param type The log types to be written to the console.
		 */
		auto console_target(const log_types& type) -> void;

		/**
		 * @brief Gets the log types that are written to the console.
		 * @return The log types that are currently set to be written to the console.
		 */
		[[nodiscard]] auto console_target() const -> log_types;

		/**
		 * @brief Sets the log types that should be written to a file.
		 * @param type The log types to be written to a file.
		 */
		auto file_target(const log_types& type) -> void;

		/**
		 * @brief Gets the log types that are written to a file.
		 * @return The log types that are currently set to be written to a file.
		 */
		[[nodiscard]] auto file_target() const -> log_types;

		/**
		 * @brief Defines the log types that should be written to a callback.
		 * @param type A `log_types` value indicating the types of log messages to be sent to a
		 * callback.
		 */
		auto callback_target(const log_types& type) -> void;

		/**
		 * @brief Retrieves the current log types that are written to a callback.
		 * @return The `log_types` currently set for callback output.
		 */
		[[nodiscard]] auto callback_target(void) const -> log_types;

		/**
		 * @brief Sets the queue for console output jobs.
		 * @param queue Shared pointer to the job queue for console output.
		 */
		auto set_console_queue(std::shared_ptr<job_queue> queue) -> void;

		/**
		 * @brief Sets the queue for file output jobs.
		 * @param queue Shared pointer to the job queue for file output.
		 */
		auto set_file_queue(std::shared_ptr<job_queue> queue) -> void;

		/**
		 * @brief Sets the queue for callback output jobs.
		 * @param queue Shared pointer to the job queue for callback output.
		 */
		auto set_callback_queue(std::shared_ptr<job_queue> queue) -> void;

		/**
		 * @brief Writes a log message (std::string version).
		 * @param type The type of the log message.
		 * @param message The content of the log message.
		 * @param start_time An optional start time for the log message.
		 */
		auto write(
			const log_types& type,
			const std::string& message,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt) -> void;

		/**
		 * @brief Writes a log message (std::wstring version).
		 * @param type The type of the log message.
		 * @param message The content of the log message as a wide string.
		 * @param start_time An optional start time for the log message.
		 */
		auto write(
			const log_types& type,
			const std::wstring& message,
			std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time
			= std::nullopt) -> void;

		/**
		 * @brief Checks if there are log messages to be processed.
		 * @return True if there are log messages in the queue, false otherwise.
		 */
		[[nodiscard]] auto should_continue_work() const -> bool override;

		/**
		 * @brief Performs initialization before starting the log collector thread.
		 * @return A result_void indicating success or an error.
		 */
		auto before_start() -> result_void override;

		/**
		 * @brief Processes log messages and distributes them to console, file, and callback queues.
		 * @return A result_void indicating success or an error.
		 */
		auto do_work() -> result_void override;

		/**
		 * @brief Performs cleanup after stopping the log collector thread.
		 * @return A result_void indicating success or an error.
		 */
		auto after_stop() -> result_void override;

	protected:
		/**
		 * @brief Enqueues a log message for processing.
		 * @param current_log_type The log type of the current message.
		 * @param target_log_type The log type to be processed.
		 * @param weak_queue A weak pointer to the job queue.
		 * @param datetime The timestamp of the log message.
		 * @param message The content of the log message.
		 * @return A result_void indicating success or an error.
		 */
		auto enqueue_log(const log_types& current_log_type,
						 const log_types& target_log_type,
						 std::weak_ptr<job_queue> weak_queue,
						 const std::string& datetime,
						 const std::string& message) -> result_void;

		/**
		 * @brief Template method for writing log messages of various string types.
		 *
		 * @tparam StringType The type of the string (std::string, std::wstring, std::u16string, or
		 * std::u32string).
		 * @param type The type of the log message.
		 * @param message The content of the log message.
		 * @param start_time An optional start time for the log message.
		 *
		 * This method provides a unified implementation for writing log messages,
		 * regardless of the string type used. It's called by the public write methods.
		 */
		template <typename StringType>
		auto write_string(const log_types& type,
						  const StringType& message,
						  std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>>
							  start_time) -> void
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

			// Add thread safety - lock before accessing shared resource
			std::lock_guard<std::mutex> lock(queue_mutex_);
			if (log_queue_ == nullptr)
			{
				std::cerr << "Cannot enqueue log: queue is null\n";
				return;
			}

			auto enqueue_result = log_queue_->enqueue(std::move(new_log_job));
			if (enqueue_result.has_error())
			{
				std::cerr << formatter::format("error enqueuing log job: {}\n",
											   enqueue_result.get_error().to_string());
			}
		}

	private:
		log_types file_log_type_;			   ///< Types of logs to write to file
		log_types console_log_type_;		   ///< Types of logs to write to console
		log_types callback_log_type_;		   ///< Types of logs to write to message callbacks

		std::shared_ptr<job_queue> log_queue_; ///< Queue for incoming log messages
		std::weak_ptr<job_queue>
			console_queue_;					  ///< Weak pointer to the queue for console output jobs
		std::weak_ptr<job_queue> file_queue_; ///< Weak pointer to the queue for file output jobs
		std::weak_ptr<job_queue>
			callback_queue_; ///< Weak pointer to the queue for callback output jobs
		
		// Mark the mutex as mutable to allow locking in const methods
		mutable std::mutex queue_mutex_; ///< Mutex to protect access to log_queue_ and writer queues
	};
} // namespace log_module