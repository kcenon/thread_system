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

#pragma once

/**
 * @file thread_pool_impl.h
 * @brief Template method implementations for thread_pool
 * @date 2025-01-11
 *
 * This file contains template method implementations for the thread_pool class.
 * It is automatically included by thread_pool.h and should not be included directly.
 *
 * Separated from thread_pool.h to reduce header size and improve compilation times.
 */

#include <kcenon/thread/core/future_job.h>

#include <future>
#include <vector>
#include <thread>
#include <memory>
#include <atomic>
#include <stdexcept>

namespace kcenon::thread {

// ============================================================================
// Unified Submit API Implementations
// ============================================================================

template<typename F, typename R>
auto thread_pool::submit(F&& callable, const submit_options& opts)
    -> std::future<R>
{
    auto job_ptr = std::make_unique<future_job<R>>(
        std::forward<F>(callable),
        opts.name.empty() ? "async_job" : opts.name
    );

    auto future = job_ptr->get_future();

    auto result = enqueue(std::move(job_ptr));
    if (result.is_err()) {
        std::promise<R> error_promise;
        error_promise.set_exception(
            std::make_exception_ptr(
                std::runtime_error(result.error().message)
            )
        );
        return error_promise.get_future();
    }

    return future;
}

template<typename F, typename R>
auto thread_pool::submit(std::vector<F>&& callables, const submit_options& opts)
    -> std::vector<std::future<R>>
{
    std::vector<std::future<R>> futures;
    futures.reserve(callables.size());

    for (auto&& callable : callables) {
        submit_options single_opts;
        single_opts.name = opts.name;
        futures.push_back(submit<F, R>(std::move(callable), single_opts));
    }

    return futures;
}

template<typename F, typename R>
auto thread_pool::submit_wait_all(std::vector<F>&& callables, const submit_options& opts)
    -> std::vector<R>
{
    auto futures = submit<F, R>(std::move(callables), opts);

    std::vector<R> results;
    results.reserve(futures.size());

    for (auto& future : futures) {
        results.push_back(future.get());
    }

    return results;
}

template<typename F, typename R>
auto thread_pool::submit_wait_any(std::vector<F>&& callables, const submit_options& opts)
    -> R
{
    if (callables.empty()) {
        throw std::invalid_argument("Empty callables vector");
    }

    auto futures = submit<F, R>(std::move(callables), opts);
    auto completed = std::make_shared<std::atomic<bool>>(false);
    auto result_promise = std::make_shared<std::promise<R>>();
    auto result_future = result_promise->get_future();

    for (std::size_t i = 0; i < futures.size(); ++i) {
        std::thread([completed, result_promise, fut = std::move(futures[i])]() mutable {
            try {
                R result = fut.get();
                bool expected = false;
                if (completed->compare_exchange_strong(expected, true)) {
                    result_promise->set_value(std::move(result));
                }
            } catch (...) {
                bool expected = false;
                if (completed->compare_exchange_strong(expected, true)) {
                    result_promise->set_exception(std::current_exception());
                }
            }
        }).detach();
    }

    return result_future.get();
}

template<typename T>
auto thread_pool::find_policy(const std::string& name) -> T*
{
    std::scoped_lock<std::mutex> lock(policies_mutex_);

    for (auto& policy : policies_) {
        if (policy && policy->get_name() == name) {
            return dynamic_cast<T*>(policy.get());
        }
    }

    return nullptr;
}

} // namespace kcenon::thread
