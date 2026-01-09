#pragma once

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

#include <kcenon/thread/core/thread_base.h>
#include <kcenon/thread/utils/formatter.h>
#include <kcenon/thread/core/job_queue.h>
#include <kcenon/thread/core/cancellation_token.h>
#include <kcenon/thread/utils/convert_string.h>
#include <kcenon/thread/forward.h>
#include <kcenon/thread/interfaces/thread_context.h>
#include "worker_policy.h"
#include <kcenon/thread/metrics/thread_pool_metrics.h>
#include <kcenon/thread/lockfree/work_stealing_deque.h>
#include <kcenon/thread/diagnostics/job_info.h>
#include <kcenon/thread/diagnostics/execution_event.h>

#include <memory>
#include <optional>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>

// Forward declarations
namespace kcenon::thread::diagnostics
{
	class thread_pool_diagnostics;
}

namespace kcenon::thread
{
	/**
	 * @class thread_worker
	 * @brief A specialized worker thread that processes jobs from a @c job_queue.
	 *
	 * The @c thread_worker class inherits from @c thread_base, leveraging its
	 * life-cycle control methods (@c start, @c stop, etc.) and provides an
	 * implementation for job processing using a shared @c job_queue. By overriding
	 * @c should_continue_work() and @c do_work(), it polls the queue for available
	 * jobs and executes them.
	 *
	 * ### Typical Usage
	 * @code
	 * auto my_queue = std::make_shared<job_queue>();
	 * auto worker   = std::make_unique<thread_worker>();
	 * worker->set_job_queue(my_queue);
	 * worker->start(); // Worker thread begins processing jobs
	 *
	 * // Enqueue jobs into my_queue...
	 *
	 * // Eventually...
	 * worker->stop();  // Waits for current job to finish, then stops
	 * @endcode
	 */
	class thread_worker : public thread_base
	{
	public:
		/**
		 * @brief Constructs a new @c thread_worker.
		 * @param use_time_tag If set to @c true (default), the worker may log or utilize
		 *                     timestamps/tags when processing jobs.
		 * @param context Optional thread context for logging and monitoring (defaults to empty context).
		 *
		 * This flag can be used to measure job durations, implement logging with
		 * timestamps, or any other time-related features in your job processing.
		 * The context provides access to logging and monitoring services.
		 */
		thread_worker(const bool& use_time_tag = true,
		             const thread_context& context = thread_context());

		/**
		 * @brief Virtual destructor. Ensures the worker thread is stopped before destruction.
		 */
		virtual ~thread_worker(void);

		/**
		 * @brief Sets the @c job_queue that this worker should process.
		 * @param job_queue A shared pointer to the queue containing jobs.
		 *
		 * Once the queue is set and @c start() is called, the worker will repeatedly poll
		 * the queue for new jobs and process them.
		 */
		auto set_job_queue(std::shared_ptr<job_queue> job_queue) -> void;

		/**
		 * @brief Sets the thread context for this worker.
		 * @param context The thread context providing access to logging and monitoring services.
		 */
		auto set_context(const thread_context& context) -> void;

        /**
         * @brief Provide shared metrics storage for this worker.
         */
        void set_metrics(std::shared_ptr<metrics::ThreadPoolMetrics> metrics);

		/**
		 * @brief Set the diagnostics instance for event tracing.
		 * @param diag Pointer to the diagnostics instance.
		 *
		 * When set, the worker will record execution events to the diagnostics
		 * instance if tracing is enabled. If nullptr, no events are recorded.
		 */
		void set_diagnostics(diagnostics::thread_pool_diagnostics* diag);

		/**
		 * @brief Set the worker policy for this worker.
		 * @param policy The worker policy configuration.
		 */
		void set_policy(const worker_policy& policy);

		/**
		 * @brief Get the current worker policy.
		 * @return The worker policy configuration.
		 */
		[[nodiscard]] const worker_policy& get_policy() const;

