#pragma once

/*
 * BSD 3-Clause License
 * 
 * Copyright (c) 2024, DongCheol Shin
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file base_writer.h
 * @brief Abstract base class for all log writers
 * 
 * This file defines the interface that all log writers must implement,
 * providing a clean abstraction for different output destinations.
 */

#include "../detail/forward_declarations.h"
#include "../types/log_types.h"
#include "../core/config.h"
#include "../../thread_base/sync/error_handling.h"
#include <string>
#include <memory>

namespace log_module {
    
    using namespace thread_module; // For result<T> types
    
    /**
     * @brief Abstract base class for all log writers
     * 
     * This class defines the interface that all log writers must implement.
     * It provides common functionality and ensures consistent behavior
     * across all writer implementations.
     */
    class base_writer {
    public:
        /**
         * @brief Virtual destructor
         */
        virtual ~base_writer() = default;
        
        /**
         * @brief Write a log message
         * 
         * @param level The log level of the message
         * @param message The formatted message to write
         * @return Result indicating success or failure
         */
        [[nodiscard]] virtual result_void write(log_types level, const std::string& message) = 0;
        
        /**
         * @brief Flush any buffered output
         * 
         * @return Result indicating success or failure
         */
        [[nodiscard]] virtual result_void flush() = 0;
        
        /**
         * @brief Close the writer and release resources
         * 
         * @return Result indicating success or failure
         */
        [[nodiscard]] virtual result_void close() = 0;
        
        /**
         * @brief Check if the writer is currently open and ready
         * 
         * @return true if the writer is ready to accept messages
         */
        [[nodiscard]] virtual bool is_open() const = 0;
        
        /**
         * @brief Get the minimum log level this writer accepts
         * 
         * @return The minimum log level
         */
        [[nodiscard]] virtual log_types get_min_level() const noexcept {
            return min_level_;
        }
        
        /**
         * @brief Set the minimum log level this writer accepts
         * 
         * @param level The new minimum log level
         */
        virtual void set_min_level(log_types level) noexcept {
            min_level_ = level;
        }
        
        /**
         * @brief Check if a log level should be written by this writer
         * 
         * @param level The log level to check
         * @return true if the level should be written
         */
        [[nodiscard]] virtual bool should_write(log_types level) const noexcept {
            return level >= min_level_;
        }
        
        /**
         * @brief Get the writer's configuration
         * 
         * @return Reference to the writer configuration
         */
        [[nodiscard]] virtual const config::writer_config& get_config() const noexcept {
            return config_;
        }
        
        /**
         * @brief Update the writer's configuration
         * 
         * @param new_config The new configuration
         * @return Result indicating success or failure
         */
        [[nodiscard]] virtual result_void update_config(const config::writer_config& new_config) {
            config_ = new_config;
            min_level_ = new_config.min_level;
            return {};
        }
        
        /**
         * @brief Get writer statistics
         * 
         * @return Statistics about the writer's operation
         */
        struct writer_stats {
            size_t messages_written = 0;
            size_t bytes_written = 0;
            size_t flush_count = 0;
            size_t error_count = 0;
            std::chrono::steady_clock::time_point last_write_time;
        };
        
        [[nodiscard]] virtual writer_stats get_stats() const noexcept {
            return stats_;
        }
        
        /**
         * @brief Reset writer statistics
         */
        virtual void reset_stats() noexcept {
            stats_ = writer_stats{};
        }
        
    protected:
        /**
         * @brief Constructor for derived classes
         * 
         * @param config Initial configuration for the writer
         */
        explicit base_writer(const config::writer_config& config = {})
            : config_(config), min_level_(config.min_level) {}
        
        /**
         * @brief Update statistics after writing a message
         * 
         * @param message_size Size of the written message
         * @param success Whether the write was successful
         */
        void update_stats(size_t message_size, bool success = true) noexcept {
            if (success) {
                ++stats_.messages_written;
                stats_.bytes_written += message_size;
            } else {
                ++stats_.error_count;
            }
            stats_.last_write_time = std::chrono::steady_clock::now();
        }
        
        /**
         * @brief Update flush statistics
         */
        void update_flush_stats() noexcept {
            ++stats_.flush_count;
        }
        
        /**
         * @brief Format a log message with timestamp and level
         * 
         * @param level The log level
         * @param message The raw message
         * @return Formatted message string
         */
        [[nodiscard]] virtual std::string format_message(log_types level, const std::string& message) const;
        
    private:
        config::writer_config config_;
        log_types min_level_ = config::default_log_level;
        mutable writer_stats stats_;
    };
    
    /**
     * @brief Shared pointer type for writers
     */
    using writer_ptr = std::shared_ptr<base_writer>;
    
    /**
     * @brief Weak pointer type for writers
     */
    using writer_weak_ptr = std::weak_ptr<base_writer>;
    
} // namespace log_module