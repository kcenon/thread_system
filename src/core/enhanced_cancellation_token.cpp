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

#include <kcenon/thread/core/enhanced_cancellation_token.h>

#include <algorithm>

namespace kcenon::thread
{
	// ========================================================================
	// Internal state structure
	// ========================================================================

	struct enhanced_cancellation_token::state
	{
		std::atomic<bool> is_cancelled{false};
		std::optional<cancellation_reason> reason;

		std::unordered_map<callback_handle, callback_type> simple_callbacks;
		std::unordered_map<callback_handle, callback_with_reason_type>
			reason_callbacks;
		std::atomic<callback_handle> next_handle{1};

		std::chrono::steady_clock::time_point deadline_point{
			std::chrono::steady_clock::time_point::max()};
		std::atomic<bool> has_deadline{false};

		mutable std::mutex mutex;
		mutable std::condition_variable cv;

		std::atomic<bool> timer_active{false};
		std::atomic<bool> timer_should_stop{false};
	};

	// ========================================================================
	// enhanced_cancellation_token implementation
	// ========================================================================

	enhanced_cancellation_token::enhanced_cancellation_token()
		: state_(std::make_shared<state>())
	{
	}

	enhanced_cancellation_token::enhanced_cancellation_token(
		std::shared_ptr<state> state)
		: state_(std::move(state))
	{
	}

	enhanced_cancellation_token::~enhanced_cancellation_token()
	{
		// If this is the last reference and timer is active, signal it to stop
		if (state_ && state_.use_count() == 1)
		{
			state_->timer_should_stop.store(true, std::memory_order_release);
			state_->cv.notify_all();
		}
	}

	auto enhanced_cancellation_token::create() -> enhanced_cancellation_token
	{
		return enhanced_cancellation_token();
	}

	auto enhanced_cancellation_token::create_with_timeout(
		std::chrono::milliseconds timeout) -> enhanced_cancellation_token
	{
		auto deadline_point = std::chrono::steady_clock::now() + timeout;
		return create_with_deadline(deadline_point);
	}

	auto enhanced_cancellation_token::create_with_deadline(
		std::chrono::steady_clock::time_point deadline_point)
		-> enhanced_cancellation_token
	{
		auto token = create();
		token.state_->deadline_point = deadline_point;
		token.state_->has_deadline.store(true, std::memory_order_release);

		// Start timeout timer in background
		std::weak_ptr<state> state_weak = token.state_;
		start_timeout_timer(state_weak, deadline_point);

		return token;
	}

	auto enhanced_cancellation_token::create_linked(
		std::initializer_list<enhanced_cancellation_token> tokens)
		-> enhanced_cancellation_token
	{
		auto new_token = create();
		std::weak_ptr<state> new_state_weak = new_token.state_;

		for (const auto& parent : tokens)
		{
			auto parent_copy = parent;
			parent_copy.register_callback(
				[new_state_weak](const cancellation_reason& /*parent_reason*/)
				{
					if (auto s = new_state_weak.lock())
					{
						std::vector<callback_type> simple_to_invoke;
						std::vector<callback_with_reason_type> reason_to_invoke;
						cancellation_reason new_reason;

						{
							std::lock_guard<std::mutex> lock(s->mutex);
							bool was_cancelled =
								s->is_cancelled.exchange(true, std::memory_order_release);
							if (!was_cancelled)
							{
								new_reason.reason_type =
									cancellation_reason::type::parent_cancelled;
								new_reason.message = "Parent token was cancelled";
								new_reason.cancel_time = std::chrono::steady_clock::now();
								s->reason = new_reason;

								for (auto& [handle, cb] : s->simple_callbacks)
								{
									simple_to_invoke.push_back(std::move(cb));
								}
								s->simple_callbacks.clear();

								for (auto& [handle, cb] : s->reason_callbacks)
								{
									reason_to_invoke.push_back(std::move(cb));
								}
								s->reason_callbacks.clear();
							}
						}

						s->cv.notify_all();

						for (const auto& cb : simple_to_invoke)
						{
							cb();
						}
						for (const auto& cb : reason_to_invoke)
						{
							cb(new_reason);
						}
					}
				});
		}

		return new_token;
	}