		/**
		 * @brief Get the local work-stealing deque for this worker.
		 * @return Pointer to the local deque (nullptr if work-stealing disabled).
		 *
		 * This deque is used for work-stealing: other workers can steal jobs
		 * from this worker's local deque when they are idle.
		 */
		[[nodiscard]] lockfree::work_stealing_deque<job*>* get_local_deque() noexcept;

		/**
		 * @brief Set the steal function for finding other workers' deques.
		 * @param steal_fn Function that returns a job to steal, or nullptr.
		 *
		 * The steal function is called when this worker's local deque and the
		 * global queue are both empty. It should try to steal work from other
		 * workers.
		 */
		void set_steal_function(std::function<job*(std::size_t)> steal_fn);

		/**
		 * @brief Get the worker ID.
		 * @return The unique ID for this worker instance.
		 */
		[[nodiscard]] std::size_t get_worker_id() const;

		/**
		 * @brief Gets the thread context for this worker.
		 * @return The thread context providing access to logging and monitoring services.
		 */
		[[nodiscard]] auto get_context(void) const -> const thread_context&;

		/**
		 * @brief Checks if the worker is currently idle (not processing a job).
		 * @return @c true if the worker is idle (waiting for jobs), @c false if actively processing.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 * - Uses atomic operations for lock-free access
		 * - Provides snapshot of current state (may change immediately after return)
		 *
		 * Use Cases:
		 * - Thread pool statistics and monitoring
		 * - Load balancing decisions
		 * - Performance analysis
		 */
		[[nodiscard]] bool is_idle() const noexcept;

		/**
		 * @brief Gets the total number of jobs successfully completed by this worker.
		 * @return Count of successfully completed jobs.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 * - Uses atomic load with relaxed memory ordering
		 */
		[[nodiscard]] std::uint64_t get_jobs_completed() const noexcept;

		/**
		 * @brief Gets the total number of jobs that failed during execution.
		 * @return Count of failed jobs.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 * - Uses atomic load with relaxed memory ordering
		 */
		[[nodiscard]] std::uint64_t get_jobs_failed() const noexcept;

		/**
		 * @brief Gets the total time spent executing jobs (busy time).
		 * @return Duration of busy time in nanoseconds.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 * - Uses atomic load with relaxed memory ordering
		 */
		[[nodiscard]] std::chrono::nanoseconds get_total_busy_time() const noexcept;

		/**
		 * @brief Gets the total time spent waiting for jobs (idle time).
		 * @return Duration of idle time in nanoseconds.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 * - Uses atomic load with relaxed memory ordering
		 */
		[[nodiscard]] std::chrono::nanoseconds get_total_idle_time() const noexcept;

		/**
		 * @brief Gets the time when the worker entered its current state.
		 * @return Time point when current state was entered.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 * - Uses atomic load with acquire memory ordering
		 */
		[[nodiscard]] std::chrono::steady_clock::time_point get_state_since() const noexcept;

		/**
		 * @brief Gets information about the currently executing job.
		 * @return Optional job_info if a job is currently executing, std::nullopt otherwise.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 * - Provides snapshot of current state
		 */
		[[nodiscard]] std::optional<diagnostics::job_info> get_current_job_info() const noexcept;

	protected:
		/**
		 * @brief Determines if there are jobs available in the queue to continue working on.
		 * @return @c true if there is work in the queue, @c false otherwise.
		 *
		 * Called in the thread's main loop (defined by @c thread_base) to decide if
		 * @c do_work() should be invoked. Returns @c true if the job queue is not empty;
		 * otherwise, @c false.
		 */
		[[nodiscard]] auto should_continue_work() const -> bool override;

		/**
		 * @brief Processes one or more jobs from the queue.
		 * @return @c common::VoidResult containing an error if the work fails, or success value otherwise.
		 *
		 * This method fetches a job from the queue (if available), executes it, and
		 * may repeat depending on the implementation. If any job fails, an error
		 * is returned. Otherwise, return a success value.
		 */
		auto do_work() -> common::VoidResult override;

