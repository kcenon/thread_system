#pragma once

/**
 * @file job_factory.h
 * @brief Factory for creating log jobs
 * 
 * This file provides factory methods for creating different types
 * of log jobs with proper configuration and initialization.
 */

#include "../detail/forward_declarations.h"
#include "../types/log_types.h"
#include "log_job.h"
#include "message_job.h"
#include <memory>
#include <string>
#include <functional>

namespace log_module {
    
    /**
     * @brief Factory class for creating log jobs
     * 
     * This class provides static methods to create different types
     * of log jobs with appropriate configurations.
     */
    class job_factory {
    public:
        /**
         * @brief Create a standard log job
         * 
         * @param level The log level
         * @param message The log message
         * @param source_location Optional source location information
         * @return Unique pointer to the created log job
         */
        static std::unique_ptr<log_job> create_log_job(
            log_types level,
            const std::string& message,
            const std::string& source_location = ""
        );
        
        /**
         * @brief Create a message job
         * 
         * @param level The log level
         * @param message The log message
         * @param timestamp Optional custom timestamp
         * @return Unique pointer to the created message job
         */
        static std::unique_ptr<message_job> create_message_job(
            log_types level,
            const std::string& message,
            std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now()
        );
        
        /**
         * @brief Create a formatted log job
         * 
         * @param level The log level
         * @param format_string Format string (printf-style or std::format-style)
         * @param args Arguments for formatting
         * @return Unique pointer to the created log job
         */
        template<typename... Args>
        static std::unique_ptr<log_job> create_formatted_job(
            log_types level,
            const std::string& format_string,
            Args&&... args
        );
        
        /**
         * @brief Create a lazy log job
         * 
         * Creates a job that will only format the message when it's actually
         * going to be logged (based on log level filtering).
         * 
         * @param level The log level
         * @param message_generator Function that generates the message
         * @return Unique pointer to the created log job
         */
        static std::unique_ptr<log_job> create_lazy_job(
            log_types level,
            std::function<std::string()> message_generator
        );
        
        /**
         * @brief Create a batch log job
         * 
         * Creates a job that can handle multiple log messages in a single job.
         * 
         * @param messages Vector of log messages with their levels
         * @return Unique pointer to the created log job
         */
        static std::unique_ptr<log_job> create_batch_job(
            const std::vector<std::pair<log_types, std::string>>& messages
        );
        
        /**
         * @brief Create a conditional log job
         * 
         * Creates a job that will only execute if a condition is met.
         * 
         * @param level The log level
         * @param message The log message
         * @param condition Function that returns true if the job should execute
         * @return Unique pointer to the created log job
         */
        static std::unique_ptr<log_job> create_conditional_job(
            log_types level,
            const std::string& message,
            std::function<bool()> condition
        );
        
        /**
         * @brief Create a high-priority log job
         * 
         * Creates a job with elevated priority for critical messages.
         * 
         * @param level The log level
         * @param message The log message
         * @return Unique pointer to the created log job
         */
        static std::unique_ptr<log_job> create_priority_job(
            log_types level,
            const std::string& message
        );
        
        /**
         * @brief Create a structured log job
         * 
         * Creates a job for structured logging with key-value pairs.
         * 
         * @param level The log level
         * @param message Base message
         * @param fields Map of field names to values
         * @return Unique pointer to the created log job
         */
        static std::unique_ptr<log_job> create_structured_job(
            log_types level,
            const std::string& message,
            const std::map<std::string, std::string>& fields
        );
        
        /**
         * @brief Job creation options
         */
        struct job_options {
            bool high_priority = false;
            bool lazy_evaluation = false;
            std::string source_location;
            std::chrono::system_clock::time_point custom_timestamp = {};
            std::map<std::string, std::string> extra_fields;
        };
        
        /**
         * @brief Create a log job with custom options
         * 
         * @param level The log level
         * @param message The log message
         * @param options Job creation options
         * @return Unique pointer to the created log job
         */
        static std::unique_ptr<log_job> create_job_with_options(
            log_types level,
            const std::string& message,
            const job_options& options
        );
        
    private:
        /**
         * @brief Apply common job settings
         * 
         * @param job The job to configure
         * @param options The options to apply
         */
        static void apply_job_options(log_job* job, const job_options& options);
    };
    
    /**
     * @brief Convenience macros for job creation
     */
    #define CREATE_LOG_JOB(level, message) \
        job_factory::create_log_job(level, message, \
        std::string(__FILE__) + ":" + std::to_string(__LINE__))
    
    #define CREATE_FORMATTED_JOB(level, format, ...) \
        job_factory::create_formatted_job(level, format, __VA_ARGS__)
    
    #define CREATE_LAZY_JOB(level, generator) \
        job_factory::create_lazy_job(level, generator)
    
} // namespace log_module