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

#include "../thread_base/job_queue.h"
#include "../thread_base/thread_base.h"

using namespace thread_module;

namespace log_module
{
	/**
	 * @class console_writer
	 * @brief A specialized thread class that continuously writes log messages to the console.
	 *
	 * The console_writer class inherits from @c thread_base, enabling it to run as a
	 * background thread. It utilizes a @c job_queue to retrieve pending log messages (or
	 * other tasks) and handles their output to the console. This design allows for asynchronous
	 * logging, ensuring that the main thread(s) can remain unblocked while console operations
	 * happen concurrently.
	 */
	class console_writer : public thread_base
	{
	public:
		/**
		 * @brief Constructs a @c console_writer and initializes its @c job_queue.
		 *
		 * The constructor sets up a @c job_queue to store incoming logging tasks. Once
		 * started, the @c console_writer thread will process these tasks in the background,
		 * directing their output to the console in a safe and orderly manner.
		 */
		console_writer(void);

		/**
		 * @brief Retrieves the @c job_queue used for console logging.
		 *
		 * @return A shared pointer to the @c job_queue instance containing pending log tasks.
		 *
		 * This method allows other components to enqueue new logging tasks or inspect
		 * the current queue state. The returned @c std::shared_ptr<job_queue> ensures
		 * shared ownership and safe access in a multi-threaded environment.
		 */
		[[nodiscard]] auto get_job_queue() const -> std::shared_ptr<job_queue>
		{
			return job_queue_;
		}

		/**
		 * @brief Determines if the thread should continue processing log messages.
		 *
		 * @return @c true if there are still tasks in the queue that need to be processed,
		 *         or if the thread is otherwise signaled to continue running;
		 *         @c false if no further processing is necessary and the thread should exit.
		 *
		 * This method is an override of the @c thread_base::should_continue_work() function.
		 * It typically checks for pending jobs in the queue or other stop conditions.
		 * If @c false is returned, the worker thread will end its execution loop.
		 */
		[[nodiscard]] auto should_continue_work() const -> bool override;

		/**
		 * @brief Performs any necessary initialization before starting the thread.
		 *
		 * @return
		 * - A result_void indicating success, or an error result with a message if initialization fails.
		 *
		 * This method runs once before the worker thread enters its main loop. It can be used
		 * to validate resources, open files, or set up configurations required by the
		 * @c console_writer.
		 */
		auto before_start() -> result_void override;

		/**
		 * @brief The primary work routine that processes and outputs console log messages.
		 *
		 * @return
		 * - A result_void indicating success, or an error result with a message if something
		 *   goes wrong during task processing.
		 *
		 * This method is repeatedly called in a loop while @c should_continue_work() returns
		 * @c true. Each iteration typically pulls one or more tasks from the @c job_queue
		 * and writes them to the console. Any errors encountered can be relayed through
		 * the error result.
		 */
		auto do_work() -> result_void override;

	private:
		/**
		 * @brief Internal queue storing console-writing jobs.
		 *
		 * The @c console_writer continuously retrieves and processes tasks from this queue
		 * while running in its own thread. Each task typically encapsulates data and logic
		 * needed to format and print a message to the console.
		 */
		std::shared_ptr<job_queue> job_queue_;
	};
} // namespace log_module