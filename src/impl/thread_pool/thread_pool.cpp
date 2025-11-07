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

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/job_queue.h>

#include <kcenon/thread/interfaces/logger_interface.h>
#include <kcenon/thread/utils/formatter.h>

using namespace utility_module;

/**
 * @file thread_pool.cpp
 * @brief Implementation of the thread pool class for managing multiple worker threads.
 *
 * This file contains the implementation of the thread_pool class, which coordinates
 * multiple worker threads processing jobs from a shared queue. The pool supports
 * adaptive queue strategies for optimal performance under varying load conditions.
 */

namespace kcenon::thread
{
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
		: thread_title_(thread_title),
		  pool_instance_id_(next_pool_instance_id_.fetch_add(1)),
		  start_pool_(false),
		  job_queue_(std::make_shared<kcenon::thread::job_queue>()),
		  context_(context),
		  pool_cancellation_token_(cancellation_token::create())
	{
		// Report initial pool registration if monitoring is available
		if (context_.monitoring())
		{
			monitoring_interface::thread_pool_metrics initial_metrics;
			initial_metrics.pool_name = thread_title_;
			initial_metrics.pool_instance_id = pool_instance_id_;
			initial_metrics.worker_threads = 0;
			initial_metrics.timestamp = std::chrono::steady_clock::now();
			context_.update_thread_pool_metrics(thread_title_, pool_instance_id_, initial_metrics);
		}
	}

