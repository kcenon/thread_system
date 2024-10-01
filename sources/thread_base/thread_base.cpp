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

namespace thread_module
{
	thread_base::thread_base(void)
		: worker_thread_(nullptr)
#ifdef USE_STD_JTHREAD
		, stop_source_(std::nullopt)
#else
		, stop_requested_(false)
#endif
		, wake_interval_(std::nullopt)
	{
	}

	thread_base::~thread_base(void) { stop(); }

	auto thread_base::get_ptr(void) -> std::shared_ptr<thread_base> { return shared_from_this(); }

	auto thread_base::set_wake_interval(
		const std::optional<std::chrono::milliseconds>& wake_interval) -> void
	{
		wake_interval_ = wake_interval;
	}

	auto thread_base::start(void) -> std::tuple<bool, std::optional<std::string>>
	{
#ifdef USE_STD_JTHREAD
		if (stop_source_.has_value())
#else
		if (worker_thread_ && worker_thread_->joinable())
#endif
		{
			return { false, "thread is already running" };
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

					auto [before_started, before_start_error] = before_start();
					if (!before_started)
					{
						std::cerr << "error before start: "
								  << before_start_error.value_or("unknown error") << std::endl;
					}

#ifdef USE_STD_JTHREAD
					while (!stop_token.stop_requested() || has_work())
#else
					while (!stop_requested_ || has_work())
#endif
					{
						std::unique_lock<std::mutex> lock(cv_mutex_);
						if (wake_interval_.has_value())
						{
#ifdef USE_STD_JTHREAD

							worker_condition_.wait_for(
								lock, wake_interval_.value(), [this, &stop_token]()
								{ return stop_token.stop_requested() || has_work(); });
#else
							worker_condition_.wait_for(lock, wake_interval_.value(), [this]()
													   { return stop_requested_ || has_work(); });
#endif
						}
						else
						{
#ifdef USE_STD_JTHREAD
							worker_condition_.wait(
								lock, [this, &stop_token]()
								{ return stop_token.stop_requested() || has_work(); });
#else
							worker_condition_.wait(lock, [this]()
												   { return stop_requested_ || has_work(); });
#endif
						}

#ifdef USE_STD_JTHREAD
						if (stop_token.stop_requested() && !has_work())
#else
						if (stop_requested_ && !has_work())
#endif
						{
							break;
						}

						try
						{
							auto [do_worked, do_work_error] = do_work();
							if (!do_worked)
							{
								std::cerr << "error doing work: "
										  << do_work_error.value_or("unknown error") << std::endl;
							}
						}
						catch (const std::exception& e)
						{
							std::cerr << e.what() << '\n';
						}
					}

					auto [after_stopped, after_stop_error] = after_stop();
					if (!after_stopped)
					{
						std::cerr << "error after stop: "
								  << after_stop_error.value_or("unknown error") << std::endl;
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

			return { false, e.what() };
		}

		return { true, std::nullopt };
	}

	auto thread_base::stop(void) -> std::tuple<bool, std::optional<std::string>>
	{
		if (worker_thread_ == nullptr)
		{
			return { false, "thread is not running" };
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

		return { true, std::nullopt };
	}
} // namespace thread_module