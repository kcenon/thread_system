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

#include "priority_job.h"

using namespace thread_module;

namespace priority_thread_pool_module
{
	/**
	 * @class callback_priority_job_t
	 * @brief A template for creating priority-based jobs that execute a user-defined callback.
	 *
	 * This class inherits from @c priority_job_t and stores a callback function along with a
	 * priority value. When scheduled by a priority-based thread pool or any task scheduler,
	 * the higher-priority jobs generally take precedence.
	 *
	 * @tparam priority_type
	 *   The type used to represent the priority level.
	 *   Typically an enum or comparable value to determine job ordering.
	 */
	template <typename priority_type>
	class callback_priority_job_t : public priority_job_t<priority_type>
	{
	public:
		/**
		 * @brief Constructs a new @c callback_priority_job_t with a callback, priority, and name.
		 *
		 * @param callback
		 *   The function object to be executed when the job is processed. It must return
		 *   an @c std::optional<std::string>, where the string typically contains additional
		 *   status or error information.
		 * @param priority
		 *   The priority level of the job (e.g., high, normal, low).
		 * @param name
		 *   The name of the job, used primarily for logging or debugging. Defaults to
		 * "priority_job".
		 *
		 * Example usage:
		 * @code
		 * auto jobCallback = []() -> std::optional<std::string> {
		 *     // Your job logic here
		 *     return {};
		 * };
		 * auto myJob = std::make_shared<callback_priority_job_t<int>>(jobCallback, 10, "MyJob");
		 * @endcode
		 */
		callback_priority_job_t(const std::function<std::optional<std::string>(void)>& callback,
								priority_type priority,
								const std::string& name = "priority_job");

		/**
		 * @brief Virtual destructor for the @c callback_priority_job_t class.
		 */
		~callback_priority_job_t(void) override;

		/**
		 * @brief Executes the stored callback function for this job.
		 *
		 * This method overrides @c priority_job_t::do_work. When invoked by the job executor,
		 * the stored callback will be called and its result will be propagated.
		 *
		 * @return std::optional<std::string>
		 *   - If the callback returns a string, it typically contains an informational or error
		 * message.
		 *   - If it returns an empty @c std::optional, the execution completed without additional
		 * info.
		 */
		[[nodiscard]] auto do_work(void) -> std::optional<std::string> override;

	private:
		/**
		 * @brief The user-provided callback function to execute when the job is processed.
		 *
		 * This function should encapsulate the main logic of the job.
		 * It must return an @c std::optional<std::string>, often representing
		 * any error messages or status feedback.
		 */
		std::function<std::optional<std::string>(void)> callback_;
	};

	/**
	 * @typedef callback_priority_job
	 * @brief Type alias for a @c callback_priority_job_t that uses @c job_priorities as its
	 * priority type.
	 */
	using callback_priority_job = callback_priority_job_t<job_priorities>;
} // namespace priority_thread_pool_module

#include "callback_priority_job.tpp"
