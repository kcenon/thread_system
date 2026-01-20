/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, kcenon
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

#include <kcenon/thread/metrics/metrics_service.h>

#include <stdexcept>

namespace kcenon::thread::metrics {

metrics_service::metrics_service()
    : basic_metrics_(std::make_shared<ThreadPoolMetrics>()) {
}

void metrics_service::record_submission(std::size_t count) {
    basic_metrics_->record_submission(count);

    if (enhanced_enabled_.load(std::memory_order_relaxed) && enhanced_metrics_) {
        for (std::size_t i = 0; i < count; ++i) {
            enhanced_metrics_->record_submission();
        }
    }
}

void metrics_service::record_enqueue(std::size_t count) {
    basic_metrics_->record_enqueue(count);
}

void metrics_service::record_enqueue_with_latency(
    std::chrono::nanoseconds latency,
    std::size_t count) {
    basic_metrics_->record_enqueue(count);

    if (enhanced_enabled_.load(std::memory_order_relaxed) && enhanced_metrics_) {
        // Distribute latency across batch items for more accurate per-job tracking
        auto per_item_latency = latency / static_cast<long>(count);
        for (std::size_t i = 0; i < count; ++i) {
            enhanced_metrics_->record_enqueue(per_item_latency);
        }
    }
}

void metrics_service::record_execution(std::uint64_t duration_ns, bool success) {
    basic_metrics_->record_execution(duration_ns, success);

    if (enhanced_enabled_.load(std::memory_order_relaxed) && enhanced_metrics_) {
        enhanced_metrics_->record_execution(
            std::chrono::nanoseconds{duration_ns}, success);
    }
}

void metrics_service::record_execution_with_wait_time(
    std::chrono::nanoseconds duration,
    std::chrono::nanoseconds wait_time,
    bool success) {
    basic_metrics_->record_execution(
        static_cast<std::uint64_t>(duration.count()), success);

    if (enhanced_enabled_.load(std::memory_order_relaxed) && enhanced_metrics_) {
        enhanced_metrics_->record_execution(duration, success);
        enhanced_metrics_->record_wait_time(wait_time);
    }
}

void metrics_service::record_idle_time(std::uint64_t duration_ns) {
    basic_metrics_->record_idle_time(duration_ns);
}

void metrics_service::record_queue_depth(std::size_t depth) {
    if (enhanced_enabled_.load(std::memory_order_relaxed) && enhanced_metrics_) {
        enhanced_metrics_->record_queue_depth(depth);
    }
}

void metrics_service::record_worker_state(
    std::size_t worker_id,
    bool busy,
    std::uint64_t duration_ns) {
    if (enhanced_enabled_.load(std::memory_order_relaxed) && enhanced_metrics_) {
        enhanced_metrics_->record_worker_state(worker_id, busy, duration_ns);
    }
}

void metrics_service::set_enhanced_metrics_enabled(bool enabled, std::size_t worker_count) {
    if (enabled && !enhanced_metrics_) {
        std::scoped_lock<std::mutex> lock(init_mutex_);
        // Double-check after acquiring lock
        if (!enhanced_metrics_) {
            enhanced_metrics_ = std::make_shared<EnhancedThreadPoolMetrics>(worker_count);
            enhanced_metrics_->set_active_workers(worker_count);
        }
    }
    enhanced_enabled_.store(enabled, std::memory_order_release);
}

bool metrics_service::is_enhanced_metrics_enabled() const {
    return enhanced_enabled_.load(std::memory_order_acquire);
}

void metrics_service::update_worker_count(std::size_t count) {
    if (enhanced_metrics_) {
        enhanced_metrics_->update_worker_count(count);
    }
}

void metrics_service::set_active_workers(std::size_t count) {
    if (enhanced_metrics_) {
        enhanced_metrics_->set_active_workers(count);
    }
}

const ThreadPoolMetrics& metrics_service::basic_metrics() const noexcept {
    return *basic_metrics_;
}

const EnhancedThreadPoolMetrics& metrics_service::enhanced_metrics() const {
    if (!enhanced_metrics_) {
        throw std::runtime_error(
            "Enhanced metrics is not enabled. "
            "Call set_enhanced_metrics_enabled(true) first.");
    }
    return *enhanced_metrics_;
}

EnhancedSnapshot metrics_service::enhanced_snapshot() const {
    if (!enhanced_enabled_.load(std::memory_order_acquire) || !enhanced_metrics_) {
        return EnhancedSnapshot{};
    }
    return enhanced_metrics_->snapshot();
}

std::shared_ptr<ThreadPoolMetrics> metrics_service::get_basic_metrics() const noexcept {
    return basic_metrics_;
}

void metrics_service::reset() {
    basic_metrics_->reset();
    if (enhanced_metrics_) {
        enhanced_metrics_->reset();
    }
}

} // namespace kcenon::thread::metrics
