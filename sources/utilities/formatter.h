#pragma once

#include <string>

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#endif

namespace utility_module
{
	class formatter
	{
	public:
		template <typename... Args>
		static auto format(
#ifdef USE_STD_FORMAT
			std::format_string<Args...> format_str,
#else
			fmt::format_string<Args...> format_str,
#endif
			Args&&... args) -> std::string
		{
			return
#ifdef USE_STD_FORMAT
				std::format
#else
				fmt::format
#endif
				(format_str, std::forward<Args>(args)...);
		}

		template <typename OutputIt, typename... Args>
		static auto format_to(
#ifdef USE_STD_FORMAT
			OutputIt out,
			std::format_string<Args...> format_str,
#else
			OutputIt out,
			fmt::format_string<Args...> format_str,
#endif
			Args&&... args) -> void
		{
#ifdef USE_STD_FORMAT
			std::format_to(out, format_str, std::forward<Args>(args)...);
#else
			fmt::format_to(out, format_str, std::forward<Args>(args)...);
#endif
		}
	};
} // namespace utility_module