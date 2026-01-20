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

#include "thread_conditions.h"

#include <mutex>
#include <atomic>
#include <chrono>
#include <optional>
#include <condition_variable>

#ifdef USE_STD_JTHREAD
#include <stop_token>
#endif

namespace kcenon::thread
{
	/**
	 * @class lifecycle_controller
	 * @brief Centralized thread lifecycle state and synchronization management.
	 *
	 * @ingroup core_threading
	 *
	 * The @c lifecycle_controller class consolidates duplicated thread lifecycle
	 * management patterns (start, stop, state transitions, condition variables)
	 * into a single reusable component. Thread classes can use composition with
	 * this controller instead of implementing these patterns themselves.
	 *
	 * ### Key Features
	 * - Thread state management (Created, Waiting, Working, Stopping, Stopped)
	 * - Condition variable signaling for wake-ups
	 * - Stop request handling (supports both std::jthread and legacy std::thread)
	 * - Thread-safe state queries and transitions
	 *
	 * ### Thread Safety
	 * All public methods are thread-safe. The class uses internal synchronization
	 * to protect state transitions and condition variable operations.
	 *
	 * ### Example Usage
	 * @code
	 * class my_thread {
	 * private:
	 *     lifecycle_controller lifecycle_;
	 *
	 * public:
	 *     void start() {
	 *         lifecycle_.initialize_for_start();
	 *         // spawn thread...
	 *     }
	 *
	 *     void stop() {
	 *         lifecycle_.request_stop();
	 *         lifecycle_.notify_all();
	 *         // join thread...
	 *         lifecycle_.set_stopped();
	 *     }
	 *
	 *     void worker_loop() {
	 *         while (!lifecycle_.is_stop_requested() || has_work()) {
	 *             lifecycle_.set_state(thread_conditions::Waiting);
	 *             lifecycle_.wait_for(timeout, [this] { return has_work(); });
	 *             lifecycle_.set_state(thread_conditions::Working);
	 *             do_work();
	 *         }
	 *     }
	 * };
	 * @endcode
	 */
	class lifecycle_controller
	{
	public:
		lifecycle_controller(const lifecycle_controller&) = delete;
		lifecycle_controller& operator=(const lifecycle_controller&) = delete;
		lifecycle_controller(lifecycle_controller&&) = delete;
		lifecycle_controller& operator=(lifecycle_controller&&) = delete;

		/**
		 * @brief Constructs a new lifecycle_controller in Created state.
		 */
		lifecycle_controller();

		/**
		 * @brief Destructor.
		 */
		~lifecycle_controller() = default;

		// =========================================================================
		// State Management
		// =========================================================================

		/**
		 * @brief Gets the current thread condition/state.
		 * @return The current thread_conditions value.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 * - Uses atomic load with acquire memory ordering
		 */
		[[nodiscard]] auto get_state() const noexcept -> thread_conditions;

		/**
		 * @brief Sets the thread condition/state.
		 * @param state The new thread_conditions value.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 * - Uses atomic store with release memory ordering
		 */
		auto set_state(thread_conditions state) noexcept -> void;

		/**
		 * @brief Checks if the thread is currently running.
		 * @return true if state is Working or Waiting, false otherwise.
		 */
		[[nodiscard]] auto is_running() const noexcept -> bool;

		/**
		 * @brief Marks the thread as stopped.
		 *
		 * Convenience method equivalent to set_state(thread_conditions::Stopped).
		 */
		auto set_stopped() noexcept -> void;

		// =========================================================================
		// Stop Request Management
		// =========================================================================

		/**
		 * @brief Initializes the controller for a new thread start.
		 *
		 * Resets the stop request flag and prepares for a new thread lifecycle.
		 * In C++20 jthread mode, creates a new stop_source.
		 *
		 * @note Must be called before spawning the worker thread.
		 */
		auto initialize_for_start() -> void;

		/**
		 * @brief Requests the thread to stop.
		 *
		 * In C++20 mode, calls request_stop() on the stop_source.
		 * In legacy mode, sets the atomic stop_requested_ flag to true.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 * - The request is visible to the worker thread immediately
		 */
		auto request_stop() noexcept -> void;

