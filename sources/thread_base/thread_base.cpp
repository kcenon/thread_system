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

#include "thread_base.h"

/**
 * @file thread_base.cpp
 * @brief Implementation of the core thread base class.
 *
 * This file contains the implementation of the thread_base class, which serves
 * as the foundation for all worker thread types in the thread system.
 */

namespace thread_module
{
	/**
	 * @brief Constructs a new thread_base instance with the specified title.
	 * 
	 * Implementation details:
	 * - Initializes the worker_thread_ pointer to nullptr (thread not started)
	 * - Sets up thread control mechanisms based on configuration:
	 *   - In C++20 mode (USE_STD_JTHREAD), initializes stop_source_ to std::nullopt
	 *   - In legacy mode, initializes stop_requested_ to false
	 * - Sets wake_interval_ to std::nullopt (no periodic wake-ups by default)
	 * - Sets thread_title_ to the provided title
	 * - Sets initial thread_condition_ to Created
	 */
	thread_base::thread_base(const std::string& thread_title)
		: worker_thread_(nullptr)
#ifdef USE_STD_JTHREAD
		, stop_source_(std::nullopt)
#else
		, stop_requested_(false)
#endif
		, wake_interval_(std::nullopt)
		, thread_title_(thread_title)
		, thread_condition_(thread_conditions::Created)
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

	auto thread_base::set_wake_interval(
		const std::optional<std::chrono::milliseconds>& wake_interval) -> void
	{
		// Add thread safety when modifying the wake interval
		std::scoped_lock<std::mutex> lock(cv_mutex_);
		wake_interval_ = wake_interval;
	}

	auto thread_base::start(void) -> result_void
	{
#ifdef USE_STD_JTHREAD
		if (stop_source_.has_value())
#else
		if (worker_thread_ && worker_thread_->joinable())
#endif
		{
			return result_void{error{error_code::thread_already_running, "thread is already running"}};
		}

		stop();

#ifdef USE_STD_JTHREAD
		stop_source_ = std::stop_source();
#else
		stop_requested_ = false;
#endif

		try
		{
#ifdef USE_STD_JTHREAD
			worker_thread_ = std::make_unique<std::jthread>(
#else
			worker_thread_ = std::make_unique<std::thread>(
#endif
				[this](void)
				{
#ifdef USE_STD_JTHREAD
					auto stop_token = stop_source_.value().get_token();
#endif

					auto work_result = before_start();
					if (work_result.has_error())
					{
						std::cerr << "error before start: " << work_result.get_error().to_string()
								  << std::endl;
					}

#ifdef USE_STD_JTHREAD
					while (!stop_token.stop_requested() || should_continue_work())
#else
					while (!stop_requested_ || should_continue_work())
#endif
					{
						thread_condition_.store(thread_conditions::Waiting);

						std::unique_lock<std::mutex> lock(cv_mutex_);
						if (wake_interval_.has_value())
						{
#ifdef USE_STD_JTHREAD
							worker_condition_.wait_for(
								lock, wake_interval_.value(), [this, &stop_token]()
								{ return stop_token.stop_requested() || should_continue_work(); });
#else
							worker_condition_.wait_for(
								lock, wake_interval_.value(),
								[this]() { return stop_requested_ || should_continue_work(); });
#endif
						}
						else
						{
#ifdef USE_STD_JTHREAD
							worker_condition_.wait(
								lock, [this, &stop_token]()
								{ return stop_token.stop_requested() || should_continue_work(); });
#else
							worker_condition_.wait(
								lock,
								[this]() { return stop_requested_ || should_continue_work(); });
#endif
						}

#ifdef USE_STD_JTHREAD
						if (stop_token.stop_requested() && !should_continue_work())
#else
						if (stop_requested_ && !should_continue_work())
#endif
						{
							thread_condition_.store(thread_conditions::Stopping);

							break;
						}

						try
						{
							thread_condition_.store(thread_conditions::Working);

							work_result = do_work();
							if (work_result.has_error())
							{
								std::cerr << "error doing work on " << thread_title_ << " : "
										  << work_result.get_error().to_string() << std::endl;
							}
						}
						catch (const std::exception& e)
						{
							std::cerr << e.what() << '\n';
						}
					}

					work_result = after_stop();
					if (work_result.has_error())
					{
						std::cerr << "error after stop: " << work_result.get_error().to_string()
								  << std::endl;
					}
				});
		}
		catch (const std::bad_alloc& e)
		{
#ifdef USE_STD_JTHREAD
			stop_source_.reset();
#else
			stop_requested_ = true;
#endif

			worker_thread_.reset();

			return result_void{error{error_code::resource_allocation_failed, e.what()}};
		}

		return {};
	}

	auto thread_base::stop(void) -> result_void
	{
		if (worker_thread_ == nullptr)
		{
			return result_void{error{error_code::thread_not_running, "thread is not running"}};
		}

		if (worker_thread_->joinable())
		{
#ifdef USE_STD_JTHREAD
			if (stop_source_.has_value())
			{
				stop_source_.value().request_stop();
			}
#else
			stop_requested_ = true;
#endif

			{
				std::scoped_lock<std::mutex> lock(cv_mutex_);
				worker_condition_.notify_all();
			}

			worker_thread_->join();
		}

#ifdef USE_STD_JTHREAD
		stop_source_.reset();
#endif
		worker_thread_.reset();

		thread_condition_.store(thread_conditions::Stopped);

		return {};
	}

	auto thread_base::is_running(void) const -> bool
	{ 
		// Use the thread_condition_ atomic flag instead of checking the pointer
		auto condition = thread_condition_.load();
		return condition == thread_conditions::Working || 
			   condition == thread_conditions::Waiting;
	}

	auto thread_base::to_string(void) const -> std::string
	{
		return formatter::format("{} is {}", thread_title_, thread_condition_.load());
	}
} // namespace thread_module