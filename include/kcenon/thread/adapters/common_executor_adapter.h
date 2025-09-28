/**
 * @file common_executor_adapter.h
 * @brief Adapter to bridge thread_system with common IExecutor interface
 *
 * This adapter enables thread_system's thread_pool to work with the
 * common executor interface, promoting loose coupling between modules.
 */

#pragma once

#if __has_include(<kcenon/common/interfaces/executor_interface.h>)
#include <kcenon/common/interfaces/executor_interface.h>
#elif __has_include(<common/interfaces/executor_interface.h>)
#include <common/interfaces/executor_interface.h>
#ifndef KCENON_COMMON_EXECUTOR_FALLBACK_DEFINED
#define KCENON_COMMON_EXECUTOR_FALLBACK_DEFINED
namespace kcenon {
namespace common {
namespace interfaces {
using ::common::interfaces::IExecutor;
using ::common::interfaces::ExecutorFactory;
using ::common::interfaces::IExecutorProvider;
} // namespace interfaces
} // namespace common
} // namespace kcenon
#endif // KCENON_COMMON_EXECUTOR_FALLBACK_DEFINED
#else
#error "Unable to locate common executor interface header."
#endif
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/typed_thread_pool.h>

#include <chrono>
#include <future>
#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

namespace kcenon {
namespace thread {
namespace adapters {

/**
 * @class thread_pool_to_common_executor
 * @brief Adapts thread_pool to kcenon::common::interfaces::IExecutor
 *
 * This adapter allows thread_system's thread_pool to be used
 * wherever the common IExecutor interface is expected.
 */
class thread_pool_to_common_executor : public kcenon::common::interfaces::IExecutor {
public:
    explicit thread_pool_to_common_executor(std::shared_ptr<thread_pool> pool)
        : pool_(std::move(pool)) {
        if (!pool_) {
            throw std::invalid_argument("Thread pool cannot be null");
        }
    }

    // IExecutor implementation
    std::future<void> submit(std::function<void()> task) override {
        return pool_->submit(std::move(task));
    }

    std::future<void> submit_delayed(std::function<void()> task,
                                    std::chrono::milliseconds delay) override {
        return pool_->submit_delayed(std::move(task), delay);
    }

    size_t worker_count() const override {
        return pool_->get_thread_count();
    }

    bool is_running() const override {
        return pool_->is_running();
    }

    size_t pending_tasks() const override {
        return pool_->get_queue_size();
    }

    void shutdown(bool wait_for_completion = true) override {
        if (wait_for_completion) {
            pool_->wait_for_all();
        }
        pool_->stop();
    }

    // Access underlying thread_pool
    std::shared_ptr<thread_pool> get_thread_pool() const {
        return pool_;
    }

private:
    std::shared_ptr<thread_pool> pool_;
};

/**
 * @class typed_pool_to_common_executor
 * @brief Adapts typed_thread_pool to kcenon::common::interfaces::IExecutor
 *
 * This adapter allows typed_thread_pool to be used with the common interface.
 */
template<typename ReturnType>
class typed_pool_to_common_executor : public kcenon::common::interfaces::IExecutor {
public:
    explicit typed_pool_to_common_executor(
        std::shared_ptr<typed_thread_pool<ReturnType>> pool)
        : pool_(std::move(pool)) {
        if (!pool_) {
            throw std::invalid_argument("Typed thread pool cannot be null");
        }
    }

    // IExecutor implementation (void tasks only)
    std::future<void> submit(std::function<void()> task) override {
        // Wrap void task to return ReturnType (requires specialization or default)
        if constexpr (std::is_void_v<ReturnType>) {
            return pool_->submit(std::move(task));
        } else {
            // For non-void return types, we need to adapt the task
            auto promise = std::make_shared<std::promise<void>>();
            auto future = promise->get_future();

            pool_->submit([task = std::move(task), promise]() -> ReturnType {
                task();
                promise->set_value();
                if constexpr (!std::is_void_v<ReturnType>) {
                    return ReturnType{};
                }
            });

            return future;
        }
    }

    std::future<void> submit_delayed(std::function<void()> task,
                                    std::chrono::milliseconds delay) override {
        // Note: typed_thread_pool may not support delayed submission directly
        // This would need implementation in the base class or workaround
        std::this_thread::sleep_for(delay);
        return submit(std::move(task));
    }

    size_t worker_count() const override {
        return pool_->get_thread_count();
    }

    bool is_running() const override {
        return pool_->is_running();
    }

    size_t pending_tasks() const override {
        return pool_->get_queue_size();
    }

    void shutdown(bool wait_for_completion = true) override {
        if (wait_for_completion) {
            pool_->wait_for_all();
        }
        pool_->stop();
    }

private:
    std::shared_ptr<typed_thread_pool<ReturnType>> pool_;
};

/**
 * @class common_executor_provider
 * @brief Provides thread_system executors as common IExecutor instances
 */
class common_executor_provider : public kcenon::common::interfaces::IExecutorProvider {
public:
    common_executor_provider(size_t default_worker_count = std::thread::hardware_concurrency())
        : default_worker_count_(default_worker_count) {}

    std::shared_ptr<kcenon::common::interfaces::IExecutor> get_executor() override {
        if (!default_executor_) {
            default_executor_ = create_executor(default_worker_count_);
        }
        return default_executor_;
    }

    std::shared_ptr<kcenon::common::interfaces::IExecutor> create_executor(size_t worker_count) override {
        auto pool = std::make_shared<thread_pool>(worker_count);
        return std::make_shared<thread_pool_to_common_executor>(pool);
    }

private:
    size_t default_worker_count_;
    std::shared_ptr<kcenon::common::interfaces::IExecutor> default_executor_;
};

/**
 * @brief Factory function to create an IExecutor from thread_pool
 */
inline std::shared_ptr<kcenon::common::interfaces::IExecutor> make_common_executor(
    std::shared_ptr<thread_pool> pool) {
    return std::make_shared<thread_pool_to_common_executor>(std::move(pool));
}

/**
 * @brief Factory function to create an IExecutor from typed_thread_pool
 */
template<typename ReturnType>
inline std::shared_ptr<kcenon::common::interfaces::IExecutor> make_common_executor(
    std::shared_ptr<typed_thread_pool<ReturnType>> pool) {
    return std::make_shared<typed_pool_to_common_executor<ReturnType>>(std::move(pool));
}

} // namespace adapters
} // namespace thread
} // namespace kcenon
