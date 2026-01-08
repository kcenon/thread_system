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

#include <kcenon/thread/core/thread_worker.h>

#include <kcenon/thread/utils/formatter.h>

#include <thread>

// Platform-specific CPU pause intrinsics
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
	#include <emmintrin.h>  // For _mm_pause()
#endif

using namespace utility_module;

/**
 * @file thread_worker.cpp
 * @brief Implementation of worker thread for thread pool execution.
 *
 * This file contains the implementation of the thread_worker class, which represents
 * individual worker threads within a thread pool. Each worker thread continuously
 * processes jobs from a shared job queue until the thread pool is shut down.
 * 
 * Key Features:
 * - Continuous job processing from shared queue
 * - Optional performance timing and logging
 * - Graceful shutdown handling
 * - Error propagation and logging
 * - Job queue association management
 * 
 * Worker Lifecycle:
 * 1. Worker is created and associated with a job queue
 * 2. Worker thread starts and enters continuous work loop
 * 3. Worker dequeues and executes jobs until queue is empty/stopped
 * 4. Worker logs execution results and timing information
 * 5. Worker shuts down gracefully when work is complete
 * 
 * Performance Characteristics:
 * - Low latency job pickup (typically <1µs)
 * - Efficient queue polling with minimal CPU overhead
 * - Optional timing measurements for performance analysis
 */

namespace kcenon::thread
{
	// Initialize static member
	std::atomic<std::size_t> thread_worker::next_worker_id_{0};
	/**
	 * @brief Constructs a worker thread with optional timing capabilities.
	 *
	 * Implementation details:
	 * - Inherits from thread_base to get thread management functionality
	 * - Sets descriptive name "thread_worker" for debugging and logging
	 * - Initializes timing flag for optional performance measurement
	 * - Job queue is not set initially (must be set before starting work)
	 * - Stores thread context for logging and monitoring
	 * - Creates a cancellation token for job cancellation support
	 *
	 * Performance Timing:
	 * - When enabled, measures execution time for each job
	 * - Uses high_resolution_clock for precise measurements
	 * - Minimal overhead when disabled (single boolean check)
	 *
	 * @param use_time_tag If true, enables timing measurements for job execution
	 * @param context Thread context providing logging and monitoring services
	 */
	thread_worker::thread_worker(const bool& use_time_tag, const thread_context& context)
		: thread_base("thread_worker"),
		  worker_id_(next_worker_id_.fetch_add(1)),
		  use_time_tag_(use_time_tag),
		  job_queue_(nullptr),
		  context_(context),
		  worker_cancellation_token_(cancellation_token::create()),
		  current_job_(nullptr)
	{
	}

	/**
	 * @brief Destroys the worker thread.
	 * 
	 * Implementation details:
	 * - Base class destructor handles thread shutdown
	 * - No manual cleanup required due to RAII design
	 * - Shared pointer to job queue is automatically released
	 */
	thread_worker::~thread_worker(void) {}

	/**
	 * @brief Associates this worker with a job queue for processing.
	 *
	 * Implementation details:
	 * - Stores shared pointer to enable job dequeuing
	 * - Thread-safe queue replacement with proper synchronization
	 * - Waits for current job completion before replacing queue
	 * - Multiple workers can share the same job queue
	 *
	 * Queue Replacement Synchronization:
	 * - Acquires mutex to prevent concurrent do_work() access
	 * - Sets replacement flag to prevent new job processing
	 * - Waits for current job to complete (current_job_ == nullptr)
	 * - Replaces queue pointer atomically
	 * - Notifies worker thread to resume
	 *
	 * Thread Safety:
	 * - Safe to call from any thread
	 * - Coordinates with do_work() via mutex and condition variable
	 * - Prevents use-after-free during queue replacement
	 *
	 * @param job_queue Shared pointer to the job queue for this worker
	 */
	auto thread_worker::set_job_queue(std::shared_ptr<job_queue> job_queue) -> void
	{
		std::unique_lock<std::mutex> lock(queue_mutex_);

		// Signal that queue replacement is in progress
		queue_being_replaced_ = true;

		// Wait for current job to complete
		// Predicate ensures we don't proceed while a job is executing
		queue_cv_.wait(lock, [this] {
			return current_job_.load(std::memory_order_acquire) == nullptr;
		});

		// Replace the queue pointer
		job_queue_ = std::move(job_queue);

		// Clear replacement flag
		queue_being_replaced_ = false;

		// Notify worker thread that replacement is complete
		queue_cv_.notify_all();
	}

