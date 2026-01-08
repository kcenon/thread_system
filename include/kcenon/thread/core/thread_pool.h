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

#include <kcenon/thread/utils/formatter.h>
#include <kcenon/thread/utils/formatter_macros.h>
#include <kcenon/thread/core/job_queue.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/core/cancellation_token.h>
#include <kcenon/thread/utils/convert_string.h>
#include <kcenon/thread/forward.h>
#include <kcenon/thread/interfaces/thread_context.h>
#include <kcenon/thread/metrics/thread_pool_metrics.h>
#include <kcenon/thread/metrics/enhanced_metrics.h>

// Include unified feature flags from common_system if available
#if __has_include(<kcenon/common/config/feature_flags.h>)
#include <kcenon/common/config/feature_flags.h>
#endif

// Common system unified interfaces
// KCENON_HAS_COMMON_EXECUTOR is defined by CMake when common_system is available
// Fallback to 0 if not defined (standalone build without common_system)
#ifndef KCENON_HAS_COMMON_EXECUTOR
#define KCENON_HAS_COMMON_EXECUTOR 0
#endif

#if KCENON_HAS_COMMON_EXECUTOR
#include <kcenon/common/interfaces/executor_interface.h>
#endif

#include "config.h"

#include <tuple>
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <optional>

// Forward declaration for diagnostics
namespace kcenon::thread::diagnostics {
	class thread_pool_diagnostics;
}

/**
 * @namespace kcenon::thread
 * @brief Thread pool implementation for managing worker threads.
 *
 * The thread_pool_module namespace provides a standard thread pool implementation
 * for processing jobs concurrently using a team of worker threads.
 *
 * Key components include:
 * - thread_pool: The primary thread pool class managing multiple workers and a shared job queue
 * - thread_worker: A specialized worker thread that processes jobs from a shared queue
 * - task: A template-based convenience wrapper for creating and submitting callable jobs
 *
 * The thread pool pattern improves performance by:
 * - Reusing threads rather than creating new ones for each task
 * - Reducing thread creation overhead
 * - Limiting the total number of threads to control resource usage
 * - Providing a simple interface for async task execution
 *
 * @see kcenon::thread for a more advanced implementation with job prioritization
 */
namespace kcenon::thread
{
	// Support both old (namespace common) and new (namespace kcenon::common) versions
	// When inside namespace kcenon::thread, 'common' resolves to kcenon::common
#if KCENON_HAS_COMMON_EXECUTOR
	namespace common_ns = common;
#endif

