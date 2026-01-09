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

/**
 * @file enhanced_cancellation_token.h
 * @brief Enhanced cancellation token with timeout, deadline, and reason support.
 */

#include "cancellation_exception.h"
#include "cancellation_reason.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

namespace kcenon::thread
{
	// Forward declarations
	class enhanced_cancellation_token;
	class cancellation_callback_guard;
	class cancellation_scope;
	class cancellation_context;

	/**
	 * @class enhanced_cancellation_token
	 * @brief Advanced cancellation token with timeout, deadline, and reason support.
	 *
	 * @ingroup cancellation
	 *
	 * Extends the basic cancellation_token with additional features:
	 * - Timeout-based automatic cancellation
	 * - Deadline-based automatic cancellation
	 * - Cancellation reason tracking
	 * - Hierarchical token linking
	 * - Callback registration with handles
	 * - Wait methods with timeout support
	 *
	 * ### Design Principles
	 * - **Thread Safe**: All operations are safe for concurrent access
	 * - **Efficient**: Minimal overhead when not cancelled
	 * - **Composable**: Tokens can be linked to create hierarchies
	 * - **Observable**: Callbacks notify when cancellation occurs
	 *
	 * ### Usage Example
	 * @code
	 * // Create token with 30 second timeout
	 * auto token = enhanced_cancellation_token::create_with_timeout(
	 *     std::chrono::seconds{30});
	 *
	 * // Use in a worker loop
	 * while (!token.is_cancelled()) {
	 *     do_work_chunk();
	 * }
	 *
	 * // Check reason if cancelled
	 * if (auto reason = token.get_reason()) {
	 *     std::cout << "Cancelled: " << reason->to_string() << std::endl;
	 * }
	 * @endcode
	 *
	 * @see cancellation_token For basic cancellation functionality
	 * @see cancellation_reason For cancellation reason details
	 */
	class enhanced_cancellation_token
	{
	public:
		// ====================================================================
		// Type definitions
		// ====================================================================

		/// Callback handle type for registration management.
		using callback_handle = std::size_t;

		/// Simple callback function type.
		using callback_type = std::function<void()>;

		/// Callback function type with reason parameter.
		using callback_with_reason_type =
			std::function<void(const cancellation_reason&)>;

		// ====================================================================
		// Construction
		// ====================================================================

		/**
		 * @brief Default constructor creates a new token.
		 */
		enhanced_cancellation_token();

		/**
		 * @brief Copy constructor (shares state).
		 */
		enhanced_cancellation_token(const enhanced_cancellation_token&) = default;

		/**
		 * @brief Move constructor.
		 */
		enhanced_cancellation_token(enhanced_cancellation_token&&) noexcept = default;

		/**
		 * @brief Copy assignment (shares state).
		 */
		auto operator=(const enhanced_cancellation_token&)
			-> enhanced_cancellation_token& = default;

		/**
		 * @brief Move assignment.
		 */
		auto operator=(enhanced_cancellation_token&&) noexcept
			-> enhanced_cancellation_token& = default;

		/**
		 * @brief Destructor.
		 */
		~enhanced_cancellation_token();

		/**
		 * @brief Creates a new cancellation token.
		 * @return A new enhanced_cancellation_token.
		 */
		[[nodiscard]] static auto create() -> enhanced_cancellation_token;

		/**
		 * @brief Creates a token that auto-cancels after the specified timeout.
		 * @param timeout Duration after which the token auto-cancels.
		 * @return A new token with timeout.
		 *
		 * The token will automatically cancel with reason_type::timeout when
		 * the specified duration elapses from the creation time.
		 */
		[[nodiscard]] static auto create_with_timeout(std::chrono::milliseconds timeout)
			-> enhanced_cancellation_token;

		/**
		 * @brief Creates a token that auto-cancels at the specified deadline.
		 * @param deadline Time point at which the token auto-cancels.
		 * @return A new token with deadline.
		 *
		 * The token will automatically cancel with reason_type::deadline when
		 * the specified time point is reached.
		 */
		[[nodiscard]] static auto create_with_deadline(
			std::chrono::steady_clock::time_point deadline) -> enhanced_cancellation_token;

		/**
		 * @brief Creates a token linked to parent tokens.
		 * @param tokens Parent tokens to link with.
		 * @return A new linked token.
		 *
		 * The new token will be cancelled when any of the parent tokens are
		 * cancelled, with reason_type::parent_cancelled.
		 */
		[[nodiscard]] static auto create_linked(
			std::initializer_list<enhanced_cancellation_token> tokens)
			-> enhanced_cancellation_token;

		/**
		 * @brief Creates a linked token with additional timeout.
		 * @param parent Parent token to link with.
		 * @param timeout Additional timeout duration.
		 * @return A new linked token with timeout.
		 *
		 * The token cancels when either:
		 * - The parent token is cancelled (reason: parent_cancelled)
		 * - The timeout expires (reason: timeout)
		 */
		[[nodiscard]] static auto create_linked_with_timeout(
			const enhanced_cancellation_token& parent,
			std::chrono::milliseconds timeout) -> enhanced_cancellation_token;

