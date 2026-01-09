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
 * @file cancellation_reason.h
 * @brief Cancellation reason structure for enhanced cancellation tokens.
 */

#include <chrono>
#include <exception>
#include <optional>
#include <string>

namespace kcenon::thread
{
	/**
	 * @struct cancellation_reason
	 * @brief Holds information about why a cancellation occurred.
	 *
	 * @ingroup cancellation
	 *
	 * When a cancellation token is cancelled, the reason provides details
	 * about what triggered the cancellation, when it occurred, and optionally
	 * any exception that caused it.
	 *
	 * ### Usage Example
	 * @code
	 * auto token = enhanced_cancellation_token::create_with_timeout(
	 *     std::chrono::seconds{30});
	 *
	 * // ... later, when cancelled ...
	 * if (token.is_cancelled()) {
	 *     auto reason = token.get_reason();
	 *     if (reason) {
	 *         std::cout << "Cancelled: " << reason->to_string() << std::endl;
	 *     }
	 * }
	 * @endcode
	 */
	struct cancellation_reason
	{
		/**
		 * @enum type
		 * @brief The type of cancellation that occurred.
		 */
		enum class type
		{
			none,              ///< No cancellation (default state)
			user_requested,    ///< Explicit cancel() call by user
			timeout,           ///< Timeout duration expired
			deadline,          ///< Deadline time point reached
			parent_cancelled,  ///< Parent token was cancelled
			pool_shutdown,     ///< Thread pool is shutting down
			error              ///< Cancellation triggered by an error
		};

		/// The type of cancellation that occurred.
		type reason_type = type::none;

		/// Human-readable message describing the cancellation.
		std::string message;

		/// Time point when the cancellation occurred.
		std::chrono::steady_clock::time_point cancel_time;

		/// Optional exception that triggered the cancellation.
		std::optional<std::exception_ptr> exception;

		/**
		 * @brief Converts the cancellation reason to a human-readable string.
		 * @return A string describing the cancellation reason.
		 */
		[[nodiscard]] auto to_string() const -> std::string
		{
			std::string type_str;
			switch (reason_type)
			{
				case type::none:
					type_str = "none";
					break;
				case type::user_requested:
					type_str = "user_requested";
					break;
				case type::timeout:
					type_str = "timeout";
					break;
				case type::deadline:
					type_str = "deadline";
					break;
				case type::parent_cancelled:
					type_str = "parent_cancelled";
					break;
				case type::pool_shutdown:
					type_str = "pool_shutdown";
					break;
				case type::error:
					type_str = "error";
					break;
			}

			std::string result = "cancellation_reason{type=" + type_str;
			if (!message.empty())
			{
				result += ", message=\"" + message + "\"";
			}
			result += "}";
			return result;
		}

		/**
		 * @brief Converts the reason type enum to a string.
		 * @param t The reason type to convert.
		 * @return A string representation of the reason type.
		 */
		[[nodiscard]] static auto type_to_string(type t) -> std::string
		{
			switch (t)
			{
				case type::none:
					return "none";
				case type::user_requested:
					return "user_requested";
				case type::timeout:
					return "timeout";
				case type::deadline:
					return "deadline";
				case type::parent_cancelled:
					return "parent_cancelled";
				case type::pool_shutdown:
					return "pool_shutdown";
				case type::error:
					return "error";
			}
			return "unknown";
		}
	};

} // namespace kcenon::thread
