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

/// @brief Typed job queue template
template<typename JobType>
class typed_job_queue_t;

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

/// @brief Typed thread pool builder template
template<typename JobType>
class typed_thread_pool_builder;

// ============================================================================
// Queue types
// ============================================================================

class adaptive_job_queue;
class bounded_job_queue;

// ============================================================================
// Internal implementation types (detail namespace)
// ============================================================================

namespace detail {
    class lockfree_job_queue;

    template<typename T>
    class concurrent_queue;
}  // namespace detail

/// @deprecated Use adaptive_job_queue or job_queue instead. lockfree_job_queue is now an internal implementation.
using lockfree_job_queue [[deprecated(
    "lockfree_job_queue is moving to detail:: namespace. "
    "Use adaptive_job_queue with policy::performance_first instead.")]] = detail::lockfree_job_queue;

/// @deprecated Use adaptive_job_queue or job_queue instead. concurrent_queue is now an internal implementation.
template<typename T>
using concurrent_queue [[deprecated(
    "concurrent_queue is moving to detail:: namespace. "
    "Use adaptive_job_queue or job_queue instead.")]] = detail::concurrent_queue<T>;

/// @deprecated MISLEADING NAME: Uses fine-grained locking, not lock-free algorithms
template<typename T>
using lockfree_queue [[deprecated(
    "MISLEADING NAME: This class uses fine-grained locking, not lock-free algorithms. "
    "Use detail::concurrent_queue<T> instead. "
    "For true lock-free queue, see detail::lockfree_job_queue with hazard pointers.")]] = detail::concurrent_queue<T>;

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
