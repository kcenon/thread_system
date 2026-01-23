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

#include <kcenon/thread/core/job.h>

#include <kcenon/thread/core/job_queue.h>

using namespace utility_module;

/**
 * @file job.cpp
 * @brief Implementation of the base job class for the thread system.
 *
 * This file contains the implementation of the job class, which serves as the
 * abstract base class for all work units in the thread system. The job class
 * provides essential functionality including:
 * - Job identification and naming
 * - Binary data storage for data-processing jobs
 * - Cancellation token support for cooperative cancellation
 * - Job queue association for scheduling
 * - Standard interface for work execution
 * 
 * Design Principles:
 * - Abstract base class requiring derived classes to implement do_work()
 * - Support for both simple and data-driven job types
 * - Thread-safe access to job properties
 * - Efficient memory management through RAII
 * - Flexible cancellation mechanism
 */

namespace kcenon::thread
{
	// Initialize static member for job ID generation
	std::atomic<std::uint64_t> job::next_job_id_{0};

	/**
	 * @brief Constructs a basic job with name only.
	 * 
	 * Implementation details:
	 * - Initializes job with descriptive name for identification
	 * - Creates empty data vector (no binary data associated)
	 * - Cancellation token is default-constructed (not cancelled)
	 * - Job queue is not associated initially (weak_ptr is empty)
	 * 
	 * Use Cases:
	 * - Simple computational jobs without input data
	 * - Lambda-based callback jobs
	 * - Jobs that generate data rather than process it
	 * 
	 * @param name Descriptive name for debugging and logging
	 */
	job::job(const std::string& name)
		: name_(name)
		, data_(std::vector<uint8_t>())
		, job_id_(next_job_id_.fetch_add(1, std::memory_order_relaxed))
		, enqueue_time_(std::chrono::steady_clock::now())
	{
	}

	/**
	 * @brief Constructs a data-processing job with binary data.
	 * 
	 * Implementation details:
	 * - Stores binary data for processing during job execution
	 * - Data is copied into the job (caller retains ownership of original)
	 * - Suitable for file processing, network data handling, etc.
	 * - Memory efficient for moderate data sizes
	 * 
	 * Data Handling:
	 * - Data is stored as std::vector<uint8_t> for flexibility
	 * - Supports any binary data format (images, documents, network packets)
	 * - Data lifetime matches job lifetime (automatic cleanup)
	 * 
	 * Performance Considerations:
	 * - Data is copied during construction (consider move semantics for large data)
	 * - Memory usage scales with data size
	 * - Efficient for data sizes up to several MB
	 * 
	 * @param data Binary data to be processed by this job
	 * @param name Descriptive name for debugging and logging
	 */
	job::job(const std::vector<uint8_t>& data, const std::string& name)
		: name_(name)
		, data_(data)
		, job_id_(next_job_id_.fetch_add(1, std::memory_order_relaxed))
		, enqueue_time_(std::chrono::steady_clock::now())
	{
	}

	/**
	 * @brief Destroys the job and cleans up resources.
	 * 
	 * Implementation details:
	 * - Virtual destructor ensures proper cleanup of derived classes
	 * - Automatically cleans up stored data and job queue reference
	 * - No manual cleanup required due to RAII design
	 * - Safe to destroy even if job is queued (weak_ptr prevents dangling references)
	 */
	job::~job(void) {}

	/**
	 * @brief Gets the descriptive name of this job.
	 * 
	 * Implementation details:
	 * - Simple accessor for job identification
	 * - Thread-safe (string is immutable after construction)
	 * - Used for debugging, logging, and diagnostics
	 * 
	 * @return Job name as provided during construction
	 */
	auto job::get_name(void) const -> std::string { return name_; }

