#pragma once

/**
 * @file config.h
 * @brief Central configuration for typed_thread_pool module
 * 
 * This file contains compile-time configuration constants and default values
 * that can be used throughout the typed_thread_pool module.
 */

#include "job_types.h"
#include <cstddef>

namespace typed_thread_pool_module::config {
    
    // Default type settings
    using default_job_type = job_types;
    
    // Performance configuration
    constexpr size_t default_queue_size = 1024;
    constexpr size_t default_worker_count = 4;
    constexpr size_t max_workers = 64;
    constexpr size_t min_workers = 1;
    
    // Queue behavior settings
    constexpr size_t default_wait_timeout_ms = 100;
    constexpr size_t default_shutdown_timeout_ms = 5000;
    
    // Feature flags
    constexpr bool enable_statistics = true;
    constexpr bool enable_priority_boost = false;
    constexpr bool enable_work_stealing = true;
    constexpr bool enable_adaptive_sizing = false;
    
    // Memory management
    constexpr size_t default_job_pool_size = 512;
    constexpr bool enable_job_recycling = true;
    
    // Debugging and monitoring
    constexpr bool enable_debug_logging = false;
    constexpr bool enable_performance_monitoring = false;
    constexpr size_t monitoring_interval_ms = 1000;
    
    /**
     * @brief Validates configuration values at compile time
     */
    static_assert(default_queue_size > 0, "Queue size must be positive");
    static_assert(default_worker_count >= min_workers, "Worker count must be at least minimum");
    static_assert(default_worker_count <= max_workers, "Worker count must not exceed maximum");
    static_assert(default_wait_timeout_ms > 0, "Wait timeout must be positive");
    static_assert(default_shutdown_timeout_ms > 0, "Shutdown timeout must be positive");
    
} // namespace typed_thread_pool_module::config