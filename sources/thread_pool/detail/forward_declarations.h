#pragma once

/**
 * @file forward_declarations.h
 * @brief Forward declarations for thread_pool module
 * 
 * This file contains forward declarations to reduce compile-time dependencies
 * and prevent circular includes within the thread_pool module.
 */

namespace thread_pool_module {
    
    // Core types
    class thread_pool;
    struct thread_pool_config;
    
    // Worker types
    class thread_worker;
    enum class worker_state;
    
    // Builder types
    class thread_pool_builder;
    class pool_factory;
    
    // Policy types
    struct worker_policy;
    enum class scheduling_policy;
    
    // Async types (forward declare if coroutines are available)
    #if __cplusplus >= 202002L && __has_include(<coroutine>)
    template<typename T>
    class task;
    
    template<typename T>
    class awaitable;
    #endif
    
} // namespace thread_pool_module