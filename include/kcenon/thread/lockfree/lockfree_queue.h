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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <type_traits>

namespace kcenon::thread {

/**
 * @class concurrent_queue
 * @brief Thread-safe MPMC queue with blocking wait support
 *
 * This class implements a thread-safe Multi-Producer Multi-Consumer (MPMC) queue
 * using fine-grained locking for simplicity and correctness. It provides both
 * non-blocking and blocking operations for flexible use cases.
 *
 * ## Implementation Notes
 *
 * This implementation uses fine-grained locking with separate head and tail
 * mutexes rather than a true lock-free algorithm. This provides:
 * - Correctness guarantee without complex memory reclamation
 * - Good performance for most use cases
 * - Blocking wait support with condition variables
 *
 * While a true lock-free Michael-Scott queue requires complex memory reclamation
 * (Hazard Pointers or epoch-based reclamation), fine-grained locking offers
 * comparable performance for most practical scenarios.
 *
 * ## Key Features
 *
 * - **Thread Safety**: Safe for concurrent access from multiple threads
 * - **Blocking Wait**: wait_dequeue provides efficient blocking with timeout
 * - **Generic**: Works with any move-constructible type
 * - **Low Contention**: Separate locks for head and tail operations
 *
 * ## Performance Characteristics
 *
 * - Enqueue: O(1), acquires tail lock only
 * - Dequeue: O(1), acquires head lock only
 * - No lock contention between enqueue and dequeue operations
 *
 * ## Example Usage
 *
 * @code
 * concurrent_queue<std::string> queue;
 *
 * // Producer thread
 * queue.enqueue("message");
 *
 * // Consumer thread (non-blocking)
 * if (auto value = queue.try_dequeue()) {
 *     process(*value);
 * }
 *
 * // Consumer thread (blocking with timeout)
 * if (auto value = queue.wait_dequeue(std::chrono::milliseconds{100})) {
 *     process(*value);
 * }
 * @endcode
 *
 * @tparam T The element type (must be move-constructible)
 */
template <typename T>
class concurrent_queue {
    static_assert(std::is_move_constructible_v<T>, "T must be move constructible");

public:
    /**
     * @brief Constructs an empty queue
     *
     * Initializes the queue with a dummy node to simplify the algorithm.
     */
    concurrent_queue() {
        auto* dummy = new node{};
        head_ = dummy;
        tail_ = dummy;
    }

    /**
     * @brief Destructor - signals shutdown and drains the queue
     */
    ~concurrent_queue() {
        shutdown();
        // Drain remaining items
        while (try_dequeue()) {
        }
        // Delete the dummy node
        delete head_;
    }

    // Non-copyable and non-movable
    concurrent_queue(const concurrent_queue&) = delete;
    concurrent_queue& operator=(const concurrent_queue&) = delete;
    concurrent_queue(concurrent_queue&&) = delete;
    concurrent_queue& operator=(concurrent_queue&&) = delete;

    /**
     * @brief Enqueues a value into the queue
     *
     * @param value The value to enqueue (moved)
     *
     * @note Thread-safe, uses tail lock only
     */
    void enqueue(T value) {
        if (shutdown_.load(std::memory_order_acquire)) {
            return;
        }

        auto* new_node = new node{std::move(value)};

        {
            std::lock_guard<std::mutex> lock(tail_mutex_);
            tail_->next = new_node;
            tail_ = new_node;
        }

        size_.fetch_add(1, std::memory_order_release);
        notify_one();
    }

    /**
     * @brief Tries to dequeue a value (non-blocking)
     *
     * @return The dequeued value, or nullopt if queue is empty
     *
     * @note Thread-safe, uses head lock only
     */
    [[nodiscard]] auto try_dequeue() -> std::optional<T> {
        std::lock_guard<std::mutex> lock(head_mutex_);

        node* old_head = head_;
        node* next = old_head->next;

        if (next == nullptr) {
            return std::nullopt;  // Queue is empty
        }

        // Extract value from next node
        std::optional<T> value;
        if (next->data.has_value()) {
            value = std::move(*next->data);
            next->data.reset();
        }

        // Advance head (old_head becomes the new dummy)
        head_ = next;
        delete old_head;

        size_.fetch_sub(1, std::memory_order_release);
        return value;
    }

