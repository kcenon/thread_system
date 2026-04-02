// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#pragma once

/**
 * @file diagnostics.h
 * @brief Stable public include for thread pool diagnostics
 *
 * This umbrella header provides a stable include path for all diagnostics types.
 * Downstream code should include this header rather than individual diagnostic headers.
 *
 * @code
 * #include <kcenon/thread/diagnostics.h>
 *
 * auto pool = std::make_shared<kcenon::thread::thread_pool>("MyPool");
 * pool->start();
 *
 * auto& diag = pool->diagnostics();
 * auto health = diag.health_check();
 * if (!health.is_operational()) {
 *     std::cerr << diag.format_thread_dump() << std::endl;
 * }
 * @endcode
 */

#include <kcenon/thread/diagnostics/job_info.h>
#include <kcenon/thread/diagnostics/thread_info.h>
#include <kcenon/thread/diagnostics/health_status.h>
#include <kcenon/thread/diagnostics/bottleneck_report.h>
#include <kcenon/thread/diagnostics/execution_event.h>
#include <kcenon/thread/diagnostics/thread_pool_diagnostics.h>
