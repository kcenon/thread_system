#pragma once

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

/**
 * @file configuration.h
 * @brief Central configuration for logger module
 * 
 * This file contains compile-time configuration constants and default values
 * that can be used throughout the logger module.
 */

#include "../types/log_types.h"
#include <chrono>
#include <cstddef>
#include <string>

namespace log_module::config {
    
    // Default log level settings
    constexpr log_types default_log_level = log_types::Information;
    constexpr log_types minimum_log_level = log_types::Debug;
    constexpr log_types maximum_log_level = log_types::Error;
    
    // Performance configuration
    constexpr size_t default_queue_size = 1024;
    constexpr size_t max_queue_size = 10000;
    constexpr size_t default_batch_size = 10;
    
    // Timing configuration
    constexpr auto default_flush_interval = std::chrono::milliseconds(100);
    constexpr auto default_shutdown_timeout = std::chrono::seconds(5);
    constexpr auto default_file_rotation_check = std::chrono::hours(1);
    
    // File writer configuration
    constexpr size_t default_max_file_size = 10 * 1024 * 1024; // 10MB
    constexpr size_t default_max_backup_files = 5;
    constexpr const char* default_log_extension = ".log";
    constexpr const char* default_timestamp_format = "%Y-%m-%d %H:%M:%S";
    
    // Console writer configuration
    constexpr bool default_color_output = true;
    constexpr bool default_console_timestamps = true;
    
    // Thread configuration
    constexpr size_t default_worker_threads = 1;
    constexpr size_t max_worker_threads = 4;
    
    // Buffer configuration
    constexpr size_t default_message_buffer_size = 4096;
    constexpr size_t max_message_buffer_size = 65536;
    constexpr size_t default_line_buffer_size = 1024;
    
    // Feature flags
    constexpr bool enable_thread_safety = true;
    constexpr bool enable_async_logging = true;
    constexpr bool enable_file_rotation = true;
    constexpr bool enable_compression = false;
    constexpr bool enable_structured_logging = false;
    constexpr bool enable_statistics = true;
    
    // Format configuration
    constexpr const char* default_log_format = "[%timestamp%] [%level%] [%thread%] %message%";
    constexpr const char* default_file_pattern = "%name%_%date%.log";
    
    // Logger naming
    constexpr const char* default_logger_name = "default";
    constexpr const char* system_logger_name = "system";
    constexpr const char* error_logger_name = "error";
    
    /**
     * @brief Logger configuration structure
     */
    struct logger_config {
        log_types min_level = default_log_level;
        size_t queue_size = default_queue_size;
        size_t worker_threads = default_worker_threads;
        std::chrono::milliseconds flush_interval = default_flush_interval;
        bool async_mode = enable_async_logging;
        std::string format = default_log_format;
        std::string name = default_logger_name;
        
        /**
         * @brief Create default configuration
         */
        static logger_config default_config() {
            return logger_config{};
        }
        
        /**
         * @brief Create high-performance configuration
         */
        static logger_config high_performance() {
            logger_config cfg;
            cfg.queue_size = max_queue_size;
            cfg.worker_threads = max_worker_threads;
            cfg.flush_interval = std::chrono::milliseconds(10);
            cfg.async_mode = true;
            return cfg;
        }
        
        /**
         * @brief Create debug configuration
         */
        static logger_config debug_config() {
            logger_config cfg;
            cfg.min_level = log_types::Debug;
            cfg.async_mode = false; // Synchronous for immediate output
            return cfg;
        }
    };
    
    /**
     * @brief Writer configuration structure
     */
    struct writer_config {
        bool enabled = true;
        log_types min_level = default_log_level;
        std::string format = default_log_format;
        size_t buffer_size = default_message_buffer_size;
    };
    
    /**
     * @brief File writer specific configuration
     */
    struct file_writer_config : writer_config {
        std::string filename;
        size_t max_file_size = default_max_file_size;
        size_t max_backup_files = default_max_backup_files;
        bool auto_flush = true;
        bool append_mode = true;
        std::chrono::hours rotation_check_interval = default_file_rotation_check;
    };
    
    /**
     * @brief Console writer specific configuration
     */
    struct console_writer_config : writer_config {
        bool colored_output = default_color_output;
        bool show_timestamps = default_console_timestamps;
        bool use_stderr_for_errors = true;
    };
    
    /**
     * @brief Validates configuration values at compile time
     */
    static_assert(default_queue_size > 0, "Queue size must be positive");
    static_assert(default_queue_size <= max_queue_size, "Default queue size must not exceed maximum");
    static_assert(default_worker_threads >= 1, "Must have at least one worker thread");
    static_assert(default_worker_threads <= max_worker_threads, "Worker threads must not exceed maximum");
    static_assert(default_max_file_size > 1024, "File size must be reasonable");
    
} // namespace log_module::config