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
 * @file pool_factory.h
 * @brief Factory for creating common thread pool configurations
 * 
 * This file provides factory methods for creating thread pools
 * with commonly used configurations and optimizations.
 */

#include "../detail/forward_declarations.h"
#include "../core/config.h"
#include "../workers/worker_policy.h"
#include <memory>
#include <string>

namespace thread_pool_module {
    
    // Forward declare thread_pool for factory methods
    class thread_pool;
    
    /**
     * @brief Factory class for creating thread pools with predefined configurations
     * 
     * This class provides static methods to create thread pools optimized
     * for specific use cases and workload patterns.
     */
    class pool_factory {
    public:
        /**
         * @brief Create a general-purpose thread pool
         * 
         * @param thread_count Number of worker threads (default: hardware concurrency)
         * @param name Optional name for the pool
         * @return Shared pointer to the created thread pool
         */
        static std::shared_ptr<thread_pool> create_general_purpose(
            size_t thread_count = 0, 
            const std::string& name = config::default_pool_name
        );
        
        /**
         * @brief Create a high-throughput thread pool
         * 
         * Optimized for maximum throughput with work stealing enabled
         * and aggressive scheduling policies.
         * 
         * @param thread_count Number of worker threads
         * @param name Optional name for the pool
         * @return Shared pointer to the created thread pool
         */
        static std::shared_ptr<thread_pool> create_high_throughput(
            size_t thread_count = 0,
            const std::string& name = "high_throughput_pool"
        );
        
        /**
         * @brief Create a low-latency thread pool
         * 
         * Optimized for minimal latency with priority scheduling
         * and reduced idle times.
         * 
         * @param thread_count Number of worker threads
         * @param name Optional name for the pool
         * @return Shared pointer to the created thread pool
         */
        static std::shared_ptr<thread_pool> create_low_latency(
            size_t thread_count = 0,
            const std::string& name = "low_latency_pool"
        );
        
        /**
         * @brief Create a power-efficient thread pool
         * 
         * Optimized for power efficiency with longer idle times
         * and CPU yielding when not busy.
         * 
         * @param thread_count Number of worker threads
         * @param name Optional name for the pool
         * @return Shared pointer to the created thread pool
         */
        static std::shared_ptr<thread_pool> create_power_efficient(
            size_t thread_count = 0,
            const std::string& name = "power_efficient_pool"
        );
        
        /**
         * @brief Create a single-threaded pool
         * 
         * Useful for sequential execution while maintaining
         * the thread pool interface.
         * 
         * @param name Optional name for the pool
         * @return Shared pointer to the created thread pool
         */
        static std::shared_ptr<thread_pool> create_single_threaded(
            const std::string& name = "single_threaded_pool"
        );
        
        /**
         * @brief Create a compute-intensive thread pool
         * 
         * Optimized for CPU-bound tasks with thread pinning
         * and minimal context switching.
         * 
         * @param thread_count Number of worker threads (default: CPU cores)
         * @param name Optional name for the pool
         * @return Shared pointer to the created thread pool
         */
        static std::shared_ptr<thread_pool> create_compute_intensive(
            size_t thread_count = 0,
            const std::string& name = "compute_pool"
        );
        
        /**
         * @brief Create an I/O bound thread pool
         * 
         * Optimized for I/O-bound tasks with higher thread counts
         * and longer timeouts.
         * 
         * @param thread_count Number of worker threads (default: 2x CPU cores)
         * @param name Optional name for the pool
         * @return Shared pointer to the created thread pool
         */
        static std::shared_ptr<thread_pool> create_io_bound(
            size_t thread_count = 0,
            const std::string& name = "io_pool"
        );
        
        /**
         * @brief Create a background task thread pool
         * 
         * Optimized for background processing with lower priority
         * and power-efficient settings.
         * 
         * @param thread_count Number of worker threads
         * @param name Optional name for the pool
         * @return Shared pointer to the created thread pool
         */
        static std::shared_ptr<thread_pool> create_background_tasks(
            size_t thread_count = 2,
            const std::string& name = "background_pool"
        );
        
    private:
        /**
         * @brief Get the default thread count based on hardware
         * 
         * @param multiplier Multiplier for hardware concurrency
         * @return Appropriate thread count
         */
        static size_t get_default_thread_count(double multiplier = 1.0);
        
        /**
         * @brief Create a thread pool with the specified policy
         * 
         * @param thread_count Number of threads
         * @param policy Worker policy to use
         * @param name Pool name
         * @return Shared pointer to the created pool
         */
        static std::shared_ptr<thread_pool> create_with_policy(
            size_t thread_count,
            const worker_policy& policy,
            const std::string& name
        );
    };
    
    /**
     * @brief Namespace for pool presets and common configurations
     */
    namespace presets {
        
        /**
         * @brief Get a worker policy optimized for web servers
         */
        worker_policy web_server_policy();
        
        /**
         * @brief Get a worker policy optimized for batch processing
         */
        worker_policy batch_processing_policy();
        
        /**
         * @brief Get a worker policy optimized for real-time systems
         */
        worker_policy realtime_policy();
        
        /**
         * @brief Get a worker policy optimized for scientific computing
         */
        worker_policy scientific_computing_policy();
        
    } // namespace presets
    
} // namespace thread_pool_module