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

#include "job_queue.h"
#include "thread_base.h"

using namespace thread_module;

namespace log_module
{
	/**
	 * @class console_writer
	 * @brief A class for writing log messages to the console.
	 *
	 * This class inherits from thread_base and provides functionality for
	 * writing log messages to the console in a separate thread. It manages
	 * its own job queue for console writing tasks.
	 */
	class console_writer : public thread_base
	{
	public:
		/**
		 * @brief Constructor for the console_writer class.
		 * Initializes the job queue for console writing tasks.
		 */
		console_writer(void);

		/**
		 * @brief Gets the job queue used by the console writer.
		 * @return A shared pointer to the job queue containing console writing tasks.
		 */
		[[nodiscard]] auto get_job_queue() const -> std::shared_ptr<job_queue>
		{
			return job_queue_;
		}

		/**
		 * @brief Checks if there is work to be done in the job queue.
		 * @return True if there are jobs in the queue, false otherwise.
		 */
		[[nodiscard]] auto has_work() const -> bool override;

		/**
		 * @brief Performs initialization before starting the console writer thread.
		 * @return A tuple containing:
		 *         - bool: Indicates whether the initialization was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto before_start() -> std::tuple<bool, std::optional<std::string>> override;

		/**
		 * @brief Performs the main work of writing log messages to the console.
		 * @return A tuple containing:
		 *         - bool: Indicates whether the operation was successful (true) or not (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		auto do_work() -> std::tuple<bool, std::optional<std::string>> override;

	private:
		/** @brief Queue for log jobs to be written to the console */
		std::shared_ptr<job_queue> job_queue_;
	};
}