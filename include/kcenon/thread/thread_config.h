// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#pragma once

/**
 * @file thread_config.h
 * @brief Public facade for thread_system configuration
 *
 * This is the primary header users should include to configure thread_system.
 * It provides access to the unified configuration structure and builder.
 *
 * ### Example Usage
 * @code{.cpp}
 * #include <kcenon/thread/thread_config.h>
 *
 * auto config = kcenon::thread::thread_system_config::builder()
 *     .with_worker_count(8)
 *     .with_queue_capacity(5000)
 *     .enable_circuit_breaker()
 *     .build();
 * @endcode
 */

#include "config/thread_system_config.h"

// For backwards compatibility, also include compile-time constants
#include "core/config.h"
