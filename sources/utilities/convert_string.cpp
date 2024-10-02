#include "convert_string.h"

#include <bit>
#include <locale>
#include <codecvt>

namespace utility_module
{
	bool has_utf8_bom(const std::string& value)
	{
		return value.size() >= 3 && std::equal(UTF8_BOM.begin(), UTF8_BOM.end(), value.begin());
	}

	size_t get_utf8_start_index(std::string_view view)
	{
		return (view.size() >= 3
				&& view.substr(0, 3)
					   == std::string_view(reinterpret_cast<const char*>(UTF8_BOM.data()), 3))
				   ? 3
				   : 0;
	}

	template <typename From, typename To>
	converter<From, To>::converter(std::basic_string_view<typename From::value_type> f,
								   const conversion_options& opts)
		: from(f), options(opts)
	{
	}

	template <typename From, typename To> To converter<From, To>::convert()
	{
		auto& facet = std::use_facet<
			std::codecvt<typename To::value_type, typename From::value_type, std::mbstate_t>>(
			std::locale());

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

	auto convert_string::split(const std::string& source, const std::string& token)
		-> std::tuple<std::optional<std::vector<std::string>>, std::optional<std::string>>
	{
		if (source.empty() == true)
		{
			return { std::nullopt, "source is empty" };
		}

		size_t offset = 0;
		size_t last_offset = 0;
		std::vector<std::string> result = {};
		std::string temp;

		while (true)
		{
			offset = source.find(token, last_offset);
			if (offset == std::string::npos)
			{
				break;
			}

			temp = source.substr(last_offset, offset - last_offset);
			if (!temp.empty())
			{
				result.push_back(std::move(temp));
			}

			last_offset = offset + token.size();
		}

		if (last_offset != 0 && last_offset != std::string::npos)
		{
			temp = source.substr(last_offset, offset - last_offset);
			if (!temp.empty())
			{
				result.push_back(std::move(temp));
			}
		}

		if (last_offset == 0)
		{
			return { std::vector<std::string>{ source }, std::nullopt };
		}

		return { result, std::nullopt };
	}

	auto convert_string::to_string(const std::wstring& message) -> std::string
	{
		return converter<std::wstring, std::string>(message).convert();
	}

	auto convert_string::to_string(const std::u16string& message) -> std::string
	{
		return converter<std::u16string, std::string>(message).convert();
	}

	auto convert_string::to_string(const std::u32string& message) -> std::string
	{
		return converter<std::u32string, std::string>(message).convert();
	}

	auto convert_string::to_wstring(const std::string& message) -> std::wstring
	{
		return converter<std::string, std::wstring>(message).convert();
	}

	auto convert_string::to_wstring(const std::u16string& message) -> std::wstring
	{
		return converter<std::u16string, std::wstring>(message).convert();
	}

	auto convert_string::to_wstring(const std::u32string& message) -> std::wstring
	{
		return converter<std::u32string, std::wstring>(message).convert();
	}

	auto convert_string::to_u16string(const std::string& message) -> std::u16string
	{
		return converter<std::string, std::u16string>(message).convert();
	}

	auto convert_string::to_u16string(const std::wstring& message) -> std::u16string
	{
		return converter<std::wstring, std::u16string>(message).convert();
	}

	auto convert_string::to_u16string(const std::u32string& message) -> std::u16string
	{
		return converter<std::u32string, std::u16string>(message).convert();
	}

	auto convert_string::to_u32string(const std::string& message) -> std::u32string
	{
		return converter<std::string, std::u32string>(message).convert();
	}

	auto convert_string::to_u32string(const std::wstring& message) -> std::u32string
	{
		return converter<std::wstring, std::u32string>(message).convert();
	}

	auto convert_string::to_u32string(const std::u16string& message) -> std::u32string
	{
		return converter<std::u16string, std::u32string>(message).convert();
	}

	auto convert_string::to_array(const std::string& value) -> std::vector<uint8_t>
	{
		if (value.empty())
		{
			return {};
		}

		std::vector<uint8_t> result;
		result.reserve(value.size());

		if (has_utf8_bom(value))
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
		size_t start_index = get_utf8_start_index(view);

		for (size_t i = start_index; i < view.size();)
		{
			if (!is_valid_utf8(view, i))
			{
				return std::string();
			}
		}

		return std::string(view.substr(start_index));
	}

	template <size_t N>
	auto convert_string::is_valid_multi_byte_sequence(std::string_view view,
													  const size_t& index) -> bool
	{
		if (index + N > view.size())
		{
			return false;
		}

		if constexpr (N > 1)
		{
			return (std::bit_cast<uint8_t>(view[index + 1]) & 0xC0) == 0x80
				   && is_valid_multi_byte_sequence<N - 1>(view, index + 1);
		}
		return true;
	}

	auto convert_string::is_valid_utf8(std::string_view view, size_t& index) -> bool
	{
		if (index >= view.size())
		{
			return false;
		}

		auto first_byte = std::bit_cast<uint8_t>(view[index]);

		if (first_byte <= 0x7F)
		{
			index++;
			return true;
		}

		if ((first_byte & 0xE0) == 0xC0)
		{
			if (is_valid_multi_byte_sequence<2>(view, index))
			{
				index += 2;
				return true;
			}
		}

		if ((first_byte & 0xF0) == 0xE0)
		{
			if (is_valid_multi_byte_sequence<3>(view, index))
			{
				index += 3;
				return true;
			}
		}

		if ((first_byte & 0xF8) == 0xF0)
		{
			if (is_valid_multi_byte_sequence<4>(view, index))
			{
				index += 4;
				return true;
			}
		}

		return false;
	}

	template class converter<std::string, std::wstring>;
	template class converter<std::string, std::u16string>;
	template class converter<std::string, std::u32string>;
	template class converter<std::wstring, std::string>;
	template class converter<std::wstring, std::u16string>;
	template class converter<std::wstring, std::u32string>;
	template class converter<std::u16string, std::string>;
	template class converter<std::u16string, std::wstring>;
	template class converter<std::u16string, std::u32string>;
	template class converter<std::u32string, std::string>;
	template class converter<std::u32string, std::wstring>;
	template class converter<std::u32string, std::u16string>;
} // namespace utility_module