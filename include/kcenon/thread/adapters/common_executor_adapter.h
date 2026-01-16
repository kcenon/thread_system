/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, 🍀☀🌕🌥 🌊
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
 * @file common_executor_adapter.h
 * @brief Adapter to bridge thread_system pools with common IExecutor interface.
 *
 * This adapter provides the recommended way to use thread_pool with the
 * common::interfaces::IExecutor interface. Direct inheritance of IExecutor
 * by thread_pool is deprecated and will be removed in v2.0.
 *
 * @par Migration Guide
 * If you were previously using thread_pool directly as an IExecutor:
 * @code
 * // Old way (deprecated):
 * auto pool = std::make_shared<kcenon::thread::thread_pool>("my_pool");
 * kcenon::common::interfaces::IExecutor* executor = pool.get();
 * executor->execute(std::move(job));
 *
 * // New way (recommended):
 * auto pool = std::make_shared<kcenon::thread::thread_pool>("my_pool");
 * auto executor = std::make_shared<kcenon::thread::adapters::thread_pool_executor_adapter>(pool);
 * executor->execute(std::move(job));
 * @endcode
 *
 * @par Benefits of the Adapter Approach
 * - Cleaner separation of concerns: thread_pool focuses on thread management
 * - Easier maintenance: IExecutor changes don't affect thread_pool core
 * - Better testability: adapter can be mocked independently
 * - Reduced compilation dependencies: conditional compilation isolated to adapter
 *
 * @see thread_pool_executor_adapter The main adapter class
 * @see common_executor_factory Factory for creating adapters
 */

#pragma once

#if __has_include(<kcenon/common/interfaces/executor_interface.h>)
#include <kcenon/common/interfaces/executor_interface.h>
#include <kcenon/common/patterns/result.h>
#elif __has_include(<common/interfaces/executor_interface.h>)
#include <common/interfaces/executor_interface.h>
#include <common/patterns/result.h>
#ifndef KCENON_COMMON_EXECUTOR_FALLBACK_DEFINED
#define KCENON_COMMON_EXECUTOR_FALLBACK_DEFINED
namespace kcenon {
namespace common {
using ::common::Result;
using ::common::VoidResult;
namespace interfaces {
using IExecutor = ::common::interfaces::IExecutor;
}
} // namespace common
} // namespace kcenon
#endif
#else
#error "Unable to locate common executor interface header."
#endif

#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/core/error_handling.h>
#include <kcenon/thread/core/thread_pool.h>

#include <atomic>
#include <chrono>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

