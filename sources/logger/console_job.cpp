#include "console_job.h"

#ifdef USE_STD_FORMAT
#include <format>
#include <iostream>
#else
#include "fmt/format.h"
#endif

namespace log_module
{
	console_job::console_job(const std::string& message)
		: job(nullptr, "console_job"), message_(message)
	{
	}

	auto console_job::do_work() -> std::tuple<bool, std::optional<std::string>>
	{
		if (message_.empty())
		{
			return { false, "empty message" };
		}

		try
		{
#ifdef USE_STD_FORMAT
			std::cout << message_;
#else
			fmt::print("{}", message_);
#endif

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
} // namespace log_module