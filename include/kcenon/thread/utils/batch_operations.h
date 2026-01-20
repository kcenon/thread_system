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
 * @file batch_operations.h
 * @brief Helper templates for batch operations to eliminate duplicated loop patterns
 *
 * This file provides reusable template functions for:
 * - Applying operations to collections (batch_apply)
 * - Collecting results from futures (collect_all)
 *
 * These helpers reduce code duplication in submit_wait_all, submit_wait_any,
 * and batch enqueue methods throughout the codebase.
 */

#include <future>
#include <vector>
#include <utility>
#include <type_traits>

namespace kcenon::thread {

namespace detail {

/**
 * @brief Apply an operation to each item in a collection, returning results.
 *
 * @tparam Container The input container type (must support range-based for)
 * @tparam Operation Callable that takes Container::value_type and returns ResultType
 * @param items Collection of items to process
 * @param op Operation to apply to each item
 * @return std::vector of results from applying op to each item
 *
 * ### Usage Example
 * @code
 * std::vector<int> numbers = {1, 2, 3};
 * auto doubled = batch_apply(std::move(numbers), [](int n) { return n * 2; });
 * // doubled = {2, 4, 6}
 * @endcode
 */
template<typename Container, typename Operation>
auto batch_apply(Container&& items, Operation&& op)
{
    using ResultType = decltype(op(std::move(*std::begin(items))));
    std::vector<ResultType> results;
    results.reserve(items.size());

    for (auto&& item : items) {
        results.push_back(op(std::move(item)));
    }

    return results;
}

/**
 * @brief Collect all results from a vector of futures.
 *
 * @tparam T The value type of the futures
 * @param futures Vector of futures to collect from
 * @return std::vector<T> containing all results
 *
 * Blocks until all futures complete, collecting results in order.
 *
 * ### Thread Safety
 * - Safe to call from any thread
 * - Each future.get() is called sequentially
 *
 * ### Exception Handling
 * - If any future throws, the exception propagates immediately
 * - Remaining futures are not waited upon
 *
 * ### Usage Example
 * @code
 * std::vector<std::future<int>> futures;
 * // ... populate futures ...
 * auto results = collect_all(futures);
 * @endcode
 */
template<typename T>
auto collect_all(std::vector<std::future<T>>& futures) -> std::vector<T>
{
    std::vector<T> results;
    results.reserve(futures.size());

    for (auto& future : futures) {
        results.push_back(future.get());
    }

    return results;
}

/**
 * @brief Specialization of collect_all for void futures.
 *
 * @param futures Vector of void futures to wait on
 *
 * Blocks until all futures complete. Since void futures don't return values,
 * this simply waits for completion.
 */
inline void collect_all(std::vector<std::future<void>>& futures)
{
    for (auto& future : futures) {
        future.get();
    }
}

} // namespace detail

} // namespace kcenon::thread
