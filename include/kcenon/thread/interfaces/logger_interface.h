// BSD 3-Clause License
//
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Copyright (c) 2025, 🍀☀🌕🌥 🌊
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

/**
 * @file logger_interface.h
 * @brief DEPRECATED: This interface is deprecated and will be removed in v2.0
 *
 * Phase 2: Logger Interface Unification
 *
 * This interface has been superseded by common::interfaces::ILogger.
 * Please migrate to the unified logger interface in common_system:
 *
 * Migration:
 *   OLD: #include <kcenon/thread/interfaces/logger_interface.h>
 *        kcenon::thread::logger_interface* logger;
 *
 *   NEW: #include <kcenon/common/interfaces/logger_interface.h>
 *        common::interfaces::ILogger* logger;
 *
 * The unified interface provides:
 * - Result<T> based error handling
 * - Consistent log_level ordering (trace=0...critical=5)
 * - Extended configuration with ILoggerRegistry
 * - Better integration across all systems
 *
 * Deprecation Timeline:
 * - v1.x: Deprecated but functional (current)
 * - v2.0: Removed entirely
 *
 * @deprecated Use common::interfaces::ILogger instead
 */

#include <memory>
#include <mutex>
#include <string>

namespace kcenon::thread {

/**
 * @brief Log level enumeration
 * @deprecated Use common::interfaces::log_level instead
 *
 * WARNING: This enumeration has inverted ordering (critical=0, trace=5).
 * The unified interface uses standard ordering (trace=0, critical=5).
 */
enum class [[deprecated("Use common::interfaces::log_level instead. Note: ordering differs - "
                         "common_system uses trace=0...critical=5")]] log_level {
    critical = 0,
    error = 1,
    warning = 2,
    info = 3,
    debug = 4,
    trace = 5
};

/**
 * @brief Logger interface for thread system
 * @deprecated Use common::interfaces::ILogger instead
 *
 * This interface allows the thread system to log messages without
 * depending on a specific logger implementation.
 *
 * MIGRATION: Replace with common::interfaces::ILogger which provides:
 * - VoidResult return types for better error handling
 * - Unified log_entry structure
 * - Compatible method signatures
 *
 * ### Thread Safety
 * Implementations must ensure all methods are thread-safe:
 * - log() must be safely callable from multiple threads concurrently
 * - is_enabled() must provide consistent results across threads
 * - flush() must be safely callable alongside log operations
 */
class [[deprecated("Use common::interfaces::ILogger instead")]] logger_interface {
public:
    virtual ~logger_interface() = default;

    /**
     * @brief Log a message with specified level
     * @param level Log level
     * @param message Log message
     *
     * Thread Safety: Must be thread-safe
     */
    virtual void log(log_level level, const std::string& message) = 0;

    /**
     * @brief Log a message with source location information
     * @param level Log level
     * @param message Log message
     * @param file Source file name
     * @param line Source line number
     * @param function Function name
     *
     * Thread Safety: Must be thread-safe
     */
    virtual void log(log_level level, const std::string& message, const std::string& file, int line,
                     const std::string& function) = 0;

    /**
     * @brief Check if logging is enabled for the specified level
     * @param level Log level to check
     * @return true if logging is enabled for this level
     *
     * Thread Safety: Must be thread-safe
     */
    virtual bool is_enabled(log_level level) const = 0;

    /**
     * @brief Flush any buffered log messages
     *
     * Thread Safety: Must be thread-safe
     */
    virtual void flush() = 0;
};

/**
 * @brief Global logger registry
 * @deprecated Use common::interfaces::ILoggerRegistry instead
 *
 * Manages the global logger instance used by the thread system.
 *
 * MIGRATION: Use the unified ILoggerRegistry interface which provides:
 * - Named logger support
 * - Thread-safe registration
 * - Default logger management
 */
class [[deprecated("Use common::interfaces::ILoggerRegistry instead")]] logger_registry {
public:
    /**
     * @brief Set the global logger instance
     * @param logger Logger implementation
     */
    static void set_logger(std::shared_ptr<logger_interface> logger);

    /**
     * @brief Get the global logger instance
     * @return Current logger instance, may be nullptr
     */
    static std::shared_ptr<logger_interface> get_logger();

    /**
     * @brief Clear the global logger instance
     */
    static void clear_logger();

private:
    static std::shared_ptr<logger_interface> logger_;
    static std::mutex mutex_;
};

// Convenience macros for logging
#ifndef THREAD_LOG_IF_ENABLED
    #define THREAD_LOG_IF_ENABLED(level, message)                                  \
        do {                                                                       \
            if (auto logger = kcenon::thread::logger_registry::get_logger()) {     \
                if (logger->is_enabled(level)) {                                   \
                    logger->log(level, message, __FILE__, __LINE__, __FUNCTION__); \
                }                                                                  \
            }                                                                      \
        } while (0)
#endif

#ifndef THREAD_LOG_CRITICAL
    #define THREAD_LOG_CRITICAL(message) \
        THREAD_LOG_IF_ENABLED(kcenon::thread::log_level::critical, message)
#endif

#ifndef THREAD_LOG_ERROR
    #define THREAD_LOG_ERROR(message) \
        THREAD_LOG_IF_ENABLED(kcenon::thread::log_level::error, message)
#endif

#ifndef THREAD_LOG_WARNING
    #define THREAD_LOG_WARNING(message) \
        THREAD_LOG_IF_ENABLED(kcenon::thread::log_level::warning, message)
#endif

#ifndef THREAD_LOG_INFO
    #define THREAD_LOG_INFO(message) THREAD_LOG_IF_ENABLED(kcenon::thread::log_level::info, message)
#endif

#ifndef THREAD_LOG_DEBUG
    #define THREAD_LOG_DEBUG(message) \
        THREAD_LOG_IF_ENABLED(kcenon::thread::log_level::debug, message)
#endif

#ifndef THREAD_LOG_TRACE
    #define THREAD_LOG_TRACE(message) \
        THREAD_LOG_IF_ENABLED(kcenon::thread::log_level::trace, message)
#endif

}  // namespace kcenon::thread