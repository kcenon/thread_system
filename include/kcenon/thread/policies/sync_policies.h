/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, DongCheol Shin
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
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>

#include <kcenon/thread/core/error_handling.h>
#include <kcenon/thread/core/job.h>
#include <kcenon/thread/core/safe_hazard_pointer.h>
#include <kcenon/thread/interfaces/queue_capabilities.h>

namespace kcenon::thread::policies {

/**
 * @brief Tag type for sync policy identification
 */
struct sync_policy_tag {};

/**
 * @class mutex_sync_policy
 * @brief Synchronization policy using mutex and condition variable
 *
 * Provides exact size/empty operations with full blocking support.
 * Suitable for scenarios requiring accurate queue state or blocking waits.
 *
 * ### Thread Safety
 * - All operations are thread-safe using mutex protection
 * - Supports blocking dequeue with condition variable
 *
 * ### Performance Characteristics
 * - Enqueue: O(1), may block on contention
 * - Dequeue: O(1), may block on contention or empty queue
 */
class mutex_sync_policy {
public:
    using policy_tag = sync_policy_tag;

    /**
     * @brief Queue capabilities for mutex sync policy
     */
    [[nodiscard]] static constexpr auto get_capabilities() -> queue_capabilities {
        return queue_capabilities{
            .exact_size = true,
            .atomic_empty_check = true,
            .lock_free = false,
            .wait_free = false,
            .supports_batch = true,
            .supports_blocking_wait = true,
            .supports_stop = true
        };
    }

    /**
     * @brief Construct mutex sync policy
     */
    mutex_sync_policy() : notify_(true), stop_(false) {}

    /**
     * @brief Destructor
     */
    ~mutex_sync_policy() = default;

    // Non-copyable, non-movable
    mutex_sync_policy(const mutex_sync_policy&) = delete;
    mutex_sync_policy& operator=(const mutex_sync_policy&) = delete;
    mutex_sync_policy(mutex_sync_policy&&) = delete;
    mutex_sync_policy& operator=(mutex_sync_policy&&) = delete;

    /**
     * @brief Enqueue a job
     * @param value Job to enqueue
     * @return VoidResult indicating success or error
     */
    [[nodiscard]] auto enqueue(std::unique_ptr<job>&& value) -> common::VoidResult {
        if (!value) {
            return common::error_info{-105, "cannot enqueue null job", "thread_system"};
        }

        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(std::move(value));

        if (notify_.load(std::memory_order_relaxed)) {
            condition_.notify_one();
        }

        return common::ok();
    }

    /**
     * @brief Dequeue a job (blocking)
     * @return Result containing the job or error
     */
    [[nodiscard]] auto dequeue() -> common::Result<std::unique_ptr<job>> {
        std::unique_lock<std::mutex> lock(mutex_);

        condition_.wait(lock, [this] {
            return !queue_.empty() || stop_.load(std::memory_order_relaxed);
        });

        if (queue_.empty()) {
            return common::error_info{-121, "queue is stopped or empty", "thread_system"};
        }

        auto value = std::move(queue_.front());
        queue_.pop_front();
        return value;
    }

    /**
     * @brief Try to dequeue a job (non-blocking)
     * @return Result containing the job or error
     */
    [[nodiscard]] auto try_dequeue() -> common::Result<std::unique_ptr<job>> {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.empty()) {
            return common::error_info{-121, "queue is empty", "thread_system"};
        }

        auto value = std::move(queue_.front());
        queue_.pop_front();
        return value;
    }

