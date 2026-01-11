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

#include <kcenon/thread/core/job.h>
#include "job_types.h"

namespace kcenon::thread
{
	/**
	 * @class typed_job_t
	 * @brief Represents a job that carries a specific priority level.
	 *
	 * This class extends the base @c job interface to include a priority value
	 * that is used by a priority-based scheduling system. The job uses the
	 * base class's weak reference to a @c job_queue for queue management.
	 *
	 * @tparam job_type The data type used to represent the priority level.
	 *         Typically, an enum or an integral type.
	 */
	template <typename job_type> class typed_job_t : public job
	{
	public:
		/**
		 * @brief Constructs a new @c typed_job_t object with the given priority and name.
		 *
		 * @param priority The priority level for this job. RealTimeer values could
		 *                 indicate higher priority, depending on your scheduling logic.
		 * @param name     An optional name for the job, useful for debugging or logging.
		 *                 Defaults to "typed_job".
		 */
		typed_job_t(job_type priority, const std::string& name = "typed_job");

		/**
		 * @brief Destroys the @c typed_job_t object.
		 *
		 * The destructor is virtual to ensure proper cleanup in derived classes.
		 */
		~typed_job_t(void) override;

		/**
		 * @brief Retrieves the priority level of this job.
		 *
		 * @return The @c job_type value that indicates this job's priority.
		 */
		[[nodiscard]] auto priority() const -> job_type { return priority_; }

	private:
		/**
		 * @brief The priority level assigned to this job.
		 */
		job_type priority_;
	};

	/**
	 * @typedef typed_job
	 * @brief A convenient alias for @c typed_job_t using the @c job_types type.
	 */
	using typed_job = typed_job_t<job_types>;

} // namespace kcenon::thread

// Note: Template implementation moved inline or will be provided by explicit instantiation