		// ====================================================================
		// Cancellation
		// ====================================================================

		/**
		 * @brief Cancels the token.
		 *
		 * Sets the token to cancelled state with reason_type::user_requested
		 * and invokes all registered callbacks.
		 */
		auto cancel() -> void;

		/**
		 * @brief Cancels the token with a message.
		 * @param message Human-readable reason for cancellation.
		 */
		auto cancel(const std::string& message) -> void;

		/**
		 * @brief Cancels the token with an exception.
		 * @param ex Exception that caused the cancellation.
		 *
		 * The cancellation reason_type will be set to error.
		 */
		auto cancel(std::exception_ptr ex) -> void;

		// ====================================================================
		// Query
		// ====================================================================

		/**
		 * @brief Checks if the token has been cancelled.
		 * @return true if cancelled, false otherwise.
		 */
		[[nodiscard]] auto is_cancelled() const -> bool;

		/**
		 * @brief Checks if cancellation has been requested.
		 * @return true if cancellation is requested, false otherwise.
		 *
		 * This is functionally equivalent to is_cancelled() for this
		 * implementation.
		 */
		[[nodiscard]] auto is_cancellation_requested() const -> bool;

		/**
		 * @brief Gets the cancellation reason.
		 * @return The cancellation reason if cancelled, std::nullopt otherwise.
		 */
		[[nodiscard]] auto get_reason() const -> std::optional<cancellation_reason>;

		/**
		 * @brief Throws if the token has been cancelled.
		 * @throws operation_cancelled_exception if cancelled.
		 */
		auto throw_if_cancelled() const -> void;

		// ====================================================================
		// Timeout/Deadline
		// ====================================================================

		/**
		 * @brief Checks if the token has a timeout or deadline.
		 * @return true if the token has a timeout/deadline, false otherwise.
		 */
		[[nodiscard]] auto has_timeout() const -> bool;

		/**
		 * @brief Gets the remaining time before timeout/deadline.
		 * @return Remaining duration, or zero if expired or no timeout.
		 */
		[[nodiscard]] auto remaining_time() const -> std::chrono::milliseconds;

		/**
		 * @brief Gets the deadline time point.
		 * @return The deadline if set, or time_point::max() if none.
		 */
		[[nodiscard]] auto deadline() const -> std::chrono::steady_clock::time_point;

		/**
		 * @brief Extends the timeout by the specified duration.
		 * @param additional Additional time to add to the deadline.
		 */
		auto extend_timeout(std::chrono::milliseconds additional) -> void;

		// ====================================================================
		// Callbacks
		// ====================================================================

		/**
		 * @brief Registers a callback to be invoked on cancellation.
		 * @param callback Function to call when cancelled.
		 * @return Handle for unregistering the callback.
		 *
		 * If already cancelled, the callback is invoked immediately.
		 */
		[[nodiscard]] auto register_callback(callback_type callback) -> callback_handle;

		/**
		 * @brief Registers a callback that receives the cancellation reason.
		 * @param callback Function to call with reason when cancelled.
		 * @return Handle for unregistering the callback.
		 *
		 * If already cancelled, the callback is invoked immediately.
		 */
		[[nodiscard]] auto register_callback(callback_with_reason_type callback)
			-> callback_handle;

		/**
		 * @brief Unregisters a previously registered callback.
		 * @param handle The handle returned from register_callback.
		 */
		auto unregister_callback(callback_handle handle) -> void;

		// ====================================================================
		// Waiting
		// ====================================================================

		/**
		 * @brief Waits until the token is cancelled.
		 *
		 * Blocks the calling thread until the token is cancelled.
		 */
		auto wait() const -> void;

		/**
		 * @brief Waits for cancellation with a timeout.
		 * @param timeout Maximum time to wait.
		 * @return true if cancelled, false if timeout expired.
		 */
		[[nodiscard]] auto wait_for(std::chrono::milliseconds timeout) const -> bool;

		/**
		 * @brief Waits for cancellation until a deadline.
		 * @param deadline Maximum time point to wait until.
		 * @return true if cancelled, false if deadline reached.
		 */
		[[nodiscard]] auto wait_until(std::chrono::steady_clock::time_point deadline) const
			-> bool;

	private:
		struct state;
		std::shared_ptr<state> state_;

		explicit enhanced_cancellation_token(std::shared_ptr<state> state);

		auto do_cancel(cancellation_reason::type reason_type,
					   const std::string& message,
					   std::optional<std::exception_ptr> ex) -> void;

		static auto start_timeout_timer(std::weak_ptr<state> state_weak,
										std::chrono::steady_clock::time_point deadline)
			-> void;
	};

