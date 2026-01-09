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
#include <kcenon/thread/core/error_handling.h>

#include <any>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace kcenon::thread
{
	/**
	 * @brief Unique job identifier for DAG scheduler
	 */
	using job_id = std::uint64_t;

	/**
	 * @brief Invalid job ID constant
	 */
	constexpr job_id INVALID_JOB_ID = 0;

	/**
	 * @enum dag_job_state
	 * @brief State of a job in the DAG scheduler
	 */
	enum class dag_job_state
	{
		pending,    ///< Waiting for dependencies to complete
		ready,      ///< Dependencies satisfied, can be executed
		running,    ///< Currently executing
		completed,  ///< Successfully completed
		failed,     ///< Execution failed
		cancelled,  ///< Cancelled by user or dependency failure
		skipped     ///< Skipped due to dependency failure
	};

	/**
	 * @brief Convert dag_job_state to string representation
	 * @param state The state to convert
	 * @return String representation of the state
	 */
	[[nodiscard]] inline auto dag_job_state_to_string(dag_job_state state) -> std::string
	{
		switch (state)
		{
			case dag_job_state::pending:   return "pending";
			case dag_job_state::ready:     return "ready";
			case dag_job_state::running:   return "running";
			case dag_job_state::completed: return "completed";
			case dag_job_state::failed:    return "failed";
			case dag_job_state::cancelled: return "cancelled";
			case dag_job_state::skipped:   return "skipped";
			default:                       return "unknown";
		}
	}

	/**
	 * @struct dag_job_info
	 * @brief Information about a job in the DAG
	 *
	 * This structure provides a snapshot of a job's state and metadata
	 * for monitoring and debugging purposes.
	 */
	struct dag_job_info
	{
		job_id id{INVALID_JOB_ID};                       ///< Unique job identifier
		std::string name;                                 ///< Human-readable job name
		dag_job_state state{dag_job_state::pending};     ///< Current job state

		std::vector<job_id> dependencies;                 ///< Jobs this job depends on
		std::vector<job_id> dependents;                   ///< Jobs that depend on this job

		std::chrono::steady_clock::time_point submit_time;  ///< When job was added to DAG
		std::chrono::steady_clock::time_point start_time;   ///< When execution started
		std::chrono::steady_clock::time_point end_time;     ///< When execution ended

		std::optional<std::string> error_message;         ///< Error message if failed
		std::optional<std::any> result;                   ///< Result value for passing between jobs

		/**
		 * @brief Calculate wait time (time from submit to start)
		 * @return Wait duration, or zero if not started
		 */
		[[nodiscard]] auto get_wait_time() const -> std::chrono::milliseconds
		{
			if (start_time == std::chrono::steady_clock::time_point{})
			{
				return std::chrono::milliseconds{0};
			}
			return std::chrono::duration_cast<std::chrono::milliseconds>(start_time - submit_time);
		}

		/**
		 * @brief Calculate execution time
		 * @return Execution duration, or zero if not completed
		 */
		[[nodiscard]] auto get_execution_time() const -> std::chrono::milliseconds
		{
			if (end_time == std::chrono::steady_clock::time_point{} ||
			    start_time == std::chrono::steady_clock::time_point{})
			{
				return std::chrono::milliseconds{0};
			}
			return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
		}
	};

	// Forward declaration
	class dag_scheduler;

	/**
	 * @class dag_job
	 * @brief A job with dependency support for DAG-based scheduling
	 *
	 * The dag_job class extends the base job class to support:
	 * - Dependency declarations on other jobs
	 * - State tracking for DAG execution
	 * - Result storage for passing data between jobs
	 *
	 * ### Thread Safety
	 * - State transitions are atomic
	 * - Result access should be synchronized externally
	 * - Dependencies should be set before adding to scheduler
	 *
	 * ### Usage Example
	 * @code
	 * auto job_a = std::make_unique<dag_job>("fetch_data");
	 * job_a->set_work([]{ return fetch_from_database(); });
	 *
	 * auto job_b = std::make_unique<dag_job>("process_data");
	 * job_b->add_dependency(job_a->get_dag_id());
	 * job_b->set_work([&scheduler, job_a_id] {
	 *     auto data = scheduler.get_result<Data>(job_a_id);
	 *     return process(data);
	 * });
	 * @endcode
	 */
	class dag_job : public job
	{
	public:
		/**
		 * @brief Constructs a new dag_job with a name
		 * @param name Human-readable name for the job
		 */
		explicit dag_job(const std::string& name = "dag_job");

		/**
		 * @brief Virtual destructor
		 */
		~dag_job() override;

		/**
		 * @brief Gets the unique DAG job identifier
		 * @return The job's unique ID within the DAG
		 *
		 * Thread Safety: Safe to call from any thread (ID is immutable)
		 */
		[[nodiscard]] auto get_dag_id() const -> job_id { return dag_id_; }

		/**
		 * @brief Gets the current state of the job
		 * @return Current dag_job_state
		 *
		 * Thread Safety: Atomic read
		 */
		[[nodiscard]] auto get_state() const -> dag_job_state
		{
			return state_.load(std::memory_order_acquire);
		}

		/**
		 * @brief Sets the job state
		 * @param new_state The new state to set
		 *
		 * Thread Safety: Atomic write
		 */
		auto set_state(dag_job_state new_state) -> void
		{
			state_.store(new_state, std::memory_order_release);
		}

		/**
		 * @brief Attempts to transition state atomically
		 * @param expected The expected current state
		 * @param desired The desired new state
		 * @return true if transition succeeded, false otherwise
		 *
		 * Thread Safety: Atomic compare-exchange
		 */
		[[nodiscard]] auto try_transition_state(dag_job_state expected, dag_job_state desired) -> bool
		{
			return state_.compare_exchange_strong(expected, desired,
			                                      std::memory_order_acq_rel,
			                                      std::memory_order_acquire);
		}

		/**
		 * @brief Gets the list of dependency job IDs
		 * @return Vector of job IDs this job depends on
		 *
		 * Thread Safety: Not thread-safe, should be set before scheduling
		 */
		[[nodiscard]] auto get_dependencies() const -> const std::vector<job_id>& { return dependencies_; }

		/**
		 * @brief Adds a dependency on another job
		 * @param dependency_id The ID of the job to depend on
		 *
		 * Thread Safety: Not thread-safe, should be called before scheduling
		 */
		auto add_dependency(job_id dependency_id) -> void
		{
			if (dependency_id != INVALID_JOB_ID)
			{
				dependencies_.push_back(dependency_id);
			}
		}

		/**
		 * @brief Adds multiple dependencies
		 * @param dependency_ids Vector of job IDs to depend on
		 *
		 * Thread Safety: Not thread-safe, should be called before scheduling
		 */
		auto add_dependencies(const std::vector<job_id>& dependency_ids) -> void
		{
			for (const auto& id : dependency_ids)
			{
				add_dependency(id);
			}
		}

		/**
		 * @brief Sets the work function to execute
		 * @param work_func The function to execute
		 *
		 * Thread Safety: Not thread-safe, should be called before scheduling
		 */
		auto set_work(std::function<common::VoidResult()> work_func) -> void
		{
			work_func_ = std::move(work_func);
		}

		/**
		 * @brief Sets the work function with result
		 * @tparam T The result type
		 * @param work_func The function to execute that returns a result
		 *
		 * Thread Safety: Not thread-safe, should be called before scheduling
		 */
		template<typename T>
		auto set_work_with_result(std::function<common::Result<T>()> work_func) -> void
		{
			work_func_ = [this, func = std::move(work_func)]() -> common::VoidResult {
				auto result = func();
				if (result.is_ok())
				{
					set_result(result.value());
					return common::ok();
				}
				return common::VoidResult(result.error());
			};
		}

		/**
		 * @brief Sets the fallback function to execute on failure
		 * @param fallback_func The fallback function
		 *
		 * Thread Safety: Not thread-safe, should be called before scheduling
		 */
		auto set_fallback(std::function<common::VoidResult()> fallback_func) -> void
		{
			fallback_func_ = std::move(fallback_func);
		}

		/**
		 * @brief Gets the fallback function
		 * @return The fallback function, or nullptr if not set
		 */
		[[nodiscard]] auto get_fallback() const -> const std::function<common::VoidResult()>&
		{
			return fallback_func_;
		}

		/**
		 * @brief Checks if a fallback function is set
		 * @return true if fallback is set
		 */
		[[nodiscard]] auto has_fallback() const -> bool
		{
			return fallback_func_ != nullptr;
		}

		/**
		 * @brief Sets the result value
		 * @tparam T The result type
		 * @param value The result value
		 *
		 * Thread Safety: Not thread-safe, should be called from worker thread only
		 */
		template<typename T>
		auto set_result(T&& value) -> void
		{
			result_ = std::forward<T>(value);
		}

		/**
		 * @brief Gets the result value
		 * @tparam T The expected result type
		 * @return The result value
		 * @throws std::bad_any_cast if type doesn't match
		 *
		 * Thread Safety: Not thread-safe, should be called after job completes
		 */
		template<typename T>
		[[nodiscard]] auto get_result() const -> const T&
		{
			return std::any_cast<const T&>(result_);
		}

		/**
		 * @brief Checks if the job has a result
		 * @return true if result is set
		 */
		[[nodiscard]] auto has_result() const -> bool
		{
			return result_.has_value();
		}

		/**
		 * @brief Gets the result as std::any
		 * @return The result as std::any
		 */
		[[nodiscard]] auto get_result_any() const -> const std::any& { return result_; }

		/**
		 * @brief Sets the error message for failed jobs
		 * @param message The error message
		 */
		auto set_error_message(const std::string& message) -> void
		{
			error_message_ = message;
		}

		/**
		 * @brief Gets the error message
		 * @return The error message, or std::nullopt if not set
		 */
		[[nodiscard]] auto get_error_message() const -> const std::optional<std::string>&
		{
			return error_message_;
		}

		/**
		 * @brief Records the start time
		 */
		auto record_start_time() -> void
		{
			start_time_ = std::chrono::steady_clock::now();
		}

		/**
		 * @brief Records the end time
		 */
		auto record_end_time() -> void
		{
			end_time_ = std::chrono::steady_clock::now();
		}

		/**
		 * @brief Gets the submit time
		 * @return The time when the job was created
		 */
		[[nodiscard]] auto get_submit_time() const -> std::chrono::steady_clock::time_point
		{
			return submit_time_;
		}

		/**
		 * @brief Gets the start time
		 * @return The time when execution started
		 */
		[[nodiscard]] auto get_start_time() const -> std::chrono::steady_clock::time_point
		{
			return start_time_;
		}

		/**
		 * @brief Gets the end time
		 * @return The time when execution ended
		 */
		[[nodiscard]] auto get_end_time() const -> std::chrono::steady_clock::time_point
		{
			return end_time_;
		}

		/**
		 * @brief Creates a dag_job_info snapshot
		 * @return Snapshot of current job state
		 */
		[[nodiscard]] auto get_info() const -> dag_job_info;

		/**
		 * @brief Executes the job's work function
		 * @return Result of execution
		 *
		 * Thread Safety: Should only be called by the scheduler
		 */
		[[nodiscard]] auto do_work() -> common::VoidResult override;

		/**
		 * @brief Returns a string representation of the job
		 * @return String describing the job
		 */
		[[nodiscard]] auto to_string() const -> std::string override;

	private:
		/**
		 * @brief Static counter for generating unique DAG job IDs
		 */
		static std::atomic<job_id> next_dag_id_;

		/**
		 * @brief Unique identifier for this job in the DAG
		 */
		job_id dag_id_;

		/**
		 * @brief Current state of the job
		 */
		std::atomic<dag_job_state> state_{dag_job_state::pending};

		/**
		 * @brief List of job IDs this job depends on
		 */
		std::vector<job_id> dependencies_;

		/**
		 * @brief The work function to execute
		 */
		std::function<common::VoidResult()> work_func_;

		/**
		 * @brief The fallback function to execute on failure
		 */
		std::function<common::VoidResult()> fallback_func_;

		/**
		 * @brief Result value for passing between jobs
		 */
		std::any result_;

		/**
		 * @brief Error message if job failed
		 */
		std::optional<std::string> error_message_;

		/**
		 * @brief Time when the job was created
		 */
		std::chrono::steady_clock::time_point submit_time_;

		/**
		 * @brief Time when execution started
		 */
		std::chrono::steady_clock::time_point start_time_;

		/**
		 * @brief Time when execution ended
		 */
		std::chrono::steady_clock::time_point end_time_;
	};

} // namespace kcenon::thread

// ----------------------------------------------------------------------------
// Formatter specializations for dag_job
// ----------------------------------------------------------------------------

#include <kcenon/thread/utils/formatter_macros.h>

DECLARE_FORMATTER(kcenon::thread::dag_job)
