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
#include "job_queue.h"
#include "thread_base.h"

#include <string>
#include <functional>

using namespace thread_module;

namespace log_module
{
	/**
	 * @class callback_writer
	 * @brief A class for handling log messages through a callback function.
	 *
	 * This class inherits from `thread_base` and provides functionality for processing
	 * log messages in a separate thread. The `callback_writer` allows setting a custom
	 * callback function to handle log messages instead of writing them to the console.
	 */
	class callback_writer : public thread_base
	{
	public:
		/**
		 * @brief Constructor for the `callback_writer` class.
		 *
		 * Initializes the job queue for managing log message tasks.
		 */
		callback_writer(void);

		/**
		 * @brief Retrieves the job queue used by the `callback_writer`.
		 *
		 * Provides access to the job queue where log message tasks are enqueued.
		 *
		 * @return std::shared_ptr<job_queue> Shared pointer to the job queue for log tasks.
		 */
		[[nodiscard]] auto get_job_queue() const -> std::shared_ptr<job_queue>
		{
			return job_queue_;
		}

		/**
		 * @brief Sets the callback function for handling log messages.
		 *
		 * Allows setting a custom function to process each log message, receiving the log type,
		 * message, and an optional tag.
		 *
		 * @param callback Function pointer to the callback that handles log messages.
		 */
		auto message_callback(
			const std::function<void(const log_types&, const std::string&, const std::string&)>&
				callback) -> void;

		/**
		 * @brief Checks if there are tasks available in the job queue.
		 *
		 * Determines if there are any log tasks waiting to be processed in the queue.
		 *
		 * @return bool True if tasks are available, false otherwise.
		 */
		[[nodiscard]] auto has_work() const -> bool override;

		/**
		 * @brief Processes log messages by executing the callback function.
		 *
		 * Retrieves and processes log messages from the queue, invoking the callback function for
		 * each.
		 *
		 * @return std::tuple<bool, std::optional<std::string>> A tuple containing:
		 *         - bool: Indicates the success of the operation (true if successful).
		 *         - std::optional<std::string>: Optional error message if processing fails.
		 */
		auto do_work() -> std::tuple<bool, std::optional<std::string>> override;

	private:
		std::shared_ptr<job_queue> job_queue_; ///< Queue containing log tasks
		std::function<void(const log_types&, const std::string&, const std::string&)>
			callback_;						   ///< Callback function to handle log messages
	};
} // namespace log_module