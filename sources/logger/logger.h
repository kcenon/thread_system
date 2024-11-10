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

#include "logger_implementation.h"

namespace log_module
{
	inline auto set_title(const std::string& title) -> void
	{
		implementation::logger::handle().set_title(title);
	}

	inline auto callback_target(const log_types& type) -> void
	{
		implementation::logger::handle().callback_target(type);
	}

	inline auto callback_target() -> log_types
	{
		return implementation::logger::handle().callback_target();
	}

	inline auto file_target(const log_types& type) -> void
	{
		implementation::logger::handle().file_target(type);
	}

	inline auto file_target() -> log_types
	{
		return implementation::logger::handle().file_target();
	}

	inline auto console_target(const log_types& type) -> void
	{
		implementation::logger::handle().console_target(type);
	}

	inline auto console_target() -> log_types
	{
		return implementation::logger::handle().console_target();
	}

	inline auto message_callback(
		const std::function<void(const log_types&, const std::string&, const std::string&)>&
			callback) -> void
	{
		implementation::logger::handle().message_callback(callback);
	}

	inline auto set_max_lines(uint32_t max_lines) -> void
	{
		implementation::logger::handle().set_max_lines(max_lines);
	}

	inline auto get_max_lines() -> uint32_t
	{
		return implementation::logger::handle().get_max_lines();
	}

	inline auto set_use_backup(bool use_backup) -> void
	{
		implementation::logger::handle().set_use_backup(use_backup);
	}

	inline auto get_use_backup() -> bool
	{
		return implementation::logger::handle().get_use_backup();
	}

	inline auto set_wake_interval(std::chrono::milliseconds interval) -> void
	{
		implementation::logger::handle().set_wake_interval(interval);
	}

	inline auto time_point() -> std::chrono::time_point<std::chrono::high_resolution_clock>
	{
		return implementation::logger::handle().time_point();
	}

	template <typename... Args>
	inline auto write(const log_types& type, const char* formats, const Args&... args) -> void
	{
		implementation::logger::handle().write(type, formats, args...);
	}

	template <typename... WideArgs>
	inline auto write(const log_types& type,
					  const wchar_t* formats,
					  const WideArgs&... args) -> void
	{
		implementation::logger::handle().write(type, formats, args...);
	}

	template <typename... Args>
	inline auto write(const log_types& type,
					  const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
					  const char* formats,
					  const Args&... args) -> void
	{
		implementation::logger::handle().write(type, time_point, formats, args...);
	}

	template <typename... WideArgs>
	inline auto write(const log_types& type,
					  const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
					  const wchar_t* formats,
					  const WideArgs&... args) -> void
	{
		implementation::logger::handle().write(type, time_point, formats, args...);
	}

	inline auto start() -> std::optional<std::string>
	{
		return implementation::logger::handle().start();
	}

	inline auto stop() -> void
	{
		implementation::logger::handle().stop();
		implementation::logger::destroy();
	}
} // namespace log_module