	auto enhanced_cancellation_token::create_linked_with_timeout(
		const enhanced_cancellation_token& parent,
		std::chrono::milliseconds timeout) -> enhanced_cancellation_token
	{
		auto deadline_point = std::chrono::steady_clock::now() + timeout;

		auto token = create();
		token.state_->deadline_point = deadline_point;
		token.state_->has_deadline.store(true, std::memory_order_release);

		// Link to parent
		std::weak_ptr<state> token_state_weak = token.state_;
		auto parent_copy = parent;
		parent_copy.register_callback(
			[token_state_weak](const cancellation_reason& /*parent_reason*/)
			{
				if (auto s = token_state_weak.lock())
				{
					std::vector<callback_type> simple_to_invoke;
					std::vector<callback_with_reason_type> reason_to_invoke;
					cancellation_reason new_reason;

					{
						std::lock_guard<std::mutex> lock(s->mutex);
						bool was_cancelled =
							s->is_cancelled.exchange(true, std::memory_order_release);
						if (!was_cancelled)
						{
							new_reason.reason_type =
								cancellation_reason::type::parent_cancelled;
							new_reason.message = "Parent token was cancelled";
							new_reason.cancel_time = std::chrono::steady_clock::now();
							s->reason = new_reason;

							for (auto& [handle, cb] : s->simple_callbacks)
							{
								simple_to_invoke.push_back(std::move(cb));
							}
							s->simple_callbacks.clear();

							for (auto& [handle, cb] : s->reason_callbacks)
							{
								reason_to_invoke.push_back(std::move(cb));
							}
							s->reason_callbacks.clear();

							s->timer_should_stop.store(true, std::memory_order_release);
						}
					}

					s->cv.notify_all();

					for (const auto& cb : simple_to_invoke)
					{
						cb();
					}
					for (const auto& cb : reason_to_invoke)
					{
						cb(new_reason);
					}
				}
			});

		// Start timeout timer
		start_timeout_timer(token_state_weak, deadline_point);

		return token;
	}

	auto enhanced_cancellation_token::cancel() -> void
	{
		do_cancel(cancellation_reason::type::user_requested, "", std::nullopt);
	}

	auto enhanced_cancellation_token::cancel(const std::string& message) -> void
	{
		do_cancel(cancellation_reason::type::user_requested, message, std::nullopt);
	}

	auto enhanced_cancellation_token::cancel(std::exception_ptr ex) -> void
	{
		do_cancel(cancellation_reason::type::error, "", ex);
	}

	auto enhanced_cancellation_token::do_cancel(cancellation_reason::type reason_type,
												const std::string& message,
												std::optional<std::exception_ptr> ex)
		-> void
	{
		std::vector<callback_type> simple_to_invoke;
		std::vector<callback_with_reason_type> reason_to_invoke;
		cancellation_reason new_reason;

		{
			std::lock_guard<std::mutex> lock(state_->mutex);
			bool was_cancelled =
				state_->is_cancelled.exchange(true, std::memory_order_release);
			if (!was_cancelled)
			{
				new_reason.reason_type = reason_type;
				new_reason.message = message;
				new_reason.cancel_time = std::chrono::steady_clock::now();
				new_reason.exception = ex;
				state_->reason = new_reason;

				for (auto& [handle, cb] : state_->simple_callbacks)
				{
					simple_to_invoke.push_back(std::move(cb));
				}
				state_->simple_callbacks.clear();

				for (auto& [handle, cb] : state_->reason_callbacks)
				{
					reason_to_invoke.push_back(std::move(cb));
				}
				state_->reason_callbacks.clear();

				state_->timer_should_stop.store(true, std::memory_order_release);
			}
		}

		state_->cv.notify_all();

		for (const auto& cb : simple_to_invoke)
		{
			cb();
		}
		for (const auto& cb : reason_to_invoke)
		{
			cb(new_reason);
		}
	}

	auto enhanced_cancellation_token::is_cancelled() const -> bool
	{
		return state_->is_cancelled.load(std::memory_order_acquire);
	}

	auto enhanced_cancellation_token::is_cancellation_requested() const -> bool
	{
		return is_cancelled();
	}

	auto enhanced_cancellation_token::get_reason() const
		-> std::optional<cancellation_reason>
	{
		std::lock_guard<std::mutex> lock(state_->mutex);
		return state_->reason;
	}

