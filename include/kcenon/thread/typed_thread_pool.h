// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#pragma once

/**
 * @file typed_thread_pool.h
 * @brief Stable public include for typed_thread_pool_t
 *
 * This umbrella header provides a stable include path for the typed_thread_pool_t
 * template class. Downstream code should include this header rather than the
 * internal core/ or impl/ paths.
 *
 * @code
 * #include <kcenon/thread/typed_thread_pool.h>
 *
 * using namespace kcenon::thread;
 * auto pool = std::make_shared<typed_thread_pool_t<job_types>>(4);
 * pool->enqueue(typed_job_t<job_types>(job_types::normal, []() { ... }));
 * @endcode
 */

#include <kcenon/thread/core/typed_thread_pool.h>