	/**
	 * @class thread_pool
	 * @brief A thread pool for concurrent execution of jobs using multiple worker threads.
	 *
	 * @ingroup thread_pools
	 *
	 * The @c thread_pool class manages a group of worker threads that process jobs from
	 * a shared @c job_queue. This implementation provides:
	 * - Efficient reuse of threads to reduce thread creation/destruction overhead
	 * - Controlled concurrency through a fixed or dynamic thread count
	 * - A simple interface for submitting jobs of various types
	 * - Graceful handling of thread startup, execution, and shutdown
	 *
	 * The thread pool is designed for scenarios where many short-lived tasks need to
	 * be executed asynchronously without creating a new thread for each task.
	 *
	 * ### Design Principles
	 * - **Worker Thread Model**: Each worker runs in its own thread, processing jobs
	 *   from the shared queue.
	 * - **Shared Job Queue**: A single, thread-safe queue holds all pending jobs.
	 * - **Job-Based Work Units**: Jobs encapsulate work to be executed.
	 * - **Non-Blocking Submission**: Adding jobs to the pool never blocks the caller thread.
	 * - **Cooperative Shutdown**: Workers can complete current jobs before stopping.
	 *
	 * ### Thread Safety
	 * All public methods of this class are thread-safe and can be called from any thread.
	 * The underlying @c job_queue is also thread-safe, allowing multiple workers to dequeue
	 * jobs concurrently.
	 *
	 * ### Performance Considerations
	 * - The number of worker threads should typically be close to the number of available
	 *   CPU cores for CPU-bound tasks.
	 * - For I/O-bound tasks, more threads may be beneficial to maximize throughput while
	 *   some threads are blocked on I/O.
	 * - Very large thread pools (significantly more threads than cores) may degrade
	 *   performance due to context switching overhead.
	 *
	 * @see thread_worker The worker thread class used by the pool
	 * @see job_queue The shared queue for storing pending jobs
	 * @see typed_kcenon::thread::typed_thread_pool For a priority-based version
	 */
	class thread_pool : public std::enable_shared_from_this<thread_pool>
#if KCENON_HAS_COMMON_EXECUTOR
	                   , public common_ns::interfaces::IExecutor
#endif
	{
	public:
		/**
		 * @brief Constructs a new @c thread_pool instance.
		 * @param thread_title An optional title or identifier for the thread pool (defaults to
		 * "thread_pool").
		 * @param context Optional thread context for logging and monitoring (defaults to empty context).
		 *
		 * This title can be used for logging or debugging purposes.
		 * The context provides access to logging and monitoring services.
		 */
		thread_pool(const std::string& thread_title = "thread_pool",
		           const thread_context& context = thread_context());

		/**
		 * @brief Constructs a new @c thread_pool instance with a custom job queue.
		 * @param thread_title A title or identifier for the thread pool.
		 * @param custom_queue A custom job queue implementation (e.g., backpressure_job_queue).
		 * @param context Optional thread context for logging and monitoring (defaults to empty context).
		 *
		 * This constructor allows injecting a specialized job queue such as
		 * @c backpressure_job_queue for rate limiting and flow control.
		 */
		thread_pool(const std::string& thread_title,
		           std::shared_ptr<job_queue> custom_queue,
		           const thread_context& context = thread_context());

		/**
		 * @brief Virtual destructor. Cleans up resources used by the thread pool.
		 *
		 * If the pool is still running, this typically calls @c stop() internally
		 * to ensure all worker threads are properly shut down.
		 */
		virtual ~thread_pool(void);

		/**
		 * @brief Retrieves a @c std::shared_ptr to this @c thread_pool instance.
		 * @return A shared pointer to the current @c thread_pool object.
		 *
		 * By inheriting from @c std::enable_shared_from_this, you can call @c get_ptr()
		 * within member functions to avoid storing a separate shared pointer.
		 */
		[[nodiscard]] auto get_ptr(void) -> std::shared_ptr<thread_pool>;

#if KCENON_HAS_COMMON_EXECUTOR
	// ============================================================================
	// IExecutor interface implementation (common_system)
	// ============================================================================

	/**
	 * @brief Submit a task for immediate execution (IExecutor)
	 * @param task The function to execute
	 * @return Future representing the task result
	 */
	std::future<void> submit(std::function<void()> task);

	/**
	 * @brief Submit a task for delayed execution (IExecutor)
	 * @param task The function to execute
	 * @param delay The delay before execution
	 * @return Future representing the task result
	 */
	std::future<void> submit_delayed(
		std::function<void()> task,
		std::chrono::milliseconds delay);

	/**
	 * @brief Execute a job with Result-based error handling (IExecutor)
	 * @param job The job to execute
	 * @return Result containing future or error
	 */
	common_ns::Result<std::future<void>> execute(
		std::unique_ptr<common_ns::interfaces::IJob>&& job) override;

	/**
	 * @brief Execute a job with delay (IExecutor)
	 * @param job The job to execute
	 * @param delay The delay before execution
	 * @return Result containing future or error
	 */
	common_ns::Result<std::future<void>> execute_delayed(
		std::unique_ptr<common_ns::interfaces::IJob>&& job,
		std::chrono::milliseconds delay) override;

	/**
	 * @brief Get the number of worker threads (IExecutor)
	 * @return Number of available workers
	 */
	size_t worker_count() const override;

	/**
	 * @brief Get the number of pending tasks (IExecutor)
	 * @return Number of tasks waiting to be executed
	 */
	size_t pending_tasks() const override;

	/**
	 * @brief Shutdown the executor gracefully (IExecutor)
	 * @param wait_for_completion Wait for all pending tasks to complete
	 */
	void shutdown(bool wait_for_completion) override;
#endif // KCENON_HAS_COMMON_EXECUTOR

        /**
         * @brief Starts the thread pool and all associated workers.
         * @return @c common::VoidResult containing an error on failure, or success value on success.
         *
         * If the pool is already running, a subsequent call to @c start() may return an error.
         * On success, each @c thread_worker in @c workers_ is started, enabling them to process
         * jobs from the @c job_queue_.
         */
        auto start(void) -> common::VoidResult;

		/**
		 * @brief Returns the shared @c job_queue used by this thread pool.
		 * @return A @c std::shared_ptr<job_queue> that stores the queued jobs.
		 *
		 * The returned queue is shared among all worker threads in the pool, which
		 * can concurrently dequeue and process jobs.
		 */
		[[nodiscard]] auto get_job_queue(void) -> std::shared_ptr<job_queue>;

        /**
         * @brief Access aggregated runtime metrics (read-only reference).
         */
        [[nodiscard]] const metrics::ThreadPoolMetrics& metrics() const noexcept;

        /**
         * @brief Reset accumulated metrics.
         */
        void reset_metrics();

        /**
         * @brief Enable or disable enhanced metrics collection.
         * @param enabled True to enable enhanced metrics (histograms, percentiles).
         *
         * When enabled, additional metrics like latency histograms and throughput
         * counters are collected. This has minimal overhead (< 100ns per operation)
         * but can be disabled for maximum performance.
         */
        void set_enhanced_metrics_enabled(bool enabled);

        /**
         * @brief Check if enhanced metrics is enabled.
         * @return True if enhanced metrics collection is enabled.
         */
        [[nodiscard]] bool is_enhanced_metrics_enabled() const;

        /**
         * @brief Access enhanced metrics (read-only reference).
         * @return Reference to enhanced metrics with histograms and percentiles.
         * @throw std::runtime_error if enhanced metrics is not enabled.
         */
        [[nodiscard]] const metrics::EnhancedThreadPoolMetrics& enhanced_metrics() const;

        /**
         * @brief Get enhanced metrics snapshot.
         * @return EnhancedSnapshot with all current metric values.
         *
         * Returns empty snapshot if enhanced metrics is not enabled.
         */
        [[nodiscard]] metrics::EnhancedSnapshot enhanced_metrics_snapshot() const;

        /**
         * @brief Enqueues a new job into the shared @c job_queue.
         * @param job A @c std::unique_ptr<job> representing the work to be done.
         * @return @c common::VoidResult containing an error on failure, or success value on success.
         */
        auto enqueue(std::unique_ptr<job>&& job) -> common::VoidResult;

        /**
         * @brief Enqueues a batch of jobs into the shared @c job_queue.
         * @param jobs A vector of @c std::unique_ptr<job> objects to be added.
         * @return @c common::VoidResult containing an error on failure, or success value on success.
         */
        auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> common::VoidResult;

        /**
         * @brief Adds a @c thread_worker to the thread pool for specialized or additional processing.
         * @param worker A @c std::unique_ptr<thread_worker> object.
         * @return @c common::VoidResult containing an error on failure, or success value on success.
         */
        auto enqueue(std::unique_ptr<thread_worker>&& worker) -> common::VoidResult;

        /**
         * @brief Adds a batch of @c thread_worker objects to the thread pool.
         * @param workers A vector of @c std::unique_ptr<thread_worker> objects.
         * @return @c common::VoidResult containing an error on failure, or success value on success.
         */
        auto enqueue_batch(std::vector<std::unique_ptr<thread_worker>>&& workers)
            -> common::VoidResult;

        /**
         * @brief Stops the thread pool and all worker threads.
         * @param immediately_stop If @c true, any ongoing jobs may be interrupted; if @c false
         *        (default), each worker attempts to finish its current job before stopping.
         * @return @c common::VoidResult containing an error on failure, or success value on success.
         */
        auto stop(const bool& immediately_stop = false) -> common::VoidResult;

		/**
		 * @brief Provides a string representation of this @c thread_pool.
		 * @return A string describing the pool, including its title and other optional details.
		 *
		 * Derived classes may override this to include more diagnostic or state-related info.
		 */
		[[nodiscard]] auto to_string(void) const -> std::string;

		/**
		 * @brief Get the pool instance id.
		 * @return Returns the unique instance id for this pool.
		 */
		[[nodiscard]] std::uint32_t get_pool_instance_id() const;

		/**
		 * @brief Collect and report current thread pool metrics.
		 * 
		 * This method gathers current metrics from the pool and reports them
		 * through the monitoring interface if available.
		 */
		void report_metrics();

		/**
		 * @brief Get the number of idle workers.
		 * @return Number of workers currently not processing jobs.
		 */
		[[nodiscard]] std::size_t get_idle_worker_count() const;

		/**
		 * @brief Gets the thread context for this pool.
		 * @return The thread context providing access to logging and monitoring services.
		 */
		[[nodiscard]] auto get_context(void) const -> const thread_context&;

		// ============================================================================
		// Simplified Public API (bool return type for convenience)
		// ============================================================================
		// These methods provide a simplified interface with bool return types
		// for easier integration with code that doesn't use result<T> types.
		// For detailed error information, prefer using the common::VoidResult methods above.

		/**
		 * @brief Submit a task to the thread pool (simplified API)
		 * @param task The task to be executed
		 * @return true if task was successfully submitted, false otherwise
		 * @note For detailed error information, use enqueue() instead
		 */
		auto submit_task(std::function<void()> task) -> bool;

		/**
		 * @brief Get the number of worker threads in the pool
		 * @return Number of active worker threads
		 */
		auto get_thread_count() const -> std::size_t;

		/**
		 * @brief Shutdown the thread pool (simplified API)
		 * @param immediate If true, stop immediately; if false, wait for current tasks to complete
		 * @return true if shutdown was successful, false otherwise
		 * @note For detailed error information, use stop() instead
		 */
		auto shutdown_pool(bool immediate = false) -> bool;

		/**
		 * @brief Check if the thread pool is currently running (IExecutor)
		 * @return true if the pool is active, false otherwise
		 */
#if KCENON_HAS_COMMON_EXECUTOR
		auto is_running() const -> bool override;
#else
		auto is_running() const -> bool;
#endif

		/**
		 * @brief Get the number of pending tasks in the queue
		 * @return Number of tasks waiting to be processed
		 */
		auto get_pending_task_count() const -> std::size_t;

		/**
		 * @brief Check health of all worker threads and restart failed workers
		 *
		 * This method performs health monitoring on all worker threads:
		 * - Detects workers that have stopped unexpectedly (consecutive failures)
		 * - Removes dead workers from the pool
		 * - Optionally restarts failed workers to maintain pool capacity
		 *
		 * Use Cases:
		 * - Periodic health checks (e.g., from a watchdog thread)
		 * - Recovery from worker failures in long-running processes
		 * - Maintaining consistent thread pool capacity
		 *
		 * Thread Safety:
		 * - Thread-safe: can be called from any thread
		 * - Acquires workers_mutex_ for safe access to workers vector
		 *
		 * @param restart_failed If true, creates new workers to replace failed ones
		 * @return Number of workers that were detected as failed/unhealthy
		 *
		 * Example:
		 * @code
		 * // Periodic health check with auto-restart
		 * auto failed_count = pool.check_worker_health(true);
		 * if (failed_count > 0) {
		 *     LOG_WARNING("Restarted {} failed workers", failed_count);
		 * }
		 * @endcode
		 */
		auto check_worker_health(bool restart_failed = true) -> std::size_t;

		/**
		 * @brief Get the current number of active (running) workers
		 * @return Number of workers currently in running state
		 *
		 * This differs from the total worker count as it only counts
		 * workers that are actively running, not stopped or stopping.
		 */
		auto get_active_worker_count() const -> std::size_t;

		/**
		 * @brief Set the worker policy for all workers in the pool.
		 * @param policy The worker policy configuration.
		 *
		 * This should be called before start() to configure work-stealing
		 * and other worker behaviors. If called after start(), only affects
		 * newly added workers.
		 */
		void set_worker_policy(const worker_policy& policy);

		/**
		 * @brief Get the current worker policy.
		 * @return The worker policy configuration.
		 */
		[[nodiscard]] const worker_policy& get_worker_policy() const;

		/**
		 * @brief Enable or disable work-stealing at runtime.
		 * @param enable Whether to enable work-stealing.
		 *
		 * This method allows toggling work-stealing behavior after pool creation.
		 * Changes take effect for subsequent job executions.
		 */
		void enable_work_stealing(bool enable);

		/**
		 * @brief Check if work-stealing is currently enabled.
		 * @return true if work-stealing is enabled, false otherwise.
		 */
		[[nodiscard]] bool is_work_stealing_enabled() const;

		// =========================================================================
		// Diagnostics
		// =========================================================================

		/**
		 * @brief Get the diagnostics interface for this pool.
		 * @return Reference to the diagnostics object.
		 *
		 * The diagnostics interface provides:
		 * - Thread dump capabilities
		 * - Job inspection
		 * - Bottleneck detection
		 * - Health check integration
		 * - Event tracing
		 */
		[[nodiscard]] auto diagnostics() -> diagnostics::thread_pool_diagnostics&;

		/**
		 * @brief Get the diagnostics interface for this pool (const version).
		 * @return Const reference to the diagnostics object.
		 */
		[[nodiscard]] auto diagnostics() const -> const diagnostics::thread_pool_diagnostics&;

	private:
		/**
		 * @brief Static counter for generating unique pool instance IDs.
		 */
		static std::atomic<std::uint32_t> next_pool_instance_id_;

		/**
		 * @brief A title or name for this thread pool, useful for identification and logging.
		 */
		std::string thread_title_;

		/**
		 * @brief Unique instance ID for this pool (for multi-pool scenarios).
		 */
		std::uint32_t pool_instance_id_{0};

		/**
		 * @brief Indicates whether the pool is currently running.
		 *
		 * Set to @c true after a successful call to @c start(), and reset to @c false after @c
		 * stop(). Used internally to prevent multiple active starts or erroneous state transitions.
		 */
		std::atomic<bool> start_pool_;

		/**
		 * @brief The shared job queue where jobs (@c job objects) are enqueued.
		 *
		 * Worker threads dequeue jobs from this queue to perform tasks. The queue persists
		 * for the lifetime of the pool or until no more references exist.
		 */
		std::shared_ptr<job_queue> job_queue_;

		/**
		 * @brief A collection of worker threads associated with this pool.
		 *
		 * Each @c thread_worker typically runs in its own thread context, processing jobs
		 * from @c job_queue_ or performing specialized logic. They are started together
		 * when @c thread_pool::start() is called.
		 *
		 * Thread Safety:
		 * - Protected by workers_mutex_ to prevent concurrent modification
		 * - Accessed during enqueue_worker() and stop() operations
		 */
		std::vector<std::unique_ptr<thread_worker>> workers_;

		/**
		 * @brief Mutex protecting concurrent access to the workers_ vector.
		 *
		 * Synchronization Strategy:
		 * - Guards all workers_ modifications (enqueue, stop)
		 * - Prevents iterator invalidation during concurrent operations
		 * - Uses scoped_lock for RAII-based protection
		 * - Held briefly to minimize contention
		 */
		mutable std::mutex workers_mutex_;

		/**
		 * @brief The thread context providing access to logging and monitoring services.
		 *
		 * This context is shared with all worker threads created by this pool,
		 * enabling consistent logging and monitoring throughout the pool.
		 */
		thread_context context_;

		/**
		 * @brief Pool-level cancellation token.
		 *
		 * This token is used to propagate cancellation to all workers and jobs
		 * when the pool is stopped. Each worker receives a linked token that
		 * combines this pool token with its own worker token, creating a
		 * hierarchical cancellation structure.
		 *
		 * Cancellation Hierarchy:
		 * - Pool stop() → cancels pool_cancellation_token_
		 * - Pool token cancellation → propagates to all linked worker tokens
		 * - Worker token cancellation → propagates to running jobs
		 *
		 * @note This token is reset when the pool is restarted to allow
		 *       fresh job execution without stale cancellation state.
		 */
		cancellation_token pool_cancellation_token_;

		/**
		 * @brief Stops the thread pool without logging (for use during static destruction).
		 *
		 * This method performs the same shutdown sequence as stop() but without
		 * any logging operations. It is specifically designed to be called from
		 * the destructor when static destruction is in progress, preventing
		 * Static Destruction Order Fiasco (SDOF) by avoiding access to potentially
		 * destroyed logger/monitoring singletons.
		 *
		 * @return @c common::VoidResult containing an error on failure, or success value on success.
		 *
		 * @note This method should only be called from the destructor during static destruction.
		 */
		auto stop_unsafe() noexcept -> common::VoidResult;

        /**
         * @brief Shared metrics collector used by workers.
         */
        std::shared_ptr<metrics::ThreadPoolMetrics> metrics_;

        /**
         * @brief Enhanced metrics collector for histograms and percentiles.
         *
         * Provides production-grade observability including latency histograms,
         * percentile calculations, and sliding window throughput tracking.
         * Lazily initialized when set_enhanced_metrics_enabled(true) is called.
         */
        std::shared_ptr<metrics::EnhancedThreadPoolMetrics> enhanced_metrics_;

        /**
         * @brief Flag indicating if enhanced metrics collection is enabled.
         */
        std::atomic<bool> enhanced_metrics_enabled_{false};

		/**
		 * @brief Worker policy configuration for this pool.
		 *
		 * Defines behavior for all workers including work-stealing settings.
		 */
		worker_policy worker_policy_;

		/**
		 * @brief Diagnostics interface for this pool.
		 *
		 * Lazily initialized on first access to diagnostics().
		 */
		mutable std::unique_ptr<diagnostics::thread_pool_diagnostics> diagnostics_;

		/**
		 * @brief Create a steal function for the given worker.
		 * @param requester_id ID of the worker requesting to steal.
		 * @return Function that attempts to steal work from other workers.
		 *
		 * The returned function implements the steal policy (random, round-robin,
		 * or adaptive) and returns a raw pointer to a stolen job.
		 */
		[[nodiscard]] std::function<job*(std::size_t)> create_steal_function();

		/**
		 * @brief Try to steal a job from another worker.
		 * @param requester_id ID of the worker requesting to steal.
		 * @return Raw pointer to stolen job, or nullptr if no work available.
		 */
		[[nodiscard]] job* steal_from_workers(std::size_t requester_id);
	};
} // namespace kcenon::thread

