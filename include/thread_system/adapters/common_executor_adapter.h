#pragma once

/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, thread_system contributors
All rights reserved.
*****************************************************************************/

#include <memory>
#include <functional>
#include <future>
#include <chrono>

// Check if common_system is available
#ifdef BUILD_WITH_COMMON_SYSTEM
#include <kcenon/common/interfaces/executor_interface.h>
#endif

#include "../thread_pool.h"
#include "../priority_thread_pool.h"

namespace thread_system {
namespace adapters {

#ifdef BUILD_WITH_COMMON_SYSTEM

/**
 * @brief Adapter to expose thread_pool as common::interfaces::IExecutor
 */
class thread_pool_executor_adapter : public ::common::interfaces::IExecutor {
public:
    /**
     * @brief Construct adapter with thread_pool instance
     */
    explicit thread_pool_executor_adapter(
        std::shared_ptr<thread_pool> pool)
        : pool_(pool) {}

    ~thread_pool_executor_adapter() override = default;

    /**
     * @brief Submit a task for immediate execution
     */
    std::future<void> submit(std::function<void()> task) override {
        if (!pool_) {
            std::promise<void> promise;
            promise.set_exception(
                std::make_exception_ptr(
                    std::runtime_error("Thread pool not initialized")));
            return promise.get_future();
        }

        return pool_->enqueue(std::move(task));
    }

    /**
     * @brief Submit a task for delayed execution
     */
    std::future<void> submit_delayed(
        std::function<void()> task,
        std::chrono::milliseconds delay) override {
        if (!pool_) {
            std::promise<void> promise;
            promise.set_exception(
                std::make_exception_ptr(
                    std::runtime_error("Thread pool not initialized")));
            return promise.get_future();
        }

        // Create a delayed task
        return pool_->enqueue([task = std::move(task), delay]() {
            std::this_thread::sleep_for(delay);
            task();
        });
    }

    /**
     * @brief Get the number of worker threads
     */
    size_t worker_count() const override {
        return pool_ ? pool_->size() : 0;
    }

    /**
     * @brief Check if the executor is running
     */
    bool is_running() const override {
        return pool_ && !pool_->stopped();
    }

    /**
     * @brief Get the number of pending tasks
     */
    size_t pending_tasks() const override {
        // Note: thread_pool might need to expose this information
        return 0; // Placeholder - actual implementation would query pool
    }

    /**
     * @brief Shutdown the executor gracefully
     */
    void shutdown(bool wait_for_completion = true) override {
        if (pool_) {
            pool_->stop(wait_for_completion);
        }
    }

private:
    std::shared_ptr<thread_pool> pool_;
};

/**
 * @brief Adapter to expose priority_thread_pool as common::interfaces::IExecutor
 */
class priority_executor_adapter : public ::common::interfaces::IExecutor {
public:
    /**
     * @brief Construct adapter with priority_thread_pool instance
     */
    explicit priority_executor_adapter(
        std::shared_ptr<priority_thread_pool> pool)
        : pool_(pool) {}

    ~priority_executor_adapter() override = default;

    /**
     * @brief Submit a task for immediate execution (default priority)
     */
    std::future<void> submit(std::function<void()> task) override {
        if (!pool_) {
            std::promise<void> promise;
            promise.set_exception(
                std::make_exception_ptr(
                    std::runtime_error("Priority pool not initialized")));
            return promise.get_future();
        }

        // Submit with default priority (MEDIUM)
        return pool_->enqueue(priority_level::MEDIUM, std::move(task));
    }

    /**
     * @brief Submit a task for delayed execution
     */
    std::future<void> submit_delayed(
        std::function<void()> task,
        std::chrono::milliseconds delay) override {
        if (!pool_) {
            std::promise<void> promise;
            promise.set_exception(
                std::make_exception_ptr(
                    std::runtime_error("Priority pool not initialized")));
            return promise.get_future();
        }

        // Create a delayed task with default priority
        return pool_->enqueue(priority_level::MEDIUM,
            [task = std::move(task), delay]() {
                std::this_thread::sleep_for(delay);
                task();
            });
    }

