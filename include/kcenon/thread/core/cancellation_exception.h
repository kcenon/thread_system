/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

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
	 * This exception is thrown by throw_if_cancelled() when the associated
	 * cancellation token has been cancelled. It carries the cancellation_reason
	 * for inspection by catch handlers.
	 *
	 * ### Design Principles
	 * - **Rich Information**: Carries full cancellation_reason for debugging
	 * - **Standard Compatible**: Derives from std::exception
	 * - **Thread Safe**: All methods are const and safe for concurrent access
	 *
	 * ### Usage Example
	 * @code
	 * try {
	 *     token.throw_if_cancelled();
	 *     do_work();
	 * } catch (const operation_cancelled_exception& ex) {
	 *     auto reason = ex.reason();
	 *     LOG_INFO("Operation cancelled: {}", reason.to_string());
	 * }
	 * @endcode
	 *
	 * @see enhanced_cancellation_token::throw_if_cancelled()
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
