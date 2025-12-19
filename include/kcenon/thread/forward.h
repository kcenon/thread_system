/**
 * @file forward.h
 * @brief Forward declarations for thread system types
 * @date 2025-11-20
 *
 * This file provides forward declarations for commonly used types in
 * thread_system. Include this header when you need to declare pointers
 * or references to thread_system types without pulling in full definitions.
 *
 * For detailed forward declarations within the core module, see
 * <kcenon/thread/core/forward_declarations.h>.
 */

#pragma once

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

// typed_thread_pool_t is a template class - forward declare requires template param
template<typename JobType>
class typed_thread_pool_t;

template<typename JobType>
class typed_thread_worker_t;

template<typename JobType>
class typed_job_t;

template<typename JobType>
class typed_job_queue_t;

// Default job type enum
enum class job_types;

// ============================================================================
// Builder and policy types
// ============================================================================

class thread_pool_builder;
class pool_factory;
struct worker_policy;
enum class scheduling_policy;
enum class worker_state;

// ============================================================================
// Queue types
// ============================================================================

class lockfree_job_queue;

template<typename T>
class lockfree_queue;

class adaptive_job_queue;
class bounded_job_queue;

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
