#pragma once

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

#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

namespace monitoring_module {

    /**
     * @class ring_buffer
     * @brief Lock-free ring buffer for single producer, single consumer scenarios.
     * 
     * @tparam T The type of elements stored in the buffer
     * 
     * @ingroup monitoring
     * 
     * Implements a high-performance circular buffer using atomic operations
     * for lock-free synchronization between a single producer and single
     * consumer thread. Ideal for real-time metrics collection where low
     * latency is critical.
     * 
     * ### Design Features
     * - Lock-free implementation using memory ordering constraints
     * - Fixed capacity with O(1) push/pop operations
     * - Cache-friendly memory layout
     * - No dynamic memory allocation after construction
     * 
     * ### Thread Safety
     * Safe for concurrent use by exactly one producer thread and one
     * consumer thread. Multiple producers or consumers require external
     * synchronization or use of thread_safe_ring_buffer.
     * 
     * ### Example
     * @code
     * ring_buffer<metrics_snapshot> buffer(1000);
     * 
     * // Producer thread
     * if (!buffer.push(snapshot)) {
     *     // Buffer full, handle overflow
     * }
     * 
     * // Consumer thread
     * metrics_snapshot item;
     * if (buffer.pop(item)) {
     *     // Process item
     * }
     * @endcode
     * 
     * @see thread_safe_ring_buffer For multi-producer scenarios
     */
    template<typename T>
    class ring_buffer {
    public:
        /**
         * @brief Constructs a ring buffer with specified capacity.
         * @param capacity Maximum number of elements the buffer can hold
         * 
         * The implementation reserves one extra slot internally to
         * distinguish between full and empty states without additional
         * flags.
         */
        explicit ring_buffer(std::size_t capacity)
            : capacity_(capacity + 1)  // Reserve one empty slot (for full/empty distinction)
            , buffer_(capacity_)
            , head_(0)
            , tail_(0) {
        }

        /**
         * @brief Adds an element to the buffer (producer operation).
         * @param item The element to add
         * @return true if successful, false if buffer is full
         * 
         * This method is wait-free and designed to be called by the
         * producer thread only. Uses release memory ordering to ensure
         * the item is visible to the consumer.
         */
        auto push(const T& item) -> bool {
            const auto current_tail = tail_.load(std::memory_order_relaxed);
            const auto next_tail = (current_tail + 1) % capacity_;

            if (next_tail == head_.load(std::memory_order_acquire)) {
                return false; // Buffer is full
            }

            buffer_[current_tail] = item;
            tail_.store(next_tail, std::memory_order_release);
            return true;
        }

        /**
         * @brief Removes an element from the buffer (consumer operation).
         * @param item Output parameter to receive the removed element
         * @return true if an element was removed, false if buffer is empty
         * 
         * This method is wait-free and designed to be called by the
         * consumer thread only. Uses acquire memory ordering to ensure
         * visibility of producer writes.
         */
        auto pop(T& item) -> bool {
            const auto current_head = head_.load(std::memory_order_relaxed);

            if (current_head == tail_.load(std::memory_order_acquire)) {
                return false; // Buffer is empty
            }

            item = buffer_[current_head];
            head_.store((current_head + 1) % capacity_, std::memory_order_release);
            return true;
        }

        /**
         * @brief Returns the current number of elements in the buffer.
         * @return Number of elements currently stored
         * 
         * @note The returned value is approximate in concurrent scenarios
         * and should be used for monitoring/debugging purposes only.
         */
        auto size() const -> std::size_t {
            const auto current_tail = tail_.load(std::memory_order_acquire);
            const auto current_head = head_.load(std::memory_order_acquire);

            if (current_tail >= current_head) {
                return current_tail - current_head;
            } else {
                return capacity_ - current_head + current_tail;
            }
        }

        /**
         * @brief Checks if the buffer is empty.
         * @return true if buffer contains no elements
         */
        auto empty() const -> bool {
            return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
        }

        /**
         * @brief Checks if the buffer is full.
         * @return true if buffer cannot accept more elements
         */
        auto full() const -> bool {
            const auto current_tail = tail_.load(std::memory_order_acquire);
            const auto next_tail = (current_tail + 1) % capacity_;
            return next_tail == head_.load(std::memory_order_acquire);
        }

        /**
         * @brief Returns the maximum capacity of the buffer.
         * @return Maximum number of elements the buffer can hold
         */
        auto capacity() const -> std::size_t {
            return capacity_ - 1;  // Actual usable size
        }

        /**
         * @brief Retrieves the most recent N items from the buffer.
         * @param count Number of recent items to retrieve
         * @return Vector containing up to count most recent items
         * 
         * Returns items in chronological order (oldest to newest).
         * If fewer than count items are available, returns all items.
         * This method provides a consistent snapshot but may miss
         * concurrent updates.
         */
        auto get_recent_items(std::size_t count) const -> std::vector<T> {
            std::vector<T> result;
            result.reserve(count);

            const auto current_tail = tail_.load(std::memory_order_acquire);
            const auto current_size = size();

            if (current_size == 0) {
                return result;
            }

            const auto items_to_copy = std::min(count, current_size);
            auto start_index = current_tail;

            // Get items_to_copy items from the back
            if (items_to_copy <= current_tail) {
                start_index = current_tail - items_to_copy;
            } else {
                start_index = capacity_ - (items_to_copy - current_tail);
            }

            for (std::size_t i = 0; i < items_to_copy; ++i) {
                const auto index = (start_index + i) % capacity_;
                result.push_back(buffer_[index]);
            }

            return result;
        }

