// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/**
 * @file lockfree_queue.h
 * @brief Backward compatibility header for concurrent_queue
 *
 * This header provides backward compatibility for code using the old include path.
 * The implementation has been moved to:
 *   include/kcenon/thread/concurrent/concurrent_queue.h
 *
 * ## Migration Guide
 *
 * Update your includes from:
 * @code
 * #include <kcenon/thread/lockfree/lockfree_queue.h>
 * @endcode
 *
 * To:
 * @code
 * #include <kcenon/thread/concurrent/concurrent_queue.h>
 * @endcode
 *
 * And update usage from:
 * @code
 * kcenon::thread::lockfree_queue<int> queue;  // Deprecated
 * @endcode
 *
 * To:
 * @code
 * kcenon::thread::detail::concurrent_queue<int> queue;  // Correct
 * @endcode
 *
 * ## Why the change?
 *
 * The name "lockfree_queue" was misleading because the implementation uses
 * fine-grained locking (mutexes), not true lock-free algorithms. The rename to
 * "concurrent_queue" accurately describes the thread-safe nature without implying
 * lock-free semantics.
 *
 * For a true lock-free queue, see lockfree_job_queue which uses the Michael-Scott
 * algorithm with hazard pointers.
 *
 * @deprecated This header will be removed in a future major version.
 *             Use <kcenon/thread/concurrent/concurrent_queue.h> instead.
 */

#pragma once

#if !defined(THREAD_SUPPRESS_LEGACY_LOCKFREE_QUEUE_WARNING) \
    && !defined(KCENON_THREAD_LOCKFREE_QUEUE_H_WARNED)
#define KCENON_THREAD_LOCKFREE_QUEUE_H_WARNED 1
#if defined(_MSC_VER)
#pragma message("<kcenon/thread/lockfree/lockfree_queue.h> is deprecated since v1.0.0 and will be removed in v2.0.0. Include <kcenon/thread/concurrent/concurrent_queue.h> instead. Define THREAD_SUPPRESS_LEGACY_LOCKFREE_QUEUE_WARNING to silence this warning.")
#elif defined(__GNUC__) || defined(__clang__)
#pragma message "<kcenon/thread/lockfree/lockfree_queue.h> is deprecated since v1.0.0 and will be removed in v2.0.0. Include <kcenon/thread/concurrent/concurrent_queue.h> instead. Define THREAD_SUPPRESS_LEGACY_LOCKFREE_QUEUE_WARNING to silence this warning."
#endif
#endif

// Include the new location
#include <kcenon/thread/concurrent/concurrent_queue.h>
