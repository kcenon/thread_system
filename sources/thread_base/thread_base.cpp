#include "thread_base.h"

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

auto thread_base::set_wake_interval(const std::optional<std::chrono::milliseconds>& wake_interval)
	-> void
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

				before_start();

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
						worker_condition_.wait(lock,
											   [this, &stop_token]() {
												   return stop_token.stop_requested() || has_work();
											   });
#else
						worker_condition_.wait(lock,
											   [this]() { return stop_requested_ || has_work(); });
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
						do_work();
					}
					catch (const std::exception& e)
					{
						std::cerr << e.what() << '\n';
					}
				}
				after_stop();
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
