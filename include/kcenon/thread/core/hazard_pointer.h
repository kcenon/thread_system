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
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

namespace kcenon::thread {

// Forward declarations
template<typename T>
class hazard_pointer_domain;

namespace detail {

/// Thread-local hazard pointer list
/// Each thread maintains a small array of hazard pointers
struct thread_hazard_list {
    static constexpr size_t MAX_HAZARDS_PER_THREAD = 4;

    std::atomic<void*> hazards[MAX_HAZARDS_PER_THREAD];
    thread_hazard_list* next;       // Linked list of all thread lists
    std::atomic<bool> active;       // Is this thread still active?

    thread_hazard_list() : next(nullptr), active(true) {
        for (auto& h : hazards) {
            h.store(nullptr, std::memory_order_relaxed);
        }
    }
};

/// Retire node for pending deletion
struct retire_node {
    void* ptr;
    std::function<void(void*)> deleter;
    retire_node* next;

    retire_node(void* p, std::function<void(void*)> d)
        : ptr(p), deleter(std::move(d)), next(nullptr) {}
};

/// Global hazard pointer registry
/// Manages all thread-local hazard lists
class hazard_pointer_registry {
public:
    static hazard_pointer_registry& instance();

    /// Get or create thread-local hazard list
    thread_hazard_list* get_thread_list();

    /// Mark current thread's list as inactive
    void mark_inactive();

    /// Scan all hazard pointers and collect protected pointers
    std::vector<void*> scan_hazard_pointers();

    /// Get total number of active threads
    size_t get_active_thread_count() const;

private:
    hazard_pointer_registry() = default;

    std::atomic<thread_hazard_list*> head_{nullptr};
    std::atomic<size_t> thread_count_{0};
};

} // namespace detail

/// Single hazard pointer that protects one object from reclamation
/// Uses RAII pattern - automatically releases protection on destruction
class hazard_pointer {
public:
    /// Default constructor - acquires a hazard pointer slot
    hazard_pointer();

    /// Move constructor
    hazard_pointer(hazard_pointer&& other) noexcept;

    /// Move assignment
    hazard_pointer& operator=(hazard_pointer&& other) noexcept;

    /// Destructor - automatically releases protection
    ~hazard_pointer();

    /// Non-copyable
    hazard_pointer(const hazard_pointer&) = delete;
    hazard_pointer& operator=(const hazard_pointer&) = delete;

    /// Protect a pointer from reclamation
    /// @param ptr Pointer to protect
    /// @note Thread-safe, can be called concurrently
    /// @note Uses memory_order_release to ensure visibility
    template<typename T>
    void protect(T* ptr) noexcept {
        if (slot_) {
            slot_->store(static_cast<void*>(ptr), std::memory_order_release);
        }
    }

    /// Release protection
    /// @note After reset(), the previously protected pointer may be reclaimed
    /// @note Keeps the slot owned but clears the protected pointer
    void reset() noexcept;

    /// Check if currently protecting a pointer
    bool is_protected() const noexcept;

    /// Get the protected pointer (may be null)
    void* get_protected() const noexcept;

private:
    // Special marker value indicating slot is owned but not protecting anything
    static inline const void* SLOT_OWNED_MARKER = reinterpret_cast<void*>(0x1);

    std::atomic<void*>* slot_;  // Pointer to thread-local hazard slot
    size_t slot_index_;         // Index in thread-local array
};

/// Domain managing hazard pointers and retirement for a specific type
/// @tparam T Type of objects protected by this domain
template<typename T>
class hazard_pointer_domain {
public:
    /// Get the global domain instance for type T
    static hazard_pointer_domain& global() {
        static hazard_pointer_domain instance;
        return instance;
    }

    /// Acquire a hazard pointer for this domain
    /// @return Hazard pointer that will automatically release on destruction
    hazard_pointer acquire() {
        return hazard_pointer();
    }

    /// Retire an object for later reclamation
    /// @param ptr Pointer to object to retire
    /// @note Object will be deleted when no hazard pointers protect it
    void retire(T* ptr);

    /// Force reclamation scan (optional, for testing)
    /// @return Number of objects reclaimed
    size_t reclaim();

    /// Get statistics
    struct stats {
        size_t hazard_pointers_allocated;
        size_t objects_retired;
        size_t objects_reclaimed;
        size_t scan_count;
    };

    stats get_stats() const;

    /// Destructor - ensures all retire objects are reclaimed
    ~hazard_pointer_domain();

private:
    hazard_pointer_domain() = default;

    // Thread-local retire list
    struct thread_retire_list {
        detail::retire_node* head = nullptr;
        size_t count = 0;

        static constexpr size_t RECLAIM_THRESHOLD = 64;  // Configurable threshold

