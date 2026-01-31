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

/**
 * @deprecated This header is deprecated. Use thread_config.h instead.
 *
 * For unified configuration, include:
 * @code{.cpp}
 * #include <kcenon/thread/thread_config.h>
 *
 * auto config = thread_system_config::builder()
 *     .with_dag_failure_policy(dag_failure_policy::retry)
 *     .with_dag_retry_params(3, std::chrono::seconds{1})
 *     .build();
 * @endcode
 */

#include "dag_job.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <string>

namespace kcenon::thread
{
	/**
	 * @enum dag_failure_policy
	 * @brief Defines how the DAG scheduler handles job failures
	 */
	enum class dag_failure_policy
	{
		fail_fast,       ///< Cancel all dependents immediately on failure
		continue_others, ///< Continue unrelated jobs, skip dependents
		retry,           ///< Retry failed job (with max retries)
		fallback         ///< Execute fallback job if available
	};

	/**
	 * @brief Convert dag_failure_policy to string representation
	 * @param policy The policy to convert
	 * @return String representation of the policy
	 */
	[[nodiscard]] inline auto dag_failure_policy_to_string(dag_failure_policy policy) -> std::string
	{
		switch (policy)
		{
			case dag_failure_policy::fail_fast:       return "fail_fast";
			case dag_failure_policy::continue_others: return "continue_others";
			case dag_failure_policy::retry:           return "retry";
			case dag_failure_policy::fallback:        return "fallback";
			default:                                  return "unknown";
		}
	}

	/**
	 * @struct dag_config
	 * @brief Configuration options for the DAG scheduler
	 *
	 * This structure contains all configurable options for DAG execution behavior.
	 */
	struct dag_config
	{
		/**
		 * @brief How to handle job failures
		 *
		 * - fail_fast: Cancel all dependent jobs immediately
		 * - continue_others: Continue unrelated jobs, mark dependents as skipped
		 * - retry: Retry the failed job up to max_retries times
		 * - fallback: Execute the job's fallback function if available
		 */
		dag_failure_policy failure_policy{dag_failure_policy::fail_fast};

		/**
		 * @brief Maximum number of retry attempts for failed jobs
		 *
		 * Only used when failure_policy is dag_failure_policy::retry.
		 * Set to 0 to disable retries.
		 */
		std::size_t max_retries{0};

		/**
		 * @brief Delay between retry attempts
		 *
		 * Only used when failure_policy is dag_failure_policy::retry.
		 */
		std::chrono::milliseconds retry_delay{1000};

		/**
		 * @brief Whether to detect and reject cycles
		 *
		 * When true, adding a dependency that would create a cycle will fail.
		 * When false, cycles are not checked (may cause infinite loops).
		 */
		bool detect_cycles{true};

		/**
		 * @brief Whether to execute ready jobs in parallel
		 *
		 * When true, jobs with all dependencies satisfied will be executed
		 * in parallel. When false, jobs are executed one at a time.
		 */
		bool execute_in_parallel{true};

		/**
		 * @brief Callback for state changes
		 *
		 * Called whenever a job's state changes. Parameters are:
		 * - job_id: The ID of the job
		 * - old_state: The previous state
		 * - new_state: The new state
		 */
		std::function<void(job_id, dag_job_state, dag_job_state)> state_callback;

		/**
		 * @brief Callback for job errors
		 *
		 * Called whenever a job fails. Parameters are:
		 * - job_id: The ID of the failed job
		 * - error_message: The error message
		 */
		std::function<void(job_id, const std::string&)> error_callback;

		/**
		 * @brief Callback for job completion
		 *
		 * Called whenever a job completes successfully. Parameter is:
		 * - job_id: The ID of the completed job
		 */
		std::function<void(job_id)> completion_callback;
	};

	/**
	 * @struct dag_stats
	 * @brief Statistics about DAG execution
	 */
	struct dag_stats
	{
		std::size_t total_jobs{0};       ///< Total number of jobs in DAG
		std::size_t completed_jobs{0};   ///< Number of successfully completed jobs
		std::size_t failed_jobs{0};      ///< Number of failed jobs
		std::size_t pending_jobs{0};     ///< Number of pending jobs
		std::size_t running_jobs{0};     ///< Number of currently running jobs
		std::size_t skipped_jobs{0};     ///< Number of skipped jobs
		std::size_t cancelled_jobs{0};   ///< Number of cancelled jobs

		std::chrono::milliseconds total_execution_time{0};    ///< Total wall-clock time
		std::chrono::milliseconds critical_path_time{0};      ///< Time of longest path
		double parallelism_efficiency{0.0};                    ///< Actual vs theoretical speedup

		/**
		 * @brief Check if all jobs are complete (success or failure)
		 * @return true if no more jobs to execute
		 */
		[[nodiscard]] auto is_complete() const -> bool
		{
			return pending_jobs == 0 && running_jobs == 0;
		}

		/**
		 * @brief Check if all jobs succeeded
		 * @return true if all jobs completed successfully
		 */
		[[nodiscard]] auto all_succeeded() const -> bool
		{
			return is_complete() && failed_jobs == 0 && cancelled_jobs == 0;
		}

		/**
		 * @brief Calculate success rate
		 * @return Success rate as a percentage (0.0 to 1.0)
		 */
		[[nodiscard]] auto success_rate() const -> double
		{
			if (total_jobs == 0) return 0.0;
			return static_cast<double>(completed_jobs) / static_cast<double>(total_jobs);
		}
	};

} // namespace kcenon::thread
