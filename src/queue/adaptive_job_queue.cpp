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

#include <kcenon/thread/queue/adaptive_job_queue.h>

namespace kcenon::thread {

// ============================================
// Constructor / Destructor
// ============================================

adaptive_job_queue::adaptive_job_queue(policy p)
    : policy_(p)
    , current_mode_(mode::mutex)
    , mutex_queue_(std::make_shared<job_queue>())
    , lockfree_queue_(std::make_unique<lockfree_job_queue>())
    , mode_start_time_(std::chrono::steady_clock::now()) {

    // Set initial mode based on policy
    switch (policy_) {
        case policy::accuracy_first:
            current_mode_.store(mode::mutex, std::memory_order_release);
            break;
        case policy::performance_first:
            current_mode_.store(mode::lock_free, std::memory_order_release);
            break;
        case policy::balanced:
        case policy::manual:
            // Default to mutex mode for balanced and manual
            current_mode_.store(mode::mutex, std::memory_order_release);
            break;
    }
}

adaptive_job_queue::~adaptive_job_queue() {
    // Stop and clear both queues
    stop();

    // Update final mode time
    update_mode_time();
}

// ============================================
// scheduler_interface implementation
// ============================================

auto adaptive_job_queue::schedule(std::unique_ptr<job>&& work) -> common::VoidResult {
    return enqueue(std::move(work));
}

auto adaptive_job_queue::get_next_job() -> common::Result<std::unique_ptr<job>> {
    return dequeue();
}

// ============================================
// Standard queue operations
// ============================================

auto adaptive_job_queue::enqueue(std::unique_ptr<job>&& j) -> common::VoidResult {
    if (stopped_.load(std::memory_order_acquire)) {
        return common::error_info{static_cast<int>(error_code::queue_stopped), "Queue is stopped", "thread_system"};
    }

    if (!j) {
        return common::error_info{static_cast<int>(error_code::invalid_argument), "Cannot enqueue null job", "thread_system"};
    }

    mode current = current_mode_.load(std::memory_order_acquire);
    common::VoidResult result = (current == mode::mutex)
        ? mutex_queue_->enqueue(std::move(j))
        : lockfree_queue_->enqueue(std::move(j));

    if (result.is_ok()) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        ++stats_.enqueue_count;
    }

    return result;
}

auto adaptive_job_queue::dequeue() -> common::Result<std::unique_ptr<job>> {
    if (stopped_.load(std::memory_order_acquire)) {
        return common::error_info{static_cast<int>(error_code::queue_stopped), "Queue is stopped", "thread_system"};
    }

    common::Result<std::unique_ptr<job>> result = common::error_info{static_cast<int>(error_code::queue_empty), "Queue is empty", "thread_system"};
    mode current = current_mode_.load(std::memory_order_acquire);

    // Try current mode first
    if (current == mode::mutex) {
        result = mutex_queue_->try_dequeue();
        // If current mode is empty, also check other queue for race condition handling
        if (result.is_err()) {
            result = lockfree_queue_->dequeue();
        }
    } else {
        result = lockfree_queue_->dequeue();
        // If current mode is empty, also check other queue for race condition handling
        if (result.is_err()) {
            result = mutex_queue_->try_dequeue();
        }
    }

    if (result.is_ok()) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        ++stats_.dequeue_count;
    }

    return result;
}

auto adaptive_job_queue::try_dequeue() -> common::Result<std::unique_ptr<job>> {
    if (stopped_.load(std::memory_order_acquire)) {
        return common::error_info{static_cast<int>(error_code::queue_stopped), "Queue is stopped", "thread_system"};
    }

    common::Result<std::unique_ptr<job>> result = common::error_info{static_cast<int>(error_code::queue_empty), "Queue is empty", "thread_system"};
    mode current = current_mode_.load(std::memory_order_acquire);

    // Try current mode first
    if (current == mode::mutex) {
        result = mutex_queue_->try_dequeue();
        // If current mode is empty, also check other queue for race condition handling
        if (result.is_err()) {
            result = lockfree_queue_->try_dequeue();
        }
    } else {
        result = lockfree_queue_->try_dequeue();
        // If current mode is empty, also check other queue for race condition handling
        if (result.is_err()) {
            result = mutex_queue_->try_dequeue();
        }
    }

    if (result.is_ok()) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        ++stats_.dequeue_count;
    }

    return result;
}