	/**
	 * @brief Default implementation of work execution (must be overridden).
	 * 
	 * Implementation details:
	 * - Base implementation always returns "not implemented" error
	 * - Forces derived classes to provide actual work implementation
	 * - Maintains compatibility with result_void error handling
	 * - Should never be called in production (indicates missing override)
	 * 
	 * Design Pattern:
	 * - Template method pattern: defines interface, requires implementation
	 * - Pure virtual in spirit (returns error instead of being pure virtual)
	 * - Enables compilation while encouraging proper inheritance
	 *
	 * Derived Class Requirements:
	 * - Must override this method to provide actual work logic
	 * - Should return common::ok() on success
	 * - Should return common::error_info{...} on failure with descriptive message
	 *
	 * @return Error indicating method needs to be implemented in derived class
	 */
	auto job::do_work(void) -> common::VoidResult {
		// Base implementation indicates missing override in derived class
		// This should never be called in production code
		return common::error_info{static_cast<int>(error_code::not_implemented), "job::do_work() must be implemented in derived class", "thread_system"};
	}

	/**
	 * @brief Sets the cancellation token for cooperative cancellation.
	 * 
	 * Implementation details:
	 * - Stores cancellation token for use during job execution
	 * - Enables cooperative cancellation (job checks token periodically)
	 * - Thread-safe assignment (cancellation_token is thread-safe)
	 * - Token can be checked during long-running operations
	 * 
	 * Cancellation Model:
	 * - Cooperative: job must check token voluntarily
	 * - Non-preemptive: job won't be forcibly stopped
	 * - Graceful: allows cleanup before termination
	 * 
	 * Usage Pattern:
	 * 1. Create cancellation_token
	 * 2. Set token on jobs before submission
	 * 3. Job checks token during execution
	 * 4. External code can signal cancellation
	 * 
	 * @param token Cancellation token for cooperative cancellation
	 */
	auto job::set_cancellation_token(const cancellation_token& token) -> void {
		cancellation_token_ = token;
	}

	/**
	 * @brief Gets the current cancellation token.
	 * 
	 * Implementation details:
	 * - Returns copy of stored cancellation token
	 * - Thread-safe operation (cancellation_token is thread-safe)
	 * - Used by derived classes to check cancellation status
	 * - Returns default token if none was set (not cancelled)
	 * 
	 * Typical Usage in Derived Classes:
	 * @code
	 * auto do_work() -> result_void {
	 *     auto token = get_cancellation_token();
	 *     for (int i = 0; i < large_number; ++i) {
	 *         if (token.is_cancelled()) {
	 *             return error{error_code::cancelled, "Job was cancelled"};
	 *         }
	 *         // Do work...
	 *     }
	 *     return {};
	 * }
	 * @endcode
	 * 
	 * @return Current cancellation token
	 */
	auto job::get_cancellation_token() const -> cancellation_token {
		return cancellation_token_;
	}

	/**
	 * @brief Associates this job with a job queue for scheduling.
	 * 
	 * Implementation details:
	 * - Stores weak reference to prevent circular dependencies
	 * - Allows job to interact with its queue if needed
	 * - Automatic cleanup when queue is destroyed
	 * - Used for advanced job management scenarios
	 * 
	 * Weak Reference Benefits:
	 * - Prevents memory leaks from circular references
	 * - Queue can be destroyed without affecting jobs
	 * - Jobs can check if queue is still alive
	 * - No impact on queue lifetime management
	 * 
	 * Use Cases:
	 * - Jobs that need to submit additional jobs
	 * - Self-scheduling or recursive jobs
	 * - Jobs that need queue statistics
	 * - Advanced workflow management
	 * 
	 * @param job_queue Shared pointer to the job queue
	 */
	auto job::set_job_queue(const std::shared_ptr<job_queue>& job_queue) -> void
	{
		job_queue_ = job_queue;
	}

