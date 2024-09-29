#include "message_job.h"

#ifdef USE_STD_FORMAT
#include <format>
#include <iostream>
#else
#include "fmt/format.h"
#endif

namespace log_module
{
	message_job::message_job(const std::string& message)
		: job(nullptr, "message_job"), message_(message)
	{
	}

	auto message_job::message(const bool& append_newline) const -> std::string
	{
		if (!append_newline)
		{
			return message_;
		}

		return
#ifdef USE_STD_FORMAT
			std::format
#else
			fmt::format
#endif
			("{}\n", message_);
	}

	auto message_job::do_work() -> std::tuple<bool, std::optional<std::string>>
	{
		if (message_.empty())
		{
			return { false, "empty message" };
		}

		return { true, std::nullopt };
	}
} // namespace log_module