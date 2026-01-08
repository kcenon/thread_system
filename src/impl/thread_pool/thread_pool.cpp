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

#include <kcenon/thread/core/job_queue.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_logger.h>
#include <kcenon/thread/utils/formatter.h>
#include <kcenon/thread/diagnostics/thread_pool_diagnostics.h>
#include <kcenon/thread/resilience/circuit_breaker.h>
#include <kcenon/thread/resilience/protected_job.h>
#include <kcenon/thread/scaling/autoscaler.h>

#include <random>

using namespace utility_module;

/**
 * @file thread_pool.cpp
 * @brief Implementation of the thread pool class for managing multiple worker threads.
 *
 * This file contains the implementation of the thread_pool class, which coordinates
 * multiple worker threads processing jobs from a shared queue. The pool supports
 * adaptive queue strategies for optimal performance under varying load conditions.
 */

namespace kcenon::thread {
// Support both old (namespace common) and new (namespace kcenon::common) versions
// When inside namespace kcenon::thread, 'common' resolves to kcenon::common
#if KCENON_HAS_COMMON_EXECUTOR
namespace common_ns = common;
#endif

// Initialize static member
std::atomic<std::uint32_t> thread_pool::next_pool_instance_id_{0};
/**
 * @brief Constructs a thread pool with adaptive job queue.
 *
 * Implementation details:
 * - Initializes with provided thread title for identification
 * - Creates adaptive job queue that automatically optimizes based on contention
 * - Pool starts in stopped state (start_pool_ = false)
 * - No workers are initially assigned (workers_ is empty)
 * - Stores thread context for logging and monitoring
 * - Creates pool-level cancellation token for hierarchical cancellation
 *
 * Adaptive Queue Strategy:
 * - ADAPTIVE mode automatically switches between mutex and lock-free implementations
 * - Provides optimal performance across different contention levels
 * - Eliminates need for manual queue strategy selection
 *
 * @param thread_title Descriptive name for this thread pool instance
 * @param context Thread context providing logging and monitoring services
 */
thread_pool::thread_pool(const std::string& thread_title, const thread_context& context)
    : thread_title_(thread_title)
    , pool_instance_id_(next_pool_instance_id_.fetch_add(1))
    , start_pool_(false)
    , job_queue_(std::make_shared<kcenon::thread::job_queue>())
    , context_(context)
    , pool_cancellation_token_(cancellation_token::create())
    , metrics_(std::make_shared<metrics::ThreadPoolMetrics>()) {
    // Report initial pool registration if monitoring is available
    if (context_.monitoring()) {
        common::interfaces::thread_pool_metrics initial_metrics(thread_title_, pool_instance_id_);
        initial_metrics.worker_threads.value = 0;
        context_.update_thread_pool_metrics(thread_title_, pool_instance_id_, initial_metrics);
    }
}

/**
 * @brief Constructs a thread pool with a custom job queue.
 *
 * Implementation details:
 * - Initializes with provided thread title for identification
 * - Uses the provided custom job queue (e.g., backpressure_job_queue)
 * - Pool starts in stopped state (start_pool_ = false)
 * - No workers are initially assigned (workers_ is empty)
 * - Stores thread context for logging and monitoring
 * - Creates pool-level cancellation token for hierarchical cancellation
 *
 * @param thread_title Descriptive name for this thread pool instance
 * @param custom_queue Custom job queue implementation
 * @param context Thread context providing logging and monitoring services
 */
thread_pool::thread_pool(const std::string& thread_title,
                         std::shared_ptr<job_queue> custom_queue,
                         const thread_context& context)
    : thread_title_(thread_title)
    , pool_instance_id_(next_pool_instance_id_.fetch_add(1))
    , start_pool_(false)
    , job_queue_(std::move(custom_queue))
    , context_(context)
    , pool_cancellation_token_(cancellation_token::create())
    , metrics_(std::make_shared<metrics::ThreadPoolMetrics>()) {
    // Report initial pool registration if monitoring is available
    if (context_.monitoring()) {
        common::interfaces::thread_pool_metrics initial_metrics(thread_title_, pool_instance_id_);
        initial_metrics.worker_threads.value = 0;
        context_.update_thread_pool_metrics(thread_title_, pool_instance_id_, initial_metrics);
    }
}

/**
 * @brief Destroys the thread pool, ensuring all workers are stopped.
 *
 * Implementation details:
 * - Checks if static destruction is in progress to avoid SDOF
 * - Uses stop_unsafe() during static destruction (no logging)
 * - Uses regular stop() during normal destruction
 * - Workers will complete current jobs before terminating
 * - Prevents resource leaks from running threads
 *
 * @see stop_unsafe() for the logging-free shutdown variant
 * @see thread_logger::is_shutting_down() for shutdown detection
 */
thread_pool::~thread_pool() {
    // Check if we're in static destruction phase
    // During static destruction, logger/monitoring singletons may already be destroyed
    if (thread_logger::is_shutting_down()) {
        // Minimal cleanup without logging to avoid SDOF
        stop_unsafe();
    } else {
        stop();
    }
}

/**
 * @brief Returns a shared pointer to this thread pool instance.
 *
 * Implementation details:
 * - Uses std::enable_shared_from_this for safe shared_ptr creation
 * - Required for passing pool reference to workers or other components
 * - Ensures proper lifetime management in multi-threaded environment
 *
 * @return Shared pointer to this thread_pool
 */
auto thread_pool::get_ptr(void) -> std::shared_ptr<thread_pool> {
    return this->shared_from_this();
}

/**
 * @brief Starts all worker threads in the pool.
 *
 * Implementation details:
 * - Validates that workers have been added to the pool
 * - Starts each worker thread individually
 * - If any worker fails to start, stops all workers and returns error
 * - Sets start_pool_ flag to true on successful startup
 * - Workers begin processing jobs from the shared queue immediately
 *
 * Startup Sequence:
 * 1. Check that workers exist
 * 2. Start each worker sequentially
 * 3. On failure: stop all workers and return error message
 * 4. On success: mark pool as started
 *
 * Error Handling:
 * - All-or-nothing startup (if one fails, all stop)
 * - Provides detailed error message from failed worker
 * - Ensures consistent pool state (either all running or all stopped)
 *
 * @return std::nullopt on success, error message on failure
 */
auto thread_pool::start(void) -> common::VoidResult {
    // Acquire lock to check workers_ safely
    std::scoped_lock<std::mutex> lock(workers_mutex_);

    // Check if pool is already running
    // Use acquire to ensure we see all previous modifications to pool state
    if (start_pool_.load(std::memory_order_acquire)) {
        return common::error_info{static_cast<int>(error_code::thread_already_running), "thread pool is already running", "thread_system"};
    }

    // Validate that workers have been added
    if (workers_.empty()) {
        return common::error_info{static_cast<int>(error_code::invalid_argument), "no workers to start", "thread_system"};
    }

    // Create fresh job queue for restart scenarios
    // Stopped queues cannot accept new jobs, so we must create a new instance
    if (job_queue_ == nullptr || job_queue_->is_stopped()) {
        job_queue_ = std::make_shared<kcenon::thread::job_queue>();

        // Update all workers with the new queue reference
        for (auto& worker : workers_) {
            worker->set_job_queue(job_queue_);
        }
    }

    // Create fresh pool cancellation token for restart scenarios
    // This ensures workers start with a non-cancelled token
    pool_cancellation_token_ = cancellation_token::create();
    metrics_->reset();

    // Attempt to start each worker
    for (auto& worker : workers_) {
        auto start_result = worker->start();
        if (start_result.is_err()) {
            // If any worker fails, stop all and return error
            stop();
            return start_result.error();
        }
    }

    // Mark pool as successfully started
    // Use release to ensure all previous modifications (worker starts, queue setup)
    // are visible to other threads before they see start_pool_ == true
    start_pool_.store(true, std::memory_order_release);

    return common::ok();
}

/**
 * @brief Returns the shared job queue used by all workers.
 *
 * Implementation details:
 * - Provides access to the adaptive job queue for external job submission
 * - Queue is shared among all workers for load balancing
 * - Adaptive queue automatically optimizes based on contention patterns
 *
 * @return Shared pointer to the job queue
 */
auto thread_pool::get_job_queue(void) -> std::shared_ptr<job_queue> {
    return job_queue_;
}

const metrics::ThreadPoolMetrics& thread_pool::metrics() const noexcept {
    return *metrics_;
}

void thread_pool::reset_metrics() {
    metrics_->reset();
    if (enhanced_metrics_) {
        enhanced_metrics_->reset();
    }
}

void thread_pool::set_enhanced_metrics_enabled(bool enabled) {
    if (enabled && !enhanced_metrics_) {
        // Lazily initialize enhanced metrics with current worker count
        std::scoped_lock<std::mutex> lock(workers_mutex_);
        enhanced_metrics_ = std::make_shared<metrics::EnhancedThreadPoolMetrics>(workers_.size());
        enhanced_metrics_->set_active_workers(workers_.size());
    }
    enhanced_metrics_enabled_.store(enabled, std::memory_order_release);
}

bool thread_pool::is_enhanced_metrics_enabled() const {
    return enhanced_metrics_enabled_.load(std::memory_order_acquire);
}

const metrics::EnhancedThreadPoolMetrics& thread_pool::enhanced_metrics() const {
    if (!enhanced_metrics_) {
        throw std::runtime_error("Enhanced metrics is not enabled. Call set_enhanced_metrics_enabled(true) first.");
    }
    return *enhanced_metrics_;
}

metrics::EnhancedSnapshot thread_pool::enhanced_metrics_snapshot() const {
    if (!enhanced_metrics_enabled_.load(std::memory_order_acquire) || !enhanced_metrics_) {
        return metrics::EnhancedSnapshot{};
    }
    return enhanced_metrics_->snapshot();
}

/**
 * @brief Adds a single job to the thread pool for processing.
 *
 * Implementation details:
 * - Validates job pointer before submission
 * - Validates queue availability
 * - Delegates to adaptive job queue for optimal scheduling
 * - Job will be processed by next available worker thread
 *
 * Queue Behavior:
 * - Adaptive queue automatically selects best strategy (mutex/lock-free)
 * - Jobs are processed in FIFO order within the selected strategy
 * - Workers are notified when jobs become available
 *
 * Thread Safety:
 * - Safe to call from multiple threads simultaneously
 * - Adaptive queue handles contention efficiently
 *
 * @param job Unique pointer to job (ownership transferred)
 * @return std::nullopt on success, error message on failure
 */
auto thread_pool::enqueue(std::unique_ptr<job>&& job) -> common::VoidResult {
    // Validate inputs
    if (job == nullptr) {
        return common::error_info{static_cast<int>(error_code::invalid_argument), "job is null", "thread_system"};
    }

    if (job_queue_ == nullptr) {
        return common::error_info{static_cast<int>(error_code::resource_allocation_failed), "job queue is null", "thread_system"};
    }

    // Check if queue has been explicitly stopped (via stop())
    // This prevents race conditions during shutdown where stop() has been called
    // but jobs might still be submitted. Note: We check the queue's stopped state
    // rather than start_pool_ to allow jobs to be enqueued before start() is called.
    if (job_queue_->is_stopped()) {
        return common::error_info{static_cast<int>(error_code::queue_stopped), "thread pool is stopped", "thread_system"};
    }

    // Delegate to adaptive queue for optimal processing
    metrics_->record_submission();

    // Record enhanced metrics if enabled
    auto start_time = std::chrono::steady_clock::now();
    auto enqueue_result = job_queue_->enqueue(std::move(job));
    if (enqueue_result.is_err()) {
        return enqueue_result.error();
    }

    metrics_->record_enqueue();

    // Record enhanced metrics
    if (enhanced_metrics_enabled_.load(std::memory_order_relaxed) && enhanced_metrics_) {
        auto end_time = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        enhanced_metrics_->record_submission();
        enhanced_metrics_->record_enqueue(latency);
        enhanced_metrics_->record_queue_depth(job_queue_->size());
    }

    return common::ok();
}

auto thread_pool::enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> common::VoidResult {
    if (jobs.empty()) {
        return common::error_info{static_cast<int>(error_code::invalid_argument), "jobs are empty", "thread_system"};
    }

    if (job_queue_ == nullptr) {
        return common::error_info{static_cast<int>(error_code::resource_allocation_failed), "job queue is null", "thread_system"};
    }

    // Check if queue has been explicitly stopped
    if (job_queue_->is_stopped()) {
        return common::error_info{static_cast<int>(error_code::queue_stopped), "thread pool is stopped", "thread_system"};
    }

    const auto batch_size = jobs.size();
    metrics_->record_submission(batch_size);

    // Record enhanced metrics if enabled
    auto start_time = std::chrono::steady_clock::now();
    auto enqueue_result = job_queue_->enqueue_batch(std::move(jobs));
    if (enqueue_result.is_err()) {
        return enqueue_result.error();
    }

    metrics_->record_enqueue(batch_size);

    // Record enhanced metrics for batch
    if (enhanced_metrics_enabled_.load(std::memory_order_relaxed) && enhanced_metrics_) {
        auto end_time = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        // Record submission for each job in batch
        for (std::size_t i = 0; i < batch_size; ++i) {
            enhanced_metrics_->record_submission();
            enhanced_metrics_->record_enqueue(latency / static_cast<long>(batch_size));
        }
        enhanced_metrics_->record_queue_depth(job_queue_->size());
    }

    return common::ok();
}

auto thread_pool::enqueue(std::unique_ptr<thread_worker>&& worker) -> common::VoidResult {
    if (worker == nullptr) {
        return common::error_info{static_cast<int>(error_code::invalid_argument), "worker is null", "thread_system"};
    }

    if (job_queue_ == nullptr) {
        return common::error_info{static_cast<int>(error_code::resource_allocation_failed), "job queue is null", "thread_system"};
    }

    worker->set_job_queue(job_queue_);
    worker->set_context(context_);
    worker->set_metrics(metrics_);

    // Acquire lock before checking start_pool_ and adding worker
    // This prevents race condition with stop():
    // - stop() acquires workers_mutex_ after atomically setting start_pool_ to false
    // - If we check start_pool_ while holding the lock, we ensure consistent state
    std::scoped_lock<std::mutex> lock(workers_mutex_);

    // Use memory_order_acquire to ensure we see all previous modifications
    // made by the thread that set start_pool_ to true (in start())
    bool is_running = start_pool_.load(std::memory_order_acquire);

    // Add worker to vector first, before starting
    // This ensures stop() will see and stop this worker if called concurrently
    workers_.emplace_back(std::move(worker));

    // Only start the worker if pool is running
    // Since we hold workers_mutex_, stop() cannot proceed until we release it
    if (is_running) {
        auto start_result = workers_.back()->start();
        if (start_result.is_err()) {
            // Remove the worker we just added since it failed to start
            workers_.pop_back();
            return start_result.error();
        }
    }

    return common::ok();
}

auto thread_pool::enqueue_batch(std::vector<std::unique_ptr<thread_worker>>&& workers)
    -> common::VoidResult {
    if (workers.empty()) {
        return common::error_info{static_cast<int>(error_code::invalid_argument), "workers are empty", "thread_system"};
    }

    if (job_queue_ == nullptr) {
        return common::error_info{static_cast<int>(error_code::resource_allocation_failed), "job queue is null", "thread_system"};
    }

    // Acquire lock before processing workers
    // This ensures atomic check-and-add operation with respect to stop()
    std::scoped_lock<std::mutex> lock(workers_mutex_);

    // Check pool running state once with acquire semantics
    bool is_running = start_pool_.load(std::memory_order_acquire);

    // Track the starting index for rollback in case of error
    std::size_t start_index = workers_.size();

    for (auto& worker : workers) {
        worker->set_job_queue(job_queue_);
        worker->set_context(context_);
        worker->set_metrics(metrics_);

        // Add worker to vector first
        workers_.emplace_back(std::move(worker));

        // Only start if pool is running
        if (is_running) {
            auto start_result = workers_.back()->start();
            if (start_result.is_err()) {
                // Rollback: remove all workers added in this batch
                workers_.erase(workers_.begin() + static_cast<std::ptrdiff_t>(start_index),
                               workers_.end());
                return start_result.error();
            }
        }
    }

    return common::ok();
}

auto thread_pool::stop(const bool& immediately_stop) -> common::VoidResult {
    // Use compare_exchange_strong to atomically check and set state
    // This prevents TOCTOU (Time-Of-Check-Time-Of-Use) race conditions
    // where multiple threads might call stop() simultaneously
    bool expected = true;
    if (!start_pool_.compare_exchange_strong(expected, false, std::memory_order_acq_rel,
                                             std::memory_order_acquire)) {
        // Pool is already stopped or being stopped by another thread
        return common::ok();
    }

    // At this point, we've atomically transitioned from running to stopped
    // and only this thread will execute the shutdown sequence

    // Cancel pool-level token to propagate cancellation to all workers and jobs
    // This triggers hierarchical cancellation:
    // 1. Pool token cancelled → linked worker tokens cancelled
    // 2. Worker tokens cancelled → running jobs receive cancellation signal
    pool_cancellation_token_.cancel();

    if (job_queue_ != nullptr) {
        job_queue_->stop();

        if (immediately_stop) {
            job_queue_->clear();
        }
    }

    // Stop workers while holding lock to ensure consistent iteration
    // This is safe because worker->stop() only signals and joins threads,
    // it does not call back into thread_pool methods
    std::scoped_lock<std::mutex> lock(workers_mutex_);
    for (auto& worker : workers_) {
        auto stop_result = worker->stop();
        if (stop_result.is_err()) {
            context_.log(common::interfaces::log_level::error, formatter::format("error stopping worker: {}",
                                                             stop_result.error().message));
        }
    }

    return common::ok();
}

/**
 * @brief Stops the thread pool without logging (for use during static destruction).
 *
 * This method performs the same shutdown sequence as stop() but without
 * any logging operations. It is specifically designed to be called from
 * the destructor when static destruction is in progress, preventing
 * Static Destruction Order Fiasco (SDOF) by avoiding access to potentially
 * destroyed logger/monitoring singletons.
 *
 * Implementation details:
 * - Same atomic state transition as stop()
 * - Cancels pool-level token for hierarchical cancellation
 * - Stops job queue without clearing (graceful shutdown)
 * - Stops all workers without logging errors
 * - noexcept guarantee for safe destructor use
 *
 * @return @c result_void containing an error on failure, or success value on success.
 */
auto thread_pool::stop_unsafe() noexcept -> common::VoidResult {
    // Use compare_exchange_strong to atomically check and set state
    // Same atomic transition as stop() to prevent race conditions
    bool expected = true;
    if (!start_pool_.compare_exchange_strong(expected, false, std::memory_order_acq_rel,
                                             std::memory_order_acquire)) {
        // Pool is already stopped or being stopped by another thread
        return common::ok();
    }

    // Cancel pool-level token to propagate cancellation to all workers and jobs
    pool_cancellation_token_.cancel();

    // Stop job queue if it exists
    if (job_queue_ != nullptr) {
        job_queue_->stop();
    }

    // Stop workers while holding lock to ensure consistent iteration
    // No logging to avoid accessing potentially destroyed singletons
    std::scoped_lock<std::mutex> lock(workers_mutex_);
    for (auto& worker : workers_) {
        // Stop worker without checking result to avoid any potential exceptions
        // during static destruction
        worker->stop();
    }

    return common::ok();
}

auto thread_pool::to_string(void) const -> std::string {
    std::string format_string;

    // Use relaxed memory order for diagnostic/logging purposes
    // Exact state ordering is not critical for debug output
    formatter::format_to(std::back_inserter(format_string), "{} is {},\n", thread_title_,
                         start_pool_.load(std::memory_order_relaxed) ? "running" : "stopped");
    formatter::format_to(std::back_inserter(format_string), "\tjob_queue: {}\n\n",
                         (job_queue_ != nullptr ? job_queue_->to_string() : "nullptr"));

    // Protect workers_ access with lock
    std::scoped_lock<std::mutex> lock(workers_mutex_);
    formatter::format_to(std::back_inserter(format_string), "\tworkers: {}\n", workers_.size());
    for (const auto& worker : workers_) {
        formatter::format_to(std::back_inserter(format_string), "\t{}\n", worker->to_string());
    }

    return format_string;
}

auto thread_pool::get_context(void) const -> const thread_context& {
    return context_;
}

std::uint32_t thread_pool::get_pool_instance_id() const {
    return pool_instance_id_;
}

void thread_pool::report_metrics() {
    if (!context_.monitoring()) {
        return;
    }

    common::interfaces::thread_pool_metrics metrics(thread_title_, pool_instance_id_);

    // Protect workers_ access with lock
    {
        std::scoped_lock<std::mutex> lock(workers_mutex_);
        metrics.worker_threads.value = static_cast<double>(workers_.size());
    }

    metrics.idle_threads.value = static_cast<double>(get_idle_worker_count());

    if (job_queue_) {
        metrics.jobs_pending.value = static_cast<double>(job_queue_->size());
    }

    // Report metrics with pool identification
    context_.update_thread_pool_metrics(thread_title_, pool_instance_id_, metrics);
}

std::size_t thread_pool::get_idle_worker_count() const {
    // Count idle workers by checking each worker's idle state
    // Thread safety: workers_mutex_ protects access to workers_ vector
    std::scoped_lock<std::mutex> lock(workers_mutex_);

    return static_cast<std::size_t>(std::count_if(
        workers_.begin(), workers_.end(),
        [](const std::unique_ptr<thread_worker>& worker) { return worker && worker->is_idle(); }));
}

// interface_thread_pool implementation
auto thread_pool::submit_task(std::function<void()> task) -> bool {
    if (!task) {
        return false;
    }

    auto callback_job = std::make_unique<kcenon::thread::callback_job>(
        [task = std::move(task)]() -> common::VoidResult {
            task();
            return common::ok();
        });

    auto result = enqueue(std::move(callback_job));
    return result.is_ok();
}

auto thread_pool::get_thread_count() const -> std::size_t {
    std::scoped_lock<std::mutex> lock(workers_mutex_);
    return workers_.size();
}

auto thread_pool::shutdown_pool(bool immediate) -> bool {
    auto result = stop(immediate);
    return result.is_ok();
}

auto thread_pool::is_running() const -> bool {
    // Use acquire to ensure we see the latest pool state
    // This is important for callers making decisions based on running state
    return start_pool_.load(std::memory_order_acquire);
}

auto thread_pool::get_pending_task_count() const -> std::size_t {
    if (job_queue_) {
        return job_queue_->size();
    }
    return 0;
}

auto thread_pool::check_worker_health(bool restart_failed) -> std::size_t {
    std::scoped_lock<std::mutex> lock(workers_mutex_);

    std::size_t failed_count = 0;

    // Remove dead workers using erase-remove idiom
    auto remove_iter =
        std::remove_if(workers_.begin(), workers_.end(),
                       [&failed_count](const std::unique_ptr<thread_worker>& worker) {
                           if (!worker || !worker->is_running()) {
                               ++failed_count;
                               return true;  // Remove this worker
                           }
                           return false;  // Keep this worker
                       });

    workers_.erase(remove_iter, workers_.end());

    // Restart workers if requested and pool is running
    if (restart_failed && failed_count > 0 && is_running()) {
        // Create new workers to replace failed ones
        for (std::size_t i = 0; i < failed_count; ++i) {
            // Create worker with default settings and context
            auto worker = std::make_unique<thread_worker>(true, context_);

            // Set job queue
            worker->set_job_queue(job_queue_);

            // Start the new worker
            auto start_result = worker->start();
            if (start_result.is_err()) {
                // Failed to start, skip this worker
                continue;
            }

            workers_.push_back(std::move(worker));
        }
    }

    return failed_count;
}

auto thread_pool::get_active_worker_count() const -> std::size_t {
    std::scoped_lock<std::mutex> lock(workers_mutex_);

    return static_cast<std::size_t>(std::count_if(workers_.begin(), workers_.end(),
                                                  [](const std::unique_ptr<thread_worker>& worker) {
                                                      return worker && worker->is_running();
                                                  }));
}

#if KCENON_HAS_COMMON_EXECUTOR
// ============================================================================
// IExecutor interface implementation
// ============================================================================

std::future<void> thread_pool::submit(std::function<void()> task) {
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    auto job_ptr =
        std::make_unique<callback_job>([task = std::move(task), promise]() mutable -> common::VoidResult {
            try {
                task();
                promise->set_value();
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
            return common::ok();
        });

    auto enqueue_result = enqueue(std::move(job_ptr));
    if (enqueue_result.is_err()) {
        // Set exception in promise if enqueue failed
        try {
            throw std::runtime_error("Failed to enqueue task: " +
                                     enqueue_result.error().message);
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    }

    return future;
}

std::future<void> thread_pool::submit_delayed(std::function<void()> task,
                                              std::chrono::milliseconds delay) {
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    // Create a delayed task that waits before executing
    auto delayed_task = [task = std::move(task), delay, promise]() mutable -> common::VoidResult {
        std::this_thread::sleep_for(delay);
        try {
            task();
            promise->set_value();
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
        return common::ok();
    };

    auto job_ptr = std::make_unique<callback_job>(std::move(delayed_task));
    auto enqueue_result = enqueue(std::move(job_ptr));
    if (enqueue_result.is_err()) {
        try {
            throw std::runtime_error("Failed to enqueue delayed task: " +
                                     enqueue_result.error().message);
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    }

    return future;
}

common_ns::Result<std::future<void>> thread_pool::execute(
    std::unique_ptr<common_ns::interfaces::IJob>&& common_job) {
    if (!common_job) {
        return common_ns::error_info{static_cast<int>(error_code::job_invalid), "Null job provided",
                                     "thread_pool"};
    }

    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    // Wrap common::IJob into thread::job - use shared_ptr for copyable lambda
    auto shared_job = std::shared_ptr<common_ns::interfaces::IJob>(std::move(common_job));
    auto job_ptr = std::make_unique<callback_job>([shared_job, promise]() -> common::VoidResult {
        auto result = shared_job->execute();
        if (result.is_ok()) {
            promise->set_value();
        } else {
            try {
                throw std::runtime_error("Job execution failed: " + result.error().message);
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        }
        return common::ok();
    });

    auto enqueue_result = enqueue(std::move(job_ptr));
    if (enqueue_result.is_err()) {
        return enqueue_result.error();
    }

    return common_ns::Result<std::future<void>>(std::move(future));
}

common_ns::Result<std::future<void>> thread_pool::execute_delayed(
    std::unique_ptr<common_ns::interfaces::IJob>&& common_job, std::chrono::milliseconds delay) {
    if (!common_job) {
        return common_ns::error_info{static_cast<int>(error_code::job_invalid), "Null job provided",
                                     "thread_pool"};
    }

    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    // Wrap common::IJob with delay - use shared_ptr for copyable lambda
    auto shared_job = std::shared_ptr<common_ns::interfaces::IJob>(std::move(common_job));
    auto job_ptr = std::make_unique<callback_job>([shared_job, delay, promise]() -> common::VoidResult {
        std::this_thread::sleep_for(delay);
        auto result = shared_job->execute();
        if (result.is_ok()) {
            promise->set_value();
        } else {
            try {
                throw std::runtime_error("Job execution failed: " + result.error().message);
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        }
        return common::ok();
    });

    auto enqueue_result = enqueue(std::move(job_ptr));
    if (enqueue_result.is_err()) {
        return enqueue_result.error();
    }

    return common_ns::Result<std::future<void>>(std::move(future));
}

size_t thread_pool::worker_count() const {
    std::scoped_lock<std::mutex> lock(workers_mutex_);
    return workers_.size();
}

size_t thread_pool::pending_tasks() const {
    return get_pending_task_count();
}

void thread_pool::shutdown(bool wait_for_completion) {
    stop(!wait_for_completion);  // immediately_stop = !wait_for_completion
}
#endif  // KCENON_HAS_COMMON_EXECUTOR

// ============================================================================
// Work-Stealing Support
// ============================================================================

void thread_pool::set_worker_policy(const worker_policy& policy) {
    worker_policy_ = policy;

    // Apply policy to existing workers
    std::scoped_lock<std::mutex> lock(workers_mutex_);
    for (auto& worker : workers_) {
        if (worker) {
            worker->set_policy(policy);
            if (policy.enable_work_stealing) {
                worker->set_steal_function(create_steal_function());
            }
        }
    }
}

const worker_policy& thread_pool::get_worker_policy() const {
    return worker_policy_;
}

void thread_pool::enable_work_stealing(bool enable) {
    worker_policy_.enable_work_stealing = enable;

    std::scoped_lock<std::mutex> lock(workers_mutex_);
    for (auto& worker : workers_) {
        if (worker) {
            worker_policy policy = worker->get_policy();
            policy.enable_work_stealing = enable;
            worker->set_policy(policy);
            if (enable) {
                worker->set_steal_function(create_steal_function());
            } else {
                worker->set_steal_function(nullptr);
            }
        }
    }
}

bool thread_pool::is_work_stealing_enabled() const {
    return worker_policy_.enable_work_stealing;
}

std::function<job*(std::size_t)> thread_pool::create_steal_function() {
    // Capture 'this' to access workers
    return [this](std::size_t requester_id) -> job* {
        return steal_from_workers(requester_id);
    };
}

job* thread_pool::steal_from_workers(std::size_t requester_id) {
    std::scoped_lock<std::mutex> lock(workers_mutex_);

    if (workers_.empty()) {
        return nullptr;
    }

    const std::size_t worker_count = workers_.size();

    // Select victim based on policy
    switch (worker_policy_.victim_selection) {
        case steal_policy::random: {
            // Random victim selection
            static thread_local std::mt19937 rng(std::random_device{}());
            std::uniform_int_distribution<std::size_t> dist(0, worker_count - 1);

            for (std::size_t attempts = 0; attempts < worker_count; ++attempts) {
                std::size_t victim_idx = dist(rng);
                if (workers_[victim_idx] &&
                    workers_[victim_idx]->get_worker_id() != requester_id) {
                    auto* deque = workers_[victim_idx]->get_local_deque();
                    if (deque) {
                        auto stolen = deque->steal();
                        if (stolen.has_value()) {
                            return *stolen;
                        }
                    }
                }
            }
            break;
        }

        case steal_policy::round_robin: {
            // Round-robin victim selection
            for (std::size_t i = 0; i < worker_count; ++i) {
                std::size_t victim_idx = (requester_id + 1 + i) % worker_count;
                if (workers_[victim_idx] &&
                    workers_[victim_idx]->get_worker_id() != requester_id) {
                    auto* deque = workers_[victim_idx]->get_local_deque();
                    if (deque) {
                        auto stolen = deque->steal();
                        if (stolen.has_value()) {
                            return *stolen;
                        }
                    }
                }
            }
            break;
        }

        case steal_policy::adaptive: {
            // Adaptive: steal from worker with most work
            std::size_t best_victim = worker_count;
            std::size_t max_size = 0;

            for (std::size_t i = 0; i < worker_count; ++i) {
                if (workers_[i] &&
                    workers_[i]->get_worker_id() != requester_id) {
                    auto* deque = workers_[i]->get_local_deque();
                    if (deque) {
                        std::size_t size = deque->size();
                        if (size > max_size) {
                            max_size = size;
                            best_victim = i;
                        }
                    }
                }
            }

            if (best_victim < worker_count && max_size > 0) {
                auto* deque = workers_[best_victim]->get_local_deque();
                if (deque) {
                    auto stolen = deque->steal();
                    if (stolen.has_value()) {
                        return *stolen;
                    }
                }
            }
            break;
        }
    }

    return nullptr;
}

// ============================================================================
// Diagnostics
// ============================================================================

auto thread_pool::diagnostics() -> diagnostics::thread_pool_diagnostics& {
    if (!diagnostics_) {
        diagnostics_ = std::make_unique<diagnostics::thread_pool_diagnostics>(*this);
    }
    return *diagnostics_;
}

auto thread_pool::diagnostics() const -> const diagnostics::thread_pool_diagnostics& {
    if (!diagnostics_) {
        diagnostics_ = std::make_unique<diagnostics::thread_pool_diagnostics>(
            const_cast<thread_pool&>(*this));
    }
    return *diagnostics_;
}

auto thread_pool::collect_worker_diagnostics() const
    -> std::vector<diagnostics::thread_info> {
    std::scoped_lock<std::mutex> lock(workers_mutex_);

    std::vector<diagnostics::thread_info> result;
    result.reserve(workers_.size());

    for (std::size_t i = 0; i < workers_.size(); ++i) {
        const auto& worker = workers_[i];
        if (!worker) {
            continue;
        }

        diagnostics::thread_info info;
        info.thread_id = worker->get_thread_id();
        info.thread_name = "Worker-" + std::to_string(i);
        info.worker_id = worker->get_worker_id();

        // Determine worker state
        if (!worker->is_running()) {
            info.state = diagnostics::worker_state::stopped;
        } else if (worker->is_idle()) {
            info.state = diagnostics::worker_state::idle;
        } else {
            info.state = diagnostics::worker_state::active;
        }

        info.state_since = worker->get_state_since();

        // Get current job info if active
        info.current_job = worker->get_current_job_info();

        // Get statistics
        info.jobs_completed = worker->get_jobs_completed();
        info.jobs_failed = worker->get_jobs_failed();
        info.total_busy_time = worker->get_total_busy_time();
        info.total_idle_time = worker->get_total_idle_time();

        // Calculate utilization
        info.update_utilization();

        result.push_back(std::move(info));
    }

    return result;
}

// ============================================================================
// Circuit Breaker
// ============================================================================

void thread_pool::enable_circuit_breaker(const circuit_breaker_config& config) {
    circuit_breaker_ = std::make_shared<circuit_breaker>(config);
}

void thread_pool::disable_circuit_breaker() {
    circuit_breaker_.reset();
}

auto thread_pool::get_circuit_breaker() -> std::shared_ptr<circuit_breaker> {
    return circuit_breaker_;
}

auto thread_pool::is_accepting_work() const -> bool {
    if (!circuit_breaker_) {
        return true;  // No circuit breaker means always accepting
    }

    auto state = circuit_breaker_->get_state();
    return state != circuit_state::open;
}

auto thread_pool::enqueue_protected(std::unique_ptr<job>&& j) -> common::VoidResult {
    if (!j) {
        return make_error_result(error_code::job_invalid, "Job is null");
    }

    // If no circuit breaker, behave like regular enqueue
    if (!circuit_breaker_) {
        return enqueue(std::move(j));
    }

    // Wrap job with circuit breaker protection
    auto protected_j = std::make_unique<protected_job>(
        std::move(j), circuit_breaker_);

    return enqueue(std::move(protected_j));
}

// ============================================================================
// Autoscaling
// ============================================================================

void thread_pool::enable_autoscaling(const autoscaling_policy& policy) {
    if (autoscaler_) {
        // Already enabled, update policy
        autoscaler_->set_policy(policy);
        return;
    }

    autoscaler_ = std::make_shared<autoscaler>(*this, policy);

    // Start autoscaler if pool is running
    if (start_pool_.load(std::memory_order_acquire)) {
        autoscaler_->start();
    }
}

void thread_pool::disable_autoscaling() {
    if (autoscaler_) {
        autoscaler_->stop();
        autoscaler_.reset();
    }
}

auto thread_pool::get_autoscaler() -> std::shared_ptr<autoscaler> {
    return autoscaler_;
}

auto thread_pool::is_autoscaling_enabled() const -> bool {
    return autoscaler_ != nullptr && autoscaler_->is_active();
}

auto thread_pool::remove_workers(std::size_t count) -> common::VoidResult {
    if (count == 0) {
        return common::ok();
    }

    std::scoped_lock<std::mutex> lock(workers_mutex_);

    // Determine minimum workers to keep
    std::size_t min_workers = 1;
    if (autoscaler_) {
        min_workers = autoscaler_->get_policy().min_workers;
    }

    if (workers_.size() <= min_workers) {
        return common::error_info{
            static_cast<int>(error_code::invalid_argument),
            "Cannot remove workers: already at minimum",
            "thread_system"
        };
    }

    // Calculate how many we can actually remove
    std::size_t max_removable = workers_.size() - min_workers;
    count = std::min(count, max_removable);

    std::size_t removed = 0;

    // First pass: remove idle workers
    auto it = workers_.begin();
    while (it != workers_.end() && removed < count) {
        if (*it && (*it)->is_idle()) {
            // Stop the worker
            (*it)->stop();
            it = workers_.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }

    // If we still need to remove more, wait briefly for workers to become idle
    if (removed < count) {
        // Just return success with what we removed
        // Remaining workers will be removed on subsequent calls
        context_.log(common::interfaces::log_level::info,
            formatter::format("Removed {} of {} requested workers (remaining are busy)",
                              removed, count));
    }

    return common::ok();
}

}  // namespace kcenon::thread