    private:
        const std::size_t capacity_;         ///< Total buffer capacity (including empty slot)
        std::vector<T> buffer_;              ///< Underlying storage
        std::atomic<std::size_t> head_;      ///< Consumer position (next item to read)
        std::atomic<std::size_t> tail_;      ///< Producer position (next item to write)
    };

    /**
     * @class thread_safe_ring_buffer
     * @brief Thread-safe ring buffer supporting multiple producers and consumers.
     * 
     * @tparam T The type of elements stored in the buffer
     * 
     * @ingroup monitoring
     * 
     * Provides a circular buffer implementation with full mutex-based
     * synchronization, supporting any number of producer and consumer
     * threads. Trades some performance for flexibility compared to
     * the lock-free ring_buffer.
     * 
     * ### Design Features
     * - Full thread safety with mutex protection
     * - Support for multiple producers and consumers
     * - Fixed capacity with O(1) operations
     * - Bulk retrieval operations
     * 
     * ### Thread Safety
     * All methods are fully thread-safe and can be called concurrently
     * from any number of threads.
     * 
     * ### Performance Considerations
     * - Higher latency than lock-free ring_buffer due to mutex overhead
     * - May experience contention under high concurrency
     * - Consider using multiple buffers for heavily concurrent scenarios
     * 
     * @see ring_buffer For single-producer single-consumer scenarios
     */
    template<typename T>
    class thread_safe_ring_buffer {
    public:
        /**
         * @brief Constructs a thread-safe ring buffer with specified capacity.
         * @param capacity Maximum number of elements the buffer can hold
         */
        explicit thread_safe_ring_buffer(std::size_t capacity)
            : capacity_(capacity)
            , buffer_(capacity)
            , head_(0)
            , tail_(0) {
        }

        /**
         * @brief Adds an element to the buffer.
         * @param item The element to add
         * @return true if successful, false if buffer is full
         * 
         * Thread-safe method that can be called from any thread.
         */
        auto push(const T& item) -> bool {
            std::lock_guard<std::mutex> lock(mutex_);

            if ((tail_ + 1) % capacity_ == head_) {
                return false; // Buffer is full
            }

            buffer_[tail_] = item;
            tail_ = (tail_ + 1) % capacity_;
            return true;
        }

        /**
         * @brief Removes an element from the buffer.
         * @param item Output parameter to receive the removed element
         * @return true if an element was removed, false if buffer is empty
         * 
         * Thread-safe method that can be called from any thread.
         */
        auto pop(T& item) -> bool {
            std::lock_guard<std::mutex> lock(mutex_);

            if (head_ == tail_) {
                return false; // Buffer is empty
            }

            item = buffer_[head_];
            head_ = (head_ + 1) % capacity_;
            return true;
        }

        /**
         * @brief Returns the current number of elements in the buffer.
         * @return Number of elements currently stored
         * 
         * Provides an exact count under mutex protection.
         */
        auto size() const -> std::size_t {
            std::lock_guard<std::mutex> lock(mutex_);
            
            if (tail_ >= head_) {
                return tail_ - head_;
            } else {
                return capacity_ - head_ + tail_;
            }
        }

        /**
         * @brief Checks if the buffer is empty.
         * @return true if buffer contains no elements
         */
        auto empty() const -> bool {
            std::lock_guard<std::mutex> lock(mutex_);
            return head_ == tail_;
        }

        /**
         * @brief Retrieves all items currently in the buffer.
         * @return Vector containing all buffered items in FIFO order
         * 
         * Creates a snapshot of all current buffer contents. The buffer
         * remains unchanged after this operation. Useful for periodic
         * bulk processing or debugging.
         */
        auto get_all_items() const -> std::vector<T> {
            std::lock_guard<std::mutex> lock(mutex_);
            std::vector<T> result;

            if (head_ == tail_) {
                return result;
            }

            // Calculate size inline to avoid recursive mutex lock
            std::size_t current_size;
            if (tail_ >= head_) {
                current_size = tail_ - head_;
            } else {
                current_size = capacity_ - head_ + tail_;
            }
            
            result.reserve(current_size);

            if (tail_ > head_) {
                for (std::size_t i = head_; i < tail_; ++i) {
                    result.push_back(buffer_[i]);
                }
            } else {
                for (std::size_t i = head_; i < capacity_; ++i) {
                    result.push_back(buffer_[i]);
                }
                for (std::size_t i = 0; i < tail_; ++i) {
                    result.push_back(buffer_[i]);
                }
            }

            return result;
        }

    private:
        const std::size_t capacity_;    ///< Maximum buffer capacity
        std::vector<T> buffer_;         ///< Underlying storage
        std::size_t head_;              ///< Consumer position
        std::size_t tail_;              ///< Producer position
        mutable std::mutex mutex_;      ///< Mutex for thread synchronization
    };

} // namespace monitoring_module