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

#include <functional>
#include <optional>
#include <string>

namespace thread_module
{
	/**
	 * @class callback_job
	 * @brief A specialized job class that encapsulates a user-defined callback.
	 *
	 * The @c callback_job class provides an interface for executing a user-supplied
	 * function within a job queue. The callback function returns an @c std::optional<std::string>,
	 * where a value (string) typically indicates an error message and @c std::nullopt signifies
	 * a successful operation.
	 *
	 * Usage Example:
	 * @code
	 * auto job = std::make_shared<callback_job>(
	 *     []() -> std::optional<std::string> {
	 *         // Perform some work here...
	 *         bool success = do_some_work();
	 *         if (!success) {
	 *             return std::string{"Work failed due to ..."};
	 *         }
	 *         return std::nullopt; // No error => success
	 *     },
	 *     "example_callback_job"
	 * );
	 * // Submit job to a queue or execute it directly...
	 * @endcode
	 */
	class callback_job : public job
	{
	public:
		/**
		 * @brief Constructs a new @c callback_job instance.
		 * @param callback A function object that, when invoked, performs the job's work.
		 *                 - Returns @c std::nullopt on success.
		 *                 - Returns a @c std::string on failure (the string can be treated as
		 *                   an error message or diagnostic detail).
		 * @param name     An optional name for this job (default is "callback_job").
		 *
		 * Example:
		 * @code
		 * // A job that reports "Error occurred" if some_condition is true
		 * callback_job(
		 *     []() {
		 *         if (some_condition) {
		 *             return std::optional<std::string>{"Error occurred"};
		 *         }
		 *         return std::nullopt;
		 *     },
		 *     "my_named_job"
		 * );
		 * @endcode
		 */
		callback_job(const std::function<std::optional<std::string>(void)>& callback,
					 const std::string& name = "callback_job");

		/**
		 * @brief Virtual destructor.
		 *
		 * Ensures derived classes can clean up resources properly.
		 */
		~callback_job(void) override;

		/**
		 * @brief Executes the callback function to perform the job's work.
		 * @return @c std::optional<std::string>
		 *         - If @c std::nullopt, the job is considered successful.
		 *         - If a @c std::string is returned, it typically represents an error message or
		 *           reason for failure.
		 *
		 * This method is called internally by the job queue or any mechanism that processes
		 * @c job instances. In user code, you generally won't call @c do_work directly unless you
		 * are bypassing a job queue and want to execute the job on demand.
		 */
		[[nodiscard]] auto do_work(void) -> std::optional<std::string> override;

	protected:
		/**
		 * @brief The user-defined callback that encapsulates the job's core logic.
		 *
		 * This function is invoked by @c do_work(). If it returns @c std::nullopt, the job
		 * is considered successful; otherwise, the returned string is considered an error message.
		 */
		std::function<std::optional<std::string>(void)> callback_;
	};
} // namespace thread_module
