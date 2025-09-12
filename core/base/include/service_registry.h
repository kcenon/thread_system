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
#include "../../../common_interfaces/service_container_interface.h"

namespace thread_module {

/**
 * @brief Lightweight service registry for dependency lookup.
 */
class service_registry : public common_interfaces::interface_service_container {
private:
    static inline std::unordered_map<std::type_index, std::any> services_{};
    static inline std::shared_mutex mutex_{};

public:
    template <typename Interface>
    static auto register_service(std::shared_ptr<Interface> service) -> void {
        std::unique_lock lock(mutex_);
        services_[std::type_index(typeid(Interface))] = std::move(service);
    }

    template <typename Interface>
    static auto get_service() -> std::shared_ptr<Interface> {
        std::shared_lock lock(mutex_);
        auto it = services_.find(std::type_index(typeid(Interface)));
        if (it != services_.end()) {
            return std::any_cast<std::shared_ptr<Interface>>(it->second);
        }
        return nullptr;
    }

protected:
    // interface_service_container implementation
    auto register_service_impl(const std::type_info& type_info, std::shared_ptr<void> service) -> bool override {
        try {
            std::unique_lock lock(mutex_);
            services_[std::type_index(type_info)] = service;
            return true;
        } catch (...) {
            return false;
        }
    }

    auto resolve_service_impl(const std::type_info& type_info) -> std::shared_ptr<void> override {
        try {
            std::shared_lock lock(mutex_);
            auto it = services_.find(std::type_index(type_info));
            if (it != services_.end()) {
                return std::any_cast<std::shared_ptr<void>>(it->second);
            }
        } catch (...) {
            // Return nullptr on any error
        }
        return nullptr;
    }

    auto contains_service_impl(const std::type_info& type_info) const -> bool override {
        std::shared_lock lock(mutex_);
        return services_.find(std::type_index(type_info)) != services_.end();
    }

public:
    auto clear_services() -> bool override {
        try {
            std::unique_lock lock(mutex_);
            services_.clear();
            return true;
        } catch (...) {
            return false;
        }
    }

    auto get_service_count() const -> std::size_t override {
        std::shared_lock lock(mutex_);
        return services_.size();
    }
};

} // namespace thread_module