namespace kcenon::thread::adapters {

namespace detail {

inline ::common::error_info make_error_info(int code, std::string message, std::string module = "thread_system") {
    return ::common::error_info{code, std::move(message), std::move(module)};
}

inline ::common::error_info make_error_info(kcenon::thread::error_code code, std::string message) {
    return ::common::error_info{
        static_cast<int>(code),
        std::move(message),
        "thread_system"
    };
}

inline std::exception_ptr to_exception(const ::common::error_info& info) {
    std::ostringstream ss;
    ss << "[" << info.module << "] " << info.message << " (code=" << info.code << ")";
    if (info.details) {
        ss << ": " << *info.details;
    }
    return std::make_exception_ptr(std::runtime_error(ss.str()));
}

inline ::common::VoidResult make_error(const ::common::error_info& info) {
    return ::common::VoidResult(info);
}

inline ::common::VoidResult make_error(kcenon::thread::error_code code, std::string message) {
    return ::common::VoidResult(make_error_info(code, std::move(message)));
}

inline ::common::error_info unexpected_pool_error() {
    return make_error_info(-1, "Thread pool unavailable");
}

inline ::common::VoidResult wrap_user_task(const std::function<void()>& task) {
    try {
        task();
        return ::common::ok();
    } catch (const std::exception& ex) {
        return make_error(kcenon::thread::error_code::job_execution_failed, ex.what());
    } catch (...) {
        return make_error(kcenon::thread::error_code::job_execution_failed,
                          "Unknown exception while executing task");
    }
}

inline std::optional<::common::error_info> enqueue_job(
    const std::shared_ptr<kcenon::thread::thread_pool>& pool,
    const std::shared_ptr<std::promise<void>>& promise,
    std::function<::common::VoidResult()> body) {
    if (!pool) {
        auto info = unexpected_pool_error();
        promise->set_exception(to_exception(info));
        return info;
    }

    auto completion_once = std::make_shared<std::once_flag>();

    auto job = std::make_unique<kcenon::thread::callback_job>(
        [promise, completion_once, body = std::move(body)]() mutable -> ::common::VoidResult {
            try {
                auto result = body();
                if (result.is_err()) {
                    auto info = result.error();
                    std::call_once(*completion_once, [&]() {
                        promise->set_exception(to_exception(info));
                    });
                    return result;
                }
                std::call_once(*completion_once, [&]() {
                    promise->set_value();
                });
                return result;
            } catch (const std::exception& ex) {
                auto info = make_error_info(
                    kcenon::thread::error_code::job_execution_failed,
                    ex.what()
                );
                std::call_once(*completion_once, [&]() {
                    promise->set_exception(to_exception(info));
                });
                return make_error(info);
            } catch (...) {
                auto info = make_error_info(
                    kcenon::thread::error_code::job_execution_failed,
                    "Unhandled exception while executing job"
                );
                std::call_once(*completion_once, [&]() {
                    promise->set_exception(to_exception(info));
                });
                return make_error(info);
            }
        });

    auto enqueue_result = pool->enqueue(std::move(job));
    if (enqueue_result.is_err()) {
        const auto& info = enqueue_result.error();
        std::call_once(*completion_once, [&]() {
            promise->set_exception(to_exception(info));
        });
        return info;
    }

    return std::nullopt;
}

inline ::common::Result<std::future<void>> schedule_task(
    const std::shared_ptr<kcenon::thread::thread_pool>& pool,
    std::function<::common::VoidResult()> body) {
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    if (auto error = enqueue_job(pool, promise, std::move(body))) {
        return ::common::Result<std::future<void>>(*error);
    }

    return ::common::Result<std::future<void>>::ok(std::move(future));
}

inline void schedule_task_async(
    std::shared_ptr<kcenon::thread::thread_pool> pool,
    std::shared_ptr<std::promise<void>> promise,
    std::function<::common::VoidResult()> body,
    std::chrono::milliseconds delay) {
    // Use the thread pool itself to handle delayed execution
    // This avoids creating detached threads or extra std::async threads
    if (!pool) {
        promise->set_exception(to_exception(unexpected_pool_error()));
        return;
    }

    // Create once_flag to protect against race between delayed_job exception and enqueue failure
    auto completion_once = std::make_shared<std::once_flag>();

    // Create a wrapper job that handles delay and then enqueues the actual task
    auto delayed_job = std::make_unique<kcenon::thread::callback_job>(
        [pool, promise, completion_once, body = std::move(body), delay]() mutable -> ::common::VoidResult {
            try {
                if (delay.count() > 0) {
                    std::this_thread::sleep_for(delay);
                }
                // Enqueue the actual job after the delay
                // Note: enqueue_job has its own once_flag protection internally
                (void)enqueue_job(pool, promise, std::move(body));
                return ::common::ok();
            } catch (...) {
                std::call_once(*completion_once, [&]() {
                    promise->set_exception(std::current_exception());
                });
                return make_error(kcenon::thread::error_code::job_execution_failed,
                                  "Exception during delayed task scheduling");
            }
        });

    // Enqueue the delayed job to the pool
    auto enqueue_result = pool->enqueue(std::move(delayed_job));
    if (enqueue_result.is_err()) {
        std::call_once(*completion_once, [&]() {
            promise->set_exception(to_exception(enqueue_result.error()));
        });
    }
}

} // namespace detail

/**
 * @brief Adapter exposing thread_pool through common::interfaces::IExecutor.
 *
 * This is the recommended way to use thread_pool with the IExecutor interface.
 * It provides a clean separation between thread_pool's core functionality and
 * the IExecutor interface contract.
 *
 * @par Example Usage
 * @code
 * #include <kcenon/thread/core/thread_pool.h>
 * #include <kcenon/thread/adapters/common_executor_adapter.h>
 *
 * // Create and configure the thread pool
 * auto pool = std::make_shared<kcenon::thread::thread_pool>("my_pool");
 * pool->enqueue(std::make_unique<kcenon::thread::thread_worker>());
 * pool->start();
 *
 * // Create the adapter for IExecutor compatibility
 * auto executor = std::make_shared<kcenon::thread::adapters::thread_pool_executor_adapter>(pool);
 *
 * // Use through IExecutor interface
 * auto future = executor->execute(std::make_unique<MyJob>());
 * @endcode
 *
 * @note The adapter holds a shared_ptr to the thread_pool, ensuring the pool
 *       remains alive as long as the adapter exists.
 */
class thread_pool_executor_adapter : public ::common::interfaces::IExecutor {
public:
    explicit thread_pool_executor_adapter(std::shared_ptr<kcenon::thread::thread_pool> pool)
        : pool_(std::move(pool)) {}

