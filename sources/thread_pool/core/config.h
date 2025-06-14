#pragma once

/**
 * @file config.h
 * @brief Central configuration for thread_pool module
 * 
 * This file contains compile-time configuration constants and default values
 * that can be used throughout the thread_pool module.
 */

#include <cstddef>
#include <chrono>

namespace thread_pool_module::config {
    
    // Thread management configuration
    constexpr size_t default_thread_count = 4;
    constexpr size_t max_threads = 64;
    constexpr size_t min_threads = 1;
    
    // Queue configuration
    constexpr size_t default_queue_size = 1024;
    constexpr size_t unlimited_queue_size = 0;
    
    // Timing configuration
    constexpr auto default_wake_interval = std::chrono::milliseconds(100);
    constexpr auto default_shutdown_timeout = std::chrono::seconds(5);
    constexpr auto default_worker_idle_timeout = std::chrono::seconds(30);
    
    // Performance configuration
    constexpr bool default_yield_on_idle = true;
    constexpr bool default_work_stealing = false;
    constexpr bool default_pin_threads = false;
    constexpr bool default_use_priorities = false;
    
    // Resource limits
    constexpr size_t max_queue_size = 1024 * 1024; // 1M jobs max
    constexpr size_t default_stack_size = 1024 * 1024; // 1MB stack
    
    // Feature flags
    constexpr bool enable_coroutines = __cplusplus >= 202002L;
    constexpr bool enable_statistics = true;
    constexpr bool enable_debugging = false;
    
    // Thread naming
    constexpr const char* default_thread_prefix = "worker";
    constexpr const char* default_pool_name = "thread_pool";
    
    /**
     * @brief Validates configuration values at compile time
     */
    static_assert(default_thread_count >= min_threads, "Thread count must be at least minimum");
    static_assert(default_thread_count <= max_threads, "Thread count must not exceed maximum");
    static_assert(default_queue_size > 0 || default_queue_size == unlimited_queue_size, 
                  "Queue size must be positive or unlimited");
    static_assert(max_queue_size > default_queue_size, "Max queue size must exceed default");
    
} // namespace thread_pool_module::config