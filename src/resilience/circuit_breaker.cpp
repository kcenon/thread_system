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

#include <kcenon/thread/resilience/circuit_breaker.h>

namespace kcenon::thread
{
	// ============================================
	// circuit_breaker::guard implementation
	// ============================================

	circuit_breaker::guard::guard(circuit_breaker& cb)
		: cb_(&cb)
		, allowed_(cb.allow_request())
		, recorded_(false)
	{
	}

	circuit_breaker::guard::~guard()
	{
		if (cb_ != nullptr && allowed_ && !recorded_)
		{
			// If not explicitly marked, assume failure
			cb_->record_failure(nullptr);
		}
	}

	circuit_breaker::guard::guard(guard&& other) noexcept
		: cb_(other.cb_)
		, allowed_(other.allowed_)
		, recorded_(other.recorded_)
	{
		other.cb_ = nullptr;
		other.recorded_ = true;  // Prevent double recording
	}

	circuit_breaker::guard& circuit_breaker::guard::operator=(guard&& other) noexcept
	{
		if (this != &other)
		{
			// Record failure for current state if needed
			if (cb_ != nullptr && allowed_ && !recorded_)
			{
				cb_->record_failure(nullptr);
			}

			cb_ = other.cb_;
			allowed_ = other.allowed_;
			recorded_ = other.recorded_;

			other.cb_ = nullptr;
			other.recorded_ = true;
		}
		return *this;
	}

	auto circuit_breaker::guard::is_allowed() const -> bool
	{
		return allowed_;
	}

	auto circuit_breaker::guard::mark_success() -> void
	{
		if (cb_ != nullptr && allowed_ && !recorded_)
		{
			cb_->record_success();
			recorded_ = true;
		}
	}

	auto circuit_breaker::guard::mark_failure(const std::exception* e) -> void
	{
		if (cb_ != nullptr && allowed_ && !recorded_)
		{
			cb_->record_failure(e);
			recorded_ = true;
		}
	}

	// ============================================
	// circuit_breaker implementation
	// ============================================

	circuit_breaker::circuit_breaker(circuit_breaker_config config)
		: config_(std::move(config))
		, window_(std::make_unique<failure_window>(config_.window_size))
		, state_change_time_(std::chrono::steady_clock::now())
	{
	}

	circuit_breaker::~circuit_breaker() = default;

	auto circuit_breaker::allow_request() -> bool
	{
		auto current_state = state_.load(std::memory_order_acquire);

		switch (current_state)
		{
			case circuit_state::closed:
				total_requests_.fetch_add(1, std::memory_order_relaxed);
				return true;

			case circuit_state::open:
			{
				// Check if we should transition to half-open
				if (should_transition_to_half_open())
				{
					std::lock_guard<std::mutex> lock(state_mutex_);
					if (state_.load(std::memory_order_acquire) == circuit_state::open)
					{
						transition_to(circuit_state::half_open);
						half_open_requests_.store(1, std::memory_order_relaxed);
						total_requests_.fetch_add(1, std::memory_order_relaxed);
						return true;
					}
				}
				rejected_requests_.fetch_add(1, std::memory_order_relaxed);
				return false;
			}

			case circuit_state::half_open:
			{
				auto requests = half_open_requests_.fetch_add(1, std::memory_order_acq_rel);
				if (requests < config_.half_open_max_requests)
				{
					total_requests_.fetch_add(1, std::memory_order_relaxed);
					return true;
				}
				// Revert the increment if we're rejecting
				half_open_requests_.fetch_sub(1, std::memory_order_relaxed);
				rejected_requests_.fetch_add(1, std::memory_order_relaxed);
				return false;
			}

			default:
				rejected_requests_.fetch_add(1, std::memory_order_relaxed);
				return false;
		}
	}

	auto circuit_breaker::record_success() -> void
	{
		window_->record_success();
		successful_requests_.fetch_add(1, std::memory_order_relaxed);
		consecutive_failures_.store(0, std::memory_order_relaxed);

		auto current_state = state_.load(std::memory_order_acquire);
		if (current_state == circuit_state::half_open)
		{
			auto successes = half_open_successes_.fetch_add(1, std::memory_order_acq_rel) + 1;
			if (successes >= config_.half_open_success_threshold)
			{
				std::lock_guard<std::mutex> lock(state_mutex_);
				if (state_.load(std::memory_order_acquire) == circuit_state::half_open)
				{
					transition_to(circuit_state::closed);
				}
			}
		}
	}