	/**
	 * @brief Sets the thread context for this worker.
	 * 
	 * Implementation details:
	 * - Stores the context for use in logging and monitoring
	 * - Should be called before starting the worker thread
	 * - Context provides access to optional services
	 * 
	 * @param context Thread context with logging and monitoring services
	 */
auto thread_worker::set_context(const thread_context& context) -> void
{
	context_ = context;
}

void thread_worker::set_metrics(std::shared_ptr<metrics::ThreadPoolMetrics> metrics)
{
	metrics_ = std::move(metrics);
}

void thread_worker::set_policy(const worker_policy& policy)
{
	policy_ = policy;

	// Initialize local deque if work-stealing is enabled
	if (policy_.enable_work_stealing && !local_deque_)
	{
		local_deque_ = std::make_unique<lockfree::work_stealing_deque<job*>>();
	}
}

const worker_policy& thread_worker::get_policy() const
{
	return policy_;
}

lockfree::work_stealing_deque<job*>* thread_worker::get_local_deque() noexcept
{
	return local_deque_.get();
}

void thread_worker::set_steal_function(std::function<job*(std::size_t)> steal_fn)
{
	steal_function_ = std::move(steal_fn);
}

std::unique_ptr<job> thread_worker::try_get_job()
{
	// If work-stealing is enabled, try local deque first (LIFO for cache locality)
	if (policy_.enable_work_stealing && local_deque_)
	{
		auto local_result = local_deque_->pop();
		if (local_result.has_value())
		{
			return std::unique_ptr<job>(*local_result);
		}
	}

	// Fallback to global queue
	std::lock_guard<std::mutex> lock(queue_mutex_);
	if (job_queue_ == nullptr)
	{
		return nullptr;
	}

	auto dequeue_result = job_queue_->try_dequeue();
	if (dequeue_result.is_ok())
	{
		return std::move(dequeue_result.value());
	}

	return nullptr;
}

std::unique_ptr<job> thread_worker::try_steal_work()
{
	if (!policy_.enable_work_stealing || !steal_function_)
	{
		return nullptr;
	}

	// Apply exponential backoff between steal attempts
	for (std::size_t attempt = 0; attempt < policy_.max_steal_attempts; ++attempt)
	{
		job* stolen = steal_function_(worker_id_);
		if (stolen != nullptr)
		{
			return std::unique_ptr<job>(stolen);
		}

		// Exponential backoff
		if (attempt > 0)
		{
			auto backoff = policy_.steal_backoff * (1 << (attempt - 1));
			std::this_thread::sleep_for(backoff);
		}
	}

	return nullptr;
}

	/**
	 * @brief Gets the thread context for this worker.
	 * 
	 * @return The thread context providing access to logging and monitoring services
	 */
	auto thread_worker::get_context(void) const -> const thread_context&
	{
		return context_;
	}

	/**
	 * @brief Determines if the worker should continue processing jobs.
	 *
	 * Implementation details:
	 * - Used by thread_base to control the work loop
	 * - Returns false if no job queue is set (prevents infinite loop)
	 * - Returns true while queue is not stopped (even if empty)
	 * - Actual job waiting is handled by non-blocking polling in do_work()
	 * - Thread-safe operation (job_queue methods are thread-safe)
	 *
	 * Work Loop Control:
	 * - Worker continues until queue is explicitly stopped
	 * - Empty queue does NOT cause worker exit - do_work() will poll for jobs
	 * - This prevents premature worker termination before jobs arrive
	 * - Worker exits gracefully only when queue is stopped
	 *
	 * Design Rationale - Solving the Two-Level Condition Variable Problem:
	 * - Thread_base waits on its own condition variable (worker_condition_)
	 * - Job_queue notifies its own condition variable (different object!)
	 * - If should_continue_work() returns false on empty queue:
	 *   1. thread_base waits on worker_condition_
	 *   2. job enqueue notifies job_queue's condition variable
	 *   3. Worker never wakes up - deadlock situation
	 *
	 * - By returning true until stopped, worker enters do_work() immediately
	 * - do_work() uses non-blocking try_dequeue() with polling
	 * - This completely avoids the two-level CV problem
	 * - CPU overhead is minimal due to sleep between polling attempts
	 *
	 * Shutdown Safety:
	 * - thread_pool::stop() can call operations in any order without race conditions
	 * - Queue stop sets is_stopped() = true (atomic operation)
	 * - Worker sees stopped flag and exits cleanly
	 * - No dependency on operation ordering
	 *
	 * Thread Safety:
	 * - Synchronizes access to job_queue_ with queue_mutex_
	 * - Prevents race conditions with set_job_queue() and do_work()
	 * - Uses lock_guard for RAII-based exception safety
	 * - Mutex marked mutable to allow locking in const method
	 *
	 * @return true if worker should continue processing, false to exit
	 */
	auto thread_worker::should_continue_work() const -> bool
	{
		// Synchronize access to job_queue_ with set_job_queue() and do_work()
		std::lock_guard<std::mutex> lock(queue_mutex_);

		if (job_queue_ == nullptr)
		{
			return false;
		}

		// Continue while queue is not stopped - do_work() handles polling for jobs
		return !job_queue_->is_stopped();
	}

