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
#include "job_priorities.h"

using namespace thread_module;

namespace priority_thread_pool_module
{
	template <typename priority_type> class priority_job_queue_t;

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
	template <typename priority_type> class priority_job_t : public job
	{
	public:
		/**
		 * @brief Constructs a new priority_job object.
		 * @param callback The function to be executed when the job is processed.
		 * @param priority The priority level of the job.
		 * @param name The name of the job (default is "priority_job").
		 */
		priority_job_t(priority_type priority, const std::string& name = "priority_job");

		/**
		 * @brief Virtual destructor for the priority_job class.
		 */
		~priority_job_t(void) override;

		/**
		 * @brief Get the priority level of the job.
		 * @return priority_type The priority level of the job.
		 */
		[[nodiscard]] auto priority() const -> priority_type;

		/**
		 * @brief Set the job queue for this job.
		 * @param job_queue A shared pointer to the priority job queue.
		 */
		auto set_job_queue(const std::shared_ptr<job_queue>& job_queue) -> void override;

		/**
		 * @brief Get the job queue associated with this job.
		 * @return std::shared_ptr<job_queue> A shared pointer to the associated priority job queue.
		 */
		[[nodiscard]] auto get_job_queue(void) const -> std::shared_ptr<job_queue> override;

	private:
		/** @brief The priority level of the job. */
		priority_type priority_;

		/** @brief A weak pointer to the associated priority job queue. */
		std::weak_ptr<priority_job_queue_t<priority_type>> job_queue_;
	};

	using priority_job = priority_job_t<job_priorities>;
} // namespace priority_thread_pool_module

#include "priority_job.tpp"