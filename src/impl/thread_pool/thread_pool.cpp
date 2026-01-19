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
#include <kcenon/thread/adapters/job_queue_adapter.h>

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
 * @brief Constructs a thread pool with a policy_queue adapter.
 *
 * Implementation details:
 * - Initializes with provided thread title for identification
 * - Uses the provided queue adapter for queue operations
 * - If adapter wraps a job_queue, extracts it for backward compatibility
 * - Pool starts in stopped state (start_pool_ = false)
 * - No workers are initially assigned (workers_ is empty)
 * - Stores thread context for logging and monitoring
 * - Creates pool-level cancellation token for hierarchical cancellation
 *
 * @param thread_title Descriptive name for this thread pool instance
 * @param queue_adapter Queue adapter wrapping job_queue or policy_queue
 * @param context Thread context providing logging and monitoring services
 */
thread_pool::thread_pool(const std::string& thread_title,
                         std::unique_ptr<pool_queue_adapter_interface> queue_adapter,
                         const thread_context& context)
    : thread_title_(thread_title)
    , pool_instance_id_(next_pool_instance_id_.fetch_add(1))
    , start_pool_(false)
    , job_queue_(queue_adapter ? queue_adapter->get_job_queue() : nullptr)
    , queue_adapter_(std::move(queue_adapter))
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

    // Handle queue initialization for restart scenarios
    // The approach differs based on whether we're using job_queue or queue_adapter
    if (queue_adapter_) {
        // Using queue_adapter (policy_queue or wrapped job_queue)
        if (queue_adapter_->is_stopped()) {
            // For policy_queue adapters, we can't easily recreate,
            // so we return an error suggesting pool recreation
            return common::error_info{
                static_cast<int>(error_code::queue_stopped),
                "queue is stopped; create a new thread_pool instance for restart",
                "thread_system"};
        }
        // Update job_queue_ reference if adapter wraps a job_queue
        job_queue_ = queue_adapter_->get_job_queue();
    } else if (job_queue_ == nullptr || job_queue_->is_stopped()) {
        // Create fresh job queue for restart scenarios
        // Stopped queues cannot accept new jobs, so we must create a new instance
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

    // Check queue availability and stopped state
    // Supports both queue_adapter_ (policy_queue) and job_queue_ (legacy)
    if (queue_adapter_) {
        if (queue_adapter_->is_stopped()) {
            return common::error_info{static_cast<int>(error_code::queue_stopped), "thread pool is stopped", "thread_system"};
        }
    } else if (job_queue_ == nullptr) {
        return common::error_info{static_cast<int>(error_code::resource_allocation_failed), "job queue is null", "thread_system"};
    } else if (job_queue_->is_stopped()) {
        return common::error_info{static_cast<int>(error_code::queue_stopped), "thread pool is stopped", "thread_system"};
    }

    // Delegate to queue for processing
    metrics_->record_submission();

    // Record enhanced metrics if enabled
    auto start_time = std::chrono::steady_clock::now();

    if (queue_adapter_) {
        auto enqueue_result = queue_adapter_->enqueue(std::move(job));
        if (enqueue_result.is_err()) {
            return enqueue_result.error();
        }
    } else {
        auto enqueue_result = job_queue_->enqueue(std::move(job));
        if (enqueue_result.is_err()) {
            return enqueue_result.error();
        }
    }

    metrics_->record_enqueue();

    // Record enhanced metrics
    if (enhanced_metrics_enabled_.load(std::memory_order_relaxed) && enhanced_metrics_) {
        auto end_time = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        enhanced_metrics_->record_submission();
        enhanced_metrics_->record_enqueue(latency);
        auto queue_size = queue_adapter_ ? queue_adapter_->size() : job_queue_->size();
        enhanced_metrics_->record_queue_depth(queue_size);
    }

    return common::ok();
}

auto thread_pool::enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> common::VoidResult {
    if (jobs.empty()) {
        return common::error_info{static_cast<int>(error_code::invalid_argument), "jobs are empty", "thread_system"};
    }

    // Check queue availability and stopped state
    // Supports both queue_adapter_ (policy_queue) and job_queue_ (legacy)
    if (queue_adapter_) {
        if (queue_adapter_->is_stopped()) {
            return common::error_info{static_cast<int>(error_code::queue_stopped), "thread pool is stopped", "thread_system"};
        }
    } else if (job_queue_ == nullptr) {
        return common::error_info{static_cast<int>(error_code::resource_allocation_failed), "job queue is null", "thread_system"};
    } else if (job_queue_->is_stopped()) {
        return common::error_info{static_cast<int>(error_code::queue_stopped), "thread pool is stopped", "thread_system"};
    }

    const auto batch_size = jobs.size();
    metrics_->record_submission(batch_size);

    // Record enhanced metrics if enabled
    auto start_time = std::chrono::steady_clock::now();

    if (queue_adapter_) {
        auto enqueue_result = queue_adapter_->enqueue_batch(std::move(jobs));
        if (enqueue_result.is_err()) {
            return enqueue_result.error();
        }
    } else {
        auto enqueue_result = job_queue_->enqueue_batch(std::move(jobs));
        if (enqueue_result.is_err()) {
            return enqueue_result.error();
        }
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
        auto queue_size = queue_adapter_ ? queue_adapter_->size() : job_queue_->size();
        enhanced_metrics_->record_queue_depth(queue_size);
    }

    return common::ok();
}