	/**
	 * @brief Gets the associated job queue if it still exists.
	 * 
	 * Implementation details:
	 * - Converts weak_ptr to shared_ptr (may return nullptr)
	 * - Thread-safe operation (weak_ptr::lock is atomic)
	 * - Returns nullptr if queue has been destroyed
	 * - Used for advanced job scheduling scenarios
	 * 
	 * Return Value Interpretation:
	 * - Valid shared_ptr: Queue is alive and accessible
	 * - nullptr: Queue has been destroyed or was never set
	 * 
	 * Safety Considerations:
	 * - Always check return value before use
	 * - Queue may be destroyed between check and use
	 * - Designed to fail gracefully if queue is unavailable
	 * 
	 * @return Shared pointer to job queue or nullptr if unavailable
	 */
	auto job::get_job_queue(void) const -> std::shared_ptr<job_queue> { return job_queue_.lock(); }

	/**
	 * @brief Provides a string representation of this job.
	 * 
	 * Implementation details:
	 * - Formats job name for display purposes
	 * - Used in debugging, logging, and diagnostics
	 * - Consistent format across all job types
	 * - Thread-safe operation (accesses immutable name)
	 * 
	 * Output Format:
	 * - "job: <name>" where <name> is the job's descriptive name
	 * - Simple, readable format for human consumption
	 * - Suitable for log files and debug output
	 * 
	 * @return Formatted string representation of the job
	 */
	auto job::to_string(void) const -> std::string { return formatter::format("job: {}", name_); }

	// ========================================================================
	// Composition Methods Implementation
	// ========================================================================

	/**
	 * @brief Ensures the components structure is allocated.
	 *
	 * Implementation details:
	 * - Lazy initialization: only allocates memory when first needed
	 * - Returns reference for convenient access after ensuring allocation
	 * - Thread-safety note: this method is not thread-safe, but composition
	 *   is typically done during job construction before submission to a queue
	 *
	 * @return Reference to the components structure
	 */
	auto job::ensure_components() -> job_components&
	{
		if (!components_)
		{
			components_ = std::make_unique<job_components>();
		}
		return *components_;
	}

	/**
	 * @brief Attaches a completion callback to this job.
	 *
	 * Implementation details:
	 * - Uses lazy initialization to avoid memory overhead for simple jobs
	 * - Returns *this to enable fluent method chaining
	 * - Callback is stored and invoked by invoke_callbacks()
	 *
	 * @param callback Function to call with the job's result
	 * @return Reference to this job for chaining
	 */
	auto job::with_on_complete(std::function<void(common::VoidResult)> callback) -> job&
	{
		ensure_components().on_complete = std::move(callback);
		return *this;
	}

	/**
	 * @brief Attaches an error callback to this job.
	 *
	 * Implementation details:
	 * - Called only when do_work() returns an error
	 * - More specific than with_on_complete() for error-only handling
	 * - Can be used in combination with with_on_complete()
	 *
	 * @param callback Function to call with the error information
	 * @return Reference to this job for chaining
	 */
	auto job::with_on_error(std::function<void(const common::error_info&)> callback) -> job&
	{
		ensure_components().on_error = std::move(callback);
		return *this;
	}

	/**
	 * @brief Sets the priority level for this job.
	 *
	 * Implementation details:
	 * - Priority is stored in optional to distinguish "not set" from "normal"
	 * - Can be queried by schedulers via get_priority()
	 * - Does not affect execution order in basic job_queue (for typed pools)
	 *
	 * @param priority The priority level to set
	 * @return Reference to this job for chaining
	 */
	auto job::with_priority(job_priority priority) -> job&
	{
		ensure_components().priority = priority;
		return *this;
	}

	/**
	 * @brief Attaches a cancellation token to this job via composition.
	 *
	 * Implementation details:
	 * - Sets the cancellation token on the job
	 * - Marks the job as having explicit cancellation for querying
	 * - Enables fluent method chaining
	 *
	 * @param token The cancellation token to use
	 * @return Reference to this job for chaining
	 */
	auto job::with_cancellation(const cancellation_token& token) -> job&
	{
		cancellation_token_ = token;
		ensure_components().has_explicit_cancellation = true;
		return *this;
	}