    std::future<void> submit(std::function<void()> task) override {
        auto result = detail::schedule_task(pool_, [task = std::move(task)]() mutable {
            return detail::wrap_user_task(task);
        });

        if (result.is_ok()) {
            return std::move(result.unwrap());
        }

        std::promise<void> failed;
        failed.set_exception(detail::to_exception(result.error()));
        return failed.get_future();
    }

    std::future<void> submit_delayed(std::function<void()> task,
                                     std::chrono::milliseconds delay) override {
        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();

        detail::schedule_task_async(pool_, promise,
            [task = std::move(task)]() mutable {
                return detail::wrap_user_task(task);
            },
            delay);

        return future;
    }

    ::common::Result<std::future<void>> execute(std::unique_ptr<::common::interfaces::IJob>&& job) override {
        return detail::schedule_task(pool_,
            [job = std::move(job)]() mutable -> ::common::VoidResult {
                try {
                    auto result = job->execute();
                    if (result.is_err()) {
                        return detail::make_error(result.error());
                    }
                    return ::common::ok();
                } catch (const std::exception& ex) {
                    return detail::make_error(
                        kcenon::thread::error_code::job_execution_failed,
                        ex.what());
                } catch (...) {
                    return detail::make_error(
                        kcenon::thread::error_code::job_execution_failed,
                        "Unknown exception while executing common job");
                }
            });
    }

    ::common::Result<std::future<void>> execute_delayed(
        std::unique_ptr<::common::interfaces::IJob>&& job,
        std::chrono::milliseconds delay) override {
        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();

        detail::schedule_task_async(pool_, promise,
            [job = std::move(job)]() mutable -> ::common::VoidResult {
                try {
                    auto result = job->execute();
                    if (result.is_err()) {
                        return detail::make_error(result.error());
                    }
                    return ::common::ok();
                } catch (const std::exception& ex) {
                    return detail::make_error(
                        kcenon::thread::error_code::job_execution_failed,
                        ex.what());
                } catch (...) {
                    return detail::make_error(
                        kcenon::thread::error_code::job_execution_failed,
                        "Unknown exception while executing common job");
                }
            },
            delay);

        return ::common::Result<std::future<void>>::ok(std::move(future));
    }

    size_t worker_count() const override {
        return pool_ ? pool_->get_active_worker_count() : 0U;
    }

    bool is_running() const override {
        return pool_ && pool_->is_running();
    }

    size_t pending_tasks() const override {
        return pool_ ? pool_->get_pending_task_count() : 0U;
    }

    void shutdown(bool wait_for_completion = true) override {
        if (!pool_) {
            return;
        }

        auto stop_result = pool_->stop(!wait_for_completion);
        if (stop_result.is_err()) {
            // Best effort: surface error via exception to aid debugging.
            const auto& err = stop_result.error();
            throw std::runtime_error(err.message);
        }
    }

    std::shared_ptr<kcenon::thread::thread_pool> get_thread_pool() const {
        return pool_;
    }

private:
    std::shared_ptr<kcenon::thread::thread_pool> pool_;
};

/**
 * @brief Factory for creating IExecutor adapters from thread_pool instances.
 *
 * Provides a convenient way to create thread_pool_executor_adapter instances.
 *
 * @par Example Usage
 * @code
 * auto pool = std::make_shared<kcenon::thread::thread_pool>("my_pool");
 * auto executor = kcenon::thread::adapters::common_executor_factory::create_from_thread_pool(pool);
 * @endcode
 */
class common_executor_factory {
public:
    /**
     * @brief Create an IExecutor adapter from a thread_pool.
     *
     * @param pool The thread_pool to wrap
     * @return Shared pointer to an IExecutor that delegates to the pool
     */
    static std::shared_ptr<::common::interfaces::IExecutor> create_from_thread_pool(
        std::shared_ptr<kcenon::thread::thread_pool> pool) {
        return std::make_shared<thread_pool_executor_adapter>(std::move(pool));
    }
};

} // namespace kcenon::thread::adapters
