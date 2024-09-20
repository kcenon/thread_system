#include "log_job.h"

#ifdef USE_STD_FORMAT
#include <format>
#else
#include "fmt/chrono.h"
#include "fmt/format.h"
#endif

#include <iomanip>
#include <sstream>

log_job::log_job(
	const std::string& message,
	std::optional<log_types> type,
	std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time)
	: job(nullptr, "log_job")
	, type_(type)
	, message_(message)
	, timestamp_(std::chrono::system_clock::now())
	, start_time_(start_time)
	, log_message_("")
{
}

auto log_job::get_type() const -> log_types
{
	if (!type_.has_value())
	{
		return log_types::None;
	}

	return type_.value_or(log_types::None);
}

auto log_job::log() const -> std::string { return log_message_; }

auto log_job::do_work() -> std::tuple<bool, std::optional<std::string>>
{
	try
	{
		auto in_time_t = std::chrono::system_clock::to_time_t(timestamp_);
		auto timeinfo = std::localtime(&in_time_t);
		auto duration = timestamp_.time_since_epoch();
		auto milliseconds
			= std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;
		auto microseconds
			= std::chrono::duration_cast<std::chrono::microseconds>(duration).count() % 1000;

		std::ostringstream time_stream;
		time_stream << std::put_time(timeinfo, "%Y-%m-%d %H:%M:%S") << '.' << std::setw(3)
					<< std::setfill('0') << milliseconds << std::setw(3) << std::setfill('0')
					<< microseconds;

		std::string formatted_time = time_stream.str();

		if (start_time_.has_value())
		{
			std::chrono::duration<double, std::milli> elapsed
				= std::chrono::high_resolution_clock::now() - start_time_.value();

			if (!type_.has_value())
			{
				log_message_ =
#ifdef USE_STD_FORMAT
					std::format
#else
					fmt::format
#endif
					("[{}][{}] [{} ms]", formatted_time, message_, elapsed.count());
			}
			else
			{
				log_message_ =
#ifdef USE_STD_FORMAT
					std::format
#else
					fmt::format
#endif
					("[{}][{}]: {} [{} ms]", formatted_time,
					 log_type_utils::to_string(type_.value()), message_, elapsed.count());
			}
		}
		else
		{
			if (!type_.has_value())
			{
				log_message_ =
#ifdef USE_STD_FORMAT
					std::format
#else
					fmt::format
#endif
					("[{}][{}]", formatted_time, message_);
			}
			else
			{
				log_message_ =
#ifdef USE_STD_FORMAT
					std::format
#else
					fmt::format
#endif
					("[{}][{}]: {}", formatted_time, type_.value(), message_);
			}
		}

		return { true, std::nullopt };
	}
	catch (const std::exception& e)
	{
		return { false, std::string(e.what()) };
	}
	catch (...)
	{
		return { false, "unknown error" };
	}
}