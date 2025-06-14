#pragma once

/**
 * @file forward_declarations.h
 * @brief Forward declarations for typed_thread_pool module
 * 
 * This file contains forward declarations to reduce compile-time dependencies
 * and prevent circular includes within the typed_thread_pool module.
 */

namespace typed_thread_pool_module {
    
    // Core types
    enum class job_types : uint8_t;
    
    // Job types
    template<typename job_type>
    class typed_job_t;
    
    template<typename job_type>
    class callback_typed_job_t;
    
    // Scheduling types
    template<typename job_type>
    class typed_job_queue_t;
    
    template<typename job_type>
    class typed_thread_worker_t;
    
    // Pool types
    template<typename job_type>
    class typed_thread_pool_t;
    
    template<typename job_type>
    class typed_thread_pool_builder;
    
    // Interface types
    template<typename job_type>
    class typed_job_interface;
    
} // namespace typed_thread_pool_module