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

#include "job_info.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <thread>

namespace kcenon::thread::diagnostics
{
	/**
	 * @enum worker_state
	 * @brief Current state of a worker thread.
	 *
	 * @ingroup diagnostics
	 *
	 * Represents the operational state of a thread worker in the pool.
	 */
	enum class worker_state
	{
		idle,       ///< Worker is waiting for jobs
		active,     ///< Worker is executing a job
		stopping,   ///< Worker is in the process of stopping
		stopped     ///< Worker has stopped
	};

	/**
	 * @brief Converts worker_state to human-readable string.
	 * @param state The state to convert.
	 * @return String representation of the state.
	 */
	[[nodiscard]] inline auto worker_state_to_string(worker_state state) -> std::string
	{
		switch (state)
		{
			case worker_state::idle: return "IDLE";
			case worker_state::active: return "ACTIVE";
			case worker_state::stopping: return "STOPPING";
			case worker_state::stopped: return "STOPPED";
			default: return "UNKNOWN";
		}
	}

	/**
	 * @struct thread_info
	 * @brief Information about a worker thread in the pool.
	 *
	 * @ingroup diagnostics
	 *
	 * Contains comprehensive information about a worker thread including
	 * its identity, current state, statistics, and optionally the job
	 * it is currently processing.
	 *
	 * ### Example Output
	 * ```
	 * Worker-0 [tid:12345] ACTIVE (2.5s)
	 *   Current Job: ProcessOrder#1234 (running 150ms)
	 *   Jobs: 1523 completed, 2 failed
	 *   Utilization: 87.3%
	 * ```
	 *
	 * ### Usage Example
	 * @code
	 * auto threads = diagnostics.dump_thread_states();
	 * for (const auto& t : threads) {
	 *     LOG_INFO("Worker {} ({}): {} jobs done, {:.1f}% utilization",
	 *              t.thread_name, worker_state_to_string(t.state),
	 *              t.jobs_completed, t.utilization * 100.0);
	 * }
	 * @endcode
	 */
	struct thread_info
	{
		/**
		 * @brief System thread ID.
		 *
		 * The native thread identifier from the operating system.
		 */
		std::thread::id thread_id;

		/**
		 * @brief Human-readable name for this thread.
		 *
		 * Typically in the format "Worker-N" where N is the worker index.
		 */
		std::string thread_name;

		/**
		 * @brief Worker ID within the pool.
		 *
		 * Unique identifier for this worker, assigned by the pool.
		 */
		std::size_t worker_id{0};

		/**
		 * @brief Current operational state of the worker.
		 */
		worker_state state{worker_state::idle};

		/**
		 * @brief Time when the worker entered its current state.
		 *
		 * Used to calculate how long the worker has been in its current state.
		 */
		std::chrono::steady_clock::time_point state_since;

		/**
		 * @brief Information about the currently executing job.
		 *
		 * Only has a value if state == active.
		 */
		std::optional<job_info> current_job;

		// =========================================================================
		// Statistics
		// =========================================================================

		/**
		 * @brief Total number of jobs successfully completed by this worker.
		 */
		std::uint64_t jobs_completed{0};

		/**
		 * @brief Total number of jobs that failed during execution.
		 */
		std::uint64_t jobs_failed{0};

		/**
		 * @brief Total time spent executing jobs (busy time).
		 */
		std::chrono::nanoseconds total_busy_time{0};

		/**
		 * @brief Total time spent waiting for jobs (idle time).
		 */
		std::chrono::nanoseconds total_idle_time{0};

		/**
		 * @brief Worker utilization ratio.
		 *
		 * Calculated as: total_busy_time / (total_busy_time + total_idle_time).
		 * Value ranges from 0.0 (never busy) to 1.0 (always busy).
		 */
		double utilization{0.0};

		// =========================================================================
		// Computed Properties
		// =========================================================================

		/**
		 * @brief Calculates the duration in the current state.
		 * @return Time since entering the current state.
		 */
		[[nodiscard]] auto state_duration() const -> std::chrono::nanoseconds
		{
			return std::chrono::steady_clock::now() - state_since;
		}

		/**
		 * @brief Gets the total number of jobs processed (completed + failed).
		 * @return Total job count.
		 */
		[[nodiscard]] auto total_jobs() const -> std::uint64_t
		{
			return jobs_completed + jobs_failed;
		}

		/**
		 * @brief Calculates the success rate.
		 * @return Success ratio (0.0 to 1.0), or 1.0 if no jobs processed.
		 */
		[[nodiscard]] auto success_rate() const -> double
		{
			auto total = total_jobs();
			if (total == 0)
			{
				return 1.0;
			}
			return static_cast<double>(jobs_completed) / static_cast<double>(total);
		}

		/**
		 * @brief Checks if the worker is currently processing a job.
		 * @return true if state == active.
		 */
		[[nodiscard]] auto is_busy() const -> bool
		{
			return state == worker_state::active;
		}

		/**
		 * @brief Checks if the worker is available to process jobs.
		 * @return true if state == idle.
		 */
		[[nodiscard]] auto is_available() const -> bool
		{
			return state == worker_state::idle;
		}

		/**
		 * @brief Recalculates utilization based on busy and idle times.
		 *
		 * Updates the utilization field based on current total_busy_time
		 * and total_idle_time values.
		 */
		auto update_utilization() -> void
		{
			auto total_time = total_busy_time + total_idle_time;
			if (total_time.count() == 0)
			{
				utilization = 0.0;
			}
			else
			{
				utilization = static_cast<double>(total_busy_time.count()) /
				              static_cast<double>(total_time.count());
			}
		}
	};

} // namespace kcenon::thread::diagnostics
