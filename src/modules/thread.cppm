// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file thread.cppm
 * @brief Primary C++20 module for thread_system.
 *
 * This is the main module interface for the thread_system library.
 * It aggregates all module partitions to provide a single import point.
 *
 * Usage:
 * @code
 * import kcenon.thread;
 *
 * using namespace kcenon::thread;
 *
 * auto pool = std::make_shared<thread_pool>("worker_pool");
 * pool->start();
 * pool->enqueue(std::make_unique<my_job>());
 * @endcode
 *
 * Module Structure:
 * - kcenon.thread:core - Thread pool, worker management, jobs
 * - kcenon.thread:queue - Queue implementations (job_queue, adaptive_job_queue)
 *
 * Dependencies:
 * - kcenon.common (Tier 0) - Result<T>, error handling, interfaces
 */

export module kcenon.thread;

import kcenon.common;

// Tier 1: Core thread management
export import :core;

// Tier 2: Queue implementations
export import :queue;

export namespace kcenon::thread {

/**
 * @brief Version information for thread_system module.
 */
struct module_version {
    static constexpr int major = 0;
    static constexpr int minor = 3;
    static constexpr int patch = 0;
    static constexpr int tweak = 0;
    static constexpr const char* string = "0.3.0.0";
    static constexpr const char* module_name = "kcenon.thread";
};

} // namespace kcenon::thread
