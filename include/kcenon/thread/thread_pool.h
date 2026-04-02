// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#pragma once

/**
 * @file thread_pool.h
 * @brief Stable public include for thread_pool and numa_thread_pool
 *
 * This umbrella header provides a stable include path for the thread_pool class
 * and its NUMA-aware variant. Downstream code should include this header rather
 * than the internal core/ path.
 *
 * @code
 * #include <kcenon/thread/thread_pool.h>
 *
 * // Standard thread pool
 * auto pool = std::make_shared<kcenon::thread::thread_pool>("my_pool");
 *
 * // NUMA-aware thread pool (for multi-socket servers)
 * auto numa_pool = std::make_shared<kcenon::thread::numa_thread_pool>("numa_pool");
 * numa_pool->enable_numa_work_stealing();
 * @endcode
 */

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/numa_thread_pool.h>
