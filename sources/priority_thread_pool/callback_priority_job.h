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
	 * @class priority_job
	 * @brief Represents a job with a priority level.
	 *
	 * This template class encapsulates a job with a callback function, a priority level,
	 * and a name. It inherits from job and std::enable_shared_from_this to allow creating
	 * shared_ptr from this.
	 *
	 * @tparam priority_type The type used to represent the priority level.
	 */
	template <typename priority_type>
	class callback_priority_job_t : public priority_job_t<priority_type>
	{
	public:
		/**
		 * @brief Constructs a new priority_job object.
		 * @param callback The function to be executed when the job is processed.
		 * @param priority The priority level of the job.
		 * @param name The name of the job (default is "priority_job").
		 */
		callback_priority_job_t(const std::function<std::optional<std::string>(void)>& callback,
								priority_type priority,
								const std::string& name = "priority_job");

		/**
		 * @brief Virtual destructor for the priority_job class.
		 */
		~callback_priority_job_t(void) override;

		/**
		 * @brief Execute the job's work.
		 * @return std::optional<std::string> A tuple containing:
		 *         - bool: Indicates whether the job was successful (true) or not (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions or additional information.
		 */
		[[nodiscard]] auto do_work(void) -> std::optional<std::string> override;

	private:
		/** @brief The callback function to be executed when the job is processed */
		std::function<std::optional<std::string>(void)> callback_;
	};

	using callback_priority_job = callback_priority_job_t<job_priorities>;
} // namespace priority_thread_pool_module

#include "callback_priority_job.tpp"