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
 * @file when_helpers.h
 * @brief Helper functions for combining multiple futures
 *
 * This file provides when_all and when_any utilities for waiting on
 * multiple futures efficiently.
 */

#include <atomic>
#include <future>
#include <memory>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace kcenon::thread {

namespace detail {

/**
 * @brief Helper to get the value type from a future
 */
template<typename Future>
struct future_value_type {
    using type = decltype(std::declval<Future>().get());
};

template<typename Future>
using future_value_type_t = typename future_value_type<Future>::type;

/**
 * @brief Index sequence helper for when_all implementation
 */
template<std::size_t... Is, typename Tuple, typename ResultTuple>
void get_all_impl(std::index_sequence<Is...>, Tuple& futures, ResultTuple& results) {
    ((std::get<Is>(results) = std::get<Is>(futures).get()), ...);
}

} // namespace detail

/**
 * @brief Wait for all futures to complete and return their results as a tuple
 *
 * @tparam Futures Variadic future types
 * @param futures The futures to wait on
 * @return A future containing a tuple of all results
 *
 * This function creates a new future that completes when all input futures
 * complete. The result is a tuple containing the values from each future.
 *
 * ### Thread Safety
 * - Thread-safe. Each input future is accessed from a separate thread.
 *
 * ### Exception Handling
 * - If any future throws an exception, it will be propagated when get()
 *   is called on the returned future.
 *
 * ### Usage Example
 * @code
 * auto f1 = pool->submit_async([]{ return 1; });
 * auto f2 = pool->submit_async([]{ return 2.0; });
 * auto f3 = pool->submit_async([]{ return std::string("hello"); });
 *
 * auto combined = when_all(std::move(f1), std::move(f2), std::move(f3));
 * auto [a, b, c] = combined.get();
 * // a = 1, b = 2.0, c = "hello"
 * @endcode
 */
