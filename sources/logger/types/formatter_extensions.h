#pragma once

/*
 * BSD 3-Clause License
 * 
 * Copyright (c) 2024, DongCheol Shin
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file formatter_extensions.h
 * @brief Logger-specific formatting extensions
 * 
 * This file provides formatting extensions and utilities
 * specifically designed for the logging system.
 */

#include "log_types.h"
#include "message_types.h"
#include "../../utilities/core/formatter.h"
#include "../../utilities/conversion/convert_string.h"
#include <string>
#include <sstream>
#include <iomanip>

namespace log_module {
    
    /**
     * @brief ANSI color codes for console output
     */
    namespace colors {
        constexpr const char* reset = "\033[0m";
        constexpr const char* bold = "\033[1m";
        constexpr const char* dim = "\033[2m";
        constexpr const char* underline = "\033[4m";
        
        // Foreground colors
        constexpr const char* black = "\033[30m";
        constexpr const char* red = "\033[31m";
        constexpr const char* green = "\033[32m";
        constexpr const char* yellow = "\033[33m";
        constexpr const char* blue = "\033[34m";
        constexpr const char* magenta = "\033[35m";
        constexpr const char* cyan = "\033[36m";
        constexpr const char* white = "\033[37m";
        
        // Bright foreground colors
        constexpr const char* bright_black = "\033[90m";
        constexpr const char* bright_red = "\033[91m";
        constexpr const char* bright_green = "\033[92m";
        constexpr const char* bright_yellow = "\033[93m";
        constexpr const char* bright_blue = "\033[94m";
        constexpr const char* bright_magenta = "\033[95m";
        constexpr const char* bright_cyan = "\033[96m";
        constexpr const char* bright_white = "\033[97m";
        
        // Background colors
        constexpr const char* bg_black = "\033[40m";
        constexpr const char* bg_red = "\033[41m";
        constexpr const char* bg_green = "\033[42m";
        constexpr const char* bg_yellow = "\033[43m";
        constexpr const char* bg_blue = "\033[44m";
        constexpr const char* bg_magenta = "\033[45m";
        constexpr const char* bg_cyan = "\033[46m";
        constexpr const char* bg_white = "\033[47m";
    }
    
    /**
     * @brief Get color code for a log level
     */
    constexpr const char* get_level_color(log_types level) {
        switch (level) {
            case log_types::trace: return colors::dim;
            case log_types::debug: return colors::cyan;
            case log_types::info: return colors::green;
            case log_types::warn: return colors::yellow;
            case log_types::error: return colors::red;
            case log_types::fatal: return colors::bright_red;
            default: return colors::reset;
        }
    }
    
    /**
     * @brief Format a log level with color
     */
    inline std::string format_colored_level(log_types level, bool use_color = true) {
        std::string level_str = to_string(level);
        
        if (!use_color) {
            return level_str;
        }
        
        return std::string(get_level_color(level)) + level_str + colors::reset;
    }
    
    /**
     * @brief Advanced timestamp formatting utilities
     */
    namespace timestamp {
        
        /**
         * @brief Format timestamp with microsecond precision
         */
        std::string format_precise(const std::chrono::system_clock::time_point& tp);
        
        /**
         * @brief Format timestamp in ISO 8601 format
         */
        std::string format_iso8601(const std::chrono::system_clock::time_point& tp);
        
        /**
         * @brief Format timestamp as Unix epoch
         */
        std::string format_epoch(const std::chrono::system_clock::time_point& tp);
        
        /**
         * @brief Format timestamp with custom format string
         */
        std::string format_custom(const std::chrono::system_clock::time_point& tp, 
                                const std::string& format);
        
        /**
         * @brief Format elapsed time since program start
         */
        std::string format_elapsed(const std::chrono::system_clock::time_point& tp);
        
    } // namespace timestamp
    
    /**
     * @brief Thread ID formatting utilities
     */
    namespace thread_format {
        
        /**
         * @brief Format thread ID as hexadecimal
         */
        std::string format_hex(const std::thread::id& thread_id);
        