        void add(T* ptr);
        size_t scan_and_reclaim(const std::vector<void*>& protected_ptrs);
        void reclaim_all();
        ~thread_retire_list();
    };

    static thread_retire_list& get_thread_retire_list() {
        static thread_local thread_retire_list list;
        return list;
    }

    // Statistics (atomic for thread-safety)
    mutable std::atomic<size_t> objects_retired_{0};
    mutable std::atomic<size_t> objects_reclaimed_{0};
    mutable std::atomic<size_t> scan_count_{0};
};

// Template implementations

template<typename T>
void hazard_pointer_domain<T>::retire(T* ptr) {
    if (!ptr) {
        return;
    }

    auto& retire_list = get_thread_retire_list();
    retire_list.add(ptr);
    objects_retired_.fetch_add(1, std::memory_order_relaxed);

    // Check if we should run reclamation
    if (retire_list.count >= thread_retire_list::RECLAIM_THRESHOLD) {
        reclaim();
    }
}

template<typename T>
size_t hazard_pointer_domain<T>::reclaim() {
    scan_count_.fetch_add(1, std::memory_order_relaxed);

    // Scan all hazard pointers to get protected set
    auto& registry = detail::hazard_pointer_registry::instance();
    auto protected_ptrs = registry.scan_hazard_pointers();

    // Reclaim objects not in protected set
    auto& retire_list = get_thread_retire_list();
    size_t reclaimed = retire_list.scan_and_reclaim(protected_ptrs);

    objects_reclaimed_.fetch_add(reclaimed, std::memory_order_relaxed);
    return reclaimed;
}

template<typename T>
auto hazard_pointer_domain<T>::get_stats() const -> stats {
    auto& registry = detail::hazard_pointer_registry::instance();

    return stats{
        .hazard_pointers_allocated =
            registry.get_active_thread_count() * detail::thread_hazard_list::MAX_HAZARDS_PER_THREAD,
        .objects_retired = objects_retired_.load(std::memory_order_relaxed),
        .objects_reclaimed = objects_reclaimed_.load(std::memory_order_relaxed),
        .scan_count = scan_count_.load(std::memory_order_relaxed)
    };
}

template<typename T>
hazard_pointer_domain<T>::~hazard_pointer_domain() {
    // Ensure all objects are reclaimed before domain destruction
    auto& retire_list = get_thread_retire_list();
    retire_list.reclaim_all();
}

template<typename T>
void hazard_pointer_domain<T>::thread_retire_list::add(T* ptr) {
    auto* node = new detail::retire_node(
        static_cast<void*>(ptr),
        [](void* p) { delete static_cast<T*>(p); }
    );

    node->next = head;
    head = node;
    ++count;
}

template<typename T>
size_t hazard_pointer_domain<T>::thread_retire_list::scan_and_reclaim(
    const std::vector<void*>& protected_ptrs) {

    size_t reclaimed = 0;
    detail::retire_node** curr = &head;

    while (*curr) {
        // Check if this pointer is protected
        bool is_protected = false;
        for (void* protected_ptr : protected_ptrs) {
            if ((*curr)->ptr == protected_ptr) {
                is_protected = true;
                break;
            }
        }

        if (!is_protected) {
            // Safe to reclaim
            detail::retire_node* to_delete = *curr;
            *curr = (*curr)->next;

            to_delete->deleter(to_delete->ptr);
            delete to_delete;

            ++reclaimed;
            --count;
        } else {
            // Keep in list
            curr = &(*curr)->next;
        }
    }

    return reclaimed;
}

template<typename T>
void hazard_pointer_domain<T>::thread_retire_list::reclaim_all() {
    // Even during thread cleanup, we must respect hazard pointers from other threads
    // Scan hazard pointers and only reclaim unprotected objects
    auto& registry = detail::hazard_pointer_registry::instance();
    auto protected_ptrs = registry.scan_hazard_pointers();

    detail::retire_node** curr = &head;
    while (*curr) {
        // Check if this pointer is protected by any thread
        bool is_protected = false;
        for (void* protected_ptr : protected_ptrs) {
            if ((*curr)->ptr == protected_ptr) {
                is_protected = true;
                break;
            }
        }

        if (!is_protected) {
            // Safe to reclaim
            detail::retire_node* to_delete = *curr;
            *curr = (*curr)->next;

            to_delete->deleter(to_delete->ptr);
            delete to_delete;
            --count;
        } else {
            // Keep in list - will leak on thread exit, but safe
            curr = &(*curr)->next;
        }
    }
}

template<typename T>
hazard_pointer_domain<T>::thread_retire_list::~thread_retire_list() {
    reclaim_all();
}

} // namespace kcenon::thread