		/**
		 * @brief Called when the worker is requested to stop.
		 *
		 * Overrides the base class hook to propagate cancellation to the currently
		 * executing job (if any). This allows jobs to cooperatively cancel when the
		 * worker thread is stopped.
		 *
		 * Thread Safety:
		 * - Called from thread requesting stop (not worker thread)
		 * - Safe concurrent access with do_work() via atomic operations
		 */
		auto on_stop_requested() -> void override;

	private:
		/**
		 * @brief Static counter for generating unique worker IDs.
		 */
		static std::atomic<std::size_t> next_worker_id_;

		/**
		 * @brief Unique ID for this worker instance.
		 */
		std::size_t worker_id_{0};

		/**
		 * @brief Indicates whether to use time tags or timestamps for job processing.
		 *
		 * When @c true, the worker might record timestamps (e.g., job start/end times)
		 * or log them for debugging/monitoring. The exact usage depends on the
		 * job and override details in derived classes.
		 */
		bool use_time_tag_;

		/**
		 * @brief A shared pointer to the job queue from which this worker obtains jobs.
		 *
		 * Multiple workers can share the same queue, enabling concurrent processing
		 * of queued jobs.
		 */
		std::shared_ptr<job_queue> job_queue_;

		/**
		 * @brief The thread context providing access to logging and monitoring services.
		 *
		 * This context enables the worker to log messages and report metrics
		 * through the configured services.
		 */
		thread_context context_;

        /**
         * @brief Shared metrics aggregator provided by the owning thread pool.
         */
        std::shared_ptr<metrics::ThreadPoolMetrics> metrics_;

		/**
		 * @brief Pointer to the diagnostics instance for event tracing.
		 *
		 * When set, the worker records execution events if tracing is enabled.
		 * Raw pointer because the diagnostics outlives the worker.
		 */
		diagnostics::thread_pool_diagnostics* diagnostics_{nullptr};

		/**
		 * @brief Cancellation token for this worker.
		 *
		 * This token is propagated to jobs during execution, allowing them to
		 * cooperatively cancel when the worker is stopped. The token is cancelled
		 * in on_stop_requested().
		 */
		cancellation_token worker_cancellation_token_;

		/**
		 * @brief Pointer to the currently executing job.
		 *
		 * This is set atomically at the start of job execution and cleared when
		 * the job completes. Used by on_stop_requested() to cancel the running job.
		 *
		 * Memory Ordering:
		 * - Release when storing (ensures job state visible to cancellation thread)
		 * - Acquire when loading (ensures we see the correct job state)
		 *
		 * @note Raw pointer used because we don't own the job (it's owned by unique_ptr
		 *       in do_work()). Safe because job lifetime is guaranteed during execution.
		 */
		std::atomic<job*> current_job_{nullptr};

		/**
		 * @brief Indicates whether the worker is currently idle (not processing a job).
		 *
		 * This flag is set to true when the worker is waiting for jobs and false
		 * when actively processing a job. Updated atomically for thread-safe access.
		 *
		 * Memory Ordering:
		 * - Relaxed ordering sufficient (no synchronization dependencies)
		 * - Value is advisory only (race conditions between check and state change are acceptable)
		 *
		 * @note Used by thread pool for statistics and monitoring purposes.
		 */
		std::atomic<bool> is_idle_{true};

		/**
		 * @brief Total number of jobs successfully completed by this worker.
		 *
		 * Incremented atomically after each successful job execution.
		 */
		std::atomic<std::uint64_t> jobs_completed_{0};

		/**
		 * @brief Total number of jobs that failed during execution.
		 *
		 * Incremented atomically when a job's do_work() returns an error.
		 */
		std::atomic<std::uint64_t> jobs_failed_{0};

		/**
		 * @brief Total time spent executing jobs (busy time) in nanoseconds.
		 *
		 * Accumulated after each job execution with the job's execution duration.
		 */
		std::atomic<std::uint64_t> total_busy_time_ns_{0};

		/**
		 * @brief Total time spent waiting for jobs (idle time) in nanoseconds.
		 *
		 * Accumulated when transitioning from idle to busy state.
		 */
		std::atomic<std::uint64_t> total_idle_time_ns_{0};

