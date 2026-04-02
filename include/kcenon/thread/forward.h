#pragma once

// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/**
 * @file forward.h
 * @brief Forward declarations for thread system types
 * @date 2025-11-20
 *
 * This file provides forward declarations for all types in thread_system.
 * Include this header when you need to declare pointers or references to
 * thread_system types without pulling in full definitions.
 *
 * This is the single authoritative source for forward declarations in the
 * thread_system library. All forward declarations are consolidated here to
 * comply with the Simple Design principle of "No Duplication".
 */

#include <cstdint>

namespace kcenon::thread {

// ============================================================================
// Core types
// ============================================================================

/// @brief Main thread pool managing worker threads and job dispatch
class thread_pool;
/// @brief Configuration parameters for thread pool initialization
struct thread_pool_config;
/// @brief Worker thread that executes jobs from the queue
class thread_worker;
/// @brief Base class for all thread types
class thread_base;
/// @brief Unit of work submitted to a thread pool
class job;
/// @brief Thread-safe queue for pending jobs
class job_queue;
/// @brief Cooperative cancellation token for job cancellation
class cancellation_token;

// ============================================================================
// Typed thread pool types
// ============================================================================

/// @brief Default job type enumeration
enum class job_types : uint8_t;

/// @brief Typed thread pool template
template<typename JobType>
class typed_thread_pool_t;

/// @brief Typed thread worker template
template<typename JobType>
class typed_thread_worker_t;

/// @brief Typed job template
template<typename JobType>
class typed_job_t;

/// @brief Callback-based typed job template
template<typename JobType>
class callback_typed_job_t;

/// @brief Typed job interface template
template<typename JobType>
class typed_job_interface;

// ============================================================================
// Builder and policy types
// ============================================================================

/// @brief Fluent builder for constructing thread pool instances
class thread_pool_builder;
/// @brief Factory for creating pre-configured thread pool instances
class pool_factory;
/// @brief Policy controlling worker thread behavior
struct worker_policy;
/// @brief Strategy for distributing jobs across workers
enum class scheduling_policy;
/// @brief Current lifecycle state of a worker thread
enum class worker_state;

// ============================================================================
// Pool policy types
// ============================================================================

/// @brief Base class for thread pool behavior policies
class pool_policy;
/// @brief Policy controlling circuit breaker failure handling
class circuit_breaker_policy;

/// @brief Typed thread pool builder template
template<typename JobType>
class typed_thread_pool_builder;

// ============================================================================
// Scaling and resilience types
// ============================================================================

// circuit_breaker and circuit_breaker_config live in kcenon::common::resilience
// and are imported via using declarations in headers that need them
/// @brief Dynamic worker count adjuster based on load metrics
class autoscaler;
/// @brief Configuration for autoscaling thresholds and intervals
struct autoscaling_policy;
/// @brief NUMA-aware work stealing scheduler
class numa_work_stealer;
/// @brief Interface for adapting pool queues to different implementations
class pool_queue_adapter_interface;

// ============================================================================
// Queue types
// ============================================================================

/// @brief Job queue that adapts sync strategy based on contention
class adaptive_job_queue;

// ============================================================================
// Policy types (in policies sub-namespace)
// ============================================================================

namespace policies {
    // Sync policies
    class mutex_sync_policy;
    class lockfree_sync_policy;
    class adaptive_sync_policy;

    // Bound policies
    class unbounded_policy;
    class bounded_policy;
    class dynamic_bounded_policy;

    // Overflow policies
    class overflow_reject_policy;
    class overflow_block_policy;
    class overflow_drop_oldest_policy;
    class overflow_drop_newest_policy;
    class overflow_timeout_policy;
}  // namespace policies

/// @brief Policy-based queue template
template<typename SyncPolicy, typename BoundPolicy, typename OverflowPolicy>
class policy_queue;

// ============================================================================
// Internal implementation types (detail namespace)
// ============================================================================

namespace detail {
    class lockfree_job_queue;

    template<typename T>
    class concurrent_queue;
}  // namespace detail

// ============================================================================
// Metrics types (in metrics sub-namespace)
// ============================================================================

namespace metrics {
    class MetricsBase;
    class ThreadPoolMetrics;
    class EnhancedThreadPoolMetrics;
    class MetricsBackend;
    class PrometheusBackend;
    class JsonBackend;
    class LoggingBackend;
    class BackendRegistry;
    struct BaseSnapshot;
    struct EnhancedSnapshot;
    struct WorkerMetrics;
}  // namespace metrics

// ============================================================================
// Diagnostics types (in diagnostics sub-namespace)
// ============================================================================

namespace diagnostics {
    class thread_pool_diagnostics;
    struct job_info;
    struct thread_info;
    struct health_status;
    struct component_health;
    struct bottleneck_report;
    struct job_execution_event;
    struct diagnostics_config;
    class execution_event_listener;

    enum class job_status;
    enum class worker_state;
    enum class health_state;
    enum class bottleneck_type;
    enum class event_type;
}  // namespace diagnostics

// ============================================================================
// Synchronization primitives (in sync sub-namespace)
// ============================================================================

namespace sync {
    template<typename Mutex>
    class scoped_lock_guard;
}

// ============================================================================
// Async operation types (C++20 only)
// ============================================================================

#if __cplusplus >= 202002L && __has_include(<coroutine>)
template<typename T>
class task;

template<typename T>
class awaitable;
#endif

} // namespace kcenon::thread
