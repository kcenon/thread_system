// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/**
 * @file compatibility.h
 * @brief Backward-compatible type aliases for renamed thread system components.
 *
 */

#pragma once

// Forward declare canonical namespaces to allow aliasing without including
// heavier headers.
namespace kcenon {
namespace thread {
namespace core {}
namespace interfaces {}
namespace impl {}
namespace utils {}
namespace detail {}
} // namespace thread
} // namespace kcenon

// Legacy namespace aliases. These remain until dependent projects
// migrate to the unified kcenon::thread namespace.
//
// @deprecated Since v1.0.0. These aliases will be removed in v2.0.0.
// Use `kcenon::thread` and `kcenon::thread::utils` directly instead.
//
// NOTE: The C++ standard does not support `[[deprecated]]` on namespace
// aliases (only on namespace definitions). Consumers that need compile-time
// warnings should migrate include-by-include; an include-guard warning is
// emitted once per translation unit below (suppressible by defining
// THREAD_SUPPRESS_LEGACY_NAMESPACE_WARNING before the include).
namespace thread_system = kcenon::thread;
namespace thread_module = kcenon::thread;
namespace thread_namespace = kcenon::thread;

// Legacy utility namespace names still expected by some consumers.
namespace utility_module = kcenon::thread::utils;

#if !defined(THREAD_SUPPRESS_LEGACY_NAMESPACE_WARNING) \
    && !defined(KCENON_THREAD_COMPATIBILITY_H_WARNED)
#define KCENON_THREAD_COMPATIBILITY_H_WARNED 1
#if defined(_MSC_VER)
#pragma message("thread_system/thread_module/thread_namespace/utility_module namespace aliases are deprecated since v1.0.0 and will be removed in v2.0.0. Use 'kcenon::thread' directly. Define THREAD_SUPPRESS_LEGACY_NAMESPACE_WARNING to silence this warning.")
#elif defined(__GNUC__) || defined(__clang__)
#pragma message "thread_system/thread_module/thread_namespace/utility_module namespace aliases are deprecated since v1.0.0 and will be removed in v2.0.0. Use 'kcenon::thread' directly. Define THREAD_SUPPRESS_LEGACY_NAMESPACE_WARNING to silence this warning."
#endif
#endif
