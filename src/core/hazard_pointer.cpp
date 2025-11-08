#include "kcenon/thread/core/hazard_pointer.h"

#include <algorithm>
#include <mutex>

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
        // Allocate new hazard list for this thread
        thread_list = new thread_hazard_list();

        // Add to global linked list
        thread_hazard_list* old_head = head_.load(std::memory_order_relaxed);
        do {
            thread_list->next = old_head;
        } while (!head_.compare_exchange_weak(
            old_head, thread_list,
            std::memory_order_release,
            std::memory_order_relaxed));

        thread_count_.fetch_add(1, std::memory_order_relaxed);

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
    static thread_local thread_hazard_list* thread_list =
        get_thread_list();

    if (thread_list) {
        // Clear all hazard pointers
        for (auto& h : thread_list->hazards) {
            h.store(nullptr, std::memory_order_release);
        }

        // Mark as inactive
        thread_list->active = false;
        thread_count_.fetch_sub(1, std::memory_order_relaxed);
    }
}

// Scan all hazard pointers and collect protected pointers
std::vector<void*> hazard_pointer_registry::scan_hazard_pointers() {
    std::vector<void*> protected_ptrs;
    protected_ptrs.reserve(256);  // Pre-allocate reasonable size

    // Marker value for owned but not protecting slots
    const void* SLOT_OWNED_MARKER = reinterpret_cast<void*>(0x1);

    // Traverse all thread lists
    thread_hazard_list* curr = head_.load(std::memory_order_acquire);

    while (curr) {
        if (curr->active) {
            // Scan this thread's hazard pointers
            for (auto& hazard : curr->hazards) {
                void* ptr = hazard.load(std::memory_order_acquire);
                // Only add if it's a real pointer (not nullptr or SLOT_OWNED_MARKER)
                if (ptr != nullptr && ptr != SLOT_OWNED_MARKER) {
                    protected_ptrs.push_back(ptr);
                }
            }
        }

        curr = curr->next;
    }

    // Sort for binary search (optimization)
    std::sort(protected_ptrs.begin(), protected_ptrs.end());

    // Remove duplicates
    protected_ptrs.erase(
        std::unique(protected_ptrs.begin(), protected_ptrs.end()),
        protected_ptrs.end());

    return protected_ptrs;
}

// Get total number of active threads
size_t hazard_pointer_registry::get_active_thread_count() const {
    return thread_count_.load(std::memory_order_relaxed);
}

} // namespace detail

// hazard_pointer implementation

hazard_pointer::hazard_pointer()
    : slot_(nullptr)
    , slot_index_(0) {

    auto* thread_list = detail::hazard_pointer_registry::instance().get_thread_list();

    // Find an available slot (slot value is nullptr means available)
    for (size_t i = 0; i < detail::thread_hazard_list::MAX_HAZARDS_PER_THREAD; ++i) {
        void* expected = nullptr;
        // Try to claim this slot with SLOT_OWNED_MARKER
        if (thread_list->hazards[i].compare_exchange_strong(
            expected, const_cast<void*>(SLOT_OWNED_MARKER),
            std::memory_order_acquire,
            std::memory_order_relaxed)) {

            slot_ = &thread_list->hazards[i];
            slot_index_ = i;
            // Slot is now owned with SLOT_OWNED_MARKER
            return;
        }
    }

    // All slots in use - this should be rare
    // In a production system, might want to dynamically allocate more
}

hazard_pointer::hazard_pointer(hazard_pointer&& other) noexcept
    : slot_(other.slot_)
    , slot_index_(other.slot_index_) {

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

} // namespace kcenon::thread
