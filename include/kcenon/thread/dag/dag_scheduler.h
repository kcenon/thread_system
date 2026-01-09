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

#include "dag_job.h"
#include "dag_job_builder.h"
#include "dag_config.h"

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/error_handling.h>

#include <atomic>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kcenon::thread
{
	/**
	 * @class dag_scheduler
	 * @brief DAG-based job scheduler with dependency management
	 *
	 * The dag_scheduler manages jobs with dependencies, ensuring they execute
	 * in the correct order. Jobs are represented as a Directed Acyclic Graph (DAG)
	 * where edges represent dependencies.
	 *
	 * ### Key Features
	 * - Automatic dependency resolution
	 * - Parallel execution of independent jobs
	 * - Cycle detection
	 * - Multiple failure handling policies
	 * - Result passing between jobs
	 * - DOT/JSON visualization export
	 *
	 * ### Thread Safety
	 * All public methods are thread-safe and can be called from any thread.
	 * Internal state is protected by a shared_mutex for optimal read performance.
	 *
	 * ### Usage Example
	 * @code
	 * auto pool = std::make_shared<thread_pool>("pool");
	 * pool->start();
	 *
	 * dag_scheduler scheduler(pool);
	 *
	 * auto job_a = scheduler.add_job(
	 *     dag_job_builder("fetch")
	 *         .work([]{ return fetch_data(); })
	 *         .build()
	 * );
	 *
	 * auto job_b = scheduler.add_job(
	 *     dag_job_builder("process")
	 *         .depends_on(job_a)
	 *         .work([]{ return process_data(); })
	 *         .build()
	 * );
	 *
	 * scheduler.execute_all().wait();
	 * @endcode
	 */
	class dag_scheduler
	{
	public:
		/**
		 * @brief Constructs a DAG scheduler with a thread pool
		 * @param pool The thread pool to use for job execution
		 * @param config Optional configuration (default: dag_config{})
		 */
		explicit dag_scheduler(std::shared_ptr<thread_pool> pool,
		                       dag_config config = {});

		/**
		 * @brief Destructor - cancels any pending jobs
		 */
		~dag_scheduler();

		// Non-copyable
		dag_scheduler(const dag_scheduler&) = delete;
		auto operator=(const dag_scheduler&) -> dag_scheduler& = delete;

		// Movable
		dag_scheduler(dag_scheduler&&) noexcept;
		auto operator=(dag_scheduler&&) noexcept -> dag_scheduler&;

		// ============================================
		// Job Management
		// ============================================

		/**
		 * @brief Adds a job to the DAG
		 * @param j The job to add
		 * @return The assigned job ID
		 *
		 * Thread Safety: Thread-safe (acquires exclusive lock)
		 */
		[[nodiscard]] auto add_job(std::unique_ptr<dag_job> j) -> job_id;

		/**
		 * @brief Adds a job using a builder
		 * @param builder The job builder
		 * @return The assigned job ID
		 *
		 * Thread Safety: Thread-safe (acquires exclusive lock)
		 */
		[[nodiscard]] auto add_job(dag_job_builder&& builder) -> job_id;

		/**
		 * @brief Adds a dependency between jobs
		 * @param dependent The job that depends on another
		 * @param dependency The job being depended on
		 * @return Success or error if cycle detected or jobs not found
		 *
		 * Thread Safety: Thread-safe (acquires exclusive lock)
		 */
		[[nodiscard]] auto add_dependency(job_id dependent, job_id dependency) -> common::VoidResult;

		/**
		 * @brief Removes a job from the DAG (only if not yet started)
		 * @param id The job ID to remove
		 * @return Success or error if job is running or not found
		 *
		 * Thread Safety: Thread-safe (acquires exclusive lock)
		 */
		[[nodiscard]] auto remove_job(job_id id) -> common::VoidResult;

		// ============================================
		// Execution Control
		// ============================================

		/**
		 * @brief Executes all jobs in dependency order
		 * @return Future that completes when all jobs are done
		 *
		 * Thread Safety: Thread-safe
		 */
		[[nodiscard]] auto execute_all() -> std::future<common::VoidResult>;

		/**
		 * @brief Executes a specific job and its dependencies
		 * @param target The target job to execute
		 * @return Future that completes when the target and dependencies are done
		 *
		 * Thread Safety: Thread-safe
		 */
		[[nodiscard]] auto execute(job_id target) -> std::future<common::VoidResult>;

		/**
		 * @brief Cancels all pending jobs
		 *
		 * Jobs that are already running will complete, but no new jobs will start.
		 *
		 * Thread Safety: Thread-safe
		 */
		auto cancel_all() -> void;

		/**
		 * @brief Waits for all jobs to complete
		 * @return Success or the first error encountered
		 *
		 * Thread Safety: Thread-safe
		 */
		[[nodiscard]] auto wait() -> common::VoidResult;

		/**
		 * @brief Resets the scheduler for reuse
		 *
		 * Clears all jobs and resets state. Cannot be called while jobs are running.
		 *
		 * Thread Safety: Thread-safe (acquires exclusive lock)
		 */
		[[nodiscard]] auto reset() -> common::VoidResult;

		// ============================================
		// Query
		// ============================================

		/**
		 * @brief Gets information about a specific job
		 * @param id The job ID
		 * @return Job info, or std::nullopt if not found
		 *
		 * Thread Safety: Thread-safe (acquires shared lock)
		 */
		[[nodiscard]] auto get_job_info(job_id id) const -> std::optional<dag_job_info>;

		/**
		 * @brief Gets information about all jobs
		 * @return Vector of job info for all jobs
		 *
		 * Thread Safety: Thread-safe (acquires shared lock)
		 */
		[[nodiscard]] auto get_all_jobs() const -> std::vector<dag_job_info>;

		/**
		 * @brief Gets jobs in a specific state
		 * @param state The state to filter by
		 * @return Vector of job info for matching jobs
		 *
		 * Thread Safety: Thread-safe (acquires shared lock)
		 */
		[[nodiscard]] auto get_jobs_in_state(dag_job_state state) const -> std::vector<dag_job_info>;

		/**
		 * @brief Gets IDs of ready jobs (dependencies satisfied)
		 * @return Vector of job IDs that can be executed
		 *
		 * Thread Safety: Thread-safe (acquires shared lock)
		 */
		[[nodiscard]] auto get_ready_jobs() const -> std::vector<job_id>;

		/**
		 * @brief Checks if the DAG has cycles
		 * @return true if cycles are detected
		 *
		 * Thread Safety: Thread-safe (acquires shared lock)
		 */
		[[nodiscard]] auto has_cycles() const -> bool;

		/**
		 * @brief Gets topological execution order
		 * @return Vector of job IDs in execution order
		 *
		 * Thread Safety: Thread-safe (acquires shared lock)
		 */
		[[nodiscard]] auto get_execution_order() const -> std::vector<job_id>;

		/**
		 * @brief Gets the result from a completed job
		 * @tparam T The expected result type
		 * @param id The job ID
		 * @return The result value
		 * @throws std::runtime_error if job not found or not completed
		 * @throws std::bad_any_cast if type doesn't match
		 *
		 * Thread Safety: Thread-safe (acquires shared lock)
		 */
		template<typename T>
		[[nodiscard]] auto get_result(job_id id) const -> const T&
		{
			std::shared_lock lock(mutex_);
			auto it = jobs_.find(id);
			if (it == jobs_.end())
			{
				throw std::runtime_error("Job not found: " + std::to_string(id));
			}
			if (it->second->get_state() != dag_job_state::completed)
			{
				throw std::runtime_error("Job not completed: " + std::to_string(id));
			}
			return it->second->get_result<T>();
		}

		// ============================================
		// Visualization
		// ============================================

		/**
		 * @brief Exports the DAG as DOT format (Graphviz)
		 * @return DOT format string
		 *
		 * Thread Safety: Thread-safe (acquires shared lock)
		 */
		[[nodiscard]] auto to_dot() const -> std::string;

		/**
		 * @brief Exports the DAG as JSON format
		 * @return JSON format string
		 *
		 * Thread Safety: Thread-safe (acquires shared lock)
		 */
		[[nodiscard]] auto to_json() const -> std::string;

		// ============================================
		// Statistics
		// ============================================

		/**
		 * @brief Gets execution statistics
		 * @return Current statistics
		 *
		 * Thread Safety: Thread-safe (acquires shared lock)
		 */
		[[nodiscard]] auto get_stats() const -> dag_stats;

		/**
		 * @brief Gets the configuration
		 * @return Current configuration
		 */
		[[nodiscard]] auto get_config() const -> const dag_config& { return config_; }

	private:
		/**
		 * @brief Thread pool for job execution
		 */
		std::shared_ptr<thread_pool> pool_;

		/**
		 * @brief Configuration
		 */
		dag_config config_;

		/**
		 * @brief Job storage (job_id -> dag_job)
		 */
		std::unordered_map<job_id, std::unique_ptr<dag_job>> jobs_;

		/**
		 * @brief Dependency graph (job -> jobs it depends on)
		 */
		std::unordered_map<job_id, std::vector<job_id>> dependencies_;

		/**
		 * @brief Reverse dependency graph (job -> jobs that depend on it)
		 */
		std::unordered_map<job_id, std::vector<job_id>> dependents_;

		/**
		 * @brief Mutex for thread-safe access
		 */
		mutable std::shared_mutex mutex_;

		/**
		 * @brief Condition variable for waiting on completion
		 */
		std::condition_variable_any completion_cv_;

		/**
		 * @brief Flag indicating execution is in progress
		 */
		std::atomic<bool> executing_{false};

		/**
		 * @brief Flag indicating cancellation was requested
		 */
		std::atomic<bool> cancelled_{false};

		/**
		 * @brief Number of jobs currently running
		 */
		std::atomic<std::size_t> running_count_{0};

		/**
		 * @brief Execution start time
		 */
		std::chrono::steady_clock::time_point execution_start_time_;

		/**
		 * @brief First error encountered during execution
		 */
		std::optional<common::error_info> first_error_;

		/**
		 * @brief Retry count per job
		 */
		std::unordered_map<job_id, std::size_t> retry_counts_;

		// ============================================
		// Internal Methods
		// ============================================

		/**
		 * @brief Called when a job completes successfully
		 * @param id The completed job ID
		 */
		auto on_job_completed(job_id id) -> void;

		/**
		 * @brief Called when a job fails
		 * @param id The failed job ID
		 * @param error The error message
		 */
		auto on_job_failed(job_id id, const std::string& error) -> void;

		/**
		 * @brief Schedules ready jobs for execution
		 */
		auto schedule_ready_jobs() -> void;

		/**
		 * @brief Executes a single job
		 * @param id The job ID to execute
		 */
		auto execute_job(job_id id) -> void;

		/**
		 * @brief Performs topological sort
		 * @return Sorted job IDs, or empty if cycle detected
		 */
		[[nodiscard]] auto topological_sort() const -> std::vector<job_id>;

		/**
		 * @brief Detects cycles using DFS
		 * @return true if cycle detected
		 */
		[[nodiscard]] auto detect_cycle() const -> bool;

		/**
		 * @brief Checks if a job's dependencies are all satisfied
		 * @param id The job ID to check
		 * @return true if all dependencies are completed
		 */
		[[nodiscard]] auto are_dependencies_satisfied(job_id id) const -> bool;

		/**
		 * @brief Marks dependents as skipped due to dependency failure
		 * @param failed_id The failed job ID
		 */
		auto skip_dependents(job_id failed_id) -> void;

		/**
		 * @brief Cancels dependents due to dependency failure
		 * @param failed_id The failed job ID
		 */
		auto cancel_dependents(job_id failed_id) -> void;

		/**
		 * @brief Gets the state color for DOT visualization
		 * @param state The job state
		 * @return Color string for DOT format
		 */
		[[nodiscard]] static auto get_state_color(dag_job_state state) -> std::string;

		/**
		 * @brief Checks if execution is complete
		 * @return true if all jobs are in terminal state
		 */
		[[nodiscard]] auto is_execution_complete() const -> bool;
	};

} // namespace kcenon::thread