	/**
	 * @brief Executes a single work cycle by processing one job from the queue.
	 *
	 * Implementation details:
	 * - Uses non-blocking try_dequeue() to avoid condition variable deadlock
	 * - Polls the queue with minimal CPU overhead via short sleep intervals
	 * - Validates job pointer before execution
	 * - Optionally measures execution timing for performance analysis
	 * - Associates job with queue for potential re-submission
	 * - Logs execution results with appropriate detail level
	 *
	 * Job Processing Workflow:
	 * 1. Validate job queue availability
	 * 2. Attempt non-blocking dequeue of next job
	 * 3. If queue is empty: sleep briefly and return (will be called again)
	 * 4. Validate dequeued job pointer
	 * 5. Optionally record start time for measurement
	 * 6. Associate job with queue for context
	 * 7. Execute job's do_work() method
	 * 8. Handle execution errors with detailed logging
	 * 9. Log successful completion with timing info if enabled
	 *
	 * Non-Blocking Polling Strategy:
	 * - Uses try_dequeue() instead of blocking dequeue()
	 * - Avoids two-level condition variable problem completely
	 * - When queue is empty: sleeps 100μs to prevent CPU spinning
	 * - Returns success immediately, will be called again by thread_base loop
	 * - should_continue_work() determines when to exit (on queue stop)
	 *
	 * Performance Characteristics:
	 * - Zero blocking on condition variables (no CV synchronization)
	 * - Minimal CPU overhead: 100μs sleep when idle
	 * - Fast job pickup when available: <1μs typical latency
	 * - Predictable behavior independent of queue notification timing
	 *
	 * Error Handling:
	 * - Missing job queue: Returns resource allocation error
	 * - Empty queue: Returns success after brief sleep (normal polling)
	 * - Null job pointer: Returns job invalid error
	 * - Job execution failure: Returns execution failed error with details
	 *
	 * Performance Measurements:
	 * - High-resolution timing when use_time_tag_ is enabled
	 * - Nanosecond precision for accurate profiling
	 * - Minimal overhead when timing is disabled
	 *
	 * Logging Behavior:
	 * - Standard success message when timing is disabled
	 * - Timestamped success message when timing is enabled
	 * - Error details are propagated up the call stack
	 *
	 * @return result_void indicating success or detailed error information
	 */
	auto thread_worker::do_work() -> common::VoidResult
	{
		// Acquire lock to safely get queue pointer
		std::unique_lock<std::mutex> lock(queue_mutex_);

		// Validate that job queue is available for processing
		if (job_queue_ == nullptr)
		{
			lock.unlock();
			return common::error_info{static_cast<int>(error_code::resource_allocation_failed), "there is no job_queue", "thread_system"};
		}

		// Make a local copy of the queue pointer while holding the lock
		// The shared_ptr keeps the queue alive even if set_job_queue() replaces it
		// No need to wait for !queue_being_replaced_ - the local copy is safe to use
		std::shared_ptr<job_queue> local_queue = job_queue_;

		// Release lock before dequeuing to allow other operations
		lock.unlock();

		std::unique_ptr<job> current_job;

		// Work-stealing path: try local deque first, then global queue, then steal
		if (policy_.enable_work_stealing)
		{
			// Step 1: Try local deque (LIFO for cache locality)
			if (local_deque_)
			{
				auto local_result = local_deque_->pop();
				if (local_result.has_value())
				{
					current_job = std::unique_ptr<job>(*local_result);
				}
			}

			// Step 2: Try global queue
			if (!current_job)
			{
				auto dequeue_result = local_queue->try_dequeue();
				if (dequeue_result.is_ok())
				{
					current_job = std::move(dequeue_result.value());
				}
			}

			// Step 3: Try to steal from other workers
			if (!current_job)
			{
				current_job = try_steal_work();
			}

			// No work found - mark as idle and sleep
			if (!current_job)
			{
				is_idle_.store(true, std::memory_order_relaxed);
				std::this_thread::sleep_for(policy_.idle_sleep_duration);
				return common::ok();
			}
		}
		else
		{
			// Original non-work-stealing path
			// Hybrid wait strategy: short spin followed by blocking dequeue
			// This approach provides:
			// - Sub-ms pickup latency (via spin loop)
			// - Near-zero idle CPU usage (via blocking dequeue)
			// - No busy-waiting overhead

			// Phase 1: Short bounded spin (16 iterations)
			// Optimized for scenarios where jobs arrive quickly
			constexpr int spin_count = 16;
			for (int i = 0; i < spin_count; ++i)
			{
				auto dequeue_result = local_queue->try_dequeue();
				if (dequeue_result.is_ok())
				{
					// Job found during spin - fast path
					current_job = std::move(dequeue_result.value());
					break;
				}

				// CPU pause hint to reduce contention and power consumption
				// Different intrinsics per compiler and architecture
				#if defined(_MSC_VER)
					// MSVC: Use _mm_pause() for x86/x64, YieldProcessor() for ARM
					#if defined(_M_IX86) || defined(_M_X64)
						_mm_pause();
					#elif defined(_M_ARM) || defined(_M_ARM64)
						__yield();
					#else
						std::this_thread::yield();
					#endif
				#elif defined(__GNUC__) || defined(__clang__)
					// GCC/Clang: Use builtin functions
					#if defined(__x86_64__) || defined(__i386__)
						__builtin_ia32_pause();
					#elif defined(__aarch64__) || defined(__arm__)
						__asm__ __volatile__("yield");
					#else
						std::this_thread::yield();
					#endif
				#else
					std::this_thread::yield();
				#endif
			}

			// Phase 2: Polling with longer sleep if spin didn't find a job
			// Use longer sleep (10ms) instead of blocking to avoid hang issues
			// This provides near-zero CPU usage while maintaining responsiveness
			if (!current_job)
			{
				is_idle_.store(true, std::memory_order_relaxed);

				// Sleep longer than spin to reduce CPU usage
				// 10ms provides good balance between latency and efficiency
				std::this_thread::sleep_for(std::chrono::milliseconds(10));

				// Return immediately - will be called again by thread_base loop
				// should_continue_work() will determine when to exit
				return common::ok();
			}
		}

		// Validate job pointer
		if (current_job == nullptr)
		{
			return common::error_info{static_cast<int>(error_code::job_invalid), "error executing job: nullptr", "thread_system"};
		}

		// Update idle time statistics before transitioning to busy state
		auto now = std::chrono::steady_clock::now();
		auto state_since = get_state_since();
		if (is_idle_.load(std::memory_order_relaxed))
		{
			auto idle_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(now - state_since);
			total_idle_time_ns_.fetch_add(
				static_cast<std::uint64_t>(idle_duration.count()),
				std::memory_order_relaxed
			);
		}

		// Mark worker as busy and update state timestamp
		is_idle_.store(false, std::memory_order_relaxed);
		state_since_rep_.store(now.time_since_epoch().count(), std::memory_order_release);
		current_job_start_time_ = now;

		// Initialize timing measurement if performance monitoring is enabled
		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>>
			started_time_point = std::nullopt;
		if (use_time_tag_)
		{
			started_time_point = std::chrono::high_resolution_clock::now();
		}

		// Associate the job with its source queue for potential re-submission
		// Use local_queue to avoid race with set_job_queue()
		current_job->set_job_queue(local_queue);

		// Set cancellation token on the job for cooperative cancellation
		// This allows the job to check if it should cancel during execution
		current_job->set_cancellation_token(worker_cancellation_token_);

		// Track currently executing job atomically for on_stop_requested()
		// Use release ordering to ensure job state is visible to cancellation thread
		current_job_.store(current_job.get(), std::memory_order_release);

		// Execute the job's work method and capture the result
		auto work_result = current_job->do_work();
		std::uint64_t execution_duration_ns = 0;
		if (started_time_point.has_value())
		{
			auto end_time = std::chrono::high_resolution_clock::now();
			execution_duration_ns = static_cast<std::uint64_t>(
				std::chrono::duration_cast<std::chrono::nanoseconds>(
					end_time - started_time_point.value()).count());
		}

		// Capture job name before clearing (for logging after job destruction)
		std::string job_name = current_job->get_name();

		// Clear current job tracking and destroy job under mutex protection
		// This prevents race condition with on_stop_requested() (Issue #225)
		// The mutex ensures job is not accessed while being destroyed
		{
			std::lock_guard<std::mutex> notify_lock(queue_mutex_);
			current_job_.store(nullptr, std::memory_order_release);

			// Explicitly destroy the job while holding the mutex
			// This is critical for thread safety - on_stop_requested() acquires
			// the same mutex before accessing current_job_, ensuring it cannot
			// access a job that is being destroyed
			current_job.reset();

			// Notify any waiting set_job_queue() that job has completed
			// This allows queue replacement to proceed safely
			// Lock is held to prevent lost wakeup between predicate check and wait
			queue_cv_.notify_all();
		}

		// Update busy time and state transition for diagnostics
		{
			auto end_now = std::chrono::steady_clock::now();
			auto busy_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
				end_now - current_job_start_time_
			);
			total_busy_time_ns_.fetch_add(
				static_cast<std::uint64_t>(busy_duration.count()),
				std::memory_order_relaxed
			);
			// Transition back to idle state
			is_idle_.store(true, std::memory_order_relaxed);
			state_since_rep_.store(end_now.time_since_epoch().count(), std::memory_order_release);
		}