    /**
     * @brief Check if queue is empty
     * @return true if empty, false otherwise
     */
    [[nodiscard]] auto empty() const -> bool {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /**
     * @brief Get queue size (exact)
     * @return Number of jobs in queue
     */
    [[nodiscard]] auto size() const -> std::size_t {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    /**
     * @brief Clear all jobs from queue
     */
    auto clear() -> void {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.clear();
    }

    /**
     * @brief Stop the queue
     */
    auto stop() -> void {
        stop_.store(true, std::memory_order_release);
        condition_.notify_all();
    }

    /**
     * @brief Check if queue is stopped
     * @return true if stopped, false otherwise
     */
    [[nodiscard]] auto is_stopped() const -> bool {
        return stop_.load(std::memory_order_acquire);
    }

    /**
     * @brief Set notify flag
     * @param notify Whether to notify on enqueue
     */
    auto set_notify(bool notify) -> void {
        notify_.store(notify, std::memory_order_relaxed);
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<std::unique_ptr<job>> queue_;
    std::atomic<bool> notify_;
    std::atomic<bool> stop_;
};

/**
 * @class lockfree_sync_policy
 * @brief Lock-free synchronization policy using Michael-Scott algorithm
 *
 * Provides high-throughput operations without locking.
 * Size and empty checks are approximate.
 *
 * ### Thread Safety
 * - All operations are thread-safe using atomic operations
 * - No blocking - uses spin-wait pattern
 *
 * ### Performance Characteristics
 * - Enqueue: O(1), wait-free
 * - Dequeue: O(1), lock-free
 */
class lockfree_sync_policy {
public:
    using policy_tag = sync_policy_tag;

    /**
     * @brief Queue capabilities for lock-free sync policy
     */
    [[nodiscard]] static constexpr auto get_capabilities() -> queue_capabilities {
        return queue_capabilities{
            .exact_size = false,
            .atomic_empty_check = false,
            .lock_free = true,
            .wait_free = false,
            .supports_batch = false,
            .supports_blocking_wait = false,
            .supports_stop = false
        };
    }

    /**
     * @brief Construct lock-free sync policy
     */
    lockfree_sync_policy() : shutdown_(false), approximate_size_(0) {
        auto dummy = new node(nullptr);
        head_.store(dummy, std::memory_order_relaxed);
        tail_.store(dummy, std::memory_order_relaxed);
    }

    /**
     * @brief Destructor
     */
    ~lockfree_sync_policy() {
        shutdown_.store(true, std::memory_order_release);

        // Drain remaining nodes
        node* current = head_.load(std::memory_order_acquire);
        while (current != nullptr) {
            node* next = current->next.load(std::memory_order_acquire);
            delete current;
            current = next;
        }
    }

    // Non-copyable, non-movable
    lockfree_sync_policy(const lockfree_sync_policy&) = delete;
    lockfree_sync_policy& operator=(const lockfree_sync_policy&) = delete;
    lockfree_sync_policy(lockfree_sync_policy&&) = delete;
    lockfree_sync_policy& operator=(lockfree_sync_policy&&) = delete;

    /**
     * @brief Enqueue a job (wait-free)
     * @param value Job to enqueue
     * @return VoidResult indicating success or error
     */
    [[nodiscard]] auto enqueue(std::unique_ptr<job>&& value) -> common::VoidResult {
        if (!value) {
            return common::error_info{-105, "cannot enqueue null job", "thread_system"};
        }

        if (shutdown_.load(std::memory_order_acquire)) {
            return common::error_info{-122, "queue is shutting down", "thread_system"};
        }

        auto new_node = new node(std::move(value));

        while (true) {
            node* tail = tail_.load(std::memory_order_acquire);
            node* next = tail->next.load(std::memory_order_acquire);

            if (tail == tail_.load(std::memory_order_acquire)) {
                if (next == nullptr) {
                    if (tail->next.compare_exchange_weak(next, new_node,
                            std::memory_order_release,
                            std::memory_order_relaxed)) {
                        tail_.compare_exchange_strong(tail, new_node,
                            std::memory_order_release,
                            std::memory_order_relaxed);
                        approximate_size_.fetch_add(1, std::memory_order_relaxed);
                        return common::ok();
                    }
                } else {
                    tail_.compare_exchange_weak(tail, next,
                        std::memory_order_release,
                        std::memory_order_relaxed);
                }
            }
        }
    }

    /**
     * @brief Dequeue a job (lock-free)
     * @return Result containing the job or error
     */
    [[nodiscard]] auto dequeue() -> common::Result<std::unique_ptr<job>> {
        while (true) {
            node* head = head_.load(std::memory_order_acquire);
            node* tail = tail_.load(std::memory_order_acquire);
            node* next = head->next.load(std::memory_order_acquire);

            if (head == head_.load(std::memory_order_acquire)) {
                if (head == tail) {
                    if (next == nullptr) {
                        return common::error_info{-121, "queue is empty", "thread_system"};
                    }
                    tail_.compare_exchange_weak(tail, next,
                        std::memory_order_release,
                        std::memory_order_relaxed);
                } else {
                    if (next != nullptr) {
                        auto value = std::move(next->data);
                        if (head_.compare_exchange_weak(head, next,
                                std::memory_order_release,
                                std::memory_order_relaxed)) {
                            approximate_size_.fetch_sub(1, std::memory_order_relaxed);
                            delete head;
                            return value;
                        }
                    }
                }
            }
        }
    }

    /**
     * @brief Try to dequeue a job (non-blocking, same as dequeue for lock-free)
     * @return Result containing the job or error
     */
    [[nodiscard]] auto try_dequeue() -> common::Result<std::unique_ptr<job>> {
        return dequeue();
    }

    /**
     * @brief Check if queue appears empty (approximate)
     * @return true if queue appears empty
     */
    [[nodiscard]] auto empty() const -> bool {
        node* head = head_.load(std::memory_order_acquire);
        node* next = head->next.load(std::memory_order_acquire);
        return next == nullptr;
    }

    /**
     * @brief Get approximate queue size
     * @return Approximate number of jobs
     */
    [[nodiscard]] auto size() const -> std::size_t {
        return approximate_size_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Clear queue (best effort for lock-free)
     */
    auto clear() -> void {
        while (true) {
            auto result = dequeue();
            if (result.is_err()) {
                break;
            }
        }
    }

    /**
     * @brief Stop the queue (sets shutdown flag)
     */
    auto stop() -> void {
        shutdown_.store(true, std::memory_order_release);
    }

    /**
     * @brief Check if queue is stopped
     * @return true if stopped
     */
    [[nodiscard]] auto is_stopped() const -> bool {
        return shutdown_.load(std::memory_order_acquire);
    }

    /**
     * @brief Set notify flag (no-op for lock-free)
     * @param notify Ignored
     */
    auto set_notify(bool /*notify*/) -> void {
        // No-op: lock-free queue doesn't use condition variables
    }

private:
    struct node {
        std::unique_ptr<job> data;
        std::atomic<node*> next{nullptr};

        explicit node(std::unique_ptr<job>&& job_data)
            : data(std::move(job_data)) {}
    };

    std::atomic<node*> head_;
    std::atomic<node*> tail_;
    std::atomic<bool> shutdown_;
    mutable std::atomic<std::size_t> approximate_size_;
};

/**
 * @class adaptive_sync_policy
 * @brief Adaptive synchronization policy that can switch modes
 *
 * Wraps both mutex and lock-free policies and can switch between them
 * based on runtime requirements.
 *
 * ### Thread Safety
 * - All operations are thread-safe
 * - Mode switching is synchronized
 */
class adaptive_sync_policy {
public:
    using policy_tag = sync_policy_tag;

    /**
     * @brief Operating mode
     */
    enum class mode {
        mutex,     ///< Using mutex sync
        lock_free  ///< Using lock-free sync
    };

    /**
     * @brief Queue capabilities (dynamic based on mode)
     */
    [[nodiscard]] auto get_capabilities() const -> queue_capabilities {
        if (current_mode_.load(std::memory_order_acquire) == mode::mutex) {
            return mutex_sync_policy::get_capabilities();
        }
        return lockfree_sync_policy::get_capabilities();
    }

    /**
     * @brief Construct adaptive sync policy
     * @param initial_mode Initial operating mode
     */
    explicit adaptive_sync_policy(mode initial_mode = mode::mutex)
        : current_mode_(initial_mode)
        , mutex_policy_(std::make_unique<mutex_sync_policy>())
        , lockfree_policy_(std::make_unique<lockfree_sync_policy>()) {}

    /**
     * @brief Destructor
     */
    ~adaptive_sync_policy() = default;

    // Non-copyable, non-movable
    adaptive_sync_policy(const adaptive_sync_policy&) = delete;
    adaptive_sync_policy& operator=(const adaptive_sync_policy&) = delete;
    adaptive_sync_policy(adaptive_sync_policy&&) = delete;
    adaptive_sync_policy& operator=(adaptive_sync_policy&&) = delete;

    /**
     * @brief Enqueue a job
     * @param value Job to enqueue
     * @return VoidResult indicating success or error
     */
    [[nodiscard]] auto enqueue(std::unique_ptr<job>&& value) -> common::VoidResult {
        if (current_mode_.load(std::memory_order_acquire) == mode::mutex) {
            return mutex_policy_->enqueue(std::move(value));
        }
        return lockfree_policy_->enqueue(std::move(value));
    }

    /**
     * @brief Dequeue a job
     * @return Result containing the job or error
     */
    [[nodiscard]] auto dequeue() -> common::Result<std::unique_ptr<job>> {
        if (current_mode_.load(std::memory_order_acquire) == mode::mutex) {
            return mutex_policy_->dequeue();
        }
        return lockfree_policy_->dequeue();
    }

    /**
     * @brief Try to dequeue a job
     * @return Result containing the job or error
     */
    [[nodiscard]] auto try_dequeue() -> common::Result<std::unique_ptr<job>> {
        if (current_mode_.load(std::memory_order_acquire) == mode::mutex) {
            return mutex_policy_->try_dequeue();
        }
        return lockfree_policy_->try_dequeue();
    }

    /**
     * @brief Check if queue is empty
     * @return true if empty
     */
    [[nodiscard]] auto empty() const -> bool {
        if (current_mode_.load(std::memory_order_acquire) == mode::mutex) {
            return mutex_policy_->empty();
        }
        return lockfree_policy_->empty();
    }

    /**
     * @brief Get queue size
     * @return Number of jobs (exact in mutex mode, approximate in lock-free)
     */
    [[nodiscard]] auto size() const -> std::size_t {
        if (current_mode_.load(std::memory_order_acquire) == mode::mutex) {
            return mutex_policy_->size();
        }
        return lockfree_policy_->size();
    }

    /**
     * @brief Clear queue
     */
    auto clear() -> void {
        if (current_mode_.load(std::memory_order_acquire) == mode::mutex) {
            mutex_policy_->clear();
        } else {
            lockfree_policy_->clear();
        }
    }

    /**
     * @brief Stop queue
     */
    auto stop() -> void {
        mutex_policy_->stop();
        lockfree_policy_->stop();
    }

    /**
     * @brief Check if stopped
     * @return true if stopped
     */
    [[nodiscard]] auto is_stopped() const -> bool {
        if (current_mode_.load(std::memory_order_acquire) == mode::mutex) {
            return mutex_policy_->is_stopped();
        }
        return lockfree_policy_->is_stopped();
    }

    /**
     * @brief Set notify flag
     * @param notify Whether to notify on enqueue
     */
    auto set_notify(bool notify) -> void {
        mutex_policy_->set_notify(notify);
    }

    /**
     * @brief Get current mode
     * @return Current operating mode
     */
    [[nodiscard]] auto current_mode() const -> mode {
        return current_mode_.load(std::memory_order_acquire);
    }

    /**
     * @brief Switch to a different mode
     * @param target_mode Target mode to switch to
     *
     * @note Data migration is NOT performed - caller is responsible
     */
    auto switch_mode(mode target_mode) -> void {
        current_mode_.store(target_mode, std::memory_order_release);
    }

private:
    std::atomic<mode> current_mode_;
    std::unique_ptr<mutex_sync_policy> mutex_policy_;
    std::unique_ptr<lockfree_sync_policy> lockfree_policy_;
};

} // namespace kcenon::thread::policies
