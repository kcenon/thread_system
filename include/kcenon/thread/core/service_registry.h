/*
 * BSD 3-Clause License
 * Copyright (c) 2025, DongCheol Shin
 */

#pragma once

#include <any>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <typeindex>
#include <unordered_map>

namespace kcenon::thread {

/**
 * @brief Lightweight service registry for dependency lookup.
 *
 * @thread_safety Thread-safe for concurrent registration and retrieval.
 *                All methods use std::shared_mutex for synchronization.
 */
class service_registry {
private:
    static inline std::unordered_map<std::type_index, std::any> services_{};
    static inline std::shared_mutex mutex_{};

public:
    /**
     * @brief Register a service implementation for the given interface type.
     *
     * @tparam Interface Service interface type
     * @param service Shared pointer to service implementation
     *
     * @thread_safety Thread-safe. Uses exclusive lock (std::unique_lock).
     * @note If service is already registered, it will be replaced.
     */
    template <typename Interface>
    static auto register_service(std::shared_ptr<Interface> service) -> void {
        std::unique_lock lock(mutex_);
        services_[std::type_index(typeid(Interface))] = std::move(service);
    }

    /**
     * @brief Retrieve a registered service by interface type.
     *
     * @tparam Interface Service interface type to lookup
     * @return std::shared_ptr<Interface> Service instance, or nullptr if not found
     *
     * @thread_safety Thread-safe for concurrent reads and writes.
     *                Uses shared lock (std::shared_lock) for read access.
     * @note Service must be registered as std::shared_ptr<Interface>.
     * @warning std::any_cast failure throws std::bad_any_cast if type mismatch occurs.
     *
     * @example
     * @code
     * auto logger = service_registry::get_service<ILogger>();
     * if (logger) {
     *     logger->log("Service retrieved successfully");
     * }
     * @endcode
     */
    template <typename Interface>
    static auto get_service() -> std::shared_ptr<Interface> {
        std::shared_lock lock(mutex_);
        auto it = services_.find(std::type_index(typeid(Interface)));
        if (it != services_.end()) {
            try {
                return std::any_cast<std::shared_ptr<Interface>>(it->second);
            } catch (const std::bad_any_cast& e) {
                // Type mismatch - service was registered with incompatible type
                return nullptr;
            }
        }
        return nullptr;
    }

    /**
     * @brief Remove all registered services.
     *
     * @thread_safety Thread-safe. Uses exclusive lock (std::unique_lock).
     * @warning Clears all services. Ensure no threads are actively using services.
     */
    static auto clear_services() -> void {
        std::unique_lock lock(mutex_);
        services_.clear();
    }

    /**
     * @brief Get the number of registered services.
     *
     * @return std::size_t Number of currently registered services
     *
     * @thread_safety Thread-safe. Uses shared lock (std::shared_lock).
     */
    static auto get_service_count() -> std::size_t {
        std::shared_lock lock(mutex_);
        return services_.size();
    }
};

} // namespace kcenon::thread

