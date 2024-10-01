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

	auto convert_string::to_array(const std::string& value) -> std::vector<uint8_t>
	{
		if (value.empty())
		{
			return {};
		}

		std::vector<uint8_t> result;
		result.reserve(value.size());

		if (value.size() >= 3 && std::equal(UTF8_BOM.begin(), UTF8_BOM.end(), value.begin()))
		{
			result.insert(result.end(), value.begin() + 3, value.end());
		}
		else
		{
			result.insert(result.end(), value.begin(), value.end());
		}

		return result;
	}

	auto convert_string::to_string(const std::vector<uint8_t>& value) -> std::string
	{
		if (value.empty())
		{
			return std::string();
		}

		std::string_view view(reinterpret_cast<const char*>(value.data()), value.size());
		size_t start_index
			= (view.size() >= 3
			   && view.substr(0, 3)
					  == std::string_view(reinterpret_cast<const char*>(UTF8_BOM.data()), 3))
				  ? 3
				  : 0;

		for (size_t i = start_index; i < view.size();)
		{
			if (static_cast<uint8_t>(view[i]) <= 0x7F)
			{
				i++;
				continue;
			}

			if ((view[i] & 0xE0) == 0xC0)
			{
				if (i + 1 >= view.size() || (view[i + 1] & 0xC0) != 0x80)
				{
					return std::string();
				}
				i += 2;
				continue;
			}

			if ((view[i] & 0xF0) == 0xE0)
			{
				if (i + 2 >= view.size() || (view[i + 1] & 0xC0) != 0x80
					|| (view[i + 2] & 0xC0) != 0x80)
				{
					return std::string();
				}
				i += 3;
				continue;
			}

			if ((view[i] & 0xF8) == 0xF0)
			{
				if (i + 3 >= view.size() || (view[i + 1] & 0xC0) != 0x80
					|| (view[i + 2] & 0xC0) != 0x80 || (view[i + 3] & 0xC0) != 0x80)
				{
					return std::string();
				}
				i += 4;
				continue;
			}

			return std::string();
		}

		return std::string(view.substr(start_index));
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