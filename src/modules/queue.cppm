// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file queue.cppm
 * @brief C++20 module partition for thread_system queue implementations.
 *
 * This module partition exports queue components:
 * - job_queue: Standard mutex-protected job queue
 * - adaptive_job_queue: Adaptive queue switching between modes
 * - lockfree_job_queue: Lock-free queue implementation
 * - lockfree_queue: Generic lock-free queue
 * - work_stealing_deque: Work-stealing double-ended queue
 * - queue_factory: Factory for creating queues
 * - queue_capabilities: Queue capability descriptors
 *
 * Part of the kcenon.thread module.
 */

module;

// Standard library includes needed before module declaration
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// Include existing headers in the global module fragment
#include <kcenon/thread/core/job_queue.h>
#include <kcenon/thread/core/bounded_job_queue.h>
#include <kcenon/thread/queue/adaptive_job_queue.h>
#include <kcenon/thread/queue/queue_factory.h>
#include <kcenon/thread/lockfree/lockfree_queue.h>
#include <kcenon/thread/lockfree/lockfree_job_queue.h>
#include <kcenon/thread/lockfree/work_stealing_deque.h>
#include <kcenon/thread/interfaces/queue_capabilities_interface.h>
#include <kcenon/thread/interfaces/queue_capabilities.h>
#include <kcenon/thread/concurrent/concurrent_queue.h>

export module kcenon.thread:queue;

import kcenon.common;

// ============================================================================
// Queue Capabilities
// ============================================================================

export namespace kcenon::thread {

// Re-export queue capability types
using ::kcenon::thread::queue_capabilities;
using ::kcenon::thread::queue_capabilities_interface;

} // namespace kcenon::thread

// ============================================================================
// Standard Job Queue
// ============================================================================

export namespace kcenon::thread {

// Re-export standard queue types
using ::kcenon::thread::job_queue;

} // namespace kcenon::thread

// ============================================================================
// Adaptive Queue
// ============================================================================

export namespace kcenon::thread {

// Re-export adaptive queue types
using ::kcenon::thread::adaptive_job_queue;

} // namespace kcenon::thread

// ============================================================================
// Lock-Free Queue
// ============================================================================

export namespace kcenon::thread {

// Re-export lock-free queue types
using ::kcenon::thread::lockfree_queue;

} // namespace kcenon::thread

export namespace kcenon::thread::detail {

// Re-export internal lock-free job queue
using ::kcenon::thread::detail::lockfree_job_queue;

} // namespace kcenon::thread::detail

// ============================================================================
// Work-Stealing Deque
// ============================================================================

export namespace kcenon::thread {

// Re-export work-stealing types
using ::kcenon::thread::work_stealing_deque;

} // namespace kcenon::thread

// ============================================================================
// Queue Factory
// ============================================================================

export namespace kcenon::thread {

// Re-export factory types
using ::kcenon::thread::queue_factory;
using ::kcenon::thread::queue_type;

} // namespace kcenon::thread

// ============================================================================
// Concurrent Queue
// ============================================================================

export namespace kcenon::thread {

// Re-export concurrent queue types
using ::kcenon::thread::concurrent_queue;

} // namespace kcenon::thread
