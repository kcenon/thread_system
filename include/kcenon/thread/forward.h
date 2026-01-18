#pragma once

/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, kcenon
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

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

class thread_pool;
struct thread_pool_config;
class thread_worker;
class thread_base;
class job;
class job_queue;
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

class thread_pool_builder;
class pool_factory;
struct worker_policy;
enum class scheduling_policy;
enum class worker_state;

// ============================================================================
// Pool policy types
// ============================================================================

class pool_policy;
class circuit_breaker_policy;

/// @brief Typed thread pool builder template
template<typename JobType>
class typed_thread_pool_builder;

// ============================================================================
// Scaling and resilience types
// ============================================================================

class circuit_breaker;
struct circuit_breaker_config;
class autoscaler;
struct autoscaling_policy;
class numa_work_stealer;
class pool_queue_adapter_interface;

// ============================================================================
// Queue types
// ============================================================================

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