		/**
		 * @brief Checks if a stop has been requested.
		 * @return true if stop has been requested, false otherwise.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 */
		[[nodiscard]] auto is_stop_requested() const noexcept -> bool;

		/**
		 * @brief Checks if the controller has an active stop source (C++20 only).
		 * @return true if a stop_source is active, false otherwise.
		 *
		 * In legacy mode, checks if stop has NOT been requested (indicating active state).
		 */
		[[nodiscard]] auto has_active_source() const noexcept -> bool;

		/**
		 * @brief Resets the stop control mechanism after thread completion.
		 *
		 * In C++20 mode, resets the stop_source.
		 * Should be called after thread join to clean up resources.
		 */
		auto reset_stop_source() noexcept -> void;

		// =========================================================================
		// Condition Variable Operations
		// =========================================================================

		/**
		 * @brief Acquires a unique lock on the condition variable mutex.
		 * @return A unique_lock holding the mutex.
		 *
		 * Use this to prepare for wait operations.
		 */
		[[nodiscard]] auto acquire_lock() -> std::unique_lock<std::mutex>;

		/**
		 * @brief Waits on the condition variable with a predicate.
		 * @tparam Predicate A callable returning bool.
		 * @param lock The unique_lock (must be holding cv_mutex_).
		 * @param pred The predicate to check.
		 *
		 * Waits until pred() returns true OR stop is requested.
		 */
		template<typename Predicate>
		auto wait(std::unique_lock<std::mutex>& lock, Predicate pred) -> void
		{
#ifdef USE_STD_JTHREAD
			if (stop_source_.has_value())
			{
				auto stop_token = stop_source_.value().get_token();
				condition_.wait(lock, [this, &stop_token, &pred]() {
					return stop_token.stop_requested() || pred();
				});
			}
			else
			{
				condition_.wait(lock, pred);
			}
#else
			condition_.wait(lock, [this, &pred]() {
				return stop_requested_.load(std::memory_order_acquire) || pred();
			});
#endif
		}

		/**
		 * @brief Waits on the condition variable with a timeout and predicate.
		 * @tparam Rep Duration rep type.
		 * @tparam Period Duration period type.
		 * @tparam Predicate A callable returning bool.
		 * @param lock The unique_lock (must be holding cv_mutex_).
		 * @param timeout The maximum duration to wait.
		 * @param pred The predicate to check.
		 * @return true if pred() is satisfied, false if timed out.
		 */
		template<typename Rep, typename Period, typename Predicate>
		auto wait_for(std::unique_lock<std::mutex>& lock,
		              const std::chrono::duration<Rep, Period>& timeout,
		              Predicate pred) -> bool
		{
#ifdef USE_STD_JTHREAD
			if (stop_source_.has_value())
			{
				auto stop_token = stop_source_.value().get_token();
				return condition_.wait_for(lock, timeout, [this, &stop_token, &pred]() {
					return stop_token.stop_requested() || pred();
				});
			}
			else
			{
				return condition_.wait_for(lock, timeout, pred);
			}
#else
			return condition_.wait_for(lock, timeout, [this, &pred]() {
				return stop_requested_.load(std::memory_order_acquire) || pred();
			});
#endif
		}

		/**
		 * @brief Notifies one waiting thread.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 */
		auto notify_one() -> void;

		/**
		 * @brief Notifies all waiting threads.
		 *
		 * Thread Safety:
		 * - Safe to call from any thread
		 */
		auto notify_all() -> void;

	private:
		/// Mutex for condition variable operations.
		std::mutex cv_mutex_;

		/// Condition variable for thread signaling.
		std::condition_variable condition_;

		/// Current thread state.
		std::atomic<thread_conditions> state_{thread_conditions::Created};

#ifdef USE_STD_JTHREAD
		/// Stop source for cooperative cancellation (C++20).
		std::optional<std::stop_source> stop_source_;
#else
		/// Atomic flag for stop request (legacy mode).
		std::atomic<bool> stop_requested_{false};
#endif
	};

} // namespace kcenon::thread
