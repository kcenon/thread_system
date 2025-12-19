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

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

namespace kcenon::thread {

/**
 * @brief Thread-local hazard pointer record with explicit memory ordering
 *
 * Each thread maintains a small array of hazard pointers.
 * All atomic operations use explicit memory_order for correctness
 * on weak memory model architectures (ARM, etc.)
 */
class safe_hazard_pointer_record {
public:
    static constexpr size_t MAX_HAZARD_POINTERS = 2;

    safe_hazard_pointer_record() {
        for (auto& hp : hazard_pointers_) {
            hp.store(nullptr, std::memory_order_relaxed);
        }
        next.store(nullptr, std::memory_order_relaxed);
        active.store(false, std::memory_order_relaxed);
    }

    /**
     * @brief Protect a pointer from reclamation
     * @param p Pointer to protect
     * @param slot Slot index (0 or 1)
     *
     * Uses memory_order_release to ensure visibility to other threads
     * scanning hazard pointers.
     */
    void protect(void* p, size_t slot = 0) noexcept {
        assert(slot < MAX_HAZARD_POINTERS);
        hazard_pointers_[slot].store(p, std::memory_order_release);
    }

    /**
     * @brief Clear hazard pointer protection
     * @param slot Slot index
     *
     * Uses memory_order_release to ensure the clear is visible
     * before any subsequent operations.
     */
    void clear(size_t slot = 0) noexcept {
        assert(slot < MAX_HAZARD_POINTERS);
        hazard_pointers_[slot].store(nullptr, std::memory_order_release);
    }

