// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#include "kcenon/thread/core/hazard_pointer.h"

#include <algorithm>
#include <mutex>
#include <stdexcept>

namespace kcenon::thread {

namespace detail {

// Global registry singleton
hazard_pointer_registry& hazard_pointer_registry::instance() {
    static hazard_pointer_registry registry;
    return registry;
}

// Get or create thread-local hazard list
thread_hazard_list* hazard_pointer_registry::get_thread_list() {
    // Thread-local storage for this thread's hazard list
    static thread_local thread_hazard_list* thread_list = nullptr;

    if (thread_list == nullptr) {
        // Try to reuse an inactive list first to prevent unbounded memory growth
        thread_hazard_list* curr = head_.load(std::memory_order_acquire);
        while (curr) {
            bool expected = false;
            // Try to claim an inactive list
            // Use acq_rel: acquire to see prior hazard pointer clears,
            // release to publish active=true to scanning threads
            if (curr->active.compare_exchange_strong(expected, true, std::memory_order_acq_rel,
                                                     std::memory_order_relaxed)) {
                thread_list = curr;
                thread_count_.fetch_add(1, std::memory_order_relaxed);
                break;
            }
            curr = curr->next;
        }

        // If no inactive list found, allocate a new one
        if (thread_list == nullptr) {
            thread_list = new thread_hazard_list();

            // Add to global linked list
            // Use acq_rel on CAS: acquire to read current list, release to
            // publish the new node so that scanning threads see it
            thread_hazard_list* old_head = head_.load(std::memory_order_acquire);
            do {
                thread_list->next = old_head;
            } while (!head_.compare_exchange_weak(old_head, thread_list, std::memory_order_acq_rel,
                                                  std::memory_order_relaxed));

            thread_count_.fetch_add(1, std::memory_order_relaxed);
        }

        // Register thread cleanup
        static thread_local struct thread_cleanup {
            thread_hazard_list* list;

            ~thread_cleanup() {
                if (list) {
                    hazard_pointer_registry::instance().mark_inactive();
                }
            }
        } cleanup{thread_list};
    }

    return thread_list;
}

// Mark current thread's list as inactive
void hazard_pointer_registry::mark_inactive() {
    static thread_local thread_hazard_list* thread_list = get_thread_list();

    if (thread_list) {
        // Clear all hazard pointers
        for (auto& h : thread_list->hazards) {
            h.store(nullptr, std::memory_order_release);
        }

        // Mark as inactive (using release to ensure visibility)
        thread_list->active.store(false, std::memory_order_release);
        thread_count_.fetch_sub(1, std::memory_order_relaxed);
    }
}

// Scan all hazard pointers and collect protected pointers
std::vector<void*> hazard_pointer_registry::scan_hazard_pointers() {
    std::vector<void*> protected_ptrs;
    protected_ptrs.reserve(256);  // Pre-allocate reasonable size

    // Marker value for owned but not protecting slots
    const void* SLOT_OWNED_MARKER = reinterpret_cast<void*>(0x1);

    // Periodically clean up inactive thread lists
    // Use scan counter to avoid overhead on every scan
    static thread_local size_t scan_counter = 0;
    static constexpr size_t CLEANUP_INTERVAL = 100;  // Clean every 100 scans
    bool should_cleanup = (++scan_counter % CLEANUP_INTERVAL == 0);

    // Traverse all thread lists
    thread_hazard_list* curr = head_.load(std::memory_order_acquire);
    size_t inactive_count = 0;

    while (curr) {
        // IMPORTANT: Scan ALL records regardless of active status.
        // A thread may be in the process of setting a hazard pointer before
        // setting active=true (race window in get_thread_list). If we skip
        // inactive records, we might miss a valid hazard pointer and
        // prematurely reclaim a protected node.
        // This matches the approach used in safe_hazard_pointer.h.
        for (auto& hazard : curr->hazards) {
            void* ptr = hazard.load(std::memory_order_acquire);
            // Only add if it's a real pointer (not nullptr or SLOT_OWNED_MARKER)
            if (ptr != nullptr && ptr != SLOT_OWNED_MARKER) {
                protected_ptrs.push_back(ptr);
            }
        }

        if (!curr->active.load(std::memory_order_acquire)) {
            ++inactive_count;
        }

        curr = curr->next;
    }

    // Sort for efficient binary search in scan_and_reclaim
    // O(N log N) but enables O(log N) lookups later
    std::sort(protected_ptrs.begin(), protected_ptrs.end());

    // Remove duplicates to minimize search space
    // Multiple threads may protect the same pointer
    protected_ptrs.erase(std::unique(protected_ptrs.begin(), protected_ptrs.end()),
                         protected_ptrs.end());

    return protected_ptrs;
}

// Get total number of active threads
size_t hazard_pointer_registry::get_active_thread_count() const {
    return thread_count_.load(std::memory_order_relaxed);
}

// Global reclamation manager implementation
global_reclamation_manager& global_reclamation_manager::instance() {
    static global_reclamation_manager manager;
    return manager;
}

void global_reclamation_manager::add_orphaned_nodes(retire_node* head, size_t count) {
    if (!head)
        return;

    // Find tail of the new list
    retire_node* tail = head;
    while (tail->next) {
        tail = tail->next;
    }

    // Atomically prepend to the global list
    // Use acq_rel: acquire to see the current list, release to publish
    // the new nodes so that reclaim() sees them
    retire_node* old_head = head_.load(std::memory_order_acquire);
    do {
        tail->next = old_head;
    } while (!head_.compare_exchange_weak(old_head, head, std::memory_order_acq_rel,
                                          std::memory_order_relaxed));

    count_.fetch_add(count, std::memory_order_relaxed);
}

size_t global_reclamation_manager::reclaim(const std::vector<void*>& protected_ptrs) {
    // Take the entire list to process
    // Use acq_rel: acquire to see all node data, release to publish
    // the nullptr so concurrent add_orphaned_nodes sees it
    retire_node* curr = head_.exchange(nullptr, std::memory_order_acq_rel);
    if (!curr)
        return 0;

    size_t reclaimed = 0;
    retire_node* keep_head = nullptr;
    retire_node* keep_tail = nullptr;
    size_t keep_count = 0;

    while (curr) {
        retire_node* next = curr->next;
        bool is_protected = false;

        // Check if protected
        // Binary search since protected_ptrs is sorted
        if (std::binary_search(protected_ptrs.begin(), protected_ptrs.end(), curr->ptr)) {
            is_protected = true;
        }

        if (!is_protected) {
            // Safe to delete
            curr->deleter(curr->ptr);
            delete curr;
            ++reclaimed;
        } else {
            // Keep node
            if (!keep_head) {
                keep_head = curr;
                keep_tail = curr;
            } else {
                keep_tail->next = curr;
                keep_tail = curr;
            }
            curr->next = nullptr;
            ++keep_count;
        }

        curr = next;
    }

    // Subtract reclaimed count (not kept, since kept nodes will be re-added
    // via add_orphaned_nodes which increments count_ itself)
    // We need to subtract the total taken (reclaimed + keep_count) because
    // add_orphaned_nodes will add keep_count back
    count_.fetch_sub(reclaimed + keep_count, std::memory_order_relaxed);

    // Add back kept nodes (this will add keep_count to count_)
    if (keep_head) {
        add_orphaned_nodes(keep_head, keep_count);
    }

    return reclaimed;
}

size_t global_reclamation_manager::get_orphaned_count() const {
    return count_.load(std::memory_order_relaxed);
}

}  // namespace detail

// hazard_pointer implementation

hazard_pointer::hazard_pointer() : slot_(nullptr), slot_index_(0) {
    auto* thread_list = detail::hazard_pointer_registry::instance().get_thread_list();

    // Find an available slot (slot value is nullptr means available)
    for (size_t i = 0; i < detail::thread_hazard_list::MAX_HAZARDS_PER_THREAD; ++i) {
        void* expected = nullptr;
        // Try to claim this slot with SLOT_OWNED_MARKER
        // Use acq_rel: acquire to synchronize with prior release of the slot,
        // release to publish ownership to scanning threads
        if (thread_list->hazards[i].compare_exchange_strong(
                expected, const_cast<void*>(SLOT_OWNED_MARKER), std::memory_order_acq_rel,
                std::memory_order_relaxed)) {
            slot_ = &thread_list->hazards[i];
            slot_index_ = i;
            // Slot is now owned with SLOT_OWNED_MARKER
            return;
        }
    }

    throw std::runtime_error("Hazard pointer slots exhausted");
}

hazard_pointer::hazard_pointer(hazard_pointer&& other) noexcept
    : slot_(other.slot_), slot_index_(other.slot_index_) {
    other.slot_ = nullptr;
    other.slot_index_ = 0;
}

hazard_pointer& hazard_pointer::operator=(hazard_pointer&& other) noexcept {
    if (this != &other) {
        reset();

        slot_ = other.slot_;
        slot_index_ = other.slot_index_;

        other.slot_ = nullptr;
        other.slot_index_ = 0;
    }
    return *this;
}

hazard_pointer::~hazard_pointer() {
    if (slot_) {
        // Release the slot back to the pool
        slot_->store(nullptr, std::memory_order_release);
    }
}

void hazard_pointer::reset() noexcept {
    if (slot_) {
        // Clear protection but keep slot owned
        slot_->store(const_cast<void*>(SLOT_OWNED_MARKER), std::memory_order_release);
    }
}

bool hazard_pointer::is_protected() const noexcept {
    if (!slot_) {
        return false;
    }
    void* ptr = slot_->load(std::memory_order_acquire);
    return ptr != nullptr && ptr != SLOT_OWNED_MARKER;
}

void* hazard_pointer::get_protected() const noexcept {
    if (!slot_) {
        return nullptr;
    }
    void* ptr = slot_->load(std::memory_order_acquire);
    // Return nullptr if slot is just owned but not protecting anything
    return (ptr == SLOT_OWNED_MARKER) ? nullptr : ptr;
}

}  // namespace kcenon::thread
