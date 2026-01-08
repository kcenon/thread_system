/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, kcenon
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

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <thread>

namespace kcenon::thread::diagnostics
{
	/**
	 * @enum job_status
	 * @brief Status of a job in the thread pool.
	 *
	 * @ingroup diagnostics
	 *
	 * Represents the current lifecycle state of a job from enqueue to completion.
	 */
	enum class job_status
	{
		pending,    ///< Job is waiting in the queue
		running,    ///< Job is currently being executed
		completed,  ///< Job completed successfully
		failed,     ///< Job failed with an error
		cancelled,  ///< Job was cancelled before completion
		timed_out   ///< Job exceeded its timeout limit
	};

	/**
	 * @brief Converts job_status to human-readable string.
	 * @param status The status to convert.
	 * @return String representation of the status.
	 */
	[[nodiscard]] inline auto job_status_to_string(job_status status) -> std::string
	{
		switch (status)
		{
			case job_status::pending: return "pending";
			case job_status::running: return "running";
			case job_status::completed: return "completed";
			case job_status::failed: return "failed";
			case job_status::cancelled: return "cancelled";
			case job_status::timed_out: return "timed_out";
			default: return "unknown";
		}
	}

	/**
	 * @struct job_info
	 * @brief Information about a job in the thread pool.
	 *
	 * @ingroup diagnostics
	 *
	 * Contains comprehensive information about a job including its identity,
	 * timing information, execution status, and error details.
	 *
	 * ### Timing Diagram
	 * ```
	 * enqueue_time                start_time                    end_time
	 *     |                          |                             |
	 *     v                          v                             v
	 *     [=======wait_time=========][====execution_time==========]
	 *     |<----- pending ---------->|<-------- running --------->|
	 * ```
	 *
	 * ### Usage Example
	 * @code
	 * auto info = diagnostics.get_active_jobs()[0];
	 * if (info.status == job_status::running) {
	 *     auto elapsed = std::chrono::steady_clock::now() - info.start_time;
	 *     LOG_INFO("Job {} running for {}ms", info.job_name,
	 *              std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
	 * }
	 * @endcode
	 */
	struct job_info
	{
		/**
		 * @brief Unique identifier for this job.
		 *
		 * Generated when the job is created, unique within the lifetime
		 * of the thread pool.
		 */
		std::uint64_t job_id{0};

		/**
		 * @brief Human-readable name or description of the job.
		 *
		 * May be empty if the job was not named. Used for logging
		 * and debugging purposes.
		 */
		std::string job_name;

		/**
		 * @brief Time when the job was added to the queue.
		 */
		std::chrono::steady_clock::time_point enqueue_time;

		/**
		 * @brief Time when the job started executing.
		 *
		 * Only valid if status >= running. Will be the same as enqueue_time
		 * for pending jobs until they start.
		 */
		std::chrono::steady_clock::time_point start_time;

		/**
		 * @brief Time when the job finished (completed, failed, or cancelled).
		 *
		 * Only has a value if the job has finished execution.
		 */
		std::optional<std::chrono::steady_clock::time_point> end_time;

		/**
		 * @brief Time spent waiting in the queue before execution.
		 *
		 * Calculated as: start_time - enqueue_time.
		 * For pending jobs, this is the current wait time.
		 */
		std::chrono::nanoseconds wait_time{0};

		/**
		 * @brief Time spent executing the job.
		 *
		 * Calculated as: end_time - start_time.
		 * For running jobs, this is the current execution time.
		 */
		std::chrono::nanoseconds execution_time{0};

		/**
		 * @brief Current status of the job.
		 */
		job_status status{job_status::pending};

		/**
		 * @brief Error message if the job failed.
		 *
		 * Only has a value if status == failed or status == timed_out.
		 */
		std::optional<std::string> error_message;

		/**
		 * @brief ID of the thread that executed/is executing the job.
		 *
		 * Only valid if status >= running.
		 */
		std::thread::id executed_by;

		/**
		 * @brief Stack trace captured when the job failed.
		 *
		 * Only has a value if status == failed and stack trace
		 * capture was enabled.
		 */
		std::optional<std::string> stack_trace;

		/**
		 * @brief Calculates total latency (wait + execution time).
		 * @return Total time from enqueue to completion.
		 */
		[[nodiscard]] auto total_latency() const -> std::chrono::nanoseconds
		{
			return wait_time + execution_time;
		}

		/**
		 * @brief Checks if the job has finished execution.
		 * @return true if completed, failed, cancelled, or timed_out.
		 */
		[[nodiscard]] auto is_finished() const -> bool
		{
			return status == job_status::completed ||
			       status == job_status::failed ||
			       status == job_status::cancelled ||
			       status == job_status::timed_out;
		}

		/**
		 * @brief Checks if the job is still active (pending or running).
		 * @return true if pending or running.
		 */
		[[nodiscard]] auto is_active() const -> bool
		{
			return status == job_status::pending ||
			       status == job_status::running;
		}
	};

} // namespace kcenon::thread::diagnostics
