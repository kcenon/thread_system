#include "convert_string.h"

#include <locale>
#include <string>
#include <vector>
#include <codecvt>
#include <string_view>

namespace utility_module
{
	auto convert_string::to_string(const std::wstring& message) -> std::string
	{
		return convert<std::wstring_view, std::string>(message);
	}

	auto convert_string::to_string(const std::u16string& message) -> std::string
	{
		return convert<std::u16string_view, std::string>(message);
	}

	auto convert_string::to_string(const std::u32string& message) -> std::string
	{
		return convert<std::u32string_view, std::string>(message);
	}

	auto convert_string::to_wstring(const std::string& message) -> std::wstring
	{
		return convert<std::string_view, std::wstring>(message);
	}

	auto convert_string::to_wstring(const std::u16string& message) -> std::wstring
	{
		return convert<std::u16string_view, std::wstring>(message);
	}

	auto convert_string::to_wstring(const std::u32string& message) -> std::wstring
	{
		return convert<std::u32string_view, std::wstring>(message);
	}

	auto convert_string::to_u16string(const std::string& message) -> std::u16string
	{
		return convert<std::string_view, std::u16string>(message);
	}

	auto convert_string::to_u16string(const std::wstring& message) -> std::u16string
	{
		return convert<std::wstring_view, std::u16string>(message);
	}

	auto convert_string::to_u16string(const std::u32string& message) -> std::u16string
	{
		return convert<std::u32string_view, std::u16string>(message);
	}

	auto convert_string::to_u32string(const std::string& message) -> std::u32string
	{
		return convert<std::string_view, std::u32string>(message);
	}

	auto convert_string::to_u32string(const std::wstring& message) -> std::u32string
	{
		return convert<std::wstring_view, std::u32string>(message);
	}

	auto convert_string::to_u32string(const std::u16string& message) -> std::u32string
	{
		return convert<std::u16string_view, std::u32string>(message);
	}

	template <typename From, typename To>
	auto convert_string::convert(std::basic_string_view<typename From::value_type> from) -> To
	{
		auto& facet = std::use_facet<
			std::codecvt<typename To::value_type, typename From::value_type, std::mbstate_t>>(
			std::locale(""));

		std::mbstate_t state{};
		std::vector<typename To::value_type> to(from.size() * facet.max_length());

		const auto* from_next = from.data();
		auto* to_next = to.data();

		const auto result = facet.in(state, from.data(), from.data() + from.size(), from_next,
									 to.data(), to.data() + to.size(), to_next);

		if (result != std::codecvt_base::ok)
		{
			return To();
		}

		return To(to.data(), to_next);
	}
} // namespace utility_module