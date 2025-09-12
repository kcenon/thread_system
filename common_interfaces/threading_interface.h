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
#include <cstddef>
#include <functional>
#include <thread>

namespace common_interfaces {

/**
 * @brief Abstract interface for thread pool implementations
 * 
 * This interface provides a clean abstraction for thread pool functionality,
 * allowing for dependency injection and implementation swapping without
 * coupling to concrete implementations.
 */
class interface_thread_pool {
public:
    virtual ~interface_thread_pool() = default;

    /**
     * @brief Submit a task to the thread pool
     * @param task The task to be executed
     * @return true if task was successfully submitted, false otherwise
     */
    virtual auto submit_task(std::function<void()> task) -> bool = 0;

    /**
     * @brief Get the number of worker threads in the pool
     * @return Number of active worker threads
     */
    virtual auto get_thread_count() const -> std::size_t = 0;

    /**
     * @brief Shutdown the thread pool
     * @param immediate If true, stop immediately; if false, wait for current tasks to complete
     * @return true if shutdown was successful, false otherwise
     */
    virtual auto shutdown_pool(bool immediate = false) -> bool = 0;

    /**
     * @brief Check if the thread pool is currently running
     * @return true if the pool is active, false otherwise
     */
    virtual auto is_running() const -> bool = 0;

    /**
     * @brief Get the number of pending tasks in the queue
     * @return Number of tasks waiting to be processed
     */
    virtual auto get_pending_task_count() const -> std::size_t = 0;
};

/**
 * @brief Abstract interface for individual thread implementations
 * 
 * This interface abstracts thread lifecycle management and provides
 * a consistent API for thread control operations.
 */
class interface_thread {
public:
    virtual ~interface_thread() = default;

    /**
     * @brief Start the thread execution
     * @return true if thread started successfully, false otherwise
     */
    virtual auto start_thread() -> bool = 0;

    /**
     * @brief Stop the thread execution
     * @param immediate If true, stop immediately; if false, allow graceful shutdown
     * @return true if thread stopped successfully, false otherwise
     */
    virtual auto stop_thread(bool immediate = false) -> bool = 0;

    /**
     * @brief Check if the thread is currently running
     * @return true if thread is active, false otherwise
     */
    virtual auto is_thread_running() const -> bool = 0;

    /**
     * @brief Get the thread identifier
     * @return Thread ID or invalid ID if thread is not running
     */
    virtual auto get_thread_id() const -> std::thread::id = 0;

    /**
     * @brief Wait for the thread to complete execution
     * @return true if thread completed successfully, false if timeout or error
     */
    virtual auto join_thread() -> bool = 0;
};

} // namespace common_interfaces