// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/**
 * @file typed_job.h
 * @brief Base typed job carrying a specific priority level.
 *
 * @see typed_thread_pool
 */

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
