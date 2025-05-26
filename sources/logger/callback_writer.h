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

#include "log_types.h"
#include "../thread_base/job_queue.h"
#include "../thread_base/thread_base.h"

#include <string>
#include <functional>

using namespace thread_module;

namespace log_module
{
	/**
	 * @class callback_writer
	 * @brief A class that processes log messages in a separate thread and delivers logs through a
	 * user-defined callback.
	 *
	 * Inherits from `thread_base` to handle log messages in an internal thread.
	 * Instead of printing logs to the console, this class uses a user-defined callback function to
	 * allow flexible integration with external systems or custom processing logic.
	 */
	class callback_writer : public thread_base
	{
	public:
		/**
		 * @brief Default constructor for the `callback_writer` class.
		 *
		 * Initializes the job queue that stores log messages. After construction, log messages are
		 * queued and then processed by invoking the registered callback function in sequence.
		 */
		callback_writer(void);

		/**
		 * @brief Returns the job queue used by this class.
		 *
		 * When a log message is generated, it is first added to this job queue. The internal thread
		 * monitors the queue and delivers the message through the callback function.
		 *
		 * @return std::shared_ptr<job_queue>
		 *         A shared pointer to the internal job queue.
		 */
		[[nodiscard]] auto get_job_queue() const -> std::shared_ptr<job_queue>
		{
			return job_queue_;
		}

		/**
		 * @brief Sets the user-defined callback function that processes log messages.
		 *
		 * The callback function is invoked whenever a log message is retrieved from the queue,
		 * receiving the log type (`log_types`), the log message string, and an optional tag string
		 * as parameters.
		 *
		 * @param callback The log callback function with the signature
		 *        `(const log_types&, const std::string&, const std::string&)`.
		 */
		auto message_callback(
			const std::function<void(const log_types&, const std::string&, const std::string&)>&
				callback) -> void;

		/**
		 * @brief Checks whether there are log messages remaining in the job queue to be processed.
		 *
		 * Internally, this determines if there are any pending jobs (log messages) in the queue.
		 * If there are, the thread continues to run; otherwise, it may be stopped.
		 *
		 * @return bool
		 *         - `true`: There are remaining jobs, so the thread should keep running.
		 *         - `false`: No pending jobs remain, so the thread can stop.
		 */
		[[nodiscard]] auto should_continue_work() const -> bool override;

		/**
		 * @brief Retrieves log messages from the queue and invokes the callback function to process
		 * them.
		 *
		 * This method is called periodically by the internal thread loop, delivering each log
		 * message in the queue to the callback function in turn. If an error occurs during
		 * processing, an error result is returned.
		 *
		 * @return result_void
		 *         - A success result indicates successful processing.
		 *         - An error result contains a description of the error that occurred.
		 */
		auto do_work() -> result_void override;

	private:
		std::shared_ptr<job_queue> job_queue_; ///< A job queue that manages queued log messages.
		std::function<void(const log_types&, const std::string&, const std::string&)>
			callback_; ///< The user-defined callback function for handling log messages.
	};
} // namespace log_module