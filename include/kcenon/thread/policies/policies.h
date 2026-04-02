// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

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
