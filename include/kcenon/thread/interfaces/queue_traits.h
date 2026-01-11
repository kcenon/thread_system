/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, DongCheol Shin
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

#include <type_traits>

namespace kcenon::thread {

// Forward declarations
class scheduler_interface;
class queue_capabilities_interface;

template<typename SyncPolicy, typename BoundPolicy, typename OverflowPolicy>
class policy_queue;

namespace policies {
    struct sync_policy_tag;
    struct bound_policy_tag;
    struct overflow_policy_tag;
}

// ============================================
// Policy detection traits
// ============================================

namespace detail {

/**
 * @brief Detect if type has sync_policy_tag
 */
template<typename T, typename = void>
struct has_sync_policy_tag : std::false_type {};

template<typename T>
struct has_sync_policy_tag<T, std::void_t<typename T::policy_tag>>
    : std::is_same<typename T::policy_tag, policies::sync_policy_tag> {};

/**
 * @brief Detect if type has bound_policy_tag
 */
template<typename T, typename = void>
struct has_bound_policy_tag : std::false_type {};

template<typename T>
struct has_bound_policy_tag<T, std::void_t<typename T::policy_tag>>
    : std::is_same<typename T::policy_tag, policies::bound_policy_tag> {};

/**
 * @brief Detect if type has overflow_policy_tag
 */
template<typename T, typename = void>
struct has_overflow_policy_tag : std::false_type {};

template<typename T>
struct has_overflow_policy_tag<T, std::void_t<typename T::policy_tag>>
    : std::is_same<typename T::policy_tag, policies::overflow_policy_tag> {};

} // namespace detail

// ============================================
// Policy type concepts (SFINAE-style)
// ============================================

/**
 * @brief Check if type is a sync policy
 */
template<typename T>
constexpr bool is_sync_policy_v = detail::has_sync_policy_tag<T>::value;

/**
 * @brief Check if type is a bound policy
 */
template<typename T>
constexpr bool is_bound_policy_v = detail::has_bound_policy_tag<T>::value;

/**
 * @brief Check if type is an overflow policy
 */
template<typename T>
constexpr bool is_overflow_policy_v = detail::has_overflow_policy_tag<T>::value;

// ============================================
// Queue type detection traits
// ============================================

namespace detail {

/**
 * @brief Primary template: not a policy_queue
 */
template<typename T>
struct is_policy_queue_impl : std::false_type {};

/**
 * @brief Specialization for policy_queue template
 */
template<typename SyncPolicy, typename BoundPolicy, typename OverflowPolicy>
struct is_policy_queue_impl<policy_queue<SyncPolicy, BoundPolicy, OverflowPolicy>>
    : std::true_type {};

/**
 * @brief Detect if type has sync_policy_type member
 */
template<typename T, typename = void>
struct has_sync_policy_type : std::false_type {};

template<typename T>
struct has_sync_policy_type<T, std::void_t<typename T::sync_policy_type>>
    : std::true_type {};

/**
 * @brief Detect if type has bound_policy_type member
 */
template<typename T, typename = void>
struct has_bound_policy_type : std::false_type {};

template<typename T>
struct has_bound_policy_type<T, std::void_t<typename T::bound_policy_type>>
    : std::true_type {};

/**
 * @brief Detect if type has overflow_policy_type member
 */
template<typename T, typename = void>
struct has_overflow_policy_type : std::false_type {};

template<typename T>
struct has_overflow_policy_type<T, std::void_t<typename T::overflow_policy_type>>
    : std::true_type {};

} // namespace detail

/**
 * @brief Check if type is a policy_queue instantiation
 *
 * This trait identifies whether a type is an instantiation of the policy_queue
 * template, allowing compile-time branching based on queue type.
 *
 * ### Usage Example
 * @code
 * template<typename Queue>
 * void process_queue(Queue& q) {
 *     if constexpr (is_policy_queue_v<Queue>) {
 *         // Access policy-specific features
 *         auto caps = q.get_capabilities();
 *     }
 * }
 * @endcode
 */
template<typename T>
constexpr bool is_policy_queue_v = detail::is_policy_queue_impl<std::remove_cv_t<T>>::value;

/**
 * @brief Check if type is a scheduler_interface
 *
 * Detects whether a type inherits from scheduler_interface.
 */
template<typename T>
constexpr bool is_scheduler_v = std::is_base_of_v<scheduler_interface, std::remove_cv_t<T>>;

/**
 * @brief Check if type is a queue_capabilities_interface
 */
template<typename T>
constexpr bool is_queue_capabilities_v =
    std::is_base_of_v<queue_capabilities_interface, std::remove_cv_t<T>>;

// ============================================
// Policy extraction traits
// ============================================

/**
 * @brief Extract policy types from a policy_queue
 *
 * This trait provides access to the template parameters of a policy_queue
 * instantiation, enabling generic code to work with policy types.
 *
 * ### Usage Example
 * @code
 * using MyQueue = policy_queue<mutex_sync_policy, bounded_policy, overflow_reject_policy>;
 * using Traits = policy_queue_traits<MyQueue>;
 *
 * static_assert(std::is_same_v<Traits::sync_policy_type, mutex_sync_policy>);
 * @endcode
 */
template<typename T, typename = void>
struct policy_queue_traits {
    static constexpr bool is_policy_queue = false;
};

template<typename T>
struct policy_queue_traits<T, std::enable_if_t<
    detail::has_sync_policy_type<T>::value &&
    detail::has_bound_policy_type<T>::value &&
    detail::has_overflow_policy_type<T>::value>>
{
    static constexpr bool is_policy_queue = true;

    using sync_policy_type = typename T::sync_policy_type;
    using bound_policy_type = typename T::bound_policy_type;
    using overflow_policy_type = typename T::overflow_policy_type;
};

// ============================================
// Queue capability detection
// ============================================

/**
 * @brief Compile-time detection of lock-free capability
 *
 * Returns true if the queue type uses a lock-free sync policy.
 */
template<typename T, typename = void>
struct is_lockfree_queue : std::false_type {};

template<typename T>
struct is_lockfree_queue<T, std::enable_if_t<policy_queue_traits<T>::is_policy_queue>>
{
    static constexpr bool value =
        policy_queue_traits<T>::sync_policy_type::get_capabilities().lock_free;
};

template<typename T>
constexpr bool is_lockfree_queue_v = is_lockfree_queue<T>::value;

/**
 * @brief Compile-time detection of bounded queue
 *
 * Returns true if the queue type uses a bounded policy.
 */
template<typename T, typename = void>
struct is_bounded_queue : std::false_type {};

template<typename T>
struct is_bounded_queue<T, std::enable_if_t<policy_queue_traits<T>::is_policy_queue>>
{
    static constexpr bool value =
        policy_queue_traits<T>::bound_policy_type::is_bounded();
};

template<typename T>
constexpr bool is_bounded_queue_v = is_bounded_queue<T>::value;

/**
 * @brief Compile-time detection of blocking overflow policy
 */
template<typename T, typename = void>
struct has_blocking_overflow : std::false_type {};

template<typename T>
struct has_blocking_overflow<T, std::enable_if_t<policy_queue_traits<T>::is_policy_queue>>
{
    static constexpr bool value =
        policy_queue_traits<T>::overflow_policy_type::blocks();
};

template<typename T>
constexpr bool has_blocking_overflow_v = has_blocking_overflow<T>::value;

} // namespace kcenon::thread
