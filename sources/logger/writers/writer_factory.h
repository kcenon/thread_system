#pragma once

/**
 * @file writer_factory.h
 * @brief Factory for creating log writers
 * 
 * This file provides factory methods for creating different types
 * of log writers with appropriate configurations.
 */

#include "../detail/forward_declarations.h"
#include "../core/config.h"
#include "base_writer.h"
#include <memory>
#include <string>
#include <functional>

namespace log_module {
    
    /**
     * @brief Factory class for creating log writers
     * 
     * This class provides static methods to create different types
     * of log writers with appropriate configurations and optimizations.
     */
    class writer_factory {
    public:
        /**
         * @brief Create a console writer
         * 
         * @param config Console writer configuration
         * @return Shared pointer to the created console writer
         */
        static writer_ptr create_console_writer(
            const config::console_writer_config& config = {}
        );
        
        /**
         * @brief Create a file writer
         * 
         * @param filename Path to the log file
         * @param config File writer configuration
         * @return Shared pointer to the created file writer
         */
        static writer_ptr create_file_writer(
            const std::string& filename,
            const config::file_writer_config& config = {}
        );
        
        /**
         * @brief Create a callback writer
         * 
         * @param callback Function to call for each log message
         * @param config Writer configuration
         * @return Shared pointer to the created callback writer
         */
        static writer_ptr create_callback_writer(
            std::function<void(log_types, const std::string&)> callback,
            const config::writer_config& config = {}
        );
        
        /**
         * @brief Create a rotating file writer
         * 
         * Creates a file writer that automatically rotates log files
         * when they reach a certain size or age.
         * 
         * @param base_filename Base name for log files
         * @param max_size Maximum size before rotation
         * @param max_files Maximum number of backup files
         * @return Shared pointer to the created rotating file writer
         */
        static writer_ptr create_rotating_file_writer(
            const std::string& base_filename,
            size_t max_size = config::default_max_file_size,
            size_t max_files = config::default_max_backup_files
        );
        
        /**
         * @brief Create a daily rotating file writer
         * 
         * Creates a file writer that creates a new log file each day.
         * 
         * @param base_filename Base name for log files
         * @param keep_days Number of days to keep old files
         * @return Shared pointer to the created daily rotating file writer
         */
        static writer_ptr create_daily_file_writer(
            const std::string& base_filename,
            size_t keep_days = 7
        );
        
        /**
         * @brief Create a syslog writer (Unix/Linux only)
         * 
         * @param facility Syslog facility to use
         * @param ident Program identification string
         * @return Shared pointer to the created syslog writer
         */
        static writer_ptr create_syslog_writer(
            const std::string& ident = "app",
            int facility = 16 // LOG_LOCAL0
        );
        
        /**
         * @brief Create a network writer
         * 
         * Creates a writer that sends log messages over the network.
         * 
         * @param host Target host address
         * @param port Target port number
         * @param protocol Protocol to use ("tcp" or "udp")
         * @return Shared pointer to the created network writer
         */
        static writer_ptr create_network_writer(
            const std::string& host,
            uint16_t port,
            const std::string& protocol = "tcp"
        );
        
        /**
         * @brief Create a buffered writer
         * 
         * Wraps another writer with buffering capabilities for better performance.
         * 
         * @param underlying_writer The writer to wrap
         * @param buffer_size Size of the internal buffer
         * @param flush_interval How often to automatically flush
         * @return Shared pointer to the created buffered writer
         */
        static writer_ptr create_buffered_writer(
            writer_ptr underlying_writer,
            size_t buffer_size = config::default_message_buffer_size,
            std::chrono::milliseconds flush_interval = config::default_flush_interval
        );
        
        /**
         * @brief Create a filtered writer
         * 
         * Wraps another writer with additional filtering capabilities.
         * 
         * @param underlying_writer The writer to wrap
         * @param filter Function that returns true if message should be written
         * @return Shared pointer to the created filtered writer
         */
        static writer_ptr create_filtered_writer(
            writer_ptr underlying_writer,
            std::function<bool(log_types, const std::string&)> filter
        );
        
        /**
         * @brief Create a composite writer
         * 
         * Creates a writer that forwards messages to multiple other writers.
         * 
         * @param writers List of writers to forward to
         * @return Shared pointer to the created composite writer
         */
        static writer_ptr create_composite_writer(
            const std::vector<writer_ptr>& writers
        );
        
        /**
         * @brief Create a null writer
         * 
         * Creates a writer that discards all messages (for testing/benchmarking).
         * 
         * @return Shared pointer to the created null writer
         */
        static writer_ptr create_null_writer();
        
        /**
         * @brief Writer creation options
         */
        struct writer_options {
            bool async_mode = false;
            bool buffered = false;
            size_t buffer_size = config::default_message_buffer_size;
            std::chrono::milliseconds flush_interval = config::default_flush_interval;
            log_types min_level = config::default_log_level;
            std::string format = config::default_log_format;
        };
        
        /**
         * @brief Create a writer from a URI-style string
         * 
         * Examples:
         * - "console://stderr?colored=true"
         * - "file:///path/to/log.txt?rotate=true&max_size=10MB"
         * - "tcp://localhost:514"
         * - "syslog://local0?ident=myapp"
         * 
         * @param uri URI describing the writer type and configuration
         * @param options Additional options for the writer
         * @return Shared pointer to the created writer
         */
        static writer_ptr create_from_uri(
            const std::string& uri,
            const writer_options& options = {}
        );
        
        /**
         * @brief Get list of available writer types
         * 
         * @return Vector of supported writer type names
         */
        static std::vector<std::string> get_available_types();
        
    private:
        /**
         * @brief Parse URI components
         */
        struct uri_components {
            std::string scheme;
            std::string host;
            std::string path;
            uint16_t port = 0;
            std::map<std::string, std::string> query_params;
        };
        
        static uri_components parse_uri(const std::string& uri);
        static writer_ptr create_from_components(const uri_components& components, const writer_options& options);
    };
    
} // namespace log_module