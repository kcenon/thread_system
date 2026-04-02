// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#pragma once

/**
 * @file typed_thread_pool.h
 * @brief Public forwarding header for type-safe thread pool.
 *
 * Provides a stable include path for typed_thread_pool, which offers
 * compile-time type safety for job submission and result retrieval.
 *
 * @code
 * #include <kcenon/thread/core/typed_thread_pool.h>
 *
 * // Create a typed thread pool for string processing
 * typed_thread_pool<std::string> pool("string_pool", 2);
 *
 * // Submit typed work
 * pool.enqueue([](const std::string& input) {
 *     return "processed: " + input;
 * }, "hello");
 * @endcode
 */

#include <kcenon/thread/impl/typed_pool/typed_thread_pool.h>

