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
 */
class common_system_executor_adapter : public ::common::interfaces::IExecutor {
public:
    /**
     * @brief Construct adapter with existing thread pool
     * @param pool Shared pointer to thread pool
     */
    explicit common_system_executor_adapter(std::shared_ptr<thread_pool> pool)
        : pool_(pool) {}

    /**
     * @brief Construct adapter with new thread pool
     * @param worker_count Number of worker threads
     */
    explicit common_system_executor_adapter(size_t worker_count = std::thread::hardware_concurrency())
        : pool_(std::make_shared<thread_pool>(worker_count)) {}

    ~common_system_executor_adapter() override = default;

    /**
     * @brief Submit a task for immediate execution
     * @param task The function to execute
     * @return Future representing the task result
     */
    std::future<void> submit(std::function<void()> task) override {
        if (!pool_) {
            std::promise<void> promise;
            promise.set_exception(std::make_exception_ptr(
                std::runtime_error("Thread pool not initialized")));
            return promise.get_future();
        }

        return pool_->enqueue(std::move(task));
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
        if (!pool_) {
            std::promise<void> promise;
            promise.set_exception(std::make_exception_ptr(
                std::runtime_error("Thread pool not initialized")));
            return promise.get_future();
        }

        // Schedule delayed execution
        return std::async(std::launch::async, [this, task = std::move(task), delay]() {
            std::this_thread::sleep_for(delay);
            pool_->enqueue(std::move(task)).wait();
        });
    }

    /**
     * @brief Get the number of worker threads
     * @return Number of available workers
     */
    size_t worker_count() const override {
        return pool_ ? pool_->size() : 0;
    }

    /**
     * @brief Check if the executor is running
     * @return true if running, false otherwise
     */
    bool is_running() const override {
        return pool_ && !pool_->is_stopped();
    }

    /**
     * @brief Get the number of pending tasks
     * @return Number of tasks waiting to be executed
     */
    size_t pending_tasks() const override {
        return pool_ ? pool_->queue_size() : 0;
    }

    /**
     * @brief Shutdown the executor gracefully
     * @param wait_for_completion Wait for all pending tasks to complete
     */
    void shutdown(bool wait_for_completion = true) override {
        if (pool_) {
            if (wait_for_completion) {
                pool_->wait();
            }
            pool_->stop();
        }
    }

    /**
     * @brief Get the underlying thread pool
     * @return Shared pointer to thread pool
     */
    std::shared_ptr<thread_pool> get_thread_pool() const {
        return pool_;
    }

private:
    std::shared_ptr<thread_pool> pool_;
};

/**
 * @brief Adapter to expose common::interfaces::IExecutor as thread_system executor
 *
 * This adapter allows a common_system executor to be used
 * through the thread_system executor interface.
 */
class executor_from_common_adapter : public executor_interface {
public:
    /**
     * @brief Construct adapter with common executor
     * @param executor Shared pointer to common executor
     */
    explicit executor_from_common_adapter(
        std::shared_ptr<::common::interfaces::IExecutor> executor)
        : executor_(executor) {}

    ~executor_from_common_adapter() override = default;

    /**
     * @brief Submit a unit of work for asynchronous execution
     */
    auto execute(std::unique_ptr<job>&& work) -> result_void override {
        if (!executor_) {
            return result_void::failure(
                error_code::invalid_argument,
                "Common executor not initialized");
        }

        try {
            // Convert job to function and submit
            executor_->submit([j = std::move(work)]() mutable {
                if (j) {
                    j->execute();
                }
            });
            return result_void::success();
        } catch (const std::exception& e) {
            return result_void::failure(
                error_code::runtime_error,
                std::string("Failed to submit job: ") + e.what());
        }
    }

    /**
     * @brief Initiate a cooperative shutdown
     */
    auto shutdown() -> result_void override {
        if (!executor_) {
            return result_void::failure(
                error_code::invalid_state,
                "Common executor not initialized");
        }

        try {
            executor_->shutdown(true);
            return result_void::success();
        } catch (const std::exception& e) {
            return result_void::failure(
                error_code::runtime_error,
                std::string("Failed to shutdown: ") + e.what());
        }
    }

private:
    std::shared_ptr<::common::interfaces::IExecutor> executor_;
};

#endif // BUILD_WITH_COMMON_SYSTEM

} // namespace kcenon::thread::adapters