auto adaptive_job_queue::empty() const -> bool {
    // Check both queues to handle race conditions during mode transitions
    // During migration, items may temporarily exist in either queue
    return mutex_queue_->empty() && lockfree_queue_->empty();
}

auto adaptive_job_queue::size() const -> std::size_t {
    mode current = current_mode_.load(std::memory_order_acquire);

    if (current == mode::mutex) {
        return mutex_queue_->size();
    } else {
        return lockfree_queue_->size();
    }
}

auto adaptive_job_queue::clear() -> void {
    std::lock_guard<std::mutex> lock(migration_mutex_);

    mutex_queue_->clear();
    // lockfree_queue doesn't have clear(), drain it
    while (true) {
        auto result = lockfree_queue_->dequeue();
        if (result.is_err()) {
            break;
        }
        // Jobs are destroyed when unique_ptr goes out of scope
    }
}

auto adaptive_job_queue::stop() -> void {
    stopped_.store(true, std::memory_order_release);
    mutex_queue_->stop();
}

auto adaptive_job_queue::is_stopped() const -> bool {
    return stopped_.load(std::memory_order_acquire);
}

// ============================================
// queue_capabilities_interface implementation
// ============================================

auto adaptive_job_queue::get_capabilities() const -> queue_capabilities {
    mode current = current_mode_.load(std::memory_order_acquire);

    if (current == mode::mutex) {
        return queue_capabilities{
            .exact_size = true,
            .atomic_empty_check = true,
            .lock_free = false,
            .wait_free = false,
            .supports_batch = true,
            .supports_blocking_wait = true,
            .supports_stop = true
        };
    } else {
        return queue_capabilities{
            .exact_size = false,
            .atomic_empty_check = false,
            .lock_free = true,
            .wait_free = false,
            .supports_batch = false,
            .supports_blocking_wait = false,
            .supports_stop = true
        };
    }
}

// ============================================
// Adaptive-specific API
// ============================================

auto adaptive_job_queue::current_mode() const -> mode {
    return current_mode_.load(std::memory_order_acquire);
}

auto adaptive_job_queue::current_policy() const -> policy {
    return policy_;
}

auto adaptive_job_queue::switch_mode(mode m) -> common::VoidResult {
    if (policy_ != policy::manual) {
        return common::error_info{static_cast<int>(error_code::invalid_argument),
            "Mode switching is only allowed with manual policy", "thread_system"};
    }

    if (current_mode_.load(std::memory_order_acquire) == m) {
        return common::ok(); // Already in target mode
    }

    migrate_to_mode(m);
    return common::ok();
}

auto adaptive_job_queue::get_stats() const -> stats {
    std::lock_guard<std::mutex> lock(stats_mutex_);

    // Create a copy of stats with updated time
    stats result = stats_;

    // Add current mode time
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - mode_start_time_).count();

    if (current_mode_.load(std::memory_order_acquire) == mode::mutex) {
        result.time_in_mutex_ms += duration;
    } else {
        result.time_in_lockfree_ms += duration;
    }

    return result;
}

auto adaptive_job_queue::require_accuracy() -> accuracy_guard {
    return accuracy_guard(*this);
}

// ============================================
// accuracy_guard implementation
// ============================================

adaptive_job_queue::accuracy_guard::accuracy_guard(adaptive_job_queue& queue)
    : queue_(&queue)
    , previous_mode_(queue.current_mode())
    , active_(true) {

    // Increment guard count
    int prev_count = queue_->accuracy_guard_count_.fetch_add(1, std::memory_order_acq_rel);

    // If this is the first guard and not already in mutex mode, switch
    if (prev_count == 0 && previous_mode_ != mode::mutex) {
        queue_->migrate_to_mode(mode::mutex);
    }
}

