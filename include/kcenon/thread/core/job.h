/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions of source code must reproduce the above copyright notice,
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

#include <kcenon/thread/utils/formatter.h>
#include <kcenon/thread/utils/convert_string.h>
#include "error_handling.h"
#include "cancellation_token.h"
#include "retry_policy.h"

#include <tuple>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <string_view>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>

namespace kcenon::thread
{
	class job_queue;

	/**
	 * @enum job_priority
	 * @brief Priority levels for job scheduling.
	 *
	 * Jobs with higher priority values are typically executed before lower priority jobs.
	 * This enum provides a standardized set of priority levels for use with composition.
	 */
	enum class job_priority
	{
		lowest = 0,     ///< Lowest priority, executed when no other jobs are pending
		low = 1,        ///< Low priority, background tasks
		normal = 2,     ///< Normal priority, default for most jobs
		high = 3,       ///< High priority, time-sensitive tasks
		highest = 4,    ///< Highest priority, critical tasks
		realtime = 5    ///< Real-time priority, should be used sparingly
	};

	// Forward declarations for future composition components
	class retry_policy;

	/**
	 * @struct job_components
	 * @brief Internal structure holding composed behaviors for a job.
	 *
	 * This structure enables the composition pattern by storing optional
	 * callbacks and behaviors that can be attached to any job instance.
	 * The components are lazily allocated only when first needed.
	 */
	struct job_components
	{
		/// Callback invoked when job completes (success or error)
		std::function<void(common::VoidResult)> on_complete;

		/// Callback invoked specifically on error
		std::function<void(const common::error_info&)> on_error;

		/// Optional priority override for this job
		std::optional<job_priority> priority;

		/// Retry policy for automatic retry on failure
		std::optional<retry_policy> retry;

		/// Optional timeout for job execution
		std::optional<std::chrono::milliseconds> timeout;

		/// Flag indicating explicit cancellation token was set via composition
		bool has_explicit_cancellation{false};
	};

	/**
	 * @class job
	 * @brief Represents a unit of work (task) to be executed, typically by a job queue.
	 *
	 * The @c job class provides a base interface for scheduling and executing discrete
	 * tasks within a multi-threaded environment. Derived classes can override the
	 * @c do_work() method to implement custom logic for their specific tasks.
	 *
	 * ### Thread-Safety
	 * - @c do_work() will generally be called from a worker thread. If your task accesses
	 *   shared data or interacts with shared resources, you must ensure your implementation
	 *   is thread-safe.
	 * - The job itself is meant to be used via @c std::shared_ptr, making it easier to
	 *   manage lifetimes across multiple threads.
	 *
	 * ### Error Handling
	 * - A job returns a @c common::VoidResult from @c do_work().
	 *   - Returning @c common::ok() indicates success.
	 *   - Returning a @c common::error_info indicates failure with a typed error code and message.
	 * - Error information can be used for logging, debugging, or to retry a job if desired.
	 *
	 * ### Usage Example
	 * @code
	 * // 1. Create a custom job that overrides do_work().
	 * class my_job : public kcenon::thread::job
	 * {
	 * public:
	 *     my_job() : job("my_custom_job") {}
	 *
	 *     common::VoidResult do_work() override
	 *     {
	 *         // Execute custom logic:
	 *         bool success = perform_operation();
	 *         if (!success)
	 *             return common::error_info{static_cast<int>(error_code::job_execution_failed), "Operation failed in my_custom_job"};
	 *
	 *         return common::ok(); // success
	 *     }
	 * };
	 *
	 * // 2. Submit the job to a queue.
	 * auto q = std::make_shared<kcenon::thread::job_queue>();
	 * auto job_ptr = std::make_shared<my_job>();
	 * q->enqueue(job_ptr);
	 * @endcode
	 */
	class job
	{
	public:
		/**
		 * @brief Gets the unique ID of this job.
		 * @return The unique job identifier.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread (ID is immutable after construction)
		 */
		[[nodiscard]] auto get_job_id() const -> std::uint64_t { return job_id_; }

		/**
		 * @brief Gets the time when this job was created (enqueued).
		 * @return Time point when the job was created.
		 */
		[[nodiscard]] auto get_enqueue_time() const -> std::chrono::steady_clock::time_point
		{
			return enqueue_time_;
		}

