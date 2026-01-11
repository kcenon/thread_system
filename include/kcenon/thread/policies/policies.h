/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, DongCheol Shin
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
 * @file policies.h
 * @brief Convenience header for all queue policies
 *
 * Include this header to access all queue policy types:
 * - Sync policies (mutex_sync_policy, lockfree_sync_policy, adaptive_sync_policy)
 * - Bound policies (unbounded_policy, bounded_policy, dynamic_bounded_policy)
 * - Overflow policies (overflow_reject_policy, overflow_block_policy, etc.)
 * - policy_queue template class
 *
 * @code
 * #include <kcenon/thread/policies/policies.h>
 *
 * using namespace kcenon::thread;
 *
 * // Create a bounded queue with mutex sync
 * policy_queue<policies::mutex_sync_policy,
 *              policies::bounded_policy,
 *              policies::overflow_reject_policy> queue(policies::bounded_policy(1000));
 * @endcode
 */

#include <kcenon/thread/policies/sync_policies.h>
#include <kcenon/thread/policies/bound_policies.h>
#include <kcenon/thread/policies/overflow_policies.h>
#include <kcenon/thread/policies/policy_queue.h>
