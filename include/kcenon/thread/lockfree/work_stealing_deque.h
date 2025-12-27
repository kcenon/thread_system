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
#include <memory>
#include <optional>
#include <vector>
#include <cstdint>

namespace kcenon::thread::lockfree {

/**
 * @class circular_array
 * @brief Dynamic circular array for work-stealing deque
 *
 * This class provides a resizable circular buffer used internally
 * by the work-stealing deque. It supports lock-free growth operations.
 *
 * @tparam T Element type stored in the array
 */
template<typename T>
class circular_array {
public:
    /**
     * @brief Constructs a circular array with given capacity
     * @param log_size Log base 2 of the capacity (capacity = 2^log_size)
     */
    explicit circular_array(std::size_t log_size)
        : log_size_(log_size)
        , size_(1ULL << log_size)
        , mask_(size_ - 1)
        , buffer_(new std::atomic<T>[size_]) {
        for (std::size_t i = 0; i < size_; ++i) {
            buffer_[i].store(T{}, std::memory_order_relaxed);
        }
    }

    ~circular_array() {
        delete[] buffer_;
    }

    // Non-copyable
    circular_array(const circular_array&) = delete;
    circular_array& operator=(const circular_array&) = delete;

    /**
     * @brief Get the capacity of the array
     * @return Number of elements the array can hold
     */
    [[nodiscard]] std::size_t size() const noexcept {
        return size_;
    }

    /**
     * @brief Get element at index with relaxed memory ordering
     * @param index Array index (will be masked for circular access)
     * @return Element at the given index
     */
    [[nodiscard]] T get(std::int64_t index) const noexcept {
        return buffer_[index & mask_].load(std::memory_order_relaxed);
    }

    /**
     * @brief Store element at index with relaxed memory ordering
     * @param index Array index (will be masked for circular access)
     * @param value Value to store
     */
    void put(std::int64_t index, T value) noexcept {
        buffer_[index & mask_].store(value, std::memory_order_relaxed);
    }

    /**
     * @brief Create a new array with double the capacity, copying elements
     * @param bottom Current bottom index
     * @param top Current top index
     * @return New circular array with doubled capacity
     */
    [[nodiscard]] circular_array* grow(std::int64_t bottom, std::int64_t top) const {
        auto* new_array = new circular_array(log_size_ + 1);
        for (std::int64_t i = top; i < bottom; ++i) {
            new_array->put(i, get(i));
        }
        return new_array;
    }

private:
    std::size_t log_size_;
    std::size_t size_;
    std::size_t mask_;
    std::atomic<T>* buffer_;
};

/**
 * @class work_stealing_deque
 * @brief Lock-free work-stealing deque based on Chase-Lev algorithm
 *
 * This class implements a work-stealing deque (double-ended queue) using
 * the Chase-Lev algorithm. It provides efficient local operations for
 * the owner thread (push/pop) and concurrent stealing for other threads.
 *
 * Key Features:
 * - Owner-side push/pop: LIFO order for cache locality
 * - Thief-side steal: FIFO order for fairness
 * - Lock-free operations with proper memory ordering
 * - Dynamic resizing when full
 *
 * Algorithm Reference:
 * "Dynamic Circular Work-Stealing Deque" (Chase & Lev, 2005)
 *
 * Memory Layout:
 * ```
 * Owner Thread:           Thief Threads:
 *     ↓ push/pop              ↓ steal
 *     ┌───────────────────────┐
 *     │ bottom              top │
 *     │   ↓                  ↑  │
 *     │ [T4][T3][T2][T1][--][--] │
 *     │   LIFO            FIFO  │
 *     │ (locality)    (fairness)│
 *     └───────────────────────┘
 * ```
 *
 * Thread Safety:
 * - push(): Single-threaded (owner only)
 * - pop(): Single-threaded (owner only)
 * - steal(): Multi-threaded (any thief)
 * - empty()/size(): Any thread (approximate)
 *
 * @tparam T Element type (must be pointer or trivially copyable)
 *
 * @note For use with job*, ensure T = job*
 */
template<typename T>
class work_stealing_deque {
public:
    /**
     * @brief Default initial log capacity (2^LOG_INITIAL_SIZE = 32 elements)
     */
    static constexpr std::size_t LOG_INITIAL_SIZE = 5;

    /**
     * @brief Constructs an empty work-stealing deque
     * @param log_initial_size Initial capacity as log base 2 (default: 5, i.e., 32 elements)
     */
    explicit work_stealing_deque(std::size_t log_initial_size = LOG_INITIAL_SIZE)
        : top_(0)
        , bottom_(0)
        , array_(new circular_array<T>(log_initial_size)) {
    }

    /**
     * @brief Destructor - cleans up the circular array
     */
    ~work_stealing_deque() {
        delete array_.load(std::memory_order_relaxed);
    }

    // Non-copyable and non-movable
    work_stealing_deque(const work_stealing_deque&) = delete;
    work_stealing_deque& operator=(const work_stealing_deque&) = delete;
    work_stealing_deque(work_stealing_deque&&) = delete;
    work_stealing_deque& operator=(work_stealing_deque&&) = delete;