		/**
		 * @brief Constructs a new @c job with an optional human-readable name.
		 *
		 * Use this constructor when your derived job class does not need to store
		 * any initial data beyond what you might manually store in derived class members.
		 *
		 * @param name A descriptive name for the job (default is "job").
		 *
		 * #### Example
		 * @code
		 * // Creating a basic job with a custom name
		 * auto my_simple_job = std::make_shared<kcenon::thread::job>("simple_job");
		 * @endcode
		 */
		job(const std::string& name = "job");

		/**
		 * @brief Constructs a new @c job with associated raw byte data and a name.
		 *
		 * This constructor is particularly useful if your job needs an inline payload or
		 * associated data that should be passed directly to @c do_work().
		 *
		 * @param data A vector of bytes serving as the job's payload.
		 * @param name A descriptive name for the job (default is "data_job").
		 *
		 * #### Example
		 * @code
		 * // Creating a job that has binary data.
		 * std::vector<uint8_t> my_data = {0xDE, 0xAD, 0xBE, 0xEF};
		 * auto data_job = std::make_shared<kcenon::thread::job>(my_data, "data_handling_job");
		 * @endcode
		 */
		job(const std::vector<uint8_t>& data, const std::string& name = "data_job");

		/**
		 * @brief Virtual destructor for the @c job class to allow proper cleanup in derived
		 * classes.
		 */
		virtual ~job(void);

		/**
		 * @brief Retrieves the name of this job.
		 * @return A string containing the name assigned to this job.
		 *
		 * The name can be useful for logging and diagnostic messages, especially
		 * when multiple jobs are running concurrently.
		 */
		[[nodiscard]] auto get_name(void) const -> std::string;

		/**
		 * @brief The core task execution method to be overridden by derived classes.
		 *
		 * @return A @c std::optional<std::string> indicating success or error:
		 * - @c std::nullopt on success, meaning no error occurred.
		 * - A non-empty string on failure, providing an error message or explanation.
		 *
		 * #### Default Behavior
		 * The base class implementation simply returns @c std::nullopt (i.e., no error).
		 * Override this method in a derived class to perform meaningful work.
		 *
		 * #### Concurrency
		 * - Typically invoked by worker threads in a @c job_queue.
		 * - Ensure that any shared data or resources accessed here are protected with
		 *   appropriate synchronization mechanisms (mutexes, locks, etc.) if needed.
		 */
		/**
	 * @brief The core task execution method to be overridden by derived classes.
	 *
	 * @return A @c common::VoidResult indicating success or error:
	 * - A success result (constructed with common::ok()) if no error occurred.
	 * - An error result (constructed with common::error_info{code, message}) on failure.
	 *
	 * #### Default Behavior
	 * The base class implementation simply returns a success result.
	 * Override this method in a derived class to perform meaningful work.
	 *
	 * #### Concurrency
	 * - Typically invoked by worker threads in a @c job_queue.
	 * - Ensure that any shared data or resources accessed here are protected with
	 *   appropriate synchronization mechanisms (mutexes, locks, etc.) if needed.
	 * - This method should check the cancellation token if one is set and return
	 *   an error with code operation_canceled if the token is cancelled.
	 */
	[[nodiscard]] virtual auto do_work(void) -> common::VoidResult;
	
	/**
	 * @brief Sets a cancellation token that can be used to cancel the job.
	 * 
	 * @param token The cancellation token to associate with this job.
	 */
	virtual auto set_cancellation_token(const cancellation_token& token) -> void;
	
	/**
	 * @brief Gets the cancellation token associated with this job.
	 * 
	 * @return The cancellation token, or a default token if none was set.
	 */
	[[nodiscard]] virtual auto get_cancellation_token() const -> cancellation_token;

		/**
		 * @brief Associates this job with a specific @c job_queue.
		 *
		 * Once assigned, the job can be aware of the queue that manages it,
		 * enabling scenarios like re-enqueuing itself upon partial failure,
		 * or querying the queue for state (depending on the queue's interface).
		 *
		 * @param job_queue A shared pointer to the @c job_queue instance.
		 *
		 * #### Implementation Detail
		 * - Stored internally as a @c std::weak_ptr. If the queue is destroyed,
		 *   the pointer becomes invalid.
		 */
		virtual auto set_job_queue(const std::shared_ptr<job_queue>& job_queue) -> void;

		/**
		 * @brief Retrieves the @c job_queue associated with this job, if any.
		 *
		 * @return A @c std::shared_ptr to the associated @c job_queue.
		 *         Will be empty if no queue was set or if the queue has already been destroyed.
		 *
		 * #### Usage Example
		 * @code
		 * auto jq = get_job_queue();
		 * if (jq)
		 * {
		 *     // Safe to use jq here
		 * }
		 * else
		 * {
		 *     // The queue is no longer valid
		 * }
		 * @endcode
		 */
		[[nodiscard]] virtual auto get_job_queue(void) const -> std::shared_ptr<job_queue>;

