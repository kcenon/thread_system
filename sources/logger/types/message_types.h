#pragma once

/**
 * @file message_types.h
 * @brief Message format definitions and utilities
 * 
 * This file defines structures and utilities for different
 * log message formats and representations.
 */

#include "log_types.h"
#include "../../utilities/core/formatter.h"
#include <string>
#include <chrono>
#include <map>
#include <thread>

namespace log_module {
    
    /**
     * @brief Structure representing a complete log message
     * 
     * This structure contains all the information needed to
     * format and output a log message.
     */
    struct log_message {
        log_types level = log_types::info;
        std::string content;
        std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
        std::thread::id thread_id = std::this_thread::get_id();
        std::string logger_name;
        std::string source_location;
        std::map<std::string, std::string> fields; // For structured logging
        
        /**
         * @brief Default constructor
         */
        log_message() = default;
        
        /**
         * @brief Constructor with basic information
         */
        log_message(log_types lvl, std::string msg, std::string name = "")
            : level(lvl), content(std::move(msg)), logger_name(std::move(name)) {}
        
        /**
         * @brief Constructor with timestamp
         */
        log_message(log_types lvl, std::string msg, 
                   std::chrono::system_clock::time_point ts, 
                   std::string name = "")
            : level(lvl), content(std::move(msg)), timestamp(ts), logger_name(std::move(name)) {}
        
        /**
         * @brief Add a field for structured logging
         */
        void add_field(const std::string& key, const std::string& value) {
            fields[key] = value;
        }
        
        /**
         * @brief Add multiple fields
         */
        void add_fields(const std::map<std::string, std::string>& new_fields) {
            fields.insert(new_fields.begin(), new_fields.end());
        }
        
        /**
         * @brief Get a field value
         */
        std::string get_field(const std::string& key, const std::string& default_value = "") const {
            auto it = fields.find(key);
            return it != fields.end() ? it->second : default_value;
        }
        
        /**
         * @brief Check if a field exists
         */
        bool has_field(const std::string& key) const {
            return fields.find(key) != fields.end();
        }
        
        /**
         * @brief Convert to string representation
         */
        std::string to_string() const;
        
        /**
         * @brief Convert to JSON string (if structured logging is enabled)
         */
        std::string to_json() const;
    };
    
    /**
     * @brief Message formatting options
     */
    struct format_options {
        bool include_timestamp = true;
        bool include_level = true;
        bool include_thread_id = false;
        bool include_logger_name = true;
        bool include_source_location = false;
        bool use_color = false;
        bool structured_format = false;
        std::string timestamp_format = "%Y-%m-%d %H:%M:%S";
        std::string level_format = "uppercase"; // "uppercase", "lowercase", "abbreviated"
    };
    
    /**
     * @brief Message formatter class
     * 
     * This class is responsible for formatting log messages
     * according to specified patterns and options.
     */
    class message_formatter {
    public:
        /**
         * @brief Constructor with format pattern
         */
        explicit message_formatter(const std::string& pattern = "[%timestamp%] [%level%] %message%");
        
        /**
         * @brief Format a log message
         */
        std::string format(const log_message& message, const format_options& options = {}) const;
        
        /**
         * @brief Set the format pattern
         */
        void set_pattern(const std::string& pattern);
        
        /**
         * @brief Get the current format pattern
         */
        const std::string& get_pattern() const { return pattern_; }
        
        /**
         * @brief Available format tokens
         */
        static const std::map<std::string, std::string>& get_available_tokens();
        
    private:
        std::string pattern_;
        
        std::string replace_tokens(const std::string& pattern, const log_message& message, 
                                 const format_options& options) const;
        std::string format_timestamp(const std::chrono::system_clock::time_point& timestamp,
                                   const std::string& format) const;
        std::string format_level(log_types level, const std::string& format_type, bool use_color) const;
        std::string format_thread_id(const std::thread::id& thread_id) const;
    };
    
    /**
     * @brief Predefined message formatters
     */
    namespace formatters {
        
        /**
         * @brief Simple format: "[LEVEL] message"
         */
        message_formatter simple();
        
        /**
         * @brief Standard format: "[timestamp] [LEVEL] message"
         */
        message_formatter standard();
        
        /**
         * @brief Detailed format: "[timestamp] [LEVEL] [thread] [logger] message"
         */
        message_formatter detailed();
        
        /**
         * @brief JSON format for structured logging
         */
        message_formatter json();
        
        /**
         * @brief Colored console format
         */
        message_formatter colored_console();
        
        /**
         * @brief Syslog-compatible format
         */
        message_formatter syslog();
        
    } // namespace formatters
    
    /**
     * @brief Message priority for queue ordering
     */
    enum class message_priority : uint8_t {
        low = 0,
        normal = 1,
        high = 2,
        critical = 3
    };
    
    /**
     * @brief Convert log type to message priority
     */
    constexpr message_priority to_priority(log_types level) {
        switch (level) {
            case log_types::trace:
            case log_types::debug:
                return message_priority::low;
            case log_types::info:
            case log_types::warn:
                return message_priority::normal;
            case log_types::error:
                return message_priority::high;
            case log_types::fatal:
                return message_priority::critical;
            default:
                return message_priority::normal;
        }
    }
    
    /**
     * @brief Message batch for efficient processing
     */
    struct message_batch {
        std::vector<log_message> messages;
        message_priority max_priority = message_priority::low;
        std::chrono::steady_clock::time_point created_at = std::chrono::steady_clock::now();
        
        void add_message(log_message msg) {
            auto priority = to_priority(msg.level);
            if (priority > max_priority) {
                max_priority = priority;
            }
            messages.emplace_back(std::move(msg));
        }
        
        bool empty() const { return messages.empty(); }
        size_t size() const { return messages.size(); }
        void clear() { 
            messages.clear(); 
            max_priority = message_priority::low;
        }
    };
    
} // namespace log_module