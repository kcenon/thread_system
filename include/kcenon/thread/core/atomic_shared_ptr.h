// BSD 3-Clause License
//
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <atomic>
#include <memory>

namespace kcenon::thread {

/**
 * @brief Thread-safe atomic shared_ptr wrapper
 *
 * Provides atomic operations on std::shared_ptr with explicit
 * memory ordering. Uses C++20 std::atomic<std::shared_ptr<T>>
 * when available, otherwise falls back to std::atomic_* functions.
 *
 * This is a safer and simpler alternative to hazard pointers
 * for most use cases, with automatic memory management.
 *
 * @tparam T The type of object managed
 *
 * @example
 * ```cpp
 * atomic_shared_ptr<Node> head;
 *
 * // Writer thread
 * auto new_node = std::make_shared<Node>(value);
 * auto old = head.exchange(new_node);
 *
 * // Reader thread
 * auto node = head.load();  // Automatic protection via shared_ptr
 * if (node) {
 *     process(node->data);
 * }
 * ```
 *
 * @note Performance: shared_ptr atomic operations may be slower than
 *       hazard pointers (~50ns vs ~10ns per acquire), but they are
 *       much simpler and safer to use correctly.
 */
template<typename T>
class atomic_shared_ptr {
public:
    /**
     * @brief Default constructor - initializes with nullptr
     */
    atomic_shared_ptr() noexcept = default;

    /**
     * @brief Construct with initial shared_ptr
     * @param ptr Initial value
     */
    explicit atomic_shared_ptr(std::shared_ptr<T> ptr) noexcept
        : ptr_(std::move(ptr)) {}

    /**
     * @brief Copy constructor - atomically copies the shared_ptr
     * @param other Source atomic_shared_ptr
     */
    atomic_shared_ptr(const atomic_shared_ptr& other)
        : ptr_(other.load()) {}

    /**
     * @brief Copy assignment - atomically copies the shared_ptr
     * @param other Source atomic_shared_ptr
     * @return Reference to this
     */
    atomic_shared_ptr& operator=(const atomic_shared_ptr& other) {
        if (this != &other) {
            store(other.load());
        }
        return *this;
    }

    /**
     * @brief Move constructor
     * @param other Source atomic_shared_ptr
     */
    atomic_shared_ptr(atomic_shared_ptr&& other) noexcept
        : ptr_(other.exchange(nullptr)) {}

    /**
     * @brief Move assignment
     * @param other Source atomic_shared_ptr
     * @return Reference to this
     */
    atomic_shared_ptr& operator=(atomic_shared_ptr&& other) noexcept {
        if (this != &other) {
            store(other.exchange(nullptr));
        }
        return *this;
    }

    /**
     * @brief Assign from shared_ptr
     * @param ptr New value
     * @return Reference to this
     */
    atomic_shared_ptr& operator=(std::shared_ptr<T> ptr) noexcept {
        store(std::move(ptr));
        return *this;
    }

    /**
     * @brief Atomically store a new value
     * @param ptr New value
     * @param order Memory ordering
     *
     * Uses memory_order_seq_cst by default for maximum safety.
     */
    void store(std::shared_ptr<T> ptr,
               std::memory_order order = std::memory_order_seq_cst) noexcept {
#if __cpp_lib_atomic_shared_ptr >= 201711L
        ptr_.store(std::move(ptr), order);
#else
        std::atomic_store_explicit(&ptr_, std::move(ptr), order);
#endif
    }

    /**
     * @brief Atomically load the current value
     * @param order Memory ordering
     * @return Current shared_ptr value
     *
     * The returned shared_ptr keeps the object alive as long as
     * it exists - no manual hazard pointer management needed.
     */
    [[nodiscard]] std::shared_ptr<T> load(
        std::memory_order order = std::memory_order_seq_cst) const noexcept {
#if __cpp_lib_atomic_shared_ptr >= 201711L
        return ptr_.load(order);
#else
        return std::atomic_load_explicit(&ptr_, order);
#endif
    }