		/**
		 * @brief Provides a string representation of the job for logging or debugging.
		 *
		 * By default, this returns the job's name. Derived classes can override
		 * to include extra diagnostic details (e.g., job status, data contents, etc.).
		 *
		 * @return A string describing the job (e.g., @c name_).
		 *
		 * #### Example
		 * @code
		 * std::shared_ptr<my_job> job_ptr = std::make_shared<my_job>();
		 * // ...
		 * std::string desc = job_ptr->to_string(); // "my_custom_job", for instance
		 * @endcode
		 */
		[[nodiscard]] virtual auto to_string(void) const -> std::string;

		// ========================================================================
		// Composition Methods (Fluent Interface)
		// ========================================================================

		/**
		 * @brief Attaches a completion callback to this job.
		 *
		 * The callback will be invoked after do_work() completes, regardless of
		 * success or failure. The callback receives the VoidResult from do_work().
		 *
		 * @param callback Function to call with the job's result
		 * @return Reference to this job for method chaining
		 *
		 * #### Thread Safety
		 * - The callback is invoked on the same thread that executes do_work()
		 * - Ensure callback is thread-safe if it accesses shared resources
		 *
		 * #### Example
		 * @code
		 * auto job = std::make_unique<my_job>()
		 *     ->with_on_complete([](auto result) {
		 *         if (result.is_ok()) {
		 *             std::cout << "Job completed successfully\n";
		 *         } else {
		 *             std::cout << "Job failed: " << result.error().message << "\n";
		 *         }
		 *     });
		 * @endcode
		 */
		auto with_on_complete(std::function<void(common::VoidResult)> callback) -> job&;

		/**
		 * @brief Attaches an error callback to this job.
		 *
		 * The callback will be invoked only if do_work() returns an error.
		 * This is more specific than with_on_complete() for error-only handling.
		 *
		 * @param callback Function to call with the error information
		 * @return Reference to this job for method chaining
		 *
		 * #### Example
		 * @code
		 * auto job = std::make_unique<my_job>()
		 *     ->with_on_error([](const auto& err) {
		 *         log_error("Job failed: code={}, message={}", err.code, err.message);
		 *     });
		 * @endcode
		 */
		auto with_on_error(std::function<void(const common::error_info&)> callback) -> job&;

		/**
		 * @brief Sets the priority level for this job.
		 *
		 * Priority can be used by job queues and schedulers to determine
		 * execution order. Higher priority jobs are typically executed first.
		 *
		 * @param priority The priority level to set
		 * @return Reference to this job for method chaining
		 *
		 * #### Example
		 * @code
		 * auto job = std::make_unique<my_job>()
		 *     ->with_priority(job_priority::high)
		 *     ->with_on_complete([](auto) { std::cout << "Done\n"; });
		 * @endcode
		 */
		auto with_priority(job_priority priority) -> job&;

		/**
		 * @brief Attaches a cancellation token to this job via composition.
		 *
		 * The cancellation token enables cooperative cancellation of the job.
		 * The job should periodically check the token and abort if cancelled.
		 *
		 * @param token The cancellation token to use
		 * @return Reference to this job for method chaining
		 *
		 * #### Thread Safety
		 * - The token itself is thread-safe for cancellation requests
		 * - The job must cooperatively check the token during execution
		 *
		 * #### Example
		 * @code
		 * cancellation_token token;
		 * auto job = std::make_unique<my_job>()
		 *     ->with_cancellation(token);
		 *
		 * // Later, from another thread
		 * token.cancel();
		 * @endcode
		 */
		auto with_cancellation(const cancellation_token& token) -> job&;

		/**
		 * @brief Attaches a retry policy to this job.
		 *
		 * When the job fails, it can be automatically retried according to
		 * the specified retry policy. The executor must support retry behavior.
		 *
		 * @param policy The retry policy to use
		 * @return Reference to this job for method chaining
		 *
		 * #### Supported Policies
		 * - retry_policy::none() - No retry
		 * - retry_policy::fixed() - Fixed delay between retries
		 * - retry_policy::linear() - Linearly increasing delay
		 * - retry_policy::exponential_backoff() - Exponential delay
		 *
		 * #### Example
		 * @code
		 * auto job = std::make_unique<my_job>()
		 *     ->with_retry(retry_policy::exponential_backoff(3))
		 *     ->with_on_error([](auto& err) { log("Retry exhausted: {}", err.message); });
		 * @endcode
		 */
		auto with_retry(const retry_policy& policy) -> job&;

