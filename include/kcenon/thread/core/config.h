#pragma once

// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/**
 * @file config.h
 * @brief Central configuration for thread_pool module
 * 
 * This file contains compile-time configuration constants and default values
 * that can be used throughout the thread_pool module.
 */

#include <cstddef>
#include <chrono>

namespace kcenon::thread::config {
    
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

    /**
     * @brief Enable work stealing for improved load balancing
     *
     * Work stealing allows idle workers to steal jobs from busy workers,
     * reducing tail latency and improving throughput in mixed workloads.
     *
     * Architecture (when enabled):
     * - Each worker maintains a local LIFO deque (Chase-Lev algorithm)
     * - Workers push/pop from their own deque (cache-friendly)
     * - Idle workers steal from top of other workers' deques (FIFO)
     * - Global queue serves as fallback for work submission
     *
     * Performance Characteristics:
     * - Reduced global queue contention (~50-70% less CAS operations)
     * - Better cache locality for job execution
     * - Lower tail latency for mixed job sizes
     * - Slight overhead for steal attempts (backoff mechanism)
     *
     * @see worker_policy for tuning steal_backoff and max_steal_attempts
     * @see steal_policy for victim selection strategies
     */
#ifdef THREAD_WORK_STEALING_ENABLED
    constexpr bool default_work_stealing = true;
#else
    constexpr bool default_work_stealing = false;
#endif

    // Work-stealing tuning parameters

    /**
     * @brief Default maximum steal attempts before backing off.
     */
    constexpr size_t default_max_steal_attempts = 3;

    /**
     * @brief Default backoff duration between steal attempts (microseconds).
     */
    constexpr auto default_steal_backoff = std::chrono::microseconds(50);

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
    
} // namespace kcenon::thread::config