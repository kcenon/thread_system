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

#include "../../logger/core/logger_implementation.h"

/**
 * @namespace log_module
 * @brief Thread-safe logging system built on the thread system foundation.
 *
 * The log_module namespace provides a comprehensive, thread-safe logging system
 * that leverages the thread_module infrastructure for asynchronous log processing.
 *
 * Key components include:
 * - Logger interface functions for different log levels
 * - Log collectors for processing and distributing log messages
 * - Writers for different output targets (console, file, callback)
 * - log_types enumeration defining various logging levels
 *
 * Features:
 * - Thread-safe logging with minimal impact on calling threads
 * - Support for multiple output targets simultaneously
 * - Filtering by log level for each output target
 * - Timestamp and type information for each log entry
 * - Formatted logging with both C-style format strings and modern formatting
 *
 * The logging system is designed to be high-performance with minimal overhead
 * by processing log messages asynchronously in dedicated worker threads.
 */
namespace log_module
{
	/**
	 * @brief Sets the title/name for the logger instance.
	 * @param title String identifier for this logger instance.
	 *
	 * The title is used in log messages to identify the source of the log and
	 * may appear in log file names or other output.
	 *
	 * @note This should typically be called once during application startup.
	 */
	inline auto set_title(const std::string& title) -> void
	{
		implementation::logger::handle().set_title(title);
	}

	/**
	 * @brief Configures which log levels are sent to the callback target.
	 * @param type The log types/levels to be sent to registered callbacks.
	 *
	 * Use this method to control which message types will trigger callback notifications.
	 * Only messages matching the specified types will be forwarded to the callback
	 * function registered with message_callback().
	 *
	 * @see message_callback() To register a callback function
	 * @see callback_target() (no-arg version) To query current settings
	 * @see log_types For available log type flags
	 */
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
	inline auto write_exception(const char* formats, const Args&... args) -> void
	{
		implementation::logger::handle().write(log_types::Exception, formats, args...);
	}

	template <typename... Args>
	inline auto write_error(const char* formats, const Args&... args) -> void
	{
		implementation::logger::handle().write(log_types::Error, formats, args...);
	}

	template <typename... Args>
	inline auto write_information(const char* formats, const Args&... args) -> void
	{
		implementation::logger::handle().write(log_types::Information, formats, args...);
	}

	template <typename... Args>
	inline auto write_debug(const char* formats, const Args&... args) -> void
	{
		implementation::logger::handle().write(log_types::Debug, formats, args...);
	}

	template <typename... Args>
	inline auto write_sequence(const char* formats, const Args&... args) -> void
	{
		implementation::logger::handle().write(log_types::Sequence, formats, args...);
	}

	template <typename... Args>
	inline auto write_parameter(const char* formats, const Args&... args) -> void
	{
		implementation::logger::handle().write(log_types::Parameter, formats, args...);
	}

	template <typename... WideArgs>
	inline auto write_exception(const wchar_t* formats, const WideArgs&... args) -> void
	{
		implementation::logger::handle().write(log_types::Exception, formats, args...);
	}

	template <typename... WideArgs>
	inline auto write_error(const wchar_t* formats, const WideArgs&... args) -> void
	{
		implementation::logger::handle().write(log_types::Error, formats, args...);
	}

	template <typename... WideArgs>
	inline auto write_information(const wchar_t* formats, const WideArgs&... args) -> void
	{
		implementation::logger::handle().write(log_types::Information, formats, args...);
	}

	template <typename... WideArgs>
	inline auto write_debug(const wchar_t* formats, const WideArgs&... args) -> void
	{
		implementation::logger::handle().write(log_types::Debug, formats, args...);
	}

	template <typename... WideArgs>
	inline auto write_sequence(const wchar_t* formats, const WideArgs&... args) -> void
	{
		implementation::logger::handle().write(log_types::Sequence, formats, args...);
	}

	template <typename... WideArgs>
	inline auto write_parameter(const wchar_t* formats, const WideArgs&... args) -> void
	{
		implementation::logger::handle().write(log_types::Parameter, formats, args...);
	}

	template <typename... Args>
	inline auto write_exception(
		const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
		const char* formats,
		const Args&... args) -> void
	{
		implementation::logger::handle().write(log_types::Exception, time_point, formats, args...);
	}

	template <typename... Args>
	inline auto write_error(
		const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
		const char* formats,
		const Args&... args) -> void
	{
		implementation::logger::handle().write(log_types::Error, time_point, formats, args...);
	}

	template <typename... Args>
	inline auto write_information(
		const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
		const char* formats,
		const Args&... args) -> void
	{
		implementation::logger::handle().write(log_types::Information, time_point, formats,
											   args...);
	}

	template <typename... Args>
	inline auto write_debug(
		const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
		const char* formats,
		const Args&... args) -> void
	{
		implementation::logger::handle().write(log_types::Debug, time_point, formats, args...);
	}

	template <typename... Args>
	inline auto write_sequence(
		const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
		const char* formats,
		const Args&... args) -> void
	{
		implementation::logger::handle().write(log_types::Sequence, time_point, formats, args...);
	}

	template <typename... Args>
	inline auto write_parameter(
		const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
		const char* formats,
		const Args&... args) -> void
	{
		implementation::logger::handle().write(log_types::Parameter, time_point, formats, args...);
	}

	template <typename... WideArgs>
	inline auto write_exception(
		const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
		const wchar_t* formats,
		const WideArgs&... args) -> void
	{
		implementation::logger::handle().write(log_types::Exception, time_point, formats, args...);
	}

	template <typename... WideArgs>
	inline auto write_error(
		const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
		const wchar_t* formats,
		const WideArgs&... args) -> void
	{
		implementation::logger::handle().write(log_types::Error, time_point, formats, args...);
	}

	template <typename... WideArgs>
	inline auto write_information(
		const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
		const wchar_t* formats,
		const WideArgs&... args) -> void
	{
		implementation::logger::handle().write(log_types::Information, time_point, formats,
											   args...);
	}

	template <typename... WideArgs>
	inline auto write_debug(
		const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
		const wchar_t* formats,
		const WideArgs&... args) -> void
	{
		implementation::logger::handle().write(log_types::Debug, time_point, formats, args...);
	}

	template <typename... WideArgs>
	inline auto write_sequence(
		const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
		const wchar_t* formats,
		const WideArgs&... args) -> void
	{
		implementation::logger::handle().write(log_types::Sequence, time_point, formats, args...);
	}

	template <typename... WideArgs>
	inline auto write_parameter(
		const std::chrono::time_point<std::chrono::high_resolution_clock>& time_point,
		const wchar_t* formats,
		const WideArgs&... args) -> void
	{
		implementation::logger::handle().write(log_types::Parameter, time_point, formats, args...);
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
