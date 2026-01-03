// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file core.cppm
 * @brief C++20 module partition for thread_system core components.
 *
 * This module partition exports core threading components:
 * - thread_pool: Thread pool management
 * - thread_worker: Worker thread implementation
 * - thread_base: Base thread class
 * - job: Job/task base class
 * - callback_job: Lambda-based jobs
 * - cancellable_job: Jobs with cancellation support
 * - cancellation_token: Cooperative cancellation
 * - error_handling: Thread-specific error types
 * - hazard_pointer: Memory reclamation for lock-free structures
 * - sync_primitives: Synchronization utilities
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
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <vector>

// Include existing headers in the global module fragment
#include <kcenon/thread/core/config.h>
#include <kcenon/thread/core/error_handling.h>
#include <kcenon/thread/core/cancellation_token.h>
#include <kcenon/thread/core/sync_primitives.h>
#include <kcenon/thread/core/hazard_pointer.h>
#include <kcenon/thread/core/safe_hazard_pointer.h>
#include <kcenon/thread/core/atomic_shared_ptr.h>
#include <kcenon/thread/core/job.h>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/core/cancellable_job.h>
#include <kcenon/thread/core/job_types.h>
#include <kcenon/thread/core/thread_base.h>
#include <kcenon/thread/core/thread_conditions.h>
#include <kcenon/thread/core/worker_policy.h>
#include <kcenon/thread/core/pool_traits.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/future_extensions.h>
#include <kcenon/thread/core/configuration_manager.h>
#include <kcenon/thread/core/service_registry.h>
#include <kcenon/thread/core/log_level.h>
#include <kcenon/thread/core/thread_logger.h>
#include <kcenon/thread/core/event_bus.h>
#include <kcenon/thread/interfaces/thread_context.h>
#include <kcenon/thread/interfaces/scheduler_interface.h>
#include <kcenon/thread/interfaces/error_handler.h>
#include <kcenon/thread/interfaces/crash_handler.h>
#include <kcenon/thread/interfaces/service_container.h>
#include <kcenon/thread/metrics/thread_pool_metrics.h>
#include <kcenon/thread/concepts/thread_concepts.h>
#include <kcenon/thread/utils/formatter.h>
#include <kcenon/thread/utils/convert_string.h>
#include <kcenon/thread/utils/synchronization.h>
#include <kcenon/thread/utils/atomic_wait.h>
#include <kcenon/thread/utils/platform_detection.h>
#include <kcenon/thread/compatibility.h>
#include <kcenon/thread/forward.h>

export module kcenon.thread:core;

import kcenon.common;

// ============================================================================
// Core Configuration and Error Handling
// ============================================================================

export namespace kcenon::thread {

// Re-export configuration types
using ::kcenon::thread::thread_config;

// Re-export error handling types
using ::kcenon::thread::error_code;
using ::kcenon::thread::make_error;
using ::kcenon::thread::is_retriable;

// Re-export cancellation types
using ::kcenon::thread::cancellation_token;
using ::kcenon::thread::cancellation_token_source;

// Re-export synchronization primitives
using ::kcenon::thread::sync::spin_lock;
using ::kcenon::thread::sync::scoped_spin_lock;

} // namespace kcenon::thread

// ============================================================================
// Memory Management (Hazard Pointers)
// ============================================================================

export namespace kcenon::thread {

// Re-export hazard pointer utilities
using ::kcenon::thread::hazard_pointer;
using ::kcenon::thread::safe_hazard_pointer;
using ::kcenon::thread::atomic_shared_ptr;

} // namespace kcenon::thread

// ============================================================================
// Job Classes
// ============================================================================

export namespace kcenon::thread {

// Re-export job types
using ::kcenon::thread::job;
using ::kcenon::thread::callback_job;
using ::kcenon::thread::cancellable_job;
using ::kcenon::thread::job_priority;

} // namespace kcenon::thread

// ============================================================================
// Thread Classes
// ============================================================================

export namespace kcenon::thread {

// Re-export thread types
using ::kcenon::thread::thread_base;
using ::kcenon::thread::thread_worker;
using ::kcenon::thread::thread_pool;

// Re-export thread conditions
using ::kcenon::thread::start_condition;
using ::kcenon::thread::stop_condition;

// Re-export worker policy
using ::kcenon::thread::worker_policy;
using ::kcenon::thread::steal_policy;

// Re-export pool traits
using ::kcenon::thread::pool_traits;
using ::kcenon::thread::is_pool_like;

} // namespace kcenon::thread

// ============================================================================
// Future Extensions
// ============================================================================

export namespace kcenon::thread {

// Re-export future utilities
using ::kcenon::thread::when_all;
using ::kcenon::thread::when_any;
using ::kcenon::thread::future_status_result;

} // namespace kcenon::thread

// ============================================================================
// Configuration and Services
// ============================================================================

export namespace kcenon::thread {

// Re-export configuration manager
using ::kcenon::thread::configuration_manager;
using ::kcenon::thread::service_registry;

// Re-export log level
using ::kcenon::thread::log_level;
using ::kcenon::thread::thread_logger;

// Re-export event bus
using ::kcenon::thread::event_bus;

} // namespace kcenon::thread

// ============================================================================
// Interfaces
// ============================================================================

export namespace kcenon::thread {

// Re-export interfaces
using ::kcenon::thread::thread_context;
using ::kcenon::thread::scheduler_interface;
using ::kcenon::thread::error_handler;
using ::kcenon::thread::crash_handler;
using ::kcenon::thread::service_container;

} // namespace kcenon::thread

// ============================================================================
// Metrics
// ============================================================================

export namespace kcenon::thread::metrics {

// Re-export metrics types
using ::kcenon::thread::metrics::ThreadPoolMetrics;
using ::kcenon::thread::metrics::WorkerMetrics;

} // namespace kcenon::thread::metrics

// ============================================================================
// Concepts
// ============================================================================

export namespace kcenon::thread::concepts {

// Re-export concepts (if available)
#if __cpp_concepts >= 201907L
using ::kcenon::thread::concepts::Executable;
using ::kcenon::thread::concepts::JobLike;
using ::kcenon::thread::concepts::PoolLike;
#endif

} // namespace kcenon::thread::concepts

// ============================================================================
// Utilities
// ============================================================================

export namespace kcenon::thread::utility_module {

// Re-export utility types
using ::utility_module::convert_string;

} // namespace kcenon::thread::utility_module