	auto enhanced_cancellation_token::throw_if_cancelled() const -> void
	{
		if (is_cancelled())
		{
			auto reason = get_reason();
			if (reason)
			{
				throw operation_cancelled_exception(*reason);
			}
			throw operation_cancelled_exception(
				cancellation_reason{cancellation_reason::type::user_requested,
									"Operation cancelled",
									std::chrono::steady_clock::now(),
									std::nullopt});
		}
	}

	auto enhanced_cancellation_token::has_timeout() const -> bool
	{
		return state_->has_deadline.load(std::memory_order_acquire);
	}

	auto enhanced_cancellation_token::remaining_time() const
		-> std::chrono::milliseconds
	{
		if (!has_timeout())
		{
			return std::chrono::milliseconds::max();
		}

		auto now = std::chrono::steady_clock::now();
		auto deadline_point = state_->deadline_point;

		if (now >= deadline_point)
		{
			return std::chrono::milliseconds::zero();
		}

		return std::chrono::duration_cast<std::chrono::milliseconds>(deadline_point -
																	 now);
	}

	auto enhanced_cancellation_token::deadline() const
		-> std::chrono::steady_clock::time_point
	{
		return state_->deadline_point;
	}

	auto enhanced_cancellation_token::extend_timeout(
		std::chrono::milliseconds additional) -> void
	{
		std::lock_guard<std::mutex> lock(state_->mutex);
		if (state_->has_deadline.load(std::memory_order_acquire))
		{
			state_->deadline_point += additional;
		}
	}

	auto enhanced_cancellation_token::register_callback(callback_type callback)
		-> callback_handle
	{
		std::unique_lock<std::mutex> lock(state_->mutex);

		if (state_->is_cancelled.load(std::memory_order_acquire))
		{
			lock.unlock();
			callback();
			return 0;
		}

		callback_handle handle =
			state_->next_handle.fetch_add(1, std::memory_order_relaxed);
		state_->simple_callbacks[handle] = std::move(callback);
		return handle;
	}

	auto enhanced_cancellation_token::register_callback(
		callback_with_reason_type callback) -> callback_handle
	{
		std::unique_lock<std::mutex> lock(state_->mutex);

		if (state_->is_cancelled.load(std::memory_order_acquire))
		{
			auto reason = state_->reason;
			lock.unlock();
			if (reason)
			{
				callback(*reason);
			}
			else
			{
				callback(cancellation_reason{cancellation_reason::type::user_requested,
											 "Operation cancelled",
											 std::chrono::steady_clock::now(),
											 std::nullopt});
			}
			return 0;
		}

		callback_handle handle =
			state_->next_handle.fetch_add(1, std::memory_order_relaxed);
		state_->reason_callbacks[handle] = std::move(callback);
		return handle;
	}

	auto enhanced_cancellation_token::unregister_callback(callback_handle handle)
		-> void
	{
		if (handle == 0)
		{
			return;
		}

		std::lock_guard<std::mutex> lock(state_->mutex);
		state_->simple_callbacks.erase(handle);
		state_->reason_callbacks.erase(handle);
	}

	auto enhanced_cancellation_token::wait() const -> void
	{
		std::unique_lock<std::mutex> lock(state_->mutex);
		state_->cv.wait(
			lock, [this]
			{ return state_->is_cancelled.load(std::memory_order_acquire); });
	}

	auto enhanced_cancellation_token::wait_for(std::chrono::milliseconds timeout) const
		-> bool
	{
		std::unique_lock<std::mutex> lock(state_->mutex);
		return state_->cv.wait_for(
			lock, timeout,
			[this]
			{ return state_->is_cancelled.load(std::memory_order_acquire); });
	}

	auto enhanced_cancellation_token::wait_until(
		std::chrono::steady_clock::time_point deadline_point) const -> bool
	{
		std::unique_lock<std::mutex> lock(state_->mutex);
		return state_->cv.wait_until(
			lock, deadline_point,
			[this]
			{ return state_->is_cancelled.load(std::memory_order_acquire); });
	}

