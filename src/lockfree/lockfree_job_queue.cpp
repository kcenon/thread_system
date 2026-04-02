// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#include <kcenon/thread/lockfree/lockfree_job_queue.h>

#include <algorithm>
#include <chrono>
#include <thread>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

namespace kcenon::thread::detail {

namespace {

// Exponential backoff for CAS retry loops.
// Phase 1 (retry < 16): CPU pause (x86) or yield (other architectures)
// Phase 2 (retry < 64): thread yield to let other threads progress
// Phase 3 (retry >= 64): short sleep with exponential increase (capped at ~1ms)
inline void backoff(int retry) {
    if (retry < 16) {
#if defined(__x86_64__) || defined(_M_X64)
        _mm_pause();
#else
        std::this_thread::yield();
#endif
    } else if (retry < 64) {
        std::this_thread::yield();
    } else {
        std::this_thread::sleep_for(
            std::chrono::microseconds(1 << std::min(retry - 64, 10)));
    }
}

}  // namespace

// Constructor: Initialize with a dummy node
lockfree_job_queue::lockfree_job_queue()
    : pool_(std::make_shared<node_pool>()) {
    // Create dummy node (Michael-Scott algorithm requires one dummy node)
    // This simplifies the algorithm by ensuring head and tail are never null
    node* dummy = new node();

    head_.store(dummy, std::memory_order_relaxed);
    tail_.store(dummy, std::memory_order_relaxed);
    approximate_size_.store(0, std::memory_order_relaxed);
}

// Destructor: Drain queue and delete all nodes safely
lockfree_job_queue::~lockfree_job_queue() {
    // Signal shutdown to prevent new operations
    shutdown_.store(true, std::memory_order_release);

    // Drain remaining jobs (release ownership)
    while (true) {
        auto result = dequeue();
        if (result.is_err()) {
            break;
        }
        // Jobs are destroyed when unique_ptr goes out of scope
    }

    // Safe cleanup: acquire semantics ensure we see all writes
    node* dummy = head_.load(std::memory_order_acquire);

    // Retire dummy node through pool-aware reclamation
    // This ensures the node is only deleted when no other thread
    // holds a hazard pointer to it (uses explicit memory ordering)
    retire_node(dummy);
}

// Enqueue operation (Michael-Scott algorithm)
auto lockfree_job_queue::enqueue(std::unique_ptr<job>&& job_ptr) -> common::VoidResult {
    if (!job_ptr) {
        return common::error_info{static_cast<int>(error_code::invalid_argument), "Cannot enqueue null job", "thread_system"};
    }

    // Acquire node from pool (reuses retired nodes, falls back to new)
    node* new_node = pool_->acquire(std::move(job_ptr));

    // Acquire hazard pointer guard for tail protection (uses safe memory ordering)
    safe_hazard_guard hp_tail;

    // Limit retries to prevent infinite loop during concurrent mode switching
    constexpr int MAX_RETRIES = 1000;

    for (int retry = 0; retry < MAX_RETRIES; ++retry) {
        // Read current tail
        node* tail = tail_.load(std::memory_order_acquire);

        // Protect tail to ensure it's not reclaimed while we read next
        hp_tail.protect(tail);

        // Verify tail hasn't changed (if it changed, our protection might be on the wrong node)
        if (tail != tail_.load(std::memory_order_acquire)) {
            backoff(retry);
            continue;
        }

        node* next = tail->next.load(std::memory_order_acquire);

        // Check if tail is still consistent
        if (tail == tail_.load(std::memory_order_acquire)) {
            if (next == nullptr) {
                // Tail is pointing to the last node, try to link new node
                if (tail->next.compare_exchange_weak(
                        next, new_node,
                        std::memory_order_release,
                        std::memory_order_relaxed)) {

                    // Successfully linked, try to swing tail (best effort)
                    tail_.compare_exchange_weak(
                        tail, new_node,
                        std::memory_order_release,
                        std::memory_order_relaxed);

                    // Update size (relaxed - just for monitoring)
                    approximate_size_.fetch_add(1, std::memory_order_relaxed);

                    return common::ok();  // Success
                }
                backoff(retry);
            } else {
                // Tail is behind, try to advance it
                tail_.compare_exchange_weak(
                    tail, next,
                    std::memory_order_release,
                    std::memory_order_relaxed);
            }
        } else {
            backoff(retry);
        }
    }

    // If we exhausted retries, return node to pool and report error
    pool_->release(new_node);
    return common::error_info{static_cast<int>(error_code::queue_busy), "Queue is busy, retry later", "thread_system"};
}

// Dequeue operation (Michael-Scott algorithm with Safe Hazard Pointers)
auto lockfree_job_queue::dequeue() -> common::Result<std::unique_ptr<job>> {
    // Acquire hazard pointer guards for protecting nodes (uses safe memory ordering)
    safe_hazard_guard hp_head;
    safe_hazard_guard hp_next;

    // Limit retries to prevent infinite loop during concurrent mode switching
    // This is important for adaptive_job_queue which may switch modes while
    // dequeue is in progress
    constexpr int MAX_OUTER_RETRIES = 100;
    constexpr int MAX_INNER_RETRIES = 10;

    for (int outer_retry = 0; outer_retry < MAX_OUTER_RETRIES; ++outer_retry) {
        // Protect head from reclamation
        node* head = head_.load(std::memory_order_acquire);
        hp_head.protect(head);

        // Verify head hasn't changed (ABA protection)
        if (head != head_.load(std::memory_order_acquire)) {
            backoff(outer_retry);
            continue;  // Head changed, retry
        }

        node* tail = tail_.load(std::memory_order_acquire);

        // Protect next node using loop until stable (with retry limit)
        node* next = nullptr;
        bool next_stable = false;
        for (int inner_retry = 0; inner_retry < MAX_INNER_RETRIES; ++inner_retry) {
            next = head->next.load(std::memory_order_acquire);
            if (next == nullptr) {
                next_stable = true;
                break;  // No next node
            }

            hp_next.protect(next);

            // Verify next is still the same after protection
            if (next == head->next.load(std::memory_order_acquire)) {
                next_stable = true;
                break;  // Stable, protected
            }
            backoff(inner_retry);
            // Next changed, retry protection
        }

        // If we couldn't stabilize next pointer, retry outer loop
        if (!next_stable) {
            backoff(outer_retry);
            continue;
        }

        // Check if head is still consistent
        if (head == head_.load(std::memory_order_acquire)) {
            if (head == tail) {
                if (next == nullptr) {
                    // Queue is empty
                    return common::error_info{static_cast<int>(error_code::queue_empty), "Queue is empty", "thread_system"};
                }

                // Tail is behind, try to advance it
                tail_.compare_exchange_weak(
                    tail, next,
                    std::memory_order_release,
                    std::memory_order_relaxed);
            } else {
                if (next == nullptr) {
                    // Inconsistent state, retry
                    backoff(outer_retry);
                    continue;
                }

                // Try to swing head to next
                if (head_.compare_exchange_weak(
                        head, next,
                        std::memory_order_release,
                        std::memory_order_relaxed)) {

                    // Successfully dequeued - now safe to read data
                    // We now own the old head node exclusively
                    std::unique_ptr<job> job_data = std::move(next->data);

                    // Retire the old head node for later reclamation (safe memory ordering)
                    // Reclaimed nodes are returned to the pool instead of deleted
                    retire_node(head);

                    // Update size (relaxed - just for monitoring)
                    approximate_size_.fetch_sub(1, std::memory_order_relaxed);

                    // Return the job data
                    return std::move(job_data);
                }
                backoff(outer_retry);
            }
        } else {
            backoff(outer_retry);
        }
    }

    // If we exhausted retries, report queue as empty
    // This is safe because the caller will retry if needed
    return common::error_info{static_cast<int>(error_code::queue_empty), "Queue is empty", "thread_system"};
}

// Check if queue is empty
auto lockfree_job_queue::empty() const -> bool {
    // Use hazard pointer protection to safely access head node
    // This prevents UAF if another thread retires the head during our check
    safe_hazard_guard hp_head;

    // Try to get a stable read of head
    // If head keeps changing due to concurrent modifications, retry a few times
    constexpr int MAX_RETRIES = 10;
    for (int retry = 0; retry < MAX_RETRIES; ++retry) {
        node* head = head_.load(std::memory_order_acquire);
        hp_head.protect(head);

        // Verify head hasn't changed after protection
        if (head != head_.load(std::memory_order_acquire)) {
            continue;  // Head changed, retry
        }

        node* next = head->next.load(std::memory_order_acquire);

        // Queue is empty if head->next is null
        return next == nullptr;
    }

    // If we exhausted retries, do one final check without verification
    // This handles the edge case where head keeps changing but we need a definitive answer
    node* head = head_.load(std::memory_order_acquire);
    hp_head.protect(head);
    node* next = head->next.load(std::memory_order_acquire);
    return next == nullptr;
}

// Get approximate size
auto lockfree_job_queue::size() const -> std::size_t {
    // Return cached size (may not be exact due to concurrent modifications)
    return approximate_size_.load(std::memory_order_relaxed);
}

// Retire a node through hazard pointers, recycling via pool on reclamation
void lockfree_job_queue::retire_node(node* n) {
    if (!n) return;

    // Capture pool by shared_ptr so the closure remains valid even if the
    // queue is destroyed before the hazard pointer domain reclaims this node
    std::shared_ptr<node_pool> pool = pool_;

    safe_hazard_pointer_domain::instance().retire(
        n,
        [pool](void* ptr) {
            pool->release(static_cast<node*>(ptr));
        }
    );
}

}  // namespace kcenon::thread::detail
