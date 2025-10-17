/**
 * @file common_executor_adapter.h
 * @brief Adapter to bridge thread_system pools with common IExecutor interface.
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
#include <mutex>
#include <optional>
#include <variant>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

namespace kcenon::thread::adapters {

namespace detail {

inline ::common::error_info make_error_info(const kcenon::thread::error& err) {
    return ::common::error_info{
        static_cast<int>(err.code()),
        err.message(),
        "thread_system"
    };
}

inline ::common::error_info make_error_info(int code, std::string message, std::string module = "thread_system") {
    return ::common::error_info{code, std::move(message), std::move(module)};
}

inline std::exception_ptr to_exception(const ::common::error_info& info) {
    std::ostringstream ss;
    ss << "[" << info.module << "] " << info.message << " (code=" << info.code << ")";
    if (info.details) {
        ss << ": " << *info.details;
    }
    return std::make_exception_ptr(std::runtime_error(ss.str()));
}

inline kcenon::thread::result_void make_thread_error(const ::common::error_info& info) {
    return kcenon::thread::result_void(kcenon::thread::error{
        static_cast<kcenon::thread::error_code>(info.code),
        info.message
    });
}

inline kcenon::thread::result_void make_thread_error(kcenon::thread::error_code code, std::string message) {
    return kcenon::thread::result_void(kcenon::thread::error{code, std::move(message)});
}

inline ::common::error_info unexpected_pool_error() {
    return make_error_info(-1, "Thread pool unavailable");
}

inline kcenon::thread::result_void wrap_user_task(const std::function<void()>& task) {
    try {
        task();
        return kcenon::thread::result_void{};
    } catch (const std::exception& ex) {
        return make_thread_error(kcenon::thread::error_code::job_execution_failed, ex.what());
    } catch (...) {
        return make_thread_error(kcenon::thread::error_code::job_execution_failed,
                                 "Unknown exception while executing task");
    }
}

inline ::common::VoidResult convert_result(kcenon::thread::result_void result) {
    if (result.has_error()) {
        const auto& err = result.get_error();
        return ::common::error_info{
            static_cast<int>(err.code()),
            err.message(),
            "thread_system"
        };
    }
    return ::common::VoidResult(std::monostate{});
}

inline std::optional<::common::error_info> enqueue_job(
    const std::shared_ptr<kcenon::thread::thread_pool>& pool,
    const std::shared_ptr<std::promise<void>>& promise,
    std::function<kcenon::thread::result_void()> body) {

    if (!pool) {
        auto info = unexpected_pool_error();
        promise->set_exception(to_exception(info));
        return info;
    }

    auto completion_flag = std::make_shared<std::atomic<bool>>(false);

    auto job = std::make_unique<kcenon::thread::callback_job>(
        [promise, completion_flag, body = std::move(body)]() mutable -> kcenon::thread::result_void {
            try {
                auto result = body();
                if (result.has_error()) {
                    auto info = make_error_info(result.get_error());
                    if (!completion_flag->exchange(true)) {
                        promise->set_exception(to_exception(info));
                    }
                    return result;
                }
                if (!completion_flag->exchange(true)) {
                    promise->set_value();
                }
                return result;
            } catch (const std::exception& ex) {
                auto info = make_error_info(kcenon::thread::error{
                    kcenon::thread::error_code::job_execution_failed,
                    ex.what()
                });
                if (!completion_flag->exchange(true)) {
                    promise->set_exception(to_exception(info));
                }
                return make_thread_error(info);
            } catch (...) {
                auto info = make_error_info(kcenon::thread::error{
                    kcenon::thread::error_code::job_execution_failed,
                    "Unhandled exception while executing job"
                });
                if (!completion_flag->exchange(true)) {
                    promise->set_exception(to_exception(info));
                }
                return make_thread_error(info);
            }
        });

    auto enqueue_result = pool->enqueue(std::move(job));
    if (enqueue_result.has_error()) {
        const auto& err = enqueue_result.get_error();
        auto info = make_error_info(err);
        if (!completion_flag->exchange(true)) {
            promise->set_exception(to_exception(info));
        }
        return info;
    }

    return std::nullopt;
}

inline ::common::Result<std::future<void>> schedule_task(
    const std::shared_ptr<kcenon::thread::thread_pool>& pool,
    std::function<kcenon::thread::result_void()> body) {
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
    std::function<kcenon::thread::result_void()> body,
    std::chrono::milliseconds delay) {
    std::thread([pool = std::move(pool), promise = std::move(promise),
                 body = std::move(body), delay]() mutable {
        try {
            if (delay.count() > 0) {
                std::this_thread::sleep_for(delay);
            }
            if (!pool) {
                promise->set_exception(to_exception(unexpected_pool_error()));
                return;
            }
            (void)enqueue_job(pool, promise, std::move(body));
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    }).detach();
}

} // namespace detail

/**
 * @brief Adapter exposing thread_pool through common::interfaces::IExecutor.
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
            [job = std::move(job)]() mutable -> kcenon::thread::result_void {
                try {
                    auto result = job->execute();
                    if (result.is_err()) {
                        return detail::make_thread_error(result.error());
                    }
                    return kcenon::thread::result_void{};
                } catch (const std::exception& ex) {
                    return detail::make_thread_error(
                        kcenon::thread::error_code::job_execution_failed,
                        ex.what());
                } catch (...) {
                    return detail::make_thread_error(
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
            [job = std::move(job)]() mutable -> kcenon::thread::result_void {
                try {
                    auto result = job->execute();
                    if (result.is_err()) {
                        return detail::make_thread_error(result.error());
                    }
                    return kcenon::thread::result_void{};
                } catch (const std::exception& ex) {
                    return detail::make_thread_error(
                        kcenon::thread::error_code::job_execution_failed,
                        ex.what());
                } catch (...) {
                    return detail::make_thread_error(
                        kcenon::thread::error_code::job_execution_failed,
                        "Unknown exception while executing common job");
                }
            },
            delay);

        return ::common::Result<std::future<void>>::ok(std::move(future));
    }

    size_t worker_count() const override {
        return pool_ ? pool_->get_thread_count() : 0U;
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
        if (stop_result.has_error()) {
            // Best effort: surface error via exception to aid debugging.
            throw std::runtime_error(stop_result.get_error().to_string());
        }
    }

    std::shared_ptr<kcenon::thread::thread_pool> get_thread_pool() const {
        return pool_;
    }

private:
    std::shared_ptr<kcenon::thread::thread_pool> pool_;
};

class common_executor_factory {
public:
    static std::shared_ptr<::common::interfaces::IExecutor> create_from_thread_pool(
        std::shared_ptr<kcenon::thread::thread_pool> pool) {
        return std::make_shared<thread_pool_executor_adapter>(std::move(pool));
    }
};

} // namespace kcenon::thread::adapters