	auto enhanced_cancellation_token::start_timeout_timer(
		std::weak_ptr<state> state_weak,
		std::chrono::steady_clock::time_point deadline_point) -> void
	{
		std::thread timer_thread(
			[state_weak, deadline_point]()
			{
				auto s = state_weak.lock();
				if (!s)
				{
					return;
				}

				s->timer_active.store(true, std::memory_order_release);

				std::unique_lock<std::mutex> lock(s->mutex);

				// Wait until deadline or cancellation
				auto result = s->cv.wait_until(
					lock, deadline_point,
					[&s]
					{
						return s->is_cancelled.load(std::memory_order_acquire) ||
							   s->timer_should_stop.load(std::memory_order_acquire);
					});

				// If not cancelled and deadline reached, cancel with timeout reason
				if (!result && !s->is_cancelled.load(std::memory_order_acquire))
				{
					std::vector<callback_type> simple_to_invoke;
					std::vector<callback_with_reason_type> reason_to_invoke;
					cancellation_reason new_reason;

					bool was_cancelled =
						s->is_cancelled.exchange(true, std::memory_order_release);
					if (!was_cancelled)
					{
						new_reason.reason_type = cancellation_reason::type::timeout;
						new_reason.message = "Timeout expired";
						new_reason.cancel_time = std::chrono::steady_clock::now();
						s->reason = new_reason;

						for (auto& [handle, cb] : s->simple_callbacks)
						{
							simple_to_invoke.push_back(std::move(cb));
						}
						s->simple_callbacks.clear();

						for (auto& [handle, cb] : s->reason_callbacks)
						{
							reason_to_invoke.push_back(std::move(cb));
						}
						s->reason_callbacks.clear();
					}

					lock.unlock();
					s->cv.notify_all();

					for (const auto& cb : simple_to_invoke)
					{
						cb();
					}
					for (const auto& cb : reason_to_invoke)
					{
						cb(new_reason);
					}
				}
				else
				{
					lock.unlock();
				}

				s->timer_active.store(false, std::memory_order_release);
			});

		timer_thread.detach();
	}

	// ========================================================================
	// cancellation_callback_guard implementation
	// ========================================================================

	cancellation_callback_guard::cancellation_callback_guard(
		enhanced_cancellation_token& token,
		std::function<void()> callback)
		: token_(&token), handle_(token.register_callback(std::move(callback)))
	{
	}

	cancellation_callback_guard::~cancellation_callback_guard()
	{
		if (token_ && handle_ != 0)
		{
			token_->unregister_callback(handle_);
		}
	}

	cancellation_callback_guard::cancellation_callback_guard(
		cancellation_callback_guard&& other) noexcept
		: token_(other.token_), handle_(other.handle_)
	{
		other.token_ = nullptr;
		other.handle_ = 0;
	}

	auto cancellation_callback_guard::operator=(
		cancellation_callback_guard&& other) noexcept -> cancellation_callback_guard&
	{
		if (this != &other)
		{
			if (token_ && handle_ != 0)
			{
				token_->unregister_callback(handle_);
			}
			token_ = other.token_;
			handle_ = other.handle_;
			other.token_ = nullptr;
			other.handle_ = 0;
		}
		return *this;
	}

	// ========================================================================
	// cancellation_scope implementation
	// ========================================================================

	cancellation_scope::cancellation_scope(enhanced_cancellation_token token)
		: token_(std::move(token))
	{
	}

	auto cancellation_scope::is_cancelled() const -> bool
	{
		return token_.is_cancelled();
	}

	auto cancellation_scope::check_cancelled() const -> void
	{
		token_.throw_if_cancelled();
	}

	auto cancellation_scope::token() const -> const enhanced_cancellation_token&
	{
		return token_;
	}

	// ========================================================================
	// cancellation_context implementation
	// ========================================================================

	namespace
	{
		thread_local std::vector<enhanced_cancellation_token> context_stack;
	}

	auto cancellation_context::current() -> enhanced_cancellation_token
	{
		if (context_stack.empty())
		{
			return enhanced_cancellation_token::create();
		}
		return context_stack.back();
	}

	auto cancellation_context::push(enhanced_cancellation_token token) -> void
	{
		context_stack.push_back(std::move(token));
	}

	auto cancellation_context::pop() -> void
	{
		if (!context_stack.empty())
		{
			context_stack.pop_back();
		}
	}

	cancellation_context::guard::guard(enhanced_cancellation_token token)
		: pushed_(true)
	{
		cancellation_context::push(std::move(token));
	}

	cancellation_context::guard::~guard()
	{
		if (pushed_)
		{
			cancellation_context::pop();
		}
	}

} // namespace kcenon::thread
