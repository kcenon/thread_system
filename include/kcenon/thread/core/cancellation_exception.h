// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#pragma once

/**
 * @file cancellation_exception.h
 * @brief Exception class for operation cancellation.
 */

#include "cancellation_reason.h"

#include <exception>
#include <string>

namespace kcenon::thread
{
	/**
	 * @class operation_cancelled_exception
	 * @brief Exception thrown when an operation is cancelled.
	 *
	 * @ingroup cancellation
	 *
	 * This exception type represents a cancelled operation. It carries the
	 * cancellation_reason for inspection by catch handlers.
	 *
	 * @note As of v1.0, public APIs use common::VoidResult via check_cancelled()
	 *       instead of throwing this exception directly.
	 *
	 * ### Design Principles
	 * - **Rich Information**: Carries full cancellation_reason for debugging
	 * - **Standard Compatible**: Derives from std::exception
	 * - **Thread Safe**: All methods are const and safe for concurrent access
	 *
	 * ### Usage Example
	 * @code
	 * auto result = token.check_cancelled();
	 * if (result.is_err()) {
	 *     LOG_INFO("Operation cancelled: {}", result.error().message);
	 *     return result;
	 * }
	 * do_work();
	 * @endcode
	 *
	 * @see enhanced_cancellation_token::check_cancelled()
	 * @see cancellation_reason
	 */
	class operation_cancelled_exception : public std::exception
	{
	public:
		/**
		 * @brief Constructs an exception with the given cancellation reason.
		 * @param reason The reason for the cancellation.
		 */
		explicit operation_cancelled_exception(cancellation_reason reason)
			: reason_(std::move(reason))
		{
			message_ = "Operation cancelled";
			if (!reason_.message.empty())
			{
				message_ += ": " + reason_.message;
			}
			else
			{
				message_ +=
					" (" + cancellation_reason::type_to_string(reason_.reason_type) + ")";
			}
		}

		/**
		 * @brief Returns a description of the exception.
		 * @return A C-string describing the exception.
		 */
		[[nodiscard]] auto what() const noexcept -> const char* override
		{
			return message_.c_str();
		}

		/**
		 * @brief Returns the cancellation reason.
		 * @return A const reference to the cancellation_reason.
		 */
		[[nodiscard]] auto reason() const noexcept -> const cancellation_reason&
		{
			return reason_;
		}

	private:
		cancellation_reason reason_;
		std::string message_;
	};

} // namespace kcenon::thread
