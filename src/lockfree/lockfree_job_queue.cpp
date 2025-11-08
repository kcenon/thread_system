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

#include <kcenon/thread/lockfree/lockfree_job_queue.h>

namespace kcenon::thread {

// Constructor: Initialize with a dummy node
lockfree_job_queue::lockfree_job_queue() {
    // Create dummy node (Michael-Scott algorithm requires one dummy node)
    // This simplifies the algorithm by ensuring head and tail are never null
    node* dummy = new node(nullptr);

    head_.store(dummy, std::memory_order_relaxed);
    tail_.store(dummy, std::memory_order_relaxed);
    approximate_size_.store(0, std::memory_order_relaxed);
}

// Destructor: Drain queue and delete all nodes
lockfree_job_queue::~lockfree_job_queue() {
    // Drain remaining jobs (release ownership)
    while (auto result = dequeue()) {
        // Jobs are destroyed when unique_ptr goes out of scope
    }

    // Delete the dummy node
    node* dummy = head_.load(std::memory_order_relaxed);
    delete dummy;
}

// Enqueue operation (Michael-Scott algorithm)
auto lockfree_job_queue::enqueue(std::unique_ptr<job>&& job_ptr) -> result_void {
    if (!job_ptr) {
        return result_void(error(error_code::invalid_argument, "Cannot enqueue null job"));
    }

    // Allocate new node with the job
    node* new_node = new node(std::move(job_ptr));

    while (true) {
        // Read current tail
        node* tail = tail_.load(std::memory_order_acquire);
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

                    return result_void();  // Success
                }
            } else {
                // Tail is behind, try to advance it
                tail_.compare_exchange_weak(
                    tail, next,
                    std::memory_order_release,
                    std::memory_order_relaxed);
            }
        }
    }
}

// Dequeue operation (Michael-Scott algorithm with Hazard Pointers)
auto lockfree_job_queue::dequeue() -> result<std::unique_ptr<job>> {
    // Acquire hazard pointers for protecting nodes
    auto hp_head = node_hp_domain::global().acquire();
    auto hp_next = node_hp_domain::global().acquire();

    while (true) {
        // Protect head from reclamation
        node* head = head_.load(std::memory_order_acquire);
        hp_head.protect(head);

        // Verify head hasn't changed (ABA protection)
        if (head != head_.load(std::memory_order_acquire)) {
            continue;  // Head changed, retry
        }

        node* tail = tail_.load(std::memory_order_acquire);

        // Protect next node using loop until stable
        node* next = nullptr;
        while (true) {
            next = head->next.load(std::memory_order_acquire);
            if (next == nullptr) {
                break;  // No next node
            }

            hp_next.protect(next);

            // Verify next is still the same after protection
            if (next == head->next.load(std::memory_order_acquire)) {
                break;  // Stable, protected
            }
            // Next changed, retry protection
        }

        // Check if head is still consistent
        if (head == head_.load(std::memory_order_acquire)) {
            if (head == tail) {
                if (next == nullptr) {
                    // Queue is empty
                    return result<std::unique_ptr<job>>(
                        error(error_code::queue_empty, "Queue is empty"));
                }

                // Tail is behind, try to advance it
                tail_.compare_exchange_weak(
                    tail, next,
                    std::memory_order_release,
                    std::memory_order_relaxed);
            } else {
                if (next == nullptr) {
                    // Inconsistent state, retry
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

                    // Retire the old head node for later reclamation
                    node_hp_domain::global().retire(head);

                    // Update size (relaxed - just for monitoring)
                    approximate_size_.fetch_sub(1, std::memory_order_relaxed);

                    // Return the job data
                    return result<std::unique_ptr<job>>(std::move(job_data));
                }
            }
        }
    }
}

// Check if queue is empty
auto lockfree_job_queue::empty() const -> bool {
    node* head = head_.load(std::memory_order_acquire);
    node* next = head->next.load(std::memory_order_acquire);

    // Queue is empty if head->next is null
    return next == nullptr;
}

// Get approximate size
auto lockfree_job_queue::size() const -> std::size_t {
    // Return cached size (may not be exact due to concurrent modifications)
    return approximate_size_.load(std::memory_order_relaxed);
}

} // namespace kcenon::thread