    /**
     * @brief Get the number of worker threads
     */
    size_t worker_count() const override {
        return pool_ ? pool_->size() : 0;
    }

    /**
     * @brief Check if the executor is running
     */
    bool is_running() const override {
        return pool_ && !pool_->stopped();
    }

    /**
     * @brief Get the number of pending tasks
     */
    size_t pending_tasks() const override {
        return pool_ ? pool_->pending_tasks() : 0;
    }

    /**
     * @brief Shutdown the executor gracefully
     */
    void shutdown(bool wait_for_completion = true) override {
        if (pool_) {
            pool_->stop(wait_for_completion);
        }
    }

    /**
     * @brief Submit a task with specific priority (extension method)
     */
    std::future<void> submit_with_priority(
        priority_level priority,
        std::function<void()> task) {
        if (!pool_) {
            std::promise<void> promise;
            promise.set_exception(
                std::make_exception_ptr(
                    std::runtime_error("Priority pool not initialized")));
            return promise.get_future();
        }

        return pool_->enqueue(priority, std::move(task));
    }

private:
    std::shared_ptr<priority_thread_pool> pool_;
};

/**
 * @brief Adapter to use common::interfaces::IExecutor in thread_system
 */
class executor_from_common_adapter {
public:
    /**
     * @brief Construct adapter with common executor
     */
    explicit executor_from_common_adapter(
        std::shared_ptr<::common::interfaces::IExecutor> executor)
        : common_executor_(executor) {}

    /**
     * @brief Submit a task for execution
     */
    template<typename Func>
    auto submit(Func&& func) -> std::future<void> {
        if (!common_executor_) {
            std::promise<void> promise;
            promise.set_exception(
                std::make_exception_ptr(
                    std::runtime_error("Common executor not initialized")));
            return promise.get_future();
        }

        return common_executor_->submit(std::forward<Func>(func));
    }

    /**
     * @brief Submit a task with delay
     */
    template<typename Func>
    auto submit_delayed(Func&& func, std::chrono::milliseconds delay)
        -> std::future<void> {
        if (!common_executor_) {
            std::promise<void> promise;
            promise.set_exception(
                std::make_exception_ptr(
                    std::runtime_error("Common executor not initialized")));
            return promise.get_future();
        }

        return common_executor_->submit_delayed(
            std::forward<Func>(func), delay);
    }

    /**
     * @brief Get worker count
     */
    size_t worker_count() const {
        return common_executor_ ? common_executor_->worker_count() : 0;
    }

    /**
     * @brief Check if running
     */
    bool is_running() const {
        return common_executor_ && common_executor_->is_running();
    }

    /**
     * @brief Shutdown
     */
    void shutdown(bool wait = true) {
        if (common_executor_) {
            common_executor_->shutdown(wait);
        }
    }

private:
    std::shared_ptr<::common::interfaces::IExecutor> common_executor_;
};

/**
 * @brief Factory for creating common_system compatible executors
 */
class common_executor_factory {
public:
    /**
     * @brief Create a common_system IExecutor from thread_pool
     */
    static std::shared_ptr<::common::interfaces::IExecutor> create_from_thread_pool(
        std::shared_ptr<thread_pool> pool) {
        return std::make_shared<thread_pool_executor_adapter>(pool);
    }

    /**
     * @brief Create a common_system IExecutor from priority_thread_pool
     */
    static std::shared_ptr<::common::interfaces::IExecutor> create_from_priority_pool(
        std::shared_ptr<priority_thread_pool> pool) {
        return std::make_shared<priority_executor_adapter>(pool);
    }

    /**
     * @brief Create a thread_system wrapper from common IExecutor
     */
    static std::unique_ptr<executor_from_common_adapter> create_from_common(
        std::shared_ptr<::common::interfaces::IExecutor> executor) {
        return std::make_unique<executor_from_common_adapter>(executor);
    }
};

#endif // BUILD_WITH_COMMON_SYSTEM

} // namespace adapters
} // namespace thread_system