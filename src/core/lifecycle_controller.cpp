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

#include <kcenon/thread/core/lifecycle_controller.h>

namespace kcenon::thread
{
	lifecycle_controller::lifecycle_controller()
		: state_(thread_conditions::Created)
#ifdef USE_STD_JTHREAD
		, stop_source_(std::nullopt)
#else
		, stop_requested_(false)
#endif
	{
	}

	auto lifecycle_controller::get_state() const noexcept -> thread_conditions
	{
		return state_.load(std::memory_order_acquire);
	}

	auto lifecycle_controller::set_state(thread_conditions state) noexcept -> void
	{
		state_.store(state, std::memory_order_release);
	}

	auto lifecycle_controller::is_running() const noexcept -> bool
	{
		auto condition = state_.load(std::memory_order_acquire);
		return condition == thread_conditions::Working ||
		       condition == thread_conditions::Waiting;
	}

	auto lifecycle_controller::set_stopped() noexcept -> void
	{
		set_state(thread_conditions::Stopped);
	}

	auto lifecycle_controller::initialize_for_start() -> void
	{
#ifdef USE_STD_JTHREAD
		stop_source_ = std::stop_source();
#else
		stop_requested_.store(false, std::memory_order_release);
#endif
		set_state(thread_conditions::Created);
	}

	auto lifecycle_controller::request_stop() noexcept -> void
	{
#ifdef USE_STD_JTHREAD
		if (stop_source_.has_value())
		{
			stop_source_.value().request_stop();
		}
#else
		stop_requested_.store(true, std::memory_order_release);
#endif
	}

	auto lifecycle_controller::is_stop_requested() const noexcept -> bool
	{
#ifdef USE_STD_JTHREAD
		if (stop_source_.has_value())
		{
			return stop_source_.value().stop_requested();
		}
		return true;
#else
		return stop_requested_.load(std::memory_order_acquire);
#endif
	}

	auto lifecycle_controller::has_active_source() const noexcept -> bool
	{
#ifdef USE_STD_JTHREAD
		return stop_source_.has_value();
#else
		// In legacy mode, check if we're in an active state (not stopped/created)
		auto state = state_.load(std::memory_order_acquire);
		return state == thread_conditions::Working ||
		       state == thread_conditions::Waiting;
#endif
	}

	auto lifecycle_controller::reset_stop_source() noexcept -> void
	{
#ifdef USE_STD_JTHREAD
		stop_source_.reset();
#endif
	}

	auto lifecycle_controller::acquire_lock() -> std::unique_lock<std::mutex>
	{
		return std::unique_lock<std::mutex>(cv_mutex_);
	}

	auto lifecycle_controller::notify_one() -> void
	{
		std::scoped_lock<std::mutex> lock(cv_mutex_);
		condition_.notify_one();
	}

	auto lifecycle_controller::notify_all() -> void
	{
		std::scoped_lock<std::mutex> lock(cv_mutex_);
		condition_.notify_all();
	}

} // namespace kcenon::thread
