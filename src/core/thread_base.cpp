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
#include <kcenon/thread/core/thread_logger.h>

/**
 * @file thread_base.cpp
 * @brief Implementation of the core thread base class.
 *
 * This file contains the implementation of the thread_base class, which serves
 * as the foundation for all worker thread types in the thread system.
 */

namespace kcenon::thread
{
	/**
	 * @brief Constructs a new thread_base instance with the specified title.
	 *
	 * Implementation details:
	 * - Initializes the worker_thread_ pointer to nullptr (thread not started)
	 * - lifecycle_controller handles stop control and state management
	 * - Sets wake_interval_ to std::nullopt (no periodic wake-ups by default)
	 * - Sets thread_title_ to the provided title
	 */
	thread_base::thread_base(const std::string& thread_title)
		: wake_interval_(std::nullopt)
		, lifecycle_()
		, worker_thread_(nullptr)
		, thread_title_(thread_title)
	{
	}

	/**
	 * @brief Destroys the thread_base instance, stopping the thread if needed.
	 * 
	 * Implementation details:
	 * - Calls stop() to ensure the thread is properly terminated
	 * - The stop() method handles joining the thread and cleaning up resources
	 * - This ensures no thread resources are leaked when the object is destroyed
	 * 
	 * @note This destructor is virtual, allowing derived classes to perform
	 * their own cleanup operations in their destructors.
	 */
	thread_base::~thread_base(void) { stop(); }

	/**
	 * @brief Sets the wake interval for periodic thread wake-ups.
	 * 
	 * Implementation details:
	 * - Uses a dedicated mutex (wake_interval_mutex_) to ensure thread-safe access
	 * - The scoped_lock ensures automatic release when the function exits
	 * - This interval controls how often the thread wakes up even when idle
	 * - Setting std::nullopt disables periodic wake-ups (thread only wakes on signals)
	 * 
	 * Thread Safety:
	 * - Safe to call from any thread while the worker thread is running
	 * - The wake_interval_mutex_ protects against data races with get_wake_interval()
	 */
	auto thread_base::set_wake_interval(
		const std::optional<std::chrono::milliseconds>& wake_interval) -> void
	{
		// Use dedicated mutex for wake_interval to prevent data races
		std::scoped_lock<std::mutex> lock(wake_interval_mutex_);
		wake_interval_ = wake_interval;
	}

	/**
	 * @brief Gets the current wake interval setting.
	 * 
	 * Implementation details:
	 * - Uses the same mutex as set_wake_interval() for consistency
	 * - Returns a copy of the current wake_interval_ value
	 * - std::nullopt indicates no periodic wake-ups are configured
	 * 
	 * Thread Safety:
	 * - Safe to call from any thread concurrently with set_wake_interval()
	 * - The mutex ensures consistent reads even during concurrent modifications
	 * 
	 * @return Current wake interval or std::nullopt if disabled
	 */
	auto thread_base::get_wake_interval() const 
		-> std::optional<std::chrono::milliseconds>
	{
		// Thread-safe read of wake_interval
		std::scoped_lock<std::mutex> lock(wake_interval_mutex_);
		return wake_interval_;
	}