		/**
		 * @brief Time point when the worker entered its current state.
		 *
		 * Updated when transitioning between idle and busy states.
		 * Used to calculate current state duration.
		 */
		std::atomic<std::chrono::steady_clock::time_point::rep> state_since_rep_{
			std::chrono::steady_clock::now().time_since_epoch().count()
		};

		/**
		 * @brief Time point when the current job started executing.
		 *
		 * Used to track job execution time for diagnostics.
		 * Only valid when a job is currently executing.
		 */
		std::chrono::steady_clock::time_point current_job_start_time_;

		/**
		 * @brief Mutex protecting job queue replacement.
		 *
		 * This mutex synchronizes access to job_queue_ during replacement operations
		 * to prevent race conditions between do_work(), set_job_queue(), and should_continue_work().
		 *
		 * @note Marked mutable to allow locking in const methods like should_continue_work().
		 * The const qualifier applies to the logical state, not the mutex itself.
		 */
		mutable std::mutex queue_mutex_;

		/**
		 * @brief Condition variable for queue replacement synchronization.
		 *
		 * Used to wait for current job completion before replacing the queue.
		 */
		std::condition_variable queue_cv_;

		/**
		 * @brief Indicates whether queue replacement is in progress.
		 *
		 * When true, the worker thread should wait before accessing the queue.
		 * Protected by queue_mutex_.
		 */
		bool queue_being_replaced_{false};

		/**
		 * @brief Worker policy configuration.
		 *
		 * Controls worker behavior including work-stealing settings.
		 */
		worker_policy policy_;

		/**
		 * @brief Local work-stealing deque for this worker.
		 *
		 * When work-stealing is enabled, jobs submitted to this worker
		 * are stored in this deque. The owner (this worker) can push/pop
		 * from the bottom (LIFO), while other workers can steal from the
		 * top (FIFO).
		 */
		std::unique_ptr<lockfree::work_stealing_deque<job*>> local_deque_;

		/**
		 * @brief Function to steal work from other workers.
		 *
		 * This function is provided by the thread pool and returns a stolen
		 * job from another worker's deque, or nullptr if no work available.
		 */
		std::function<job*(std::size_t)> steal_function_;

		/**
		 * @brief Counter for round-robin steal victim selection.
		 */
		std::size_t steal_victim_index_{0};

		/**
		 * @brief Try to get a job from local deque first, then global queue.
		 * @return A unique_ptr to the job, or nullptr if no work available.
		 */
		[[nodiscard]] std::unique_ptr<job> try_get_job();

		/**
		 * @brief Try to steal work from other workers.
		 * @return A unique_ptr to the stolen job, or nullptr if stealing failed.
		 */
		[[nodiscard]] std::unique_ptr<job> try_steal_work();
	};
} // namespace kcenon::thread

// ----------------------------------------------------------------------------
// Formatter specializations for thread_worker
// ----------------------------------------------------------------------------

/**
 * @brief Specialization of std::formatter for @c kcenon::thread::thread_worker.
 *
 * Allows @c thread_worker objects to be formatted as strings using C++20 std::format.
 *
 * ### Example
 * @code
 * auto worker = std::make_unique<kcenon::thread::thread_worker>();
 * std::string output = std::format("Worker status: {}", *worker);
 * @endcode
 */
template <>
struct std::formatter<kcenon::thread::thread_worker> : std::formatter<std::string_view>
{
	/**
	 * @brief Formats a @c thread_worker object as a string.
	 * @tparam FormatContext The type of the format context.
	 * @param item The @c thread_worker to format.
	 * @param ctx  The format context for output.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const kcenon::thread::thread_worker& item, FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character @c kcenon::thread::thread_worker.
 *
 * Enables wide-string formatting of @c thread_worker objects using C++20 std::format.
 */
template <>
struct std::formatter<kcenon::thread::thread_worker, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a @c thread_worker object as a wide string.
	 * @tparam FormatContext The type of the format context.
	 * @param item The @c thread_worker to format.
	 * @param ctx  The wide-character format context for output.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const kcenon::thread::thread_worker& item, FormatContext& ctx) const
	{
		auto str = item.to_string();
		auto wstr = utility_module::convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
