#pragma once

/**
 * @file thread_pool_builder.h
 * @brief A builder class for creating and configuring thread pools
 */

#include "thread_pool.h"
#include "../thread_base/sync/error_handling.h"

#include <memory>
#include <string>
#include <optional>
#include <chrono>

namespace thread_pool_module {

/**
 * @class thread_pool_config
 * @brief Configuration options for a thread pool
 */
struct thread_pool_config {
    // Number of worker threads to create
    std::size_t thread_count = 4;
    
    // The prefix to use for worker thread names
    std::string thread_name_prefix = "worker";
    
    // Whether to use work stealing between workers
    bool use_work_stealing = false;
    
    // Whether to use priority-based scheduling
    bool use_types = false;
    
    // The interval at which workers will wake to check for work
    std::optional<std::chrono::milliseconds> wake_interval = std::nullopt;
    
    // The maximum number of jobs the queue can hold (0 = unlimited)
    std::size_t max_queue_size = 0;
    
    // Whether threads should yield when idle
    bool yield_on_idle = true;
    
    // Whether to pin threads to specific CPU cores
    bool pin_threads_to_cores = false;
};

/**
 * @class thread_pool_builder
 * @brief A builder class for creating and configuring thread pools with a fluent interface
 *
 * This class provides a fluent interface for configuring and creating thread pools.
 * It allows you to set various options and then build a thread pool with those options.
 */
class thread_pool_builder {
public:
    /**
     * @brief Constructs a new thread pool builder with default configuration.
     */
    thread_pool_builder() = default;
    
    /**
     * @brief Sets the number of worker threads in the pool.
     * @param count The number of threads (default is 4)
     * @return A reference to this builder for method chaining
     */
    thread_pool_builder& with_thread_count(std::size_t count) {
        config_.thread_count = count;
        return *this;
    }
    
    /**
     * @brief Sets the prefix used for worker thread names.
     * @param prefix The prefix string (default is "worker")
     * @return A reference to this builder for method chaining
     */
    thread_pool_builder& with_thread_name_prefix(std::string prefix) {
        config_.thread_name_prefix = std::move(prefix);
        return *this;
    }
    
    /**
     * @brief Enables or disables work stealing between workers.
     * @param enabled Whether to enable work stealing (default is false)
     * @return A reference to this builder for method chaining
     */
    thread_pool_builder& with_work_stealing(bool enabled = true) {
        config_.use_work_stealing = enabled;
        return *this;
    }
    
    /**
     * @brief Enables or disables priority-based scheduling.
     * @param enabled Whether to enable types (default is false)
     * @return A reference to this builder for method chaining
     */
    thread_pool_builder& with_types(bool enabled = true) {
        config_.use_types = enabled;
        return *this;
    }
    
    /**
     * @brief Sets the interval at which workers will wake to check for work.
     * @param interval The wake interval
     * @return A reference to this builder for method chaining
     */
    thread_pool_builder& with_wake_interval(std::chrono::milliseconds interval) {
        config_.wake_interval = interval;
        return *this;
    }
    
    /**
     * @brief Sets the maximum number of jobs the queue can hold.
     * @param size The maximum queue size (0 = unlimited)
     * @return A reference to this builder for method chaining
     */
    thread_pool_builder& with_max_queue_size(std::size_t size) {
        config_.max_queue_size = size;
        return *this;
    }
    
    /**
     * @brief Sets whether threads should yield when idle.
     * @param yield Whether to yield on idle (default is true)
     * @return A reference to this builder for method chaining
     */
    thread_pool_builder& with_yield_on_idle(bool yield = true) {
        config_.yield_on_idle = yield;
        return *this;
    }
    
    /**
     * @brief Sets whether to pin threads to specific CPU cores.
     * @param pin Whether to pin threads (default is false)
     * @return A reference to this builder for method chaining
     */
    thread_pool_builder& with_thread_pinning(bool pin = true) {
        config_.pin_threads_to_cores = pin;
        return *this;
    }
    
    /**
     * @brief Builds a thread pool with the configured options.
     * @return A result containing a shared pointer to the thread pool, or an error
     */
    result_t<std::shared_ptr<thread_pool>> build() {
        try {
            auto pool = std::make_shared<thread_pool>(config_.thread_name_prefix);
            
            // Configure the job queue based on the options
            auto job_queue = pool->get_job_queue();
            // Additional queue configuration would go here
            
            // Create worker threads
            for (std::size_t i = 0; i < config_.thread_count; ++i) {
                std::string worker_name = config_.thread_name_prefix + "_" + std::to_string(i);
                auto worker = std::make_unique<thread_worker>();
                
                // Set worker name and configuration
                // Additional worker configuration would go here
                
                auto result = pool->enqueue(std::move(worker));
                if (result.has_value()) {
                    return thread_module::error{thread_module::error_code::thread_start_failure, *result};
                }
            }
            
            return pool;
        } catch (const std::exception& e) {
            return thread_module::error{thread_module::error_code::resource_allocation_failed, e.what()};
        }
    }
    
    /**
     * @brief Builds and starts a thread pool with the configured options.
     * @return A result containing a shared pointer to the thread pool, or an error
     */
    result_t<std::shared_ptr<thread_pool>> build_and_start() {
        auto result = build();
        if (!result) {
            return result;
        }
        
        auto pool = result.value();
        auto start_error = pool->start();
        if (start_error) {
            return thread_module::error{thread_module::error_code::thread_start_failure, *start_error};
        }
        
        return pool;
    }
    
private:
    thread_pool_config config_;
};

} // namespace thread_pool_module