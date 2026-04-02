// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#pragma once

#include <format>
#include <string>
#include <string_view>
#include <type_traits>

namespace kcenon::thread::utils
{
	/**
	 * @class enum_formatter
	 * @brief A generic formatter for enum types, using a user-provided converter functor.
	 *
	 * The @c enum_formatter template allows formatting an enum value by converting
	 * it to a string (via a custom @c Converter functor) and then passing that string
	 * to C++20's @c std::format.
	 *
	 * @tparam T The enum type to be formatted.
	 * @tparam Converter A callable object (e.g., a functor or lambda) that accepts
	 *                   an enum value of type @c T and returns its string representation.
	 *
	 * ### Example
	 * @code
	 * // 1) Define an enum and a corresponding converter functor:
	 * enum class color { Red, Green, Blue };
	 *
	 * struct color_converter {
	 *     std::string operator()(color c) const {
	 *         switch (c) {
	 *         case color::Red:   return "Red";
	 *         case color::Green: return "Green";
	 *         case color::Blue:  return "Blue";
	 *         }
	 *         return "Unknown";
	 *     }
	 * };
	 *
	 * // 2) Use enum_formatter<Color, color_converter> to format:
	 * // e.g. std::format("Color: {}", color::Green) // -> "Color: Green"
	 * @endcode
	 */
	template <typename T, typename Converter> class enum_formatter
	{
	private:
		/**
		 * @brief Internal helper that dispatches to @c std::format based on character type.
		 * @tparam CharT The character type (@c char or @c wchar_t).
		 * @param out An output iterator to which formatted text is appended.
		 * @param value The enum value to be converted to string and then formatted.
		 * @return An iterator pointing to the end of the inserted sequence.
		 */
		template <typename CharT> static auto do_format(auto& out, const T& value)
		{
			if constexpr (std::is_same_v<CharT, wchar_t>)
			{
				return std::format_to(out, L"{}", Converter{}(value));
			}
			else
			{
				return std::format_to(out, "{}", Converter{}(value));
			}
		}

	public:
		/**
		 * @brief A no-op parse function required by the formatting library.
		 * @param context The format parse context.
		 * @return An iterator pointing to the end of @p context.
		 *
		 * The formatter does not accept any custom formatting specifiers,
		 * so it simply returns @c context.begin().
		 */
		constexpr auto parse(auto& context) { return context.begin(); }

		/**
		 * @brief Formats the enum value into a provided format context.
		 * @tparam FormatContext The type of the format context (provided by std::format).
		 * @param value The enum value to format.
		 * @param context The format context, which holds the output iterator and additional state.
		 * @return An iterator pointing to the end of the formatted output.
		 *
		 * Internally calls the helper @c do_format() to convert the enum to a string using @c
		 * Converter and then writes it.
		 */
		template <typename FormatContext> auto format(const T& value, FormatContext& context) const
		{
			using char_type =
				typename std::iterator_traits<typename FormatContext::iterator>::value_type;
			return do_format<char_type>(context.out(), value);
		}
	};

	/**
	 * @class formatter
	 * @brief Provides convenience methods for string formatting using C++20 <format>.
	 *
	 * The @c formatter class offers static functions to format strings (both narrow and wide)
	 * into a @c std::string / @c std::wstring, or directly to an output iterator.
	 *
	 * ### Example
	 * @code
	 * // Basic usage:
	 * std::string result = formatter::format("Hello, {}", "World");
	 *
	 * // Writing directly to a buffer:
	 * std::array<char, 50> buffer;
	 * auto it = formatter::format_to(buffer.begin(), "Number: {}", 42);
	 * // 'it' now points to the end of the written text
	 * @endcode
	 */
	class formatter
	{
	public:
		/**
		 * @brief Formats a narrow-character string with the given arguments.
		 * @tparam FormatArgs Parameter pack of argument types.
		 * @param formats A format string (e.g. "Name: {}, Age: {}").
		 * @param args The arguments to substitute into @p formats.
		 * @return A @c std::string containing the formatted result.
		 */
		template <typename... FormatArgs>
		static auto format(const char* formats, const FormatArgs&... args) -> std::string
		{
			return std::vformat(formats, std::make_format_args(args...));
		}

		/**
		 * @brief Formats a wide-character string with the given arguments.
		 * @tparam FormatArgs Parameter pack of argument types.
		 * @param formats A wide format string (e.g. L"Count: {}, Value: {}").
		 * @param args The arguments to substitute into @p formats.
		 * @return A @c std::wstring containing the formatted result.
		 */
		template <typename... FormatArgs>
		static auto format(const wchar_t* formats, const FormatArgs&... args) -> std::wstring
		{
			return std::vformat(formats, std::make_wformat_args(args...));
		}

		/**
		 * @brief Formats a narrow-character string directly to an output iterator.
		 * @tparam OutputIt The type of the output iterator (e.g. a back_inserter for a container).
		 * @tparam FormatArgs Parameter pack of argument types.
		 * @param out The output iterator where formatted text will be written.
		 * @param formats The narrow format string.
		 * @param args The arguments to substitute into @p formats.
		 * @return The output iterator advanced to the end of the written text.
		 *
		 * This method is useful when writing to a buffer, a container, or custom iterators.
		 */
		template <typename OutputIt, typename... FormatArgs>
		static auto format_to(OutputIt out, const char* formats, const FormatArgs&... args)
			-> OutputIt
		{
			return std::vformat_to(out, formats, std::make_format_args(args...));
		}

		/**
		 * @brief Formats a wide-character string directly to an output iterator.
		 * @tparam OutputIt The type of the output iterator (e.g. a back_inserter for a wstring).
		 * @tparam FormatArgs Parameter pack of argument types.
		 * @param out The output iterator where formatted text will be written.
		 * @param formats The wide format string.
		 * @param args The arguments to substitute into @p formats.
		 * @return The output iterator advanced to the end of the written text.
		 */
		template <typename OutputIt, typename... FormatArgs>
		static auto format_to(OutputIt out, const wchar_t* formats, const FormatArgs&... args)
			-> OutputIt
		{
			return std::vformat_to(out, formats, std::make_wformat_args(args...));
		}
	};
} // namespace kcenon::thread::utils

// Backward compatibility: provide aliases in utility_module namespace
namespace utility_module
{
	using formatter = kcenon::thread::utils::formatter;
	template <typename T, typename Converter>
	using enum_formatter = kcenon::thread::utils::enum_formatter<T, Converter>;
} // namespace utility_module