    /**
     * @brief Dequeues a value with blocking wait
     *
     * @param timeout Maximum time to wait
     * @return The dequeued value, or nullopt if timeout or shutdown
     */
    template <typename Rep, typename Period>
    [[nodiscard]] auto wait_dequeue(const std::chrono::duration<Rep, Period>& timeout)
        -> std::optional<T> {
        // First try without waiting
        if (auto value = try_dequeue()) {
            return value;
        }

        // Wait for notification
        std::unique_lock<std::mutex> lock(cv_mutex_);
        auto deadline = std::chrono::steady_clock::now() + timeout;

        while (!shutdown_.load(std::memory_order_acquire)) {
            // Try to dequeue
            lock.unlock();
            if (auto value = try_dequeue()) {
                return value;
            }
            lock.lock();

            // Check if we should stop waiting
            if (std::chrono::steady_clock::now() >= deadline) {
                return std::nullopt;
            }

            // Wait for notification or timeout
            cv_.wait_until(lock, deadline);
        }

        // Final attempt after shutdown
        return try_dequeue();
    }

    /**
     * @brief Dequeues a value with indefinite blocking wait
     *
     * @return The dequeued value, or nullopt if shutdown
     */
    [[nodiscard]] auto wait_dequeue() -> std::optional<T> {
        return wait_dequeue(std::chrono::hours{24 * 365});
    }

    /**
     * @brief Checks if the queue appears empty
     */
    [[nodiscard]] auto empty() const noexcept -> bool {
        return size_.load(std::memory_order_acquire) == 0;
    }

    /**
     * @brief Gets approximate queue size
     */
    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return size_.load(std::memory_order_acquire);
    }

    /**
     * @brief Wakes one waiting consumer
     */
    void notify_one() {
        std::lock_guard<std::mutex> lock(cv_mutex_);
        cv_.notify_one();
    }

    /**
     * @brief Wakes all waiting consumers
     */
    void notify_all() {
        std::lock_guard<std::mutex> lock(cv_mutex_);
        cv_.notify_all();
    }

    /**
     * @brief Signals shutdown and wakes all waiters
     */
    void shutdown() {
        shutdown_.store(true, std::memory_order_release);
        notify_all();
    }

    /**
     * @brief Checks if shutdown has been signaled
     */
    [[nodiscard]] auto is_shutdown() const noexcept -> bool {
        return shutdown_.load(std::memory_order_acquire);
    }

private:
    struct node {
        std::optional<T> data;
        node* next{nullptr};

        node() = default;
        explicit node(T value) : data(std::move(value)) {}
    };

    node* head_;
    node* tail_;

    mutable std::mutex head_mutex_;
    mutable std::mutex tail_mutex_;

    std::atomic<std::size_t> size_{0};
    std::atomic<bool> shutdown_{false};

    // For blocking wait support
    mutable std::mutex cv_mutex_;
    std::condition_variable cv_;
};

/**
 * @brief Backward compatibility alias for concurrent_queue
 *
 * @deprecated Use concurrent_queue instead. This alias will be removed in a future version.
 *
 * The name "lockfree_queue" is misleading as the implementation uses fine-grained
 * locking rather than true lock-free algorithms. The new name "concurrent_queue"
 * more accurately describes the thread-safe, concurrent nature of this container.
 *
 * @tparam T The element type (must be move-constructible)
 */
template <typename T>
using lockfree_queue [[deprecated("Use concurrent_queue instead. "
    "This class uses fine-grained locking, not lock-free algorithms.")]] = concurrent_queue<T>;

}  // namespace kcenon::thread