		/**
		 * @brief Sets a timeout for job execution.
		 *
		 * If the job does not complete within the specified duration,
		 * it may be cancelled (requires executor support for timeout handling).
		 *
		 * @param timeout Maximum execution time allowed
		 * @return Reference to this job for method chaining
		 *
		 * #### Note
		 * - Timeout enforcement requires executor/queue support
		 * - The job should check cancellation token for cooperative timeout
		 *
		 * #### Example
		 * @code
		 * auto job = std::make_unique<my_job>()
		 *     ->with_timeout(std::chrono::seconds(30))
		 *     ->with_on_error([](auto& err) { log("Timed out: {}", err.message); });
		 * @endcode
		 */
		auto with_timeout(std::chrono::milliseconds timeout) -> job&;

		/**
		 * @brief Gets the priority level of this job.
		 *
		 * @return The job's priority, or job_priority::normal if not set
		 */
		[[nodiscard]] auto get_priority() const -> job_priority;

		/**
		 * @brief Gets the retry policy of this job.
		 *
		 * @return The job's retry policy, or std::nullopt if not set
		 */
		[[nodiscard]] auto get_retry_policy() const -> std::optional<retry_policy>;

		/**
		 * @brief Gets the timeout duration for this job.
		 *
		 * @return The job's timeout, or std::nullopt if not set
		 */
		[[nodiscard]] auto get_timeout() const -> std::optional<std::chrono::milliseconds>;

		/**
		 * @brief Checks if this job has an explicit cancellation set via composition.
		 *
		 * @return true if with_cancellation() was called
		 */
		[[nodiscard]] auto has_explicit_cancellation() const -> bool;

		/**
		 * @brief Checks if this job has any composed components.
		 *
		 * @return true if any with_*() method has been called, false otherwise
		 */
		[[nodiscard]] auto has_components() const -> bool;

	protected:
		/**
		 * @brief Invokes the completion callbacks if they are set.
		 *
		 * This method should be called by derived classes or the job execution
		 * framework after do_work() completes. It handles invoking both the
		 * on_complete and on_error callbacks as appropriate.
		 *
		 * @param result The result from do_work()
		 */
		auto invoke_callbacks(const common::VoidResult& result) -> void;
		/**
		 * @brief The descriptive name of the job, used primarily for identification and logging.
		 */
		std::string name_;

		/**
		 * @brief An optional container of raw byte data that may be used by the job.
		 *
		 * If the constructor without the data parameter is used, this vector will remain empty
		 * unless manually populated by derived classes or other means.
		 */
		std::vector<uint8_t> data_;

		/**
		 * @brief A weak reference to the @c job_queue that currently manages this job.
		 *
		 * This reference can expire if the queue is destroyed, so always lock it into a
		 * @c std::shared_ptr before use to avoid invalid access.
		 */
		std::weak_ptr<job_queue> job_queue_;

		/**
		 * @brief The cancellation token associated with this job.
		 *
		 * This token can be used to cancel the job during execution. The job should
		 * periodically check this token and abort if it is cancelled.
		 */
		cancellation_token cancellation_token_;

	private:
		/**
		 * @brief Static counter for generating unique job IDs.
		 */
		static std::atomic<std::uint64_t> next_job_id_;

		/**
		 * @brief Unique identifier for this job.
		 *
		 * Generated automatically during construction, unique within the
		 * lifetime of the application.
		 */
		std::uint64_t job_id_;

		/**
		 * @brief Time when the job was created.
		 *
		 * Captured during construction, used for wait time calculations
		 * in diagnostics.
		 */
		std::chrono::steady_clock::time_point enqueue_time_;

		/**
		 * @brief Composed components for this job.
		 *
		 * Lazily allocated when first with_*() method is called.
		 * This enables the composition pattern without memory overhead
		 * for jobs that don't use composition.
		 */
		std::unique_ptr<job_components> components_;

		/**
		 * @brief Ensures components_ is allocated (lazy initialization).
		 */
		auto ensure_components() -> job_components&;
	};
} // namespace kcenon::thread

// ----------------------------------------------------------------------------
// Formatter specializations for job
// ----------------------------------------------------------------------------

#include <kcenon/thread/utils/formatter_macros.h>

// Generate formatter specializations for kcenon::thread::job
DECLARE_FORMATTER(kcenon::thread::job)