    /**
     * @brief Push an element onto the bottom of the deque (owner only)
     * @param item Element to push
     *
     * Time Complexity: O(1) amortized (O(n) when resizing)
     *
     * @note This method should only be called by the owner thread.
     *       Calling from multiple threads results in undefined behavior.
     */
    void push(T item) {
        std::int64_t b = bottom_.load(std::memory_order_relaxed);
        std::int64_t t = top_.load(std::memory_order_acquire);
        circular_array<T>* a = array_.load(std::memory_order_relaxed);

        // Check if array needs to grow
        if (b - t > static_cast<std::int64_t>(a->size()) - 1) {
            // Grow the array
            circular_array<T>* new_array = a->grow(b, t);
            // Store old array for cleanup (in a real implementation,
            // you would use hazard pointers or epoch-based reclamation)
            old_arrays_.push_back(a);
            array_.store(new_array, std::memory_order_release);
            a = new_array;
        }

        a->put(b, item);
        std::atomic_thread_fence(std::memory_order_release);
        bottom_.store(b + 1, std::memory_order_relaxed);
    }

    /**
     * @brief Pop an element from the bottom of the deque (owner only)
     * @return The popped element, or std::nullopt if empty
     *
     * Time Complexity: O(1)
     *
     * @note This method should only be called by the owner thread.
     *       Uses LIFO order for better cache locality.
     */
    [[nodiscard]] std::optional<T> pop() {
        std::int64_t b = bottom_.load(std::memory_order_relaxed) - 1;
        circular_array<T>* a = array_.load(std::memory_order_relaxed);
        bottom_.store(b, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        std::int64_t t = top_.load(std::memory_order_relaxed);

        if (t <= b) {
            // Non-empty queue
            T item = a->get(b);
            if (t == b) {
                // Last element - compete with thieves
                if (!top_.compare_exchange_strong(
                        t, t + 1,
                        std::memory_order_seq_cst,
                        std::memory_order_relaxed)) {
                    // Lost race with a thief
                    bottom_.store(b + 1, std::memory_order_relaxed);
                    return std::nullopt;
                }
                bottom_.store(b + 1, std::memory_order_relaxed);
            }
            return item;
        } else {
            // Empty queue
            bottom_.store(b + 1, std::memory_order_relaxed);
            return std::nullopt;
        }
    }

    /**
     * @brief Steal an element from the top of the deque (thief threads)
     * @return The stolen element, or std::nullopt if empty or contention
     *
     * Time Complexity: O(1)
     *
     * @note This method can be called concurrently by multiple thief threads.
     *       Uses FIFO order for fairness.
     */
    [[nodiscard]] std::optional<T> steal() {
        std::int64_t t = top_.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        std::int64_t b = bottom_.load(std::memory_order_acquire);

        if (t < b) {
            // Non-empty queue
            circular_array<T>* a = array_.load(std::memory_order_consume);
            T item = a->get(t);

            if (!top_.compare_exchange_strong(
                    t, t + 1,
                    std::memory_order_seq_cst,
                    std::memory_order_relaxed)) {
                // Lost race with another thief or owner
                return std::nullopt;
            }
            return item;
        }
        return std::nullopt;
    }

    /**
     * @brief Check if the deque appears empty
     * @return true if the deque appears empty, false otherwise
     *
     * @note This is a snapshot view; the deque may change immediately after.
     *       Use for hints only, not for synchronization.
     */
    [[nodiscard]] bool empty() const noexcept {
        std::int64_t b = bottom_.load(std::memory_order_relaxed);
        std::int64_t t = top_.load(std::memory_order_relaxed);
        return b <= t;
    }

    /**
     * @brief Get approximate size of the deque
     * @return Approximate number of elements
     *
     * @note This is a best-effort estimate due to concurrent modifications.
     */
    [[nodiscard]] std::size_t size() const noexcept {
        std::int64_t b = bottom_.load(std::memory_order_relaxed);
        std::int64_t t = top_.load(std::memory_order_relaxed);
        std::int64_t diff = b - t;
        return diff > 0 ? static_cast<std::size_t>(diff) : 0;
    }

    /**
     * @brief Get the capacity of the current array
     * @return Current capacity
     */
    [[nodiscard]] std::size_t capacity() const noexcept {
        return array_.load(std::memory_order_relaxed)->size();
    }

    /**
     * @brief Clear all old arrays (for memory cleanup)
     *
     * @note Should only be called when no operations are in progress.
     *       Typically called during shutdown or periodic cleanup.
     */
    void cleanup_old_arrays() {
        for (auto* old_array : old_arrays_) {
            delete old_array;
        }
        old_arrays_.clear();
    }

private:
    // Cache line padding to prevent false sharing
    static constexpr std::size_t CACHE_LINE_SIZE = 64;

    alignas(CACHE_LINE_SIZE) std::atomic<std::int64_t> top_;
    alignas(CACHE_LINE_SIZE) std::atomic<std::int64_t> bottom_;
    alignas(CACHE_LINE_SIZE) std::atomic<circular_array<T>*> array_;

    // Storage for old arrays (simple approach - could use hazard pointers)
    std::vector<circular_array<T>*> old_arrays_;
};

} // namespace kcenon::thread::lockfree