	/**
	 * @brief Destroys the thread pool, ensuring all workers are stopped.
	 * 
	 * Implementation details:
	 * - Automatically calls stop() to ensure clean shutdown
	 * - Workers will complete current jobs before terminating
	 * - Prevents resource leaks from running threads
	 */
	thread_pool::~thread_pool() { stop(); }

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
	auto thread_pool::get_ptr(void) -> std::shared_ptr<thread_pool>
	{
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
    auto thread_pool::start(void) -> result_void
    {
        // Acquire lock to check workers_ safely
        std::scoped_lock<std::mutex> lock(workers_mutex_);

        // Check if pool is already running
        // Use acquire to ensure we see all previous modifications to pool state
        if (start_pool_.load(std::memory_order_acquire))
        {
            return error{error_code::thread_already_running, "thread pool is already running"};
        }

        // Validate that workers have been added
        if (workers_.empty())
        {
            return error{error_code::invalid_argument, "no workers to start"};
        }

        // Create fresh job queue for restart scenarios
        // Stopped queues cannot accept new jobs, so we must create a new instance
        if (job_queue_ == nullptr || job_queue_->is_stopped())
        {
            job_queue_ = std::make_shared<kcenon::thread::job_queue>();

            // Update all workers with the new queue reference
            for (auto& worker : workers_)
            {
                worker->set_job_queue(job_queue_);
            }
        }

        // Create fresh pool cancellation token for restart scenarios
        // This ensures workers start with a non-cancelled token
        pool_cancellation_token_ = cancellation_token::create();

        // Attempt to start each worker
        for (auto& worker : workers_)
        {
            auto start_result = worker->start();
            if (start_result.has_error())
            {
                // If any worker fails, stop all and return error
                stop();
                return start_result.get_error();
            }
        }

        // Mark pool as successfully started
        // Use release to ensure all previous modifications (worker starts, queue setup)
        // are visible to other threads before they see start_pool_ == true
        start_pool_.store(true, std::memory_order_release);

        return {};
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
	auto thread_pool::get_job_queue(void) -> std::shared_ptr<job_queue> { return job_queue_; }

	// executor_interface
	auto thread_pool::execute(std::unique_ptr<job>&& work) -> result_void
	{
		return enqueue(std::move(work));
	}

	auto thread_pool::shutdown() -> result_void { return stop(false); }

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
    auto thread_pool::enqueue(std::unique_ptr<job>&& job) -> result_void
    {
        // Validate inputs
        if (job == nullptr)
        {
            return error{error_code::invalid_argument, "job is null"};
        }

        if (job_queue_ == nullptr)
        {
            return error{error_code::resource_allocation_failed, "job queue is null"};
        }

        // Check if queue has been explicitly stopped (via stop())
        // This prevents race conditions during shutdown where stop() has been called
        // but jobs might still be submitted. Note: We check the queue's stopped state
        // rather than start_pool_ to allow jobs to be enqueued before start() is called.
        if (job_queue_->is_stopped())
        {
            return error{error_code::queue_stopped, "thread pool is stopped"};
        }

        // Delegate to adaptive queue for optimal processing
        auto enqueue_result = job_queue_->enqueue(std::move(job));
        if (enqueue_result.has_error())
        {
            return enqueue_result.get_error();
        }

        return {};
    }

    auto thread_pool::enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs)
        -> result_void
    {
        if (jobs.empty())
        {
            return error{error_code::invalid_argument, "jobs are empty"};
        }

        if (job_queue_ == nullptr)
        {
            return error{error_code::resource_allocation_failed, "job queue is null"};
        }

        // Check if queue has been explicitly stopped
        if (job_queue_->is_stopped())
        {
            return error{error_code::queue_stopped, "thread pool is stopped"};
        }

        auto enqueue_result = job_queue_->enqueue_batch(std::move(jobs));
        if (enqueue_result.has_error())
        {
            return enqueue_result.get_error();
        }

        return {};
    }

    auto thread_pool::enqueue(std::unique_ptr<thread_worker>&& worker) -> result_void
    {
        if (worker == nullptr)
        {
            return error{error_code::invalid_argument, "worker is null"};
        }

        if (job_queue_ == nullptr)
        {
            return error{error_code::resource_allocation_failed, "job queue is null"};
        }

        worker->set_job_queue(job_queue_);
        worker->set_context(context_);

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
        if (is_running)
        {
            auto start_result = workers_.back()->start();
            if (start_result.has_error())
            {
                // Remove the worker we just added since it failed to start
                workers_.pop_back();
                return start_result.get_error();
            }
        }

        return {};
    }

    auto thread_pool::enqueue_batch(std::vector<std::unique_ptr<thread_worker>>&& workers)
        -> result_void
    {
        if (workers.empty())
        {
            return error{error_code::invalid_argument, "workers are empty"};
        }

        if (job_queue_ == nullptr)
        {
            return error{error_code::resource_allocation_failed, "job queue is null"};
        }

        // Acquire lock before processing workers
        // This ensures atomic check-and-add operation with respect to stop()
        std::scoped_lock<std::mutex> lock(workers_mutex_);

        // Check pool running state once with acquire semantics
        bool is_running = start_pool_.load(std::memory_order_acquire);

        // Track the starting index for rollback in case of error
        std::size_t start_index = workers_.size();

        for (auto& worker : workers)
        {
            worker->set_job_queue(job_queue_);
            worker->set_context(context_);

            // Add worker to vector first
            workers_.emplace_back(std::move(worker));

            // Only start if pool is running
            if (is_running)
            {
                auto start_result = workers_.back()->start();
                if (start_result.has_error())
                {
                    // Rollback: remove all workers added in this batch
                    workers_.erase(workers_.begin() + start_index, workers_.end());
                    return start_result.get_error();
                }
            }
        }

        return {};
    }

    auto thread_pool::stop(const bool& immediately_stop) -> result_void
    {
        // Use compare_exchange_strong to atomically check and set state
        // This prevents TOCTOU (Time-Of-Check-Time-Of-Use) race conditions
        // where multiple threads might call stop() simultaneously
        bool expected = true;
        if (!start_pool_.compare_exchange_strong(expected, false,
                                                  std::memory_order_acq_rel,
                                                  std::memory_order_acquire))
        {
            // Pool is already stopped or being stopped by another thread
            return {};
        }

        // At this point, we've atomically transitioned from running to stopped
        // and only this thread will execute the shutdown sequence

        // Cancel pool-level token to propagate cancellation to all workers and jobs
        // This triggers hierarchical cancellation:
        // 1. Pool token cancelled → linked worker tokens cancelled
        // 2. Worker tokens cancelled → running jobs receive cancellation signal
        pool_cancellation_token_.cancel();

        if (job_queue_ != nullptr)
        {
            job_queue_->stop();

            if (immediately_stop)
            {
                job_queue_->clear();
            }
        }

        // Stop workers while holding lock to ensure consistent iteration
        // This is safe because worker->stop() only signals and joins threads,
        // it does not call back into thread_pool methods
        std::scoped_lock<std::mutex> lock(workers_mutex_);
        for (auto& worker : workers_)
        {
            auto stop_result = worker->stop();
            if (stop_result.has_error())
            {
                context_.log(log_level::error,
                            formatter::format("error stopping worker: {}",
                                            stop_result.get_error().to_string()));
            }
        }

        return {};
    }

	auto thread_pool::to_string(void) const -> std::string
	{
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
		for (const auto& worker : workers_)
		{
			formatter::format_to(std::back_inserter(format_string), "\t{}\n", worker->to_string());
		}

		return format_string;
	}

	auto thread_pool::get_context(void) const -> const thread_context&
	{
		return context_;
	}

	std::uint32_t thread_pool::get_pool_instance_id() const
	{
		return pool_instance_id_;
	}

	void thread_pool::report_metrics()
	{
		if (!context_.monitoring())
		{
			return;
		}

		monitoring_interface::thread_pool_metrics metrics;
		metrics.pool_name = thread_title_;
		metrics.pool_instance_id = pool_instance_id_;

		// Protect workers_ access with lock
		{
			std::scoped_lock<std::mutex> lock(workers_mutex_);
			metrics.worker_threads = workers_.size();
		}

		metrics.idle_threads = get_idle_worker_count();

		if (job_queue_)
		{
			metrics.jobs_pending = job_queue_->size();
		}

		metrics.timestamp = std::chrono::steady_clock::now();

		// Report metrics with pool identification
		context_.update_thread_pool_metrics(thread_title_, pool_instance_id_, metrics);
	}

	std::size_t thread_pool::get_idle_worker_count() const
	{
		// Count idle workers by checking each worker's idle state
		// Thread safety: workers_mutex_ protects access to workers_ vector
		std::scoped_lock<std::mutex> lock(workers_mutex_);

		return std::count_if(workers_.begin(), workers_.end(),
			[](const std::unique_ptr<thread_worker>& worker) {
				return worker && worker->is_idle();
			});
	}

	// interface_thread_pool implementation
	auto thread_pool::submit_task(std::function<void()> task) -> bool
	{
		if (!task) 
		{
			return false;
		}

		auto callback_job = std::make_unique<kcenon::thread::callback_job>(
			[task = std::move(task)]() -> kcenon::thread::result_void {
				task();
				return {};
			});

		auto result = enqueue(std::move(callback_job));
		return result.has_error() == false;
	}

	auto thread_pool::get_thread_count() const -> std::size_t
	{
		std::scoped_lock<std::mutex> lock(workers_mutex_);
		return workers_.size();
	}

	auto thread_pool::shutdown_pool(bool immediate) -> bool
	{
		auto result = stop(immediate);
		return result.has_error() == false;
	}

	auto thread_pool::is_running() const -> bool
	{
		// Use acquire to ensure we see the latest pool state
		// This is important for callers making decisions based on running state
		return start_pool_.load(std::memory_order_acquire);
	}

	auto thread_pool::get_pending_task_count() const -> std::size_t
	{
		if (job_queue_)
		{
			return job_queue_->size();
		}
		return 0;
	}

	auto thread_pool::check_worker_health(bool restart_failed) -> std::size_t
	{
		std::scoped_lock<std::mutex> lock(workers_mutex_);

		std::size_t failed_count = 0;

		// Remove dead workers using erase-remove idiom
		auto remove_iter = std::remove_if(
			workers_.begin(),
			workers_.end(),
			[&failed_count](const std::unique_ptr<thread_worker>& worker) {
				if (!worker || !worker->is_running()) {
					++failed_count;
					return true;  // Remove this worker
				}
				return false;  // Keep this worker
			}
		);

		workers_.erase(remove_iter, workers_.end());

		// Restart workers if requested and pool is running
		if (restart_failed && failed_count > 0 && is_running())
		{
			// Create new workers to replace failed ones
			for (std::size_t i = 0; i < failed_count; ++i)
			{
				// Create worker with default settings and context
				auto worker = std::make_unique<thread_worker>(true, context_);

				// Set job queue
				worker->set_job_queue(job_queue_);

				// Start the new worker
				auto start_result = worker->start();
				if (start_result.has_error())
				{
					// Failed to start, skip this worker
					continue;
				}

				workers_.push_back(std::move(worker));
			}
		}

		return failed_count;
	}

	auto thread_pool::get_active_worker_count() const -> std::size_t
	{
		std::scoped_lock<std::mutex> lock(workers_mutex_);

		return std::count_if(
			workers_.begin(),
			workers_.end(),
			[](const std::unique_ptr<thread_worker>& worker) {
				return worker && worker->is_running();
			}
		);
	}

#ifdef THREAD_HAS_COMMON_EXECUTOR
	// ============================================================================
	// IExecutor interface implementation
	// ============================================================================

	std::future<void> thread_pool::submit(std::function<void()> task)
	{
		auto promise = std::make_shared<std::promise<void>>();
		auto future = promise->get_future();

		auto job_ptr = std::make_unique<callback_job>([task = std::move(task), promise]() mutable -> result_void {
			try {
				task();
				promise->set_value();
			} catch (...) {
				promise->set_exception(std::current_exception());
			}
			return result_void{};
		});

		auto enqueue_result = enqueue(std::move(job_ptr));
		if (enqueue_result.has_error()) {
			// Set exception in promise if enqueue failed
			try {
				throw std::runtime_error("Failed to enqueue task: " +
					enqueue_result.get_error().to_string());
			} catch (...) {
				promise->set_exception(std::current_exception());
			}
		}

		return future;
	}

	std::future<void> thread_pool::submit_delayed(
		std::function<void()> task,
		std::chrono::milliseconds delay)
	{
		auto promise = std::make_shared<std::promise<void>>();
		auto future = promise->get_future();

		// Create a delayed task that waits before executing
		auto delayed_task = [task = std::move(task), delay, promise]() mutable -> result_void {
			std::this_thread::sleep_for(delay);
			try {
				task();
				promise->set_value();
			} catch (...) {
				promise->set_exception(std::current_exception());
			}
			return result_void{};
		};

		auto job_ptr = std::make_unique<callback_job>(std::move(delayed_task));
		auto enqueue_result = enqueue(std::move(job_ptr));
		if (enqueue_result.has_error()) {
			try {
				throw std::runtime_error("Failed to enqueue delayed task: " +
					enqueue_result.get_error().to_string());
			} catch (...) {
				promise->set_exception(std::current_exception());
			}
		}

		return future;
	}

	kcenon::common::Result<std::future<void>> thread_pool::execute(
		std::unique_ptr<kcenon::common::interfaces::IJob>&& common_job)
	{
		if (!common_job) {
			return kcenon::common::error_info{
				static_cast<int>(error_code::job_invalid),
				"Null job provided",
				"thread_pool"
			};
		}

		auto promise = std::make_shared<std::promise<void>>();
		auto future = promise->get_future();

		// Wrap common::IJob into thread::job - use shared_ptr for copyable lambda
		auto shared_job = std::shared_ptr<kcenon::common::interfaces::IJob>(std::move(common_job));
		auto job_ptr = std::make_unique<callback_job>([
			shared_job,
			promise
		]() -> result_void {
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
			return result_void{};
		});

		auto enqueue_result = enqueue(std::move(job_ptr));
		if (enqueue_result.has_error()) {
			return detail::to_common_error(enqueue_result.get_error());
		}

		return kcenon::common::Result<std::future<void>>(std::move(future));
	}

	kcenon::common::Result<std::future<void>> thread_pool::execute_delayed(
		std::unique_ptr<kcenon::common::interfaces::IJob>&& common_job,
		std::chrono::milliseconds delay)
	{
		if (!common_job) {
			return kcenon::common::error_info{
				static_cast<int>(error_code::job_invalid),
				"Null job provided",
				"thread_pool"
			};
		}

		auto promise = std::make_shared<std::promise<void>>();
		auto future = promise->get_future();

		// Wrap common::IJob with delay - use shared_ptr for copyable lambda
		auto shared_job = std::shared_ptr<kcenon::common::interfaces::IJob>(std::move(common_job));
		auto job_ptr = std::make_unique<callback_job>([
			shared_job,
			delay,
			promise
		]() -> result_void {
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
			return result_void{};
		});

		auto enqueue_result = enqueue(std::move(job_ptr));
		if (enqueue_result.has_error()) {
			return detail::to_common_error(enqueue_result.get_error());
		}

		return kcenon::common::Result<std::future<void>>(std::move(future));
	}

	size_t thread_pool::worker_count() const
	{
		std::scoped_lock<std::mutex> lock(workers_mutex_);
		return workers_.size();
	}

	size_t thread_pool::pending_tasks() const
	{
		return get_pending_task_count();
	}

	void thread_pool::shutdown(bool wait_for_completion)
	{
		stop(!wait_for_completion);  // immediately_stop = !wait_for_completion
	}
#endif // THREAD_HAS_COMMON_EXECUTOR

} // namespace kcenon::thread