template<typename... Futures>
auto when_all(Futures&&... futures)
    -> std::future<std::tuple<detail::future_value_type_t<std::decay_t<Futures>>...>>
{
    using result_tuple = std::tuple<detail::future_value_type_t<std::decay_t<Futures>>...>;

    auto promise = std::make_shared<std::promise<result_tuple>>();
    auto result_future = promise->get_future();

    // Store futures in a tuple for processing
    auto futures_tuple = std::make_shared<std::tuple<std::decay_t<Futures>...>>(
        std::forward<Futures>(futures)...);

    // Launch async operation to wait for all
    std::thread([promise, futures_tuple]() mutable {
        try {
            result_tuple results;
            detail::get_all_impl(
                std::make_index_sequence<sizeof...(Futures)>{},
                *futures_tuple,
                results);
            promise->set_value(std::move(results));
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    }).detach();

    return result_future;
}

/**
 * @brief Overload of when_all for no arguments
 */
inline auto when_all_empty() -> std::future<std::tuple<>>
{
    std::promise<std::tuple<>> promise;
    promise.set_value(std::tuple<>{});
    return promise.get_future();
}

/**
 * @brief Wait for any future to complete and return its result
 *
 * @tparam T The common result type of all futures
 * @param futures Vector of futures
 * @return A future containing the first completed result
 *
 * This function creates a new future that completes when any of the input
 * futures completes. The result is the value from the first completed future.
 *
 * ### Thread Safety
 * - Thread-safe. Uses atomic operations for completion tracking.
 *
 * ### Usage Example
 * @code
 * std::vector<std::future<int>> futures;
 * futures.push_back(pool->submit_async([]{ return fetch_from_server_a(); }));
 * futures.push_back(pool->submit_async([]{ return fetch_from_server_b(); }));
 *
 * auto first = when_any(std::move(futures));
 * int result = first.get();  // First completed result
 * @endcode
 */
template<typename T>
auto when_any(std::vector<std::future<T>>&& futures)
    -> std::future<T>
{
    if (futures.empty()) {
        std::promise<T> promise;
        promise.set_exception(
            std::make_exception_ptr(std::invalid_argument("Empty futures vector"))
        );
        return promise.get_future();
    }

    auto promise = std::make_shared<std::promise<T>>();
    auto result_future = promise->get_future();
    auto completed = std::make_shared<std::atomic<bool>>(false);
    auto futures_ptr = std::make_shared<std::vector<std::future<T>>>(std::move(futures));

    // Launch threads for each future
    for (std::size_t i = 0; i < futures_ptr->size(); ++i) {
        std::thread([promise, completed, futures_ptr, i]() {
            try {
                auto& future = (*futures_ptr)[i];
                T result = future.get();

                bool expected = false;
                if (completed->compare_exchange_strong(expected, true)) {
                    promise->set_value(std::move(result));
                }
            } catch (...) {
                bool expected = false;
                if (completed->compare_exchange_strong(expected, true)) {
                    promise->set_exception(std::current_exception());
                }
            }
        }).detach();
    }

    return result_future;
}

/**
 * @brief Specialization of when_any for void futures
 */
inline auto when_any(std::vector<std::future<void>>&& futures)
    -> std::future<void>
{
    if (futures.empty()) {
        std::promise<void> promise;
        promise.set_exception(
            std::make_exception_ptr(std::invalid_argument("Empty futures vector"))
        );
        return promise.get_future();
    }

    auto promise = std::make_shared<std::promise<void>>();
    auto result_future = promise->get_future();
    auto completed = std::make_shared<std::atomic<bool>>(false);
    auto futures_ptr = std::make_shared<std::vector<std::future<void>>>(std::move(futures));

    for (std::size_t i = 0; i < futures_ptr->size(); ++i) {
        std::thread([promise, completed, futures_ptr, i]() {
            try {
                (*futures_ptr)[i].get();

                bool expected = false;
                if (completed->compare_exchange_strong(expected, true)) {
                    promise->set_value();
                }
            } catch (...) {
                bool expected = false;
                if (completed->compare_exchange_strong(expected, true)) {
                    promise->set_exception(std::current_exception());
                }
            }
        }).detach();
    }

    return result_future;
}

/**
 * @brief Wait for any future to complete with index
 *
 * @tparam T The common result type of all futures
 * @param futures Vector of futures
 * @return A future containing a pair of (index, value) for the first completed
 *
 * Similar to when_any, but also returns the index of which future completed first.
 *
 * ### Usage Example
 * @code
 * std::vector<std::future<int>> futures;
 * futures.push_back(pool->submit_async([]{ return fetch_from_server_a(); }));
 * futures.push_back(pool->submit_async([]{ return fetch_from_server_b(); }));
 *
 * auto first = when_any_with_index(std::move(futures));
 * auto [idx, result] = first.get();  // idx = which completed, result = value
 * @endcode
 */
template<typename T>
auto when_any_with_index(std::vector<std::future<T>>&& futures)
    -> std::future<std::pair<std::size_t, T>>
{
    if (futures.empty()) {
        std::promise<std::pair<std::size_t, T>> promise;
        promise.set_exception(
            std::make_exception_ptr(std::invalid_argument("Empty futures vector"))
        );
        return promise.get_future();
    }

    auto promise = std::make_shared<std::promise<std::pair<std::size_t, T>>>();
    auto result_future = promise->get_future();
    auto completed = std::make_shared<std::atomic<bool>>(false);
    auto futures_ptr = std::make_shared<std::vector<std::future<T>>>(std::move(futures));

    for (std::size_t i = 0; i < futures_ptr->size(); ++i) {
        std::thread([promise, completed, futures_ptr, i]() {
            try {
                auto& future = (*futures_ptr)[i];
                T result = future.get();

                bool expected = false;
                if (completed->compare_exchange_strong(expected, true)) {
                    promise->set_value(std::make_pair(i, std::move(result)));
                }
            } catch (...) {
                bool expected = false;
                if (completed->compare_exchange_strong(expected, true)) {
                    promise->set_exception(std::current_exception());
                }
            }
        }).detach();
    }

    return result_future;
}

} // namespace kcenon::thread