    /**
     * @brief Atomically exchange the value
     * @param ptr New value
     * @param order Memory ordering
     * @return Previous value
     */
    [[nodiscard]] std::shared_ptr<T> exchange(
        std::shared_ptr<T> ptr,
        std::memory_order order = std::memory_order_seq_cst) noexcept {
#if __cpp_lib_atomic_shared_ptr >= 201711L
        return ptr_.exchange(std::move(ptr), order);
#else
        return std::atomic_exchange_explicit(&ptr_, std::move(ptr), order);
#endif
    }

    /**
     * @brief Atomically compare and exchange (weak)
     * @param expected Expected value (updated on failure)
     * @param desired New value if comparison succeeds
     * @param success Memory ordering on success
     * @param failure Memory ordering on failure
     * @return true if exchange succeeded
     *
     * May spuriously fail even if comparison would succeed.
     * Use in loops for lock-free algorithms.
     */
    bool compare_exchange_weak(
        std::shared_ptr<T>& expected,
        std::shared_ptr<T> desired,
        std::memory_order success = std::memory_order_seq_cst,
        std::memory_order failure = std::memory_order_seq_cst) noexcept {
#if __cpp_lib_atomic_shared_ptr >= 201711L
        return ptr_.compare_exchange_weak(expected, std::move(desired), success, failure);
#else
        return std::atomic_compare_exchange_weak_explicit(
            &ptr_, &expected, std::move(desired), success, failure);
#endif
    }

    /**
     * @brief Atomically compare and exchange (strong)
     * @param expected Expected value (updated on failure)
     * @param desired New value if comparison succeeds
     * @param success Memory ordering on success
     * @param failure Memory ordering on failure
     * @return true if exchange succeeded
     *
     * Only fails if comparison actually fails (no spurious failures).
     */
    bool compare_exchange_strong(
        std::shared_ptr<T>& expected,
        std::shared_ptr<T> desired,
        std::memory_order success = std::memory_order_seq_cst,
        std::memory_order failure = std::memory_order_seq_cst) noexcept {
#if __cpp_lib_atomic_shared_ptr >= 201711L
        return ptr_.compare_exchange_strong(expected, std::move(desired), success, failure);
#else
        return std::atomic_compare_exchange_strong_explicit(
            &ptr_, &expected, std::move(desired), success, failure);
#endif
    }

    /**
     * @brief Conversion operator to shared_ptr
     * @return Current value (atomically loaded)
     */
    [[nodiscard]] operator std::shared_ptr<T>() const noexcept {
        return load();
    }

    /**
     * @brief Dereference operator
     * @return Reference to managed object
     * @warning Undefined behavior if pointer is null
     */
    [[nodiscard]] T& operator*() const noexcept {
        return *load();
    }

    /**
     * @brief Member access operator
     * @return Pointer to managed object
     * @warning Undefined behavior if pointer is null
     */
    [[nodiscard]] T* operator->() const noexcept {
        return load().get();
    }

    /**
     * @brief Boolean conversion - check if not null
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return load() != nullptr;
    }

    /**
     * @brief Get raw pointer (unsafe - for debugging only)
     * @return Raw pointer, may become dangling
     * @warning The returned pointer is not protected. Only use for
     *          debugging or when you guarantee lifetime externally.
     */
    [[nodiscard]] T* get_unsafe() const noexcept {
        return load().get();
    }

    /**
     * @brief Reset to nullptr
     */
    void reset() noexcept {
        store(nullptr);
    }

    /**
     * @brief Reset to new value
     * @param ptr New managed object
     */
    void reset(T* ptr) noexcept {
        store(std::shared_ptr<T>(ptr));
    }

private:
#if __cpp_lib_atomic_shared_ptr >= 201711L
    std::atomic<std::shared_ptr<T>> ptr_{nullptr};
#else
    std::shared_ptr<T> ptr_{nullptr};
#endif
};

/**
 * @brief Make an atomic_shared_ptr with a new object
 *
 * @tparam T Object type
 * @tparam Args Constructor argument types
 * @param args Constructor arguments
 * @return atomic_shared_ptr containing new object
 */
template<typename T, typename... Args>
[[nodiscard]] atomic_shared_ptr<T> make_atomic_shared(Args&&... args) {
    return atomic_shared_ptr<T>(std::make_shared<T>(std::forward<Args>(args)...));
}

}  // namespace kcenon::thread