// ----------------------------------------------------------------------------
// Formatter specializations for thread_pool
// ----------------------------------------------------------------------------

/**
 * @brief Specialization of std::formatter for @c kcenon::thread::thread_pool.
 *
 * Enables formatting of @c thread_pool objects as strings using C++20 std::format.
 *
 * ### Example
 * @code
 * auto pool = std::make_shared<kcenon::thread::thread_pool>("MyPool");
 * std::string output = std::format("Pool Info: {}", *pool); // e.g. "Pool Info: [thread_pool: MyPool]"
 * @endcode
 */
template <>
struct std::formatter<kcenon::thread::thread_pool> : std::formatter<std::string_view>
{
	/**
	 * @brief Formats a @c thread_pool object as a string.
	 * @tparam FormatContext The type of the format context.
	 * @param item The @c thread_pool to format.
	 * @param ctx  The format context for the output.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const kcenon::thread::thread_pool& item, FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character @c kcenon::thread::thread_pool.
 *
 * Allows wide-string formatting of @c thread_pool objects using C++20 std::format.
 */
template <>
struct std::formatter<kcenon::thread::thread_pool, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a @c thread_pool object as a wide string.
	 * @tparam FormatContext The type of the format context.
	 * @param item The @c thread_pool to format.
	 * @param ctx  The wide-character format context.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const kcenon::thread::thread_pool& item, FormatContext& ctx) const
	{
		auto str = item.to_string();
		auto wstr = utility_module::convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