        /**
         * @brief Format thread ID as decimal
         */
        std::string format_decimal(const std::thread::id& thread_id);
        
        /**
         * @brief Format thread ID with custom width
         */
        std::string format_fixed_width(const std::thread::id& thread_id, size_t width = 8);
        
    } // namespace thread_format
    
    /**
     * @brief Message content formatting utilities
     */
    namespace content {
        
        /**
         * @brief Escape special characters in log message
         */
        std::string escape_content(const std::string& content);
        
        /**
         * @brief Truncate message to maximum length
         */
        std::string truncate(const std::string& content, size_t max_length = 1024);
        
        /**
         * @brief Replace newlines with escape sequences
         */
        std::string escape_newlines(const std::string& content);
        
        /**
         * @brief Remove control characters
         */
        std::string sanitize(const std::string& content);
        
        /**
         * @brief Format multiline messages with proper indentation
         */
        std::string format_multiline(const std::string& content, const std::string& indent = "  ");
        
    } // namespace content
    
    /**
     * @brief JSON formatting utilities for structured logging
     */
    namespace json {
        
        /**
         * @brief Escape string for JSON
         */
        std::string escape_string(const std::string& str);
        
        /**
         * @brief Format log message as JSON
         */
        std::string format_message(const log_message& message);
        
        /**
         * @brief Format fields map as JSON object
         */
        std::string format_fields(const std::map<std::string, std::string>& fields);
        
        /**
         * @brief Create compact JSON representation
         */
        std::string format_compact(const log_message& message);
        
        /**
         * @brief Create pretty-printed JSON representation
         */
        std::string format_pretty(const log_message& message, size_t indent = 2);
        
    } // namespace json
    
    /**
     * @brief Performance metrics formatting
     */
    namespace metrics {
        
        /**
         * @brief Format byte count with units
         */
        std::string format_bytes(size_t bytes);
        
        /**
         * @brief Format duration with appropriate units
         */
        std::string format_duration(std::chrono::nanoseconds duration);
        
        /**
         * @brief Format rate (messages per second)
         */
        std::string format_rate(double rate);
        
        /**
         * @brief Format percentage
         */
        std::string format_percentage(double percentage);
        
    } // namespace metrics
    
    /**
     * @brief Custom formatter for specific log patterns
     */
    class pattern_formatter {
    public:
        /**
         * @brief Constructor with pattern string
         */
        explicit pattern_formatter(const std::string& pattern);
        
        /**
         * @brief Format a log message using the pattern
         */
        std::string format(const log_message& message) const;
        
        /**
         * @brief Set new pattern
         */
        void set_pattern(const std::string& pattern);
        
        /**
         * @brief Get current pattern
         */
        const std::string& get_pattern() const { return pattern_; }
        
        /**
         * @brief Available pattern tokens
         */
        static std::vector<std::string> get_available_tokens();
        
    private:
        std::string pattern_;
        
        struct token_info {
            std::string name;
            std::function<std::string(const log_message&)> formatter;
        };
        
        static const std::map<std::string, token_info>& get_token_map();
        
        std::string replace_tokens(const std::string& pattern, const log_message& message) const;
    };
    
} // namespace log_module

// Formatter specializations for log types
#ifdef USE_STD_FORMAT

template<>
struct std::formatter<log_module::log_types> : std::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const log_module::log_types& level, FormatContext& ctx) const {
        return std::formatter<std::string_view>::format(log_module::to_string(level), ctx);
    }
};

template<>
struct std::formatter<log_module::log_message> : std::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const log_module::log_message& message, FormatContext& ctx) const {
        return std::formatter<std::string_view>::format(message.to_string(), ctx);
    }
};

#else

template<>
struct fmt::formatter<log_module::log_types> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const log_module::log_types& level, FormatContext& ctx) const {
        return fmt::formatter<std::string_view>::format(log_module::to_string(level), ctx);
    }
};

template<>
struct fmt::formatter<log_module::log_message> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const log_module::log_message& message, FormatContext& ctx) const {
        return fmt::formatter<std::string_view>::format(message.to_string(), ctx);
    }
};

#endif