	/**
	 * @brief Attaches a retry policy to this job.
	 *
	 * Implementation details:
	 * - Stores the retry policy in components for executor access
	 * - The executor is responsible for implementing retry logic
	 * - Enables fluent method chaining
	 *
	 * @param policy The retry policy to use
	 * @return Reference to this job for chaining
	 */
	auto job::with_retry(const retry_policy& policy) -> job&
	{
		ensure_components().retry = policy;
		return *this;
	}

	/**
	 * @brief Sets a timeout for job execution.
	 *
	 * Implementation details:
	 * - Stores timeout duration in components for executor access
	 * - The executor is responsible for implementing timeout logic
	 * - Enables fluent method chaining
	 *
	 * @param timeout Maximum execution time allowed
	 * @return Reference to this job for chaining
	 */
	auto job::with_timeout(std::chrono::milliseconds timeout) -> job&
	{
		ensure_components().timeout = timeout;
		return *this;
	}

	/**
	 * @brief Gets the priority level of this job.
	 *
	 * Implementation details:
	 * - Returns normal priority if not explicitly set
	 * - Returns the composed priority if set via with_priority()
	 *
	 * @return The job's priority level
	 */
	auto job::get_priority() const -> job_priority
	{
		if (components_ && components_->priority.has_value())
		{
			return components_->priority.value();
		}
		return job_priority::normal;
	}

	/**
	 * @brief Gets the retry policy of this job.
	 *
	 * Implementation details:
	 * - Returns the retry policy if set via with_retry()
	 * - Returns std::nullopt if no retry policy was configured
	 *
	 * @return The job's retry policy or std::nullopt
	 */
	auto job::get_retry_policy() const -> std::optional<retry_policy>
	{
		if (components_ && components_->retry.has_value())
		{
			return components_->retry;
		}
		return std::nullopt;
	}

	/**
	 * @brief Gets the timeout duration for this job.
	 *
	 * Implementation details:
	 * - Returns the timeout if set via with_timeout()
	 * - Returns std::nullopt if no timeout was configured
	 *
	 * @return The job's timeout or std::nullopt
	 */
	auto job::get_timeout() const -> std::optional<std::chrono::milliseconds>
	{
		if (components_ && components_->timeout.has_value())
		{
			return components_->timeout;
		}
		return std::nullopt;
	}

	/**
	 * @brief Checks if this job has an explicit cancellation set via composition.
	 *
	 * Implementation details:
	 * - Returns true if with_cancellation() was called
	 * - Distinguishes between explicit composition and set_cancellation_token()
	 *
	 * @return true if with_cancellation() was called
	 */
	auto job::has_explicit_cancellation() const -> bool
	{
		return components_ && components_->has_explicit_cancellation;
	}

	/**
	 * @brief Checks if this job has any composed components.
	 *
	 * Implementation details:
	 * - Simple null check on the components pointer
	 * - Useful for conditional logic based on composition state
	 *
	 * @return true if any with_*() method has been called
	 */
	auto job::has_components() const -> bool
	{
		return components_ != nullptr;
	}

	/**
	 * @brief Invokes the completion callbacks if they are set.
	 *
	 * Implementation details:
	 * - Invokes on_complete callback with the full result
	 * - Invokes on_error callback only if result is an error
	 * - Order: on_error is called first (if applicable), then on_complete
	 * - Exceptions from callbacks are not caught (propagate to caller)
	 *
	 * @param result The result from do_work()
	 */
	auto job::invoke_callbacks(const common::VoidResult& result) -> void
	{
		if (!components_)
		{
			return;
		}

		// Invoke error callback first if result is an error
		if (result.is_err() && components_->on_error)
		{
			components_->on_error(result.error());
		}

		// Always invoke on_complete callback if set
		if (components_->on_complete)
		{
			components_->on_complete(result);
		}
	}
} // namespace kcenon::thread