		if (work_result.is_err())
		{
			// Increment failed job counter
			jobs_failed_.fetch_add(1, std::memory_order_relaxed);

			if (metrics_)
			{
				metrics_->record_execution(0, false);
			}
			return common::error_info{static_cast<int>(error_code::job_execution_failed),
				formatter::format("error executing job: {}", work_result.error().message), "thread_system"};
		}

		// Increment completed job counter
		jobs_completed_.fetch_add(1, std::memory_order_relaxed);

		// Log successful job completion based on timing configuration
		// Note: Using captured job_name since job is already destroyed
		if (!started_time_point.has_value())
		{
			// Standard logging without timing information
			context_.log(common::interfaces::log_level::debug,
			            formatter::format("job executed successfully: {} on thread_worker",
			                            job_name));
		}
		else
		{
			// Enhanced logging with execution timing information
			context_.log(common::interfaces::log_level::debug,
			            formatter::format("job executed successfully: {} on thread_worker ({}ns)",
			                            job_name, execution_duration_ns));

			// Update worker metrics if monitoring is available
			if (context_.monitoring())
			{
				common::interfaces::worker_metrics metrics(worker_id_);
				metrics.jobs_processed.value = 1;
				metrics.total_processing_time_ns.value = static_cast<double>(execution_duration_ns);
				// Use proper worker ID instead of thread hash
				context_.update_worker_metrics(worker_id_, metrics);
			}
		}

