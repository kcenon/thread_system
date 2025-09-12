/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
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

#pragma once

#include <memory>
#include <typeinfo>

namespace common_interfaces {

/**
 * @brief Abstract interface for dependency injection container
 * 
 * This interface provides a clean abstraction for service registration and
 * resolution, enabling loose coupling between components and facilitating
 * testing through dependency injection.
 */
class interface_service_container {
public:
    virtual ~interface_service_container() = default;

    /**
     * @brief Register a service implementation with the container
     * @tparam T The interface type to register
     * @param service The service implementation instance
     * @return true if service was registered successfully, false otherwise
     */
    template<typename T>
    auto register_service(std::shared_ptr<T> service) -> bool {
        return register_service_impl(typeid(T), std::static_pointer_cast<void>(service));
    }

    /**
     * @brief Resolve a service instance from the container
     * @tparam T The interface type to resolve
     * @return Shared pointer to the service instance, or nullptr if not found
     */
    template<typename T>
    auto resolve_service() -> std::shared_ptr<T> {
        auto service = resolve_service_impl(typeid(T));
        return std::static_pointer_cast<T>(service);
    }

    /**
     * @brief Check if a service is registered in the container
     * @tparam T The interface type to check
     * @return true if service is available, false otherwise
     */
    template<typename T>
    auto contains_service() const -> bool {
        return contains_service_impl(typeid(T));
    }

    /**
     * @brief Clear all registered services from the container
     * @return true if all services were cleared successfully, false otherwise
     */
    virtual auto clear_services() -> bool = 0;

    /**
     * @brief Get the number of registered services
     * @return Number of registered services in the container
     */
    virtual auto get_service_count() const -> std::size_t = 0;

protected:
    /**
     * @brief Internal implementation for service registration
     * @param type_info Type information for the service interface
     * @param service Void pointer to the service instance
     * @return true if registration was successful, false otherwise
     */
    virtual auto register_service_impl(const std::type_info& type_info, std::shared_ptr<void> service) -> bool = 0;

    /**
     * @brief Internal implementation for service resolution
     * @param type_info Type information for the service interface
     * @return Void pointer to the service instance, or nullptr if not found
     */
    virtual auto resolve_service_impl(const std::type_info& type_info) -> std::shared_ptr<void> = 0;

    /**
     * @brief Internal implementation for service existence check
     * @param type_info Type information for the service interface
     * @return true if service exists, false otherwise
     */
    virtual auto contains_service_impl(const std::type_info& type_info) const -> bool = 0;
};

} // namespace common_interfaces