adaptive_job_queue::accuracy_guard::~accuracy_guard() {
    if (!active_ || !queue_) {
        return;
    }

    // Decrement guard count
    int prev_count = queue_->accuracy_guard_count_.fetch_sub(1, std::memory_order_acq_rel);

    // If this was the last guard and we should restore previous mode
    if (prev_count == 1) {
        // Restore based on policy
        switch (queue_->policy_) {
            case policy::accuracy_first:
                // Stay in mutex mode
                break;
            case policy::performance_first:
                queue_->migrate_to_mode(mode::lock_free);
                break;
            case policy::balanced:
                // Let balanced policy decide
                {
                    mode target = queue_->determine_mode_for_balanced();
                    if (target != mode::mutex) {
                        queue_->migrate_to_mode(target);
                    }
                }
                break;
            case policy::manual:
                // Restore previous mode
                if (previous_mode_ != mode::mutex) {
                    queue_->migrate_to_mode(previous_mode_);
                }
                break;
        }
    }
}

adaptive_job_queue::accuracy_guard::accuracy_guard(accuracy_guard&& other) noexcept
    : queue_(other.queue_)
    , previous_mode_(other.previous_mode_)
    , active_(other.active_) {
    other.active_ = false;
    other.queue_ = nullptr;
}

// ============================================
// Private methods
// ============================================

void adaptive_job_queue::migrate_to_mode(mode target) {
    std::lock_guard<std::mutex> lock(migration_mutex_);

    mode current = current_mode_.load(std::memory_order_acquire);
    if (current == target) {
        return; // Already in target mode
    }

    // Update time tracking before mode change
    update_mode_time();

    // IMPORTANT: Update mode FIRST, then drain the old queue.
    // This ensures new enqueues go to the target queue while we drain,
    // preventing an infinite drain loop where the producer keeps adding
    // to the source queue faster than we can drain it.
    current_mode_.store(target, std::memory_order_release);

    if (target == mode::mutex) {
        // Migrate from lock-free to mutex
        // Drain remaining jobs from lockfree queue and move to mutex queue
        while (true) {
            auto result = lockfree_queue_->dequeue();
            if (result.is_err()) {
                break;
            }
            (void)mutex_queue_->enqueue(std::move(result.value()));
        }
    } else {
        // Migrate from mutex to lock-free
        // Drain mutex queue and move jobs to lockfree queue
        auto jobs = mutex_queue_->dequeue_batch();
        for (auto& job : jobs) {
            (void)lockfree_queue_->enqueue(std::move(job));
        }
    }

    // Update statistics
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        ++stats_.mode_switches;
    }

    // Reset mode start time
    mode_start_time_ = std::chrono::steady_clock::now();
}

void adaptive_job_queue::update_mode_time() {
    std::lock_guard<std::mutex> lock(stats_mutex_);

    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - mode_start_time_).count();

    if (current_mode_.load(std::memory_order_acquire) == mode::mutex) {
        stats_.time_in_mutex_ms += duration;
    } else {
        stats_.time_in_lockfree_ms += duration;
    }

    mode_start_time_ = now;
}

auto adaptive_job_queue::determine_mode_for_balanced() const -> mode {
    // For balanced policy, use heuristics based on recent activity
    // Current simple heuristic: prefer lock-free for high throughput

    std::lock_guard<std::mutex> lock(stats_mutex_);

    // If we've processed a lot of jobs, prefer lock-free for performance
    uint64_t total_ops = stats_.enqueue_count + stats_.dequeue_count;
    if (total_ops > 10000) {
        return mode::lock_free;
    }

    // For lower throughput, prefer mutex for accuracy
    return mode::mutex;
}

} // namespace kcenon::thread
