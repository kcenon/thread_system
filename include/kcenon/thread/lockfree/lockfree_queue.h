/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
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

// Include the new location
#include <kcenon/thread/concurrent/concurrent_queue.h>
