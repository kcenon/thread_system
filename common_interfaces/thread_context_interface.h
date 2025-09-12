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
#include <optional>
#include <string>

namespace common_interfaces {

// Forward declarations for optional service interfaces
class interface_logger;
class interface_monitoring;

/**
 * @brief Abstract interface for thread execution context
 * 
 * This interface provides access to optional services that threads may need
 * during execution, such as logging and monitoring capabilities. It enables
 * clean separation of concerns and dependency injection.
 */
class interface_thread_context {
public:
    virtual ~interface_thread_context() = default;

    /**
     * @brief Get the logger service from the context
     * @return Shared pointer to logger interface, or nullptr if not available
     */
    virtual auto get_logger() const -> std::shared_ptr<interface_logger> = 0;

    /**
     * @brief Get the monitoring service from the context
     * @return Shared pointer to monitoring interface, or nullptr if not available
     */
    virtual auto get_monitoring() const -> std::shared_ptr<interface_monitoring> = 0;

    /**
     * @brief Set the logger service for this context
     * @param logger Logger service implementation
     * @return true if logger was set successfully, false otherwise
     */
    virtual auto set_logger(std::shared_ptr<interface_logger> logger) -> bool = 0;

    /**
     * @brief Set the monitoring service for this context
     * @param monitoring Monitoring service implementation
     * @return true if monitoring was set successfully, false otherwise
     */
    virtual auto set_monitoring(std::shared_ptr<interface_monitoring> monitoring) -> bool = 0;

    /**
     * @brief Get the context name/identifier
     * @return Context name as string
     */
    virtual auto get_context_name() const -> std::string = 0;

    /**
     * @brief Set the context name/identifier
     * @param name The context name to set
     * @return true if name was set successfully, false otherwise
     */
    virtual auto set_context_name(const std::string& name) -> bool = 0;

    /**
     * @brief Check if logging is available in this context
     * @return true if logger is available, false otherwise
     */
    virtual auto has_logger() const -> bool = 0;

    /**
     * @brief Check if monitoring is available in this context
     * @return true if monitoring is available, false otherwise
     */
    virtual auto has_monitoring() const -> bool = 0;
};

/**
 * @brief Abstract interface for logger service
 * 
 * This interface provides basic logging functionality that can be injected
 * into thread contexts for logging purposes.
 */
class interface_logger {
public:
    virtual ~interface_logger() = default;

    /**
     * @brief Log an informational message
     * @param message The message to log
     * @return true if message was logged successfully, false otherwise
     */
    virtual auto log_info(const std::string& message) -> bool = 0;

    /**
     * @brief Log an error message
     * @param message The error message to log
     * @return true if message was logged successfully, false otherwise
     */
    virtual auto log_error(const std::string& message) -> bool = 0;

    /**
     * @brief Log a warning message
     * @param message The warning message to log
     * @return true if message was logged successfully, false otherwise
     */
    virtual auto log_warning(const std::string& message) -> bool = 0;

    /**
     * @brief Log a debug message
     * @param message The debug message to log
     * @return true if message was logged successfully, false otherwise
     */
    virtual auto log_debug(const std::string& message) -> bool = 0;
};

/**
 * @brief Abstract interface for monitoring service
 * 
 * This interface provides basic monitoring functionality that can be injected
 * into thread contexts for performance and health monitoring.
 */
class interface_monitoring {
public:
    virtual ~interface_monitoring() = default;

    /**
     * @brief Record a metric value
     * @param name The metric name
     * @param value The metric value
     * @return true if metric was recorded successfully, false otherwise
     */
    virtual auto record_metric(const std::string& name, double value) -> bool = 0;

    /**
     * @brief Increment a counter metric
     * @param name The counter name
     * @param increment The value to increment by (default: 1)
     * @return true if counter was incremented successfully, false otherwise
     */
    virtual auto increment_counter(const std::string& name, std::size_t increment = 1) -> bool = 0;

    /**
     * @brief Record timing information
     * @param name The timer name
     * @param duration_ms Duration in milliseconds
     * @return true if timing was recorded successfully, false otherwise
     */
    virtual auto record_timing(const std::string& name, double duration_ms) -> bool = 0;

    /**
     * @brief Set a gauge value
     * @param name The gauge name
     * @param value The gauge value
     * @return true if gauge was set successfully, false otherwise
     */
    virtual auto set_gauge(const std::string& name, double value) -> bool = 0;
};

} // namespace common_interfaces