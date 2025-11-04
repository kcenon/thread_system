/*
 * BSD 3-Clause License
 * Copyright (c) 2025, kcenon
 */

#pragma once

#include <memory>
#include <future>
#include <functional>

// Check if common_system is available
#ifdef BUILD_WITH_COMMON_SYSTEM
#include <kcenon/common/interfaces/executor_interface.h>
#include <kcenon/common/patterns/result.h>
#include <kcenon/common/adapters/typed_adapter.h>
#endif

#include "../core/thread_pool.h"
#include "../interfaces/executor_interface.h"

namespace kcenon::thread::adapters {

#ifdef BUILD_WITH_COMMON_SYSTEM

/**
 * @brief Adapter to expose thread_pool as common::interfaces::IExecutor
 *
 * This adapter allows thread_system's thread_pool to be used
 * through the standard common_system executor interface.
 *
 * Now inherits from typed_adapter for:
 * - Type safety and wrapper depth tracking
 * - Automatic prevention of infinite adapter chains (max depth: 2)
 * - Unwrap support to access underlying thread_pool
 */
class common_system_executor_adapter
    : public ::common::adapters::typed_adapter<::common::interfaces::IExecutor, thread_pool> {
    using base_type = ::common::adapters::typed_adapter<::common::interfaces::IExecutor, thread_pool>;
public:
    /**
     * @brief Construct adapter with existing thread pool
     * @param pool Shared pointer to thread pool
     */
    explicit common_system_executor_adapter(std::shared_ptr<thread_pool> pool)
        : base_type(pool) {}

    /**
     * @brief Construct adapter with new thread pool
     * @param worker_count Number of worker threads
     */
    explicit common_system_executor_adapter(size_t worker_count = std::thread::hardware_concurrency())
        : base_type(std::make_shared<thread_pool>(worker_count)) {}

    ~common_system_executor_adapter() override = default;

    /**
     * @brief Submit a task for immediate execution
     * @param task The function to execute
     * @return Future representing the task result
     */
    std::future<void> submit(std::function<void()> task) override {
        if (!this->impl_) {
            std::promise<void> promise;
            promise.set_exception(std::make_exception_ptr(
                std::runtime_error("Thread pool not initialized")));
            return promise.get_future();
        }

        return this->impl_->enqueue(std::move(task));
    }

    /**
     * @brief Submit a task for delayed execution
     * @param task The function to execute
     * @param delay The delay before execution
     * @return Future representing the task result
     */
    std::future<void> submit_delayed(
        std::function<void()> task,
        std::chrono::milliseconds delay) override {
        if (!this->impl_) {
            std::promise<void> promise;
            promise.set_exception(std::make_exception_ptr(
                std::runtime_error("Thread pool not initialized")));
            return promise.get_future();
        }

        // Use the thread pool itself to handle delayed execution
        // This avoids creating an extra thread via std::async
        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();
        auto pool = this->impl_;

        pool->enqueue([pool, promise, task = std::move(task), delay]() mutable {
            try {
                if (delay.count() > 0) {
                    std::this_thread::sleep_for(delay);
                }
                task();
                promise->set_value();
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        });

        return future;
    }

    /**
     * @brief Get the number of worker threads
     * @return Number of available workers
     */
    size_t worker_count() const override {
        return this->impl_ ? this->impl_->size() : 0;
    }

    /**
     * @brief Check if the executor is running
     * @return true if running, false otherwise
     */
    bool is_running() const override {
        return this->impl_ && !this->impl_->is_stopped();
    }

    /**
     * @brief Get the number of pending tasks
     * @return Number of tasks waiting to be executed
     */
    size_t pending_tasks() const override {
        return this->impl_ ? this->impl_->queue_size() : 0;
    }

    /**
     * @brief Shutdown the executor gracefully
     * @param wait_for_completion Wait for all pending tasks to complete
     */
    void shutdown(bool wait_for_completion = true) override {
        if (this->impl_) {
            if (wait_for_completion) {
                this->impl_->wait();
            }
            this->impl_->stop();
        }
    }

    /**
     * @brief Execute a job with Result-based error handling
     * @param job The job to execute
     * @return Result containing future or error
     */
    ::common::Result<std::future<void>> execute(std::unique_ptr<::common::interfaces::IJob>&& job) override {
        if (!this->impl_) {
            return ::common::make_error<std::future<void>>(
                -1, "Thread pool not initialized", "thread_system");
        }

        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();
        auto pool = this->impl_;

        pool->enqueue([promise, job = std::move(job)]() mutable {
            try {
                auto result = job->execute();
                if (result.is_err()) {
                    auto& err = result.get_error();
                    promise->set_exception(std::make_exception_ptr(
                        std::runtime_error(err.message)));
                } else {
                    promise->set_value();
                }
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        });

        return ::common::Result<std::future<void>>::ok(std::move(future));
    }

    /**
     * @brief Execute a job with delay
     * @param job The job to execute
     * @param delay The delay before execution
     * @return Result containing future or error
     */
    ::common::Result<std::future<void>> execute_delayed(
        std::unique_ptr<::common::interfaces::IJob>&& job,
        std::chrono::milliseconds delay) override {
        if (!this->impl_) {
            return ::common::make_error<std::future<void>>(
                -1, "Thread pool not initialized", "thread_system");
        }

        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();
        auto pool = this->impl_;

        pool->enqueue([pool, promise, job = std::move(job), delay]() mutable {
            try {
                if (delay.count() > 0) {
                    std::this_thread::sleep_for(delay);
                }
                auto result = job->execute();
                if (result.is_err()) {
                    auto& err = result.get_error();
                    promise->set_exception(std::make_exception_ptr(
                        std::runtime_error(err.message)));
                } else {
                    promise->set_value();
                }
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        });

        return ::common::Result<std::future<void>>::ok(std::move(future));
    }

    /**
     * @brief Get the underlying thread pool (convenience method)
     * @return Shared pointer to thread pool
     * @note This is equivalent to unwrap() from typed_adapter
     */
    std::shared_ptr<thread_pool> get_thread_pool() const {
        return this->unwrap();
    }
};


#endif // BUILD_WITH_COMMON_SYSTEM

} // namespace kcenon::thread::adapters