    /**
     * @brief Check if a pointer is protected by this record
     * @param p Pointer to check
     * @return true if protected
     *
     * Uses memory_order_acquire to synchronize with protect() calls.
     */
    [[nodiscard]] bool contains(void* p) const noexcept {
        for (const auto& hp : hazard_pointers_) {
            if (hp.load(std::memory_order_acquire) == p) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Get protected pointer at slot
     * @param slot Slot index
     * @return Protected pointer or nullptr
     */
    [[nodiscard]] void* get(size_t slot = 0) const noexcept {
        assert(slot < MAX_HAZARD_POINTERS);
        return hazard_pointers_[slot].load(std::memory_order_acquire);
    }

    // Linked list for global registry
    std::atomic<safe_hazard_pointer_record*> next{nullptr};
    std::atomic<bool> active{false};

private:
    std::array<std::atomic<void*>, MAX_HAZARD_POINTERS> hazard_pointers_;
};

/**
 * @brief Global Hazard Pointer Domain Manager
 *
 * Manages all thread-local hazard pointer records centrally.
 * Provides safe memory reclamation with explicit memory ordering
 * guarantees.
 *
 * Thread-safe and lock-free for acquire/release operations.
 */
class safe_hazard_pointer_domain {
public:
    using retire_callback = std::function<void(void*)>;

    /**
     * @brief Get singleton instance
     */
    static safe_hazard_pointer_domain& instance() {
        static safe_hazard_pointer_domain domain;
        return domain;
    }

    /**
     * @brief Acquire a hazard pointer record for current thread
     * @return Pointer to acquired record
     *
     * Lock-free acquisition with proper memory ordering.
     */
    safe_hazard_pointer_record* acquire() {
        // 1. Try to reuse an inactive record first
        safe_hazard_pointer_record* p = head_.load(std::memory_order_acquire);
        while (p != nullptr) {
            bool expected = false;
            if (p->active.compare_exchange_strong(
                    expected, true,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed)) {
                // Clear hazard pointers immediately after acquiring to avoid
                // stale pointers from previous use affecting collect()
                p->clear(0);
                p->clear(1);
                active_count_.fetch_add(1, std::memory_order_relaxed);
                return p;
            }
            p = p->next.load(std::memory_order_acquire);
        }

        // 2. Create new record (constructor already clears hazard pointers)
        auto* new_record = new safe_hazard_pointer_record();
        new_record->active.store(true, std::memory_order_relaxed);

        // 3. Add to list (lock-free)
        safe_hazard_pointer_record* old_head;
        do {
            old_head = head_.load(std::memory_order_relaxed);
            new_record->next.store(old_head, std::memory_order_relaxed);
        } while (!head_.compare_exchange_weak(
            old_head, new_record,
            std::memory_order_release,
            std::memory_order_relaxed));

        active_count_.fetch_add(1, std::memory_order_relaxed);
        return new_record;
    }

    /**
     * @brief Release a hazard pointer record
     * @param record Record to release
     *
     * Clears all hazard pointers and marks record as inactive.
     */
    void release(safe_hazard_pointer_record* record) noexcept {
        if (record == nullptr) {
            return;
        }

        record->clear(0);
        record->clear(1);
        record->active.store(false, std::memory_order_release);
        active_count_.fetch_sub(1, std::memory_order_relaxed);
    }

    /**
     * @brief Retire a pointer for later reclamation
     * @param p Pointer to retire
     * @param deleter Deletion callback
     *
     * Thread-safe. Triggers collection when threshold is reached.
     * Handles duplicate addresses by removing old entries (memory reuse scenario).
     */
    void retire(void* p, retire_callback deleter) {
        if (p == nullptr) {
            return;
        }

        bool should_collect = false;
        {
            std::lock_guard<std::mutex> lock(retire_mutex_);

            // Remove any existing entry with the same address to handle memory reuse.
            // This can happen when memory is freed and reallocated to the same address.
            // In this case, the old entry's deleter must NOT be called since the memory
            // is now occupied by a new valid object.
            auto it = std::remove_if(retired_list_.begin(), retired_list_.end(),
                [p](const auto& entry) { return entry.first == p; });
            if (it != retired_list_.end()) {
                size_t removed = std::distance(it, retired_list_.end());
                retired_list_.erase(it, retired_list_.end());
                retired_count_.fetch_sub(removed, std::memory_order_relaxed);
            }

            retired_list_.emplace_back(p, std::move(deleter));
            retired_count_.fetch_add(1, std::memory_order_relaxed);

            // Check threshold inside lock to avoid race
            size_t threshold = get_adaptive_threshold();
            should_collect = (retired_count_.load(std::memory_order_relaxed) >= threshold);
        }

        // Trigger collection after releasing lock to avoid deadlock
        if (should_collect) {
            collect();
        }
    }

    /**
     * @brief Collect reclaimable objects
     *
     * Scans all hazard pointers and deletes objects that
     * are not protected.
     */
    void collect() {
        std::lock_guard<std::mutex> lock(retire_mutex_);
        collect_internal();
    }

    /**
     * @brief Get current retired count
     */
    [[nodiscard]] size_t retired_count() const noexcept {
        return retired_count_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Get active thread count
     */
    [[nodiscard]] size_t active_count() const noexcept {
        return active_count_.load(std::memory_order_relaxed);
    }

private:
    safe_hazard_pointer_domain() = default;

    ~safe_hazard_pointer_domain() {
        // Delete all records
        auto* p = head_.load(std::memory_order_relaxed);
        while (p != nullptr) {
            auto* next = p->next.load(std::memory_order_relaxed);
            delete p;
            p = next;
        }

        // Force delete remaining retired objects
        for (auto& [ptr, deleter] : retired_list_) {
            if (deleter && ptr) {
                deleter(ptr);
            }
        }
    }

    // Prevent copying
    safe_hazard_pointer_domain(const safe_hazard_pointer_domain&) = delete;
    safe_hazard_pointer_domain& operator=(const safe_hazard_pointer_domain&) = delete;

    /**
     * @brief Collect reclaimable objects (internal, lock held)
     */
    void collect_internal() {
        if (retired_list_.empty()) {
            return;
        }

        // Gather all currently protected pointers
        // IMPORTANT: Check ALL records, not just active ones, to avoid race condition
        // where a record is being reused while we're scanning.
        // The hazard pointer value is set before active=true, so we must check
        // the pointer value itself, not the active flag.
        std::unordered_set<void*> hazards;
        hazards.reserve(active_count_.load(std::memory_order_relaxed) *
                        safe_hazard_pointer_record::MAX_HAZARD_POINTERS);

        safe_hazard_pointer_record* p = head_.load(std::memory_order_acquire);
        while (p != nullptr) {
            // Check all records regardless of active status to avoid race condition
            for (size_t i = 0; i < safe_hazard_pointer_record::MAX_HAZARD_POINTERS; ++i) {
                void* hp = p->get(i);
                if (hp != nullptr) {
                    hazards.insert(hp);
                }
            }
            p = p->next.load(std::memory_order_acquire);
        }

        // Delete objects not in hazard set
        size_t reclaimed = 0;
        auto it = retired_list_.begin();
        while (it != retired_list_.end()) {
            if (hazards.find(it->first) == hazards.end()) {
                // Safe to delete
                if (it->second && it->first) {
                    it->second(it->first);
                }
                it = retired_list_.erase(it);
                ++reclaimed;
            } else {
                ++it;
            }
        }

        retired_count_.fetch_sub(reclaimed, std::memory_order_relaxed);
    }

    /**
     * @brief Get adaptive threshold based on active thread count
     */
    [[nodiscard]] size_t get_adaptive_threshold() const noexcept {
        static constexpr size_t BASE_THRESHOLD = 64;
        static constexpr size_t PER_THREAD_THRESHOLD = 16;
        static constexpr size_t MAX_THRESHOLD = 512;

        size_t active = active_count_.load(std::memory_order_relaxed);
        return std::min(MAX_THRESHOLD, BASE_THRESHOLD + active * PER_THREAD_THRESHOLD);
    }

    std::atomic<safe_hazard_pointer_record*> head_{nullptr};
    std::atomic<size_t> active_count_{0};
    std::atomic<size_t> retired_count_{0};
    std::mutex retire_mutex_;
    std::vector<std::pair<void*, retire_callback>> retired_list_;
};

/**
 * @brief RAII-style Hazard Pointer Guard
 *
 * Automatically acquires and releases hazard pointer record.
 * Use this for exception-safe hazard pointer management.
 *
 * @example
 * ```cpp
 * // Protect a pointer during access
 * safe_hazard_guard guard(ptr);
 * // ptr is safe to use in this scope
 * auto value = ptr->data;
 * // guard destructor releases protection
 * ```
 */
class safe_hazard_guard {
public:
    /**
     * @brief Construct guard, optionally protecting a pointer
     * @param p Pointer to protect (optional)
     * @param slot Slot index (0 or 1)
     */
    explicit safe_hazard_guard(void* p = nullptr, size_t slot = 0)
        : record_(safe_hazard_pointer_domain::instance().acquire())
        , slot_(slot) {
        if (p != nullptr) {
            record_->protect(p, slot_);
        }
    }

    /**
     * @brief Destructor - releases the record
     */
    ~safe_hazard_guard() {
        if (record_ != nullptr) {
            safe_hazard_pointer_domain::instance().release(record_);
        }
    }

    // Non-copyable
    safe_hazard_guard(const safe_hazard_guard&) = delete;
    safe_hazard_guard& operator=(const safe_hazard_guard&) = delete;

    // Movable
    safe_hazard_guard(safe_hazard_guard&& other) noexcept
        : record_(other.record_)
        , slot_(other.slot_) {
        other.record_ = nullptr;
    }

    safe_hazard_guard& operator=(safe_hazard_guard&& other) noexcept {
        if (this != &other) {
            if (record_ != nullptr) {
                safe_hazard_pointer_domain::instance().release(record_);
            }
            record_ = other.record_;
            slot_ = other.slot_;
            other.record_ = nullptr;
        }
        return *this;
    }

    /**
     * @brief Protect a pointer
     * @param p Pointer to protect
     */
    void protect(void* p) noexcept {
        if (record_ != nullptr) {
            record_->protect(p, slot_);
        }
    }

    /**
     * @brief Clear protection
     */
    void clear() noexcept {
        if (record_ != nullptr) {
            record_->clear(slot_);
        }
    }

    /**
     * @brief Get protected pointer
     */
    [[nodiscard]] void* get() const noexcept {
        return record_ != nullptr ? record_->get(slot_) : nullptr;
    }

    /**
     * @brief Check if valid
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return record_ != nullptr;
    }

private:
    safe_hazard_pointer_record* record_;
    size_t slot_;
};

/**
 * @brief Retire a pointer for safe deletion
 *
 * @tparam T Object type
 * @param p Pointer to retire
 *
 * The object will be deleted when no hazard pointers protect it.
 *
 * @example
 * ```cpp
 * Node* old_node = head.exchange(new_node);
 * safe_retire_hazard(old_node);  // Will be deleted when safe
 * ```
 */
template<typename T>
void safe_retire_hazard(T* p) {
    if (p == nullptr) {
        return;
    }
    safe_hazard_pointer_domain::instance().retire(
        p,
        [](void* ptr) { delete static_cast<T*>(ptr); }
    );
}

/**
 * @brief Typed hazard pointer domain
 *
 * @tparam T Type of objects managed
 *
 * Provides type-safe retire and reclaim operations.
 */
template<typename T>
class typed_safe_hazard_domain {
public:
    static typed_safe_hazard_domain& instance() {
        static typed_safe_hazard_domain domain;
        return domain;
    }

    void retire(T* p) {
        safe_retire_hazard(p);
    }

    void collect() {
        safe_hazard_pointer_domain::instance().collect();
    }

private:
    typed_safe_hazard_domain() = default;
};

}  // namespace kcenon::thread
