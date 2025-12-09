#pragma once

/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2024, DongCheol Shin
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file thread_concepts.h
 * @brief C++20 Concepts for thread_system
 *
 * This file provides unified C++20 Concepts for the thread_system library.
 * It defines concepts for:
 * - Callable validation (Callable, VoidCallable, ReturningCallable)
 * - Job type constraints (JobType, JobCallable)
 * - Duration and future-like type detection
 * - Thread pool job validation (PoolJob)
 *
 * When USE_STD_CONCEPTS is defined, these are true C++20 concepts.
 * Otherwise, constexpr bool fallbacks are provided for C++17 compatibility.
 *
 * @note This header centralizes all concept definitions to avoid duplication
 *       across pool_traits.h and type_traits.h.
 */

#include <type_traits>
#include <functional>
#include <chrono>
#include <string>

#ifdef USE_STD_CONCEPTS
#include <concepts>
#endif

namespace kcenon::thread::concepts {

// ============================================================================
// Type Traits (always available)
// ============================================================================

/**
 * @brief Type trait to detect if a type is a std::chrono::duration
 */
template<typename T>
struct is_duration : std::false_type {};

template<typename Rep, typename Period>
struct is_duration<std::chrono::duration<Rep, Period>> : std::true_type {};

template<typename T>
inline constexpr bool is_duration_v = is_duration<T>::value;

/**
 * @brief SFINAE helper to detect if a type has a get() method
 */
template<typename T, typename = void>
struct has_get_method : std::false_type {};

template<typename T>
struct has_get_method<T, std::void_t<decltype(std::declval<T>().get())>>
    : std::true_type {};

template<typename T>
inline constexpr bool has_get_method_v = has_get_method<T>::value;

/**
 * @brief SFINAE helper to detect if a type has a wait() method
 */
template<typename T, typename = void>
struct has_wait_method : std::false_type {};

template<typename T>
struct has_wait_method<T, std::void_t<decltype(std::declval<T>().wait())>>
    : std::true_type {};

template<typename T>
inline constexpr bool has_wait_method_v = has_wait_method<T>::value;

/**
 * @brief Type trait to detect future-like types (has get() and wait() methods)
 */
template<typename T>
struct is_future_like : std::bool_constant<has_get_method_v<T> && has_wait_method_v<T>> {};

template<typename T>
inline constexpr bool is_future_like_v = is_future_like<T>::value;

/**
 * @brief Type trait to extract return type from a callable
 */
template<typename F, typename = void>
struct callable_return_type {
    using type = void;
};

template<typename F>
struct callable_return_type<F, std::enable_if_t<std::is_invocable_v<F>>> {
    using type = std::invoke_result_t<F>;
};

template<typename F>
using callable_return_type_t = typename callable_return_type<F>::type;

// ============================================================================
// C++20 Concepts (when USE_STD_CONCEPTS is defined)
// ============================================================================

#ifdef USE_STD_CONCEPTS

/**
 * @brief Concept for types that can be invoked with no arguments
 */
template<typename F>
concept Callable = std::is_invocable_v<F>;

/**
 * @brief Concept for callable types that return void
 */
template<typename F>
concept VoidCallable = Callable<F> && std::is_void_v<std::invoke_result_t<F>>;

/**
 * @brief Concept for callable types that return a non-void result
 */
template<typename F>
concept ReturningCallable = Callable<F> && !std::is_void_v<std::invoke_result_t<F>>;

/**
 * @brief Concept for callables with specific argument types
 */
template<typename F, typename... Args>
concept CallableWith = std::is_invocable_v<F, Args...>;

/**
 * @brief Concept for std::chrono::duration types
 */
template<typename T>
concept Duration = is_duration_v<T>;

/**
 * @brief Concept for future-like types (has get() and wait())
 */
template<typename T>
concept FutureLike = is_future_like_v<T>;

/**
 * @brief Concept for valid job type parameters
 *
 * A valid job type must be either:
 * - An enumeration type, or
 * - An integral type (excluding bool for clarity)
 *
 * This ensures job types can be used for priority comparison and categorization.
 */
template<typename T>
concept JobType = std::is_enum_v<T> ||
                  (std::is_integral_v<T> && !std::is_same_v<T, bool>);

/**
 * @brief Concept for callable job functions
 *
 * A valid job callable must:
 * - Be invocable with no arguments
 * - Return void, bool, or something convertible to std::string
 */
template<typename F>
concept JobCallable = std::is_invocable_v<F> &&
                      (std::is_void_v<std::invoke_result_t<F>> ||
                       std::is_same_v<std::invoke_result_t<F>, bool> ||
                       std::is_convertible_v<std::invoke_result_t<F>, std::string>);

/**
 * @brief Concept for valid thread pool job types
 *
 * A pool job must be callable and either:
 * - Return void, or
 * - Return something convertible to bool
 */
template<typename Job>
concept PoolJob = Callable<Job> &&
                  (VoidCallable<Job> ||
                   std::is_convertible_v<callable_return_type_t<Job>, bool>);

#else
// ============================================================================
// C++17 Fallback (constexpr bool instead of concepts)
// ============================================================================

/**
 * @brief Fallback for Callable concept
 */
template<typename F>
inline constexpr bool Callable = std::is_invocable_v<F>;

/**
 * @brief Fallback for VoidCallable concept
 */
template<typename F>
inline constexpr bool VoidCallable = std::is_invocable_v<F> &&
                                     std::is_void_v<std::invoke_result_t<F>>;

/**
 * @brief Fallback for ReturningCallable concept
 */
template<typename F>
inline constexpr bool ReturningCallable = std::is_invocable_v<F> &&
                                          !std::is_void_v<std::invoke_result_t<F>>;

/**
 * @brief Fallback for CallableWith concept
 */
template<typename F, typename... Args>
inline constexpr bool CallableWith = std::is_invocable_v<F, Args...>;

/**
 * @brief Fallback for Duration concept
 */
template<typename T>
inline constexpr bool Duration = is_duration_v<T>;

/**
 * @brief Fallback for FutureLike concept
 */
template<typename T>
inline constexpr bool FutureLike = is_future_like_v<T>;

/**
 * @brief Fallback for JobType concept
 */
template<typename T>
inline constexpr bool JobType = std::is_enum_v<T> ||
                                (std::is_integral_v<T> && !std::is_same_v<T, bool>);

/**
 * @brief Fallback for JobCallable concept
 */
template<typename F>
inline constexpr bool JobCallable = std::is_invocable_v<F> &&
                                    (std::is_void_v<std::invoke_result_t<F>> ||
                                     std::is_same_v<std::invoke_result_t<F>, bool> ||
                                     std::is_convertible_v<std::invoke_result_t<F>, std::string>);

/**
 * @brief Fallback for PoolJob concept
 */
template<typename Job>
inline constexpr bool PoolJob = Callable<Job> &&
                                (VoidCallable<Job> ||
                                 std::is_convertible_v<callable_return_type_t<Job>, bool>);

#endif // USE_STD_CONCEPTS

// ============================================================================
// Concept Validation Helpers
// ============================================================================

/**
 * @brief SFINAE helper for JobType validation
 */
template<typename T, typename = void>
struct is_valid_job_type : std::false_type {};

template<typename T>
struct is_valid_job_type<T, std::enable_if_t<JobType<T>>> : std::true_type {};

template<typename T>
inline constexpr bool is_valid_job_type_v = is_valid_job_type<T>::value;

/**
 * @brief Helper to check if a callable is noexcept
 */
template<typename F>
struct is_nothrow_callable : std::bool_constant<std::is_nothrow_invocable_v<F>> {};

template<typename F>
inline constexpr bool is_nothrow_callable_v = is_nothrow_callable<F>::value;

} // namespace kcenon::thread::concepts