	auto circuit_breaker::record_failure(const std::exception* error) -> void
	{
		// Check failure predicate if configured
		if (config_.failure_predicate && error != nullptr)
		{
			if (!config_.failure_predicate(*error))
			{
				// This exception type doesn't count as a failure
				return;
			}
		}

		window_->record_failure();
		failed_requests_.fetch_add(1, std::memory_order_relaxed);
		auto failures = consecutive_failures_.fetch_add(1, std::memory_order_acq_rel) + 1;

		auto current_state = state_.load(std::memory_order_acquire);

		if (current_state == circuit_state::closed)
		{
			// Check if we should open
			if (failures >= config_.failure_threshold || should_transition_to_open())
			{
				std::lock_guard<std::mutex> lock(state_mutex_);
				if (state_.load(std::memory_order_acquire) == circuit_state::closed)
				{
					transition_to(circuit_state::open);
				}
			}
		}
		else if (current_state == circuit_state::half_open)
		{
			// Any failure in half-open state goes back to open
			std::lock_guard<std::mutex> lock(state_mutex_);
			if (state_.load(std::memory_order_acquire) == circuit_state::half_open)
			{
				transition_to(circuit_state::open);
			}
		}
	}

	auto circuit_breaker::get_state() const -> circuit_state
	{
		return state_.load(std::memory_order_acquire);
	}

	auto circuit_breaker::trip() -> void
	{
		std::lock_guard<std::mutex> lock(state_mutex_);
		if (state_.load(std::memory_order_acquire) != circuit_state::open)
		{
			transition_to(circuit_state::open);
		}
	}

	auto circuit_breaker::reset() -> void
	{
		std::lock_guard<std::mutex> lock(state_mutex_);
		transition_to(circuit_state::closed);
		window_->reset();
		consecutive_failures_.store(0, std::memory_order_relaxed);
		half_open_requests_.store(0, std::memory_order_relaxed);
		half_open_successes_.store(0, std::memory_order_relaxed);
	}

	auto circuit_breaker::get_stats() const -> stats
	{
		return stats{
			.current_state = state_.load(std::memory_order_acquire),
			.state_since = state_change_time_,
			.total_requests = total_requests_.load(std::memory_order_relaxed),
			.successful_requests = successful_requests_.load(std::memory_order_relaxed),
			.failed_requests = failed_requests_.load(std::memory_order_relaxed),
			.rejected_requests = rejected_requests_.load(std::memory_order_relaxed),
			.failure_rate = window_->failure_rate(),
			.consecutive_failures = consecutive_failures_.load(std::memory_order_relaxed),
			.state_transitions = state_transitions_.load(std::memory_order_relaxed)
		};
	}

	auto circuit_breaker::make_guard() -> guard
	{
		return guard(*this);
	}

	auto circuit_breaker::transition_to(circuit_state new_state) -> void
	{
		auto old_state = state_.load(std::memory_order_acquire);
		if (old_state == new_state)
		{
			return;
		}

		state_.store(new_state, std::memory_order_release);
		state_change_time_ = std::chrono::steady_clock::now();
		state_transitions_.fetch_add(1, std::memory_order_relaxed);

		if (new_state == circuit_state::open)
		{
			open_time_ = state_change_time_;
		}
		else if (new_state == circuit_state::half_open)
		{
			half_open_requests_.store(0, std::memory_order_relaxed);
			half_open_successes_.store(0, std::memory_order_relaxed);
		}
		else if (new_state == circuit_state::closed)
		{
			consecutive_failures_.store(0, std::memory_order_relaxed);
		}

		if (config_.state_change_callback)
		{
			config_.state_change_callback(old_state, new_state);
		}
	}

	auto circuit_breaker::should_transition_to_open() const -> bool
	{
		auto total = window_->total_requests();
		if (total < config_.minimum_requests)
		{
			return false;
		}

		auto failure_rate = window_->failure_rate();
		return failure_rate >= config_.failure_rate_threshold;
	}

	auto circuit_breaker::should_transition_to_half_open() const -> bool
	{
		auto now = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - open_time_);
		return elapsed >= config_.open_duration;
	}

	auto circuit_breaker::should_transition_to_closed() const -> bool
	{
		auto successes = half_open_successes_.load(std::memory_order_acquire);
		return successes >= config_.half_open_success_threshold;
	}

} // namespace kcenon::thread
