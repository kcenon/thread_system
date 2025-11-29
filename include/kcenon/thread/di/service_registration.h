// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file service_registration.h
 * @brief Service container registration for thread_system services.
 *
 * This header provides functions to register thread_system services
 * with the unified service container from common_system.
 *
 * @see TICKET-103 for integration requirements.
 */

#pragma once

#include <memory>
#include <thread>

#ifdef BUILD_WITH_COMMON_SYSTEM

#include <kcenon/common/di/service_container.h>
#include <kcenon/common/interfaces/executor_interface.h>

#include "../adapters/common_system_executor_adapter.h"
#include "../core/thread_pool.h"

namespace kcenon::thread::di {

/**
 * @brief Configuration for thread pool service registration
 */
struct executor_registration_config {
    /// Number of worker threads (0 = hardware_concurrency)
    size_t worker_count = 0;

    /// Service lifetime
    common::di::service_lifetime lifetime = common::di::service_lifetime::singleton;
};

/**
 * @brief Register executor services with the service container.
 *
 * Registers IExecutor implementation using thread_system's thread_pool.
 * By default, registers as a singleton with hardware_concurrency workers.
 *
 * @param container The service container to register with
 * @param config Optional configuration for the executor
 * @return VoidResult indicating success or registration error
 *
 * @code
 * auto& container = common::di::service_container::global();
 *
 * // Register with default configuration
 * auto result = register_executor_services(container);
 *
 * // Or with custom configuration
 * executor_registration_config config;
 * config.worker_count = 8;
 * auto result = register_executor_services(container, config);
 *
 * // Then resolve executor anywhere in the application
 * auto executor = container.resolve<common::interfaces::IExecutor>().value();
 * auto future = executor->submit([]() { /* work */ });
 * @endcode
 */
inline common::VoidResult register_executor_services(
    common::di::IServiceContainer& container,
    const executor_registration_config& config = {}) {

    // Check if already registered
    if (container.is_registered<common::interfaces::IExecutor>()) {
        return common::make_error<std::monostate>(
            common::di::di_error_codes::already_registered,
            "IExecutor is already registered",
            "thread_system::di"
        );
    }

    // Determine worker count
    size_t workers = config.worker_count;
    if (workers == 0) {
        workers = std::thread::hardware_concurrency();
        if (workers == 0) {
            workers = 4;  // fallback default
        }
    }

    // Register executor factory
    return container.register_factory<common::interfaces::IExecutor>(
        [workers](common::di::IServiceContainer&) -> std::shared_ptr<common::interfaces::IExecutor> {
            return std::make_shared<adapters::common_system_executor_adapter>(workers);
        },
        config.lifetime
    );
}

/**
 * @brief Register a pre-configured thread pool instance.
 *
 * Use this when you have already created a thread pool and want
 * to register it with the container.
 *
 * @param container The service container to register with
 * @param pool The thread pool instance to register
 * @return VoidResult indicating success or registration error
 *
 * @code
 * // Create thread pool with custom settings
 * auto pool = std::make_shared<thread_pool>(16);
 *
 * // Register the instance
 * register_executor_instance(container, pool);
 * @endcode
 */
inline common::VoidResult register_executor_instance(
    common::di::IServiceContainer& container,
    std::shared_ptr<thread_pool> pool) {

    if (!pool) {
        return common::make_error<std::monostate>(
            common::error_codes::INVALID_ARGUMENT,
            "Cannot register null thread pool instance",
            "thread_system::di"
        );
    }

    auto adapter = std::make_shared<adapters::common_system_executor_adapter>(pool);
    return container.register_instance<common::interfaces::IExecutor>(adapter);
}

/**
 * @brief Unregister executor services from the container.
 *
 * @param container The service container to unregister from
 * @return VoidResult indicating success or error
 */
inline common::VoidResult unregister_executor_services(
    common::di::IServiceContainer& container) {

    return container.unregister<common::interfaces::IExecutor>();
}

/**
 * @brief Register all thread_system services with the container.
 *
 * Convenience function to register all available thread_system services.
 *
 * @param container The service container to register with
 * @param executor_config Optional configuration for executor
 * @return VoidResult indicating success or registration error
 */
inline common::VoidResult register_thread_services(
    common::di::IServiceContainer& container,
    const executor_registration_config& executor_config = {}) {

    // Register IExecutor
    auto result = register_executor_services(container, executor_config);
    if (result.is_err()) {
        return result;
    }

    return common::VoidResult::ok({});
}

} // namespace kcenon::thread::di

#endif // BUILD_WITH_COMMON_SYSTEM