	/**
	 * @brief Starts the worker thread and begins execution loop.
	 *
	 * Implementation details:
	 * - First checks if thread is already running via lifecycle_controller
	 * - Calls stop() first to ensure clean state (idempotent operation)
	 * - Initializes lifecycle_controller for the new thread
	 * - Creates worker thread that executes the main work loop
	 *
	 * Main Work Loop Logic:
	 * 1. Calls before_start() hook for derived class initialization
	 * 2. Enters main loop while not stopped and has work to do
	 * 3. Uses lifecycle_controller for state and wait management
	 * 4. Calls do_work() hook for actual work execution
	 * 5. Handles exceptions from do_work() gracefully
	 * 6. Calls after_stop() hook for cleanup when exiting
	 *
	 * @return Empty result on success, error on failure
	 */
	auto thread_base::start(void) -> common::VoidResult
	{
		// Check if thread is already running using lifecycle_controller
		if (lifecycle_.has_active_source())
		{
			return common::error_info{static_cast<int>(error_code::thread_already_running), "thread is already running", "thread_system"};
		}

		// Ensure clean state by stopping any existing thread first
		stop();

		// Initialize lifecycle controller for the new thread
		lifecycle_.initialize_for_start();

		try
		{
			// Create the worker thread using platform-appropriate thread type
#ifdef USE_STD_JTHREAD
			worker_thread_ = std::make_unique<std::jthread>(
#else
			worker_thread_ = std::make_unique<std::thread>(
#endif
				[this](void)  // Capture 'this' to access member functions and variables
				{
					// Phase 1: Call derived class initialization hook
					auto work_result = before_start();
					if (work_result.is_err())
					{
						std::cerr << "error before start: " << work_result.error().message
								  << std::endl;
					}

					// Phase 2: Main work loop - continues until stop requested and no more work
					while (!lifecycle_.is_stop_requested() || should_continue_work())
					{
						// Update thread state to indicate it's waiting for work
						lifecycle_.set_state(thread_conditions::Waiting);

						// Get current wake interval with thread-safe access
						auto interval = get_wake_interval();

						// Use lifecycle_controller for condition variable operations
						auto lock = lifecycle_.acquire_lock();

						// Wait strategy depends on whether wake interval is configured
						if (interval.has_value())
						{
							// Timed wait: wake up after interval OR when condition is met
							lifecycle_.wait_for(lock, interval.value(),
								[this]() { return should_continue_work(); });
						}
						else
						{
							// Indefinite wait: only wake up when condition is met
							lifecycle_.wait(lock,
								[this]() { return should_continue_work(); });
						}

						// Check if we should exit the loop
						if (lifecycle_.is_stop_requested() && !should_continue_work())
						{
							// Update state to indicate graceful shutdown in progress
							lifecycle_.set_state(thread_conditions::Stopping);
							break;
						}

						// Execute the actual work with exception protection
						try
						{
							// Update state to indicate active work is being performed
							lifecycle_.set_state(thread_conditions::Working);

							// Call derived class work implementation
							work_result = do_work();
							if (work_result.is_err())
							{
								// Use structured logger instead of raw std::cerr
								kcenon::thread::thread_logger::instance().log(
									kcenon::thread::log_level::error,
									thread_title_,
									"Work execution failed",
									work_result.error().message);
							}

							// Reset consecutive failures counter on successful completion
							consecutive_failures_.store(0, std::memory_order_relaxed);
						}
						catch (const std::exception& e)
						{
							// Track consecutive failures to prevent infinite error loops
							int failures = consecutive_failures_.fetch_add(1, std::memory_order_relaxed) + 1;

							kcenon::thread::thread_logger::instance().log(
								kcenon::thread::log_level::error,
								thread_title_,
								"Unhandled exception in worker thread (failure " + std::to_string(failures) + ")",
								e.what());

							// Stop thread if too many consecutive failures occur
							if (failures >= max_consecutive_failures)
							{
								kcenon::thread::thread_logger::instance().log(
									kcenon::thread::log_level::critical,
									thread_title_,
									"Too many consecutive failures, stopping thread",
									"");
								break;  // Exit the main loop
							}

							// Exponential backoff: 100ms, 200ms, 400ms, ..., max 10 seconds
							// This prevents CPU spinning and log flooding while giving time for transient issues to resolve
							auto backoff_ms = std::min(100 * (1 << std::min(failures - 1, 10)), 10000);
							std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
						}
					}

					// Phase 3: Call derived class cleanup hook after main loop exits
					work_result = after_stop();
					if (work_result.is_err())
					{
						kcenon::thread::thread_logger::instance().log(
							kcenon::thread::log_level::error,
							thread_title_,
							"Error during cleanup",
							work_result.error().message);
					}
				});  // End of lambda function passed to thread constructor
		}
		catch (const std::bad_alloc& e)
		{
			// Exception-safe cleanup: reset all resources if thread creation fails
			lifecycle_.reset_stop_source();
			worker_thread_.reset();

			return common::error_info{static_cast<int>(error_code::resource_allocation_failed), e.what(), "thread_system"};
		}

		// Thread creation successful
		return common::ok();
	}

	/**
	 * @brief Stops the worker thread and waits for it to complete.
	 *
	 * Implementation details:
	 * - This method is idempotent - safe to call multiple times
	 * - First checks if there's actually a thread to stop
	 * - Uses lifecycle_controller for stop signaling and state management
	 * - Notifies condition variable to wake up waiting thread
	 * - Joins the thread to wait for complete shutdown
	 * - Cleans up all thread-related resources
	 *
	 * Shutdown Sequence:
	 * 1. Signal stop request via lifecycle_controller
	 * 2. Call derived class hook for cancellation propagation
	 * 3. Notify condition variable to wake sleeping thread
	 * 4. Wait for thread to exit its main loop and complete after_stop()
	 * 5. Clean up thread object and reset lifecycle_controller
	 *
	 * @return Empty result on success, error if thread wasn't running
	 */
	auto thread_base::stop(void) -> common::VoidResult
	{
		// Early exit if no thread to stop (idempotent behavior)
		if (worker_thread_ == nullptr)
		{
			return common::error_info{static_cast<int>(error_code::thread_not_running), "thread is not running", "thread_system"};
		}

		// Only attempt to stop if thread is actually joinable
		if (worker_thread_->joinable())
		{
			// Self-stop detection: prevent deadlock if thread tries to stop itself
			// Calling join() from the same thread would cause deadlock
			if (worker_thread_->get_id() == std::this_thread::get_id())
			{
				return common::error_info{static_cast<int>(error_code::invalid_argument),
					"cannot stop thread from within itself - would cause deadlock", "thread_system"};
			}

			// Step 1: Signal the thread to stop via lifecycle_controller
			lifecycle_.request_stop();

			// Step 1.5: Call derived class hook for cancellation propagation
			// This allows derived classes (e.g., thread_worker) to cancel running jobs
			on_stop_requested();

			// Step 2: Wake up the thread if it's waiting on condition variable
			lifecycle_.notify_all();

			// Step 3: Wait for the thread to complete its shutdown sequence
			worker_thread_->join();  // Blocks until thread exits
		}

		// Step 4: Clean up thread resources
		lifecycle_.reset_stop_source();
		worker_thread_.reset();  // Release thread object

		// Step 5: Update thread state to indicate complete shutdown
		lifecycle_.set_stopped();

		return common::ok();
	}

	/**
	 * @brief Checks if the worker thread is currently active.
	 *
	 * Implementation details:
	 * - Uses lifecycle_controller for state checking
	 * - Considers both Working and Waiting states as "running"
	 * - Thread-safe operation
	 *
	 * @return true if thread is actively running (Working or Waiting)
	 */
	auto thread_base::is_running(void) const -> bool
	{
		return lifecycle_.is_running();
	}

	/**
	 * @brief Provides a string representation of the thread's current state.
	 *
	 * Implementation details:
	 * - Uses the formatter utility to create consistent output format
	 * - Includes both thread title and current condition
	 * - Useful for logging and debugging purposes
	 * - Thread-safe via lifecycle_controller
	 *
	 * @return Formatted string showing thread title and current state
	 */
	auto thread_base::to_string(void) const -> std::string
	{
		return utility_module::formatter::format("{} is {}", thread_title_, lifecycle_.get_state());
	}

	/**
	 * @brief Gets the native thread ID of the worker thread.
	 *
	 * Implementation details:
	 * - Returns the std::thread::id from the underlying thread object
	 * - Returns default-constructed (empty) id if thread is not running
	 * - Thread-safe as worker_thread_ pointer is only modified in start()/stop()
	 *
	 * @return The std::thread::id of the worker thread
	 */
	auto thread_base::get_thread_id() const -> std::thread::id
	{
		if (worker_thread_ && worker_thread_->joinable())
		{
			return worker_thread_->get_id();
		}
		return std::thread::id{};
	}
} // namespace kcenon::thread