auto thread_pool::enqueue(std::unique_ptr<thread_worker>&& worker) -> common::VoidResult {
    if (worker == nullptr) {
        return common::error_info{static_cast<int>(error_code::invalid_argument), "worker is null", "thread_system"};
    }

    // Get job_queue from adapter if available, otherwise use direct job_queue_
    std::shared_ptr<job_queue> worker_queue;
    if (queue_adapter_) {
        worker_queue = queue_adapter_->get_job_queue();
        if (!worker_queue) {
            // policy_queue adapter without job_queue backend
            // Workers currently require job_queue; this limitation may be lifted in future versions
            return common::error_info{
                static_cast<int>(error_code::resource_allocation_failed),
                "policy_queue adapter without job_queue backend not yet supported for workers",
                "thread_system"};
        }
    } else if (job_queue_ == nullptr) {
        return common::error_info{static_cast<int>(error_code::resource_allocation_failed), "job queue is null", "thread_system"};
    } else {
        worker_queue = job_queue_;
    }

    worker->set_job_queue(worker_queue);
    worker->set_context(context_);
    worker->set_metrics(metrics_);
    worker->set_diagnostics(&diagnostics());

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

    // Get diagnostics pointer once outside the loop for efficiency
    auto* diag = &diagnostics();

    for (auto& worker : workers) {
        worker->set_job_queue(job_queue_);
        worker->set_context(context_);
        worker->set_metrics(metrics_);
        worker->set_diagnostics(diag);

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

    // Stop the queue (supports both queue_adapter_ and job_queue_)
    if (queue_adapter_) {
        queue_adapter_->stop();
        if (immediately_stop) {
            queue_adapter_->clear();
        }
    } else if (job_queue_ != nullptr) {
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

    // Stop the queue (supports both queue_adapter_ and job_queue_)
    if (queue_adapter_) {
        queue_adapter_->stop();
    } else if (job_queue_ != nullptr) {
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

    // Get queue string representation (supports both queue_adapter_ and job_queue_)
    std::string queue_str;
    if (queue_adapter_) {
        queue_str = queue_adapter_->to_string();
    } else if (job_queue_ != nullptr) {
        queue_str = job_queue_->to_string();
    } else {
        queue_str = "nullptr";
    }
    formatter::format_to(std::back_inserter(format_string), "\tjob_queue: {}\n\n", queue_str);

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

    // Get pending jobs count (supports both queue_adapter_ and job_queue_)
    if (queue_adapter_) {
        metrics.jobs_pending.value = static_cast<double>(queue_adapter_->size());
    } else if (job_queue_) {
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

auto thread_pool::is_running() const -> bool {
    // Use acquire to ensure we see the latest pool state
    // This is important for callers making decisions based on running state
    return start_pool_.load(std::memory_order_acquire);
}

auto thread_pool::get_pending_task_count() const -> std::size_t {
    // Supports both queue_adapter_ and job_queue_
    if (queue_adapter_) {
        return queue_adapter_->size();
    }
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
        // Get diagnostics pointer for new workers
        auto* diag = &diagnostics();

        // Create new workers to replace failed ones
        for (std::size_t i = 0; i < failed_count; ++i) {
            // Create worker with default settings and context
            auto worker = std::make_unique<thread_worker>(true, context_);

            // Set job queue and diagnostics
            worker->set_job_queue(job_queue_);
            worker->set_metrics(metrics_);
            worker->set_diagnostics(diag);

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
// Pool Policies
// ============================================================================

void thread_pool::add_policy(std::unique_ptr<pool_policy> policy) {
    if (!policy) {
        return;
    }

    std::scoped_lock<std::mutex> lock(policies_mutex_);
    policies_.push_back(std::move(policy));
}

auto thread_pool::get_policies() const -> const std::vector<std::unique_ptr<pool_policy>>& {
    return policies_;
}

auto thread_pool::remove_policy(const std::string& name) -> bool {
    std::scoped_lock<std::mutex> lock(policies_mutex_);

    auto it = std::remove_if(policies_.begin(), policies_.end(),
        [&name](const std::unique_ptr<pool_policy>& policy) {
            return policy && policy->get_name() == name;
        });

    if (it != policies_.end()) {
        policies_.erase(it, policies_.end());
        return true;
    }

    return false;
}

// ============================================================================
// Internal Methods (for friend classes)
// ============================================================================

auto thread_pool::remove_workers_internal(std::size_t count, std::size_t min_workers)
    -> common::VoidResult
{
    if (count == 0) {
        return common::ok();
    }

    std::scoped_lock<std::mutex> lock(workers_mutex_);

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