		if (metrics_)
		{
			metrics_->record_execution(execution_duration_ns, true);
		}

		return common::ok();
	}

	std::size_t thread_worker::get_worker_id() const
	{
		return worker_id_;
	}

	/**
	 * @brief Checks if the worker is currently idle (not processing a job).
	 *
	 * Implementation details:
	 * - Returns current state of is_idle_ flag
	 * - Relaxed memory ordering sufficient (advisory-only value)
	 * - No synchronization needed (snapshot of current state)
	 *
	 * @return true if worker is idle, false if actively processing a job
	 */
	bool thread_worker::is_idle() const noexcept
	{
		return is_idle_.load(std::memory_order_relaxed);
	}

	/**
	 * @brief Propagates cancellation signal to the currently executing job.
	 *
	 * Implementation details:
	 * - Called from thread_base::stop() when worker shutdown is requested
	 * - First cancels the worker's cancellation token (affects all future jobs)
	 * - Then directly cancels the current job's token if a job is running
	 * - Uses mutex synchronization to prevent use-after-free race condition
	 *
	 * Cancellation Propagation:
	 * 1. Cancel worker_cancellation_token_ (prevents new jobs from starting)
	 * 2. Acquire queue_mutex_ to safely access current job
	 * 3. If a job is running, get its cancellation token and cancel it
	 * 4. Job will detect cancellation on its next is_cancelled() check
	 *
	 * Thread Safety (Issue #225 fix):
	 * - Called from stop() thread (not worker thread)
	 * - Uses queue_mutex_ to synchronize with do_work() job destruction
	 * - Prevents data race between job destructor and virtual method call
	 * - This fixes EXC_BAD_ACCESS on ARM64 caused by use-after-free
	 *
	 * Race Condition Fixed:
	 * - Before: on_stop_requested() could access job while do_work() was destroying it
	 * - After: queue_mutex_ ensures job is not destroyed while being accessed
	 * - do_work() now destroys job while holding the mutex
	 *
	 * @note This implements cooperative cancellation - the job must check
	 *       its cancellation token periodically to actually stop execution.
	 */
	auto thread_worker::on_stop_requested() -> void
	{
		// Cancel the worker's token first
		// This ensures any future jobs will see cancellation immediately
		// Jobs receive this token via set_cancellation_token() in do_work()
		worker_cancellation_token_.cancel();

		// Acquire mutex to safely access current job
		// This prevents race condition with do_work() job destruction (Issue #225)
		std::lock_guard<std::mutex> lock(queue_mutex_);

		// Load the currently executing job pointer atomically
		auto* job_ptr = current_job_.load(std::memory_order_acquire);

		// If a job is currently executing, cancel it directly
		if (job_ptr != nullptr)
		{
			// Get the job's cancellation token and cancel it
			// This provides redundancy in case the job cached its token before
			// we cancelled worker_cancellation_token_
			auto job_token = job_ptr->get_cancellation_token();
			job_token.cancel();

			// Log cancellation attempt for debugging
			context_.log(common::interfaces::log_level::debug,
			            formatter::format("Cancellation requested for job: {} on worker {}",
			                            job_ptr->get_name(), worker_id_));
		}
	}

	std::uint64_t thread_worker::get_jobs_completed() const noexcept
	{
		return jobs_completed_.load(std::memory_order_relaxed);
	}

	std::uint64_t thread_worker::get_jobs_failed() const noexcept
	{
		return jobs_failed_.load(std::memory_order_relaxed);
	}

	std::chrono::nanoseconds thread_worker::get_total_busy_time() const noexcept
	{
		return std::chrono::nanoseconds{total_busy_time_ns_.load(std::memory_order_relaxed)};
	}

	std::chrono::nanoseconds thread_worker::get_total_idle_time() const noexcept
	{
		return std::chrono::nanoseconds{total_idle_time_ns_.load(std::memory_order_relaxed)};
	}

	std::chrono::steady_clock::time_point thread_worker::get_state_since() const noexcept
	{
		auto rep = state_since_rep_.load(std::memory_order_acquire);
		return std::chrono::steady_clock::time_point{
			std::chrono::steady_clock::duration{rep}
		};
	}

	std::optional<diagnostics::job_info> thread_worker::get_current_job_info() const noexcept
	{
		std::lock_guard<std::mutex> lock(queue_mutex_);
		auto* job_ptr = current_job_.load(std::memory_order_acquire);
		if (job_ptr == nullptr)
		{
			return std::nullopt;
		}

		diagnostics::job_info info;
		info.job_id = job_ptr->get_job_id();
		info.job_name = job_ptr->get_name();
		info.status = diagnostics::job_status::running;
		info.start_time = current_job_start_time_;
		info.enqueue_time = job_ptr->get_enqueue_time();
		info.executed_by = std::this_thread::get_id();

		auto now = std::chrono::steady_clock::now();
		info.execution_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
			now - current_job_start_time_
		);
		// Calculate wait time from enqueue to start
		info.wait_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
			current_job_start_time_ - job_ptr->get_enqueue_time()
		);

		return info;
	}
} // namespace kcenon::thread
