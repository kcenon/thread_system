#pragma once

/**
 * @file forward_declarations.h
 * @brief Forward declarations for logger module
 * 
 * This file contains forward declarations to reduce compile-time dependencies
 * and prevent circular includes within the logger module.
 */

namespace log_module {
    
    // Core types
    class logger_implementation;
    class log_collector;
    
    // Job types
    class log_job;
    class message_job;
    class job_factory;
    
    // Writer types
    class base_writer;
    class console_writer;
    class file_writer;
    class callback_writer;
    class writer_factory;
    
    // Type definitions
    enum class log_types : uint8_t;
    
    // Configuration types
    struct logger_config;
    struct writer_config;
    
} // namespace log_module