// ============================================================================
// Backward Compatibility: Re-export to detail namespace
// ============================================================================

namespace kcenon::thread::detail {

using kcenon::thread::concepts::is_duration;
using kcenon::thread::concepts::is_duration_v;
using kcenon::thread::concepts::is_future_like;
using kcenon::thread::concepts::is_future_like_v;
using kcenon::thread::concepts::callable_return_type;
using kcenon::thread::concepts::callable_return_type_t;
using kcenon::thread::concepts::is_valid_job_type;
using kcenon::thread::concepts::is_valid_job_type_v;
using kcenon::thread::concepts::is_nothrow_callable;
using kcenon::thread::concepts::is_nothrow_callable_v;

#ifdef USE_STD_CONCEPTS
using kcenon::thread::concepts::Callable;
using kcenon::thread::concepts::VoidCallable;
using kcenon::thread::concepts::ReturningCallable;
using kcenon::thread::concepts::CallableWith;
using kcenon::thread::concepts::Duration;
using kcenon::thread::concepts::FutureLike;
using kcenon::thread::concepts::JobType;
using kcenon::thread::concepts::JobCallable;
using kcenon::thread::concepts::PoolJob;
#else
// For C++17 fallback, we need to use inline constexpr
template<typename F>
inline constexpr bool Callable = concepts::Callable<F>;

template<typename F>
inline constexpr bool VoidCallable = concepts::VoidCallable<F>;

template<typename F>
inline constexpr bool ReturningCallable = concepts::ReturningCallable<F>;

template<typename F, typename... Args>
inline constexpr bool CallableWith = concepts::CallableWith<F, Args...>;

template<typename T>
inline constexpr bool Duration = concepts::Duration<T>;

template<typename T>
inline constexpr bool FutureLike = concepts::FutureLike<T>;

template<typename T>
inline constexpr bool JobType = concepts::JobType<T>;

template<typename F>
inline constexpr bool JobCallable = concepts::JobCallable<F>;

template<typename Job>
inline constexpr bool PoolJob = concepts::PoolJob<Job>;
#endif

} // namespace kcenon::thread::detail