	/**
	 * @class cancellation_callback_guard
	 * @brief RAII guard for automatic callback unregistration.
	 *
	 * @ingroup cancellation
	 *
	 * Automatically unregisters a callback when the guard goes out of scope.
	 * Useful for ensuring callbacks are properly cleaned up.
	 *
	 * ### Usage Example
	 * @code
	 * {
	 *     cancellation_callback_guard guard(token, [] {
	 *         cleanup_resources();
	 *     });
	 *
	 *     do_interruptible_work();
	 * }  // Callback auto-unregistered here
	 * @endcode
	 */
	class cancellation_callback_guard
	{
	public:
		/**
		 * @brief Constructs a guard and registers the callback.
		 * @param token The token to register with.
		 * @param callback The callback to register.
		 */
		cancellation_callback_guard(enhanced_cancellation_token& token,
									std::function<void()> callback);

		/**
		 * @brief Destructor unregisters the callback.
		 */
		~cancellation_callback_guard();

		// Non-copyable
		cancellation_callback_guard(const cancellation_callback_guard&) = delete;
		auto operator=(const cancellation_callback_guard&)
			-> cancellation_callback_guard& = delete;

		/**
		 * @brief Move constructor.
		 */
		cancellation_callback_guard(cancellation_callback_guard&& other) noexcept;

		/**
		 * @brief Move assignment.
		 */
		auto operator=(cancellation_callback_guard&& other) noexcept
			-> cancellation_callback_guard&;

	private:
		enhanced_cancellation_token* token_;
		enhanced_cancellation_token::callback_handle handle_;
	};

	/**
	 * @class cancellation_scope
	 * @brief Structured cancellation scope with check points.
	 *
	 * @ingroup cancellation
	 *
	 * Provides a convenient way to check for cancellation at various points
	 * in code execution.
	 *
	 * ### Usage Example
	 * @code
	 * void process_request(enhanced_cancellation_token token) {
	 *     cancellation_scope scope(token);
	 *
	 *     scope.check_cancelled();  // Throws if cancelled
	 *     step_1();
	 *
	 *     scope.check_cancelled();
	 *     step_2();
	 *
	 *     scope.check_cancelled();
	 *     step_3();
	 * }
	 * @endcode
	 */
	class cancellation_scope
	{
	public:
		/**
		 * @brief Constructs a scope with the given token.
		 * @param token The cancellation token to monitor.
		 */
		explicit cancellation_scope(enhanced_cancellation_token token);

		/**
		 * @brief Destructor.
		 */
		~cancellation_scope() = default;

		// Non-copyable, non-movable
		cancellation_scope(const cancellation_scope&) = delete;
		auto operator=(const cancellation_scope&) -> cancellation_scope& = delete;
		cancellation_scope(cancellation_scope&&) = delete;
		auto operator=(cancellation_scope&&) -> cancellation_scope& = delete;

		/**
		 * @brief Checks if the token is cancelled.
		 * @return true if cancelled, false otherwise.
		 */
		[[nodiscard]] auto is_cancelled() const -> bool;

		/**
		 * @brief Throws if the token is cancelled.
		 * @throws operation_cancelled_exception if cancelled.
		 */
		auto check_cancelled() const -> void;

		/**
		 * @brief Gets the underlying token.
		 * @return A const reference to the token.
		 */
		[[nodiscard]] auto token() const -> const enhanced_cancellation_token&;

	private:
		enhanced_cancellation_token token_;
	};

	/**
	 * @class cancellation_context
	 * @brief Thread-local cancellation context for implicit token propagation.
	 *
	 * @ingroup cancellation
	 *
	 * Provides a way to implicitly propagate cancellation tokens through
	 * the call stack using thread-local storage.
	 *
	 * ### Usage Example
	 * @code
	 * void outer_function(enhanced_cancellation_token token) {
	 *     cancellation_context::guard guard(token);
	 *
	 *     inner_function();  // Can access token via current()
	 * }
	 *
	 * void inner_function() {
	 *     auto token = cancellation_context::current();
	 *     if (token.is_cancelled()) {
	 *         return;
	 *     }
	 *     // do work
	 * }
	 * @endcode
	 */
	class cancellation_context
	{
	public:
		/**
		 * @brief Gets the current thread's cancellation token.
		 * @return The current token, or a new uncancelled token if none.
		 */
		[[nodiscard]] static auto current() -> enhanced_cancellation_token;

		/**
		 * @brief Pushes a token to the thread-local stack.
		 * @param token The token to push.
		 */
		static auto push(enhanced_cancellation_token token) -> void;

		/**
		 * @brief Pops a token from the thread-local stack.
		 */
		static auto pop() -> void;

		/**
		 * @class guard
		 * @brief RAII guard for push/pop operations.
		 */
		class guard
		{
		public:
			/**
			 * @brief Constructs a guard and pushes the token.
			 * @param token The token to push.
			 */
			explicit guard(enhanced_cancellation_token token);

			/**
			 * @brief Destructor pops the token.
			 */
			~guard();

			// Non-copyable, non-movable
			guard(const guard&) = delete;
			auto operator=(const guard&) -> guard& = delete;
			guard(guard&&) = delete;
			auto operator=(guard&&) -> guard& = delete;

		private:
			bool pushed_;
		};

	private:
		cancellation_context() = delete;
	};

} // namespace kcenon::thread
