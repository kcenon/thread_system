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

#pragma once

/**
 * @file formatters.h
 * @brief Unified header for all std::formatter specializations in thread_system.
 * @date 2026-01-23
 *
 * This header provides a single include point for all std::formatter specializations
 * in the thread_system library. Instead of including individual *_fmt.h headers,
 * users can include this file to get formatting support for all thread_system types.
 *
 * ## Usage
 * @code
 * #include <kcenon/thread/formatters.h>
 *
 * auto pool = std::make_shared<kcenon::thread::thread_pool>("MyPool");
 * auto cond = kcenon::thread::thread_conditions::Working;
 *
 * // All types now support std::format
 * std::string output = std::format("Pool: {}, State: {}", *pool, cond);
 * @endcode
 *
 * ## Included Formatters
 *
 * ### Core Types
 * - thread_pool: Thread pool management class
 * - thread_worker: Worker thread class
 * - thread_conditions: Thread state enumeration
 *
 * ### Typed Pool Types
 * - job_types: Job priority enumeration
 * - typed_thread_pool_t<T>: Templated typed thread pool
 * - typed_thread_worker_t<T>: Templated typed worker
 *
 * ## Notes
 *
 * This header consolidates formatter definitions that were previously spread across
 * multiple files. The individual type headers still contain their formatter
 * specializations inline for backward compatibility.
 *
 * For enum types, you can also use the kcenon::thread::utils::enum_formatter template
 * with a custom converter functor.
 *
 * @see formatter_macros.h for DECLARE_FORMATTER macro
 * @see formatter.h for enum_formatter and formatter utility class
 */

#include <kcenon/thread/utils/formatter.h>
#include <kcenon/thread/utils/formatter_macros.h>

// Core type formatters
// These headers define their formatter specializations inline
#include <kcenon/thread/core/thread_conditions.h>

// Include thread_pool_fmt.h with internal flag to suppress deprecation warning
#define KCENON_THREAD_INTERNAL_INCLUDE
#include <kcenon/thread/core/thread_pool_fmt.h>
#undef KCENON_THREAD_INTERNAL_INCLUDE

// Note: thread_worker formatter is defined in thread_worker.h
// Include it only when thread_worker functionality is needed

// Typed pool formatters
// These are template specializations defined in their respective headers
#include <kcenon/thread/impl/typed_pool/job_types.h>

// Note: typed_thread_pool_t and typed_thread_worker_t formatters are templates
// and remain in their original headers for proper template instantiation

