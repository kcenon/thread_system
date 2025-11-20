// BSD 3-Clause License
// Copyright (c) 2025

#pragma once

#include <atomic>
#include <cstdint>

namespace kcenon::thread::metrics {

/**
 * @brief Lightweight metrics container shared between thread_pool and workers.
 */
class ThreadPoolMetrics {
public:
    struct Snapshot {
        std::uint64_t tasks_submitted;
        std::uint64_t tasks_enqueued;
        std::uint64_t tasks_executed;
        std::uint64_t tasks_failed;
        std::uint64_t total_busy_time_ns;
        std::uint64_t total_idle_time_ns;
    };

    void record_submission(std::size_t count = 1) {
        tasks_submitted_.fetch_add(count, std::memory_order_relaxed);
    }

    void record_enqueue(std::size_t count = 1) {
        tasks_enqueued_.fetch_add(count, std::memory_order_relaxed);
    }

    void record_execution(std::uint64_t duration_ns, bool success) {
        if (success) {
            tasks_executed_.fetch_add(1, std::memory_order_relaxed);
        } else {
            tasks_failed_.fetch_add(1, std::memory_order_relaxed);
        }
        total_busy_time_ns_.fetch_add(duration_ns, std::memory_order_relaxed);
    }

    void record_idle_time(std::uint64_t duration_ns) {
        total_idle_time_ns_.fetch_add(duration_ns, std::memory_order_relaxed);
    }

    void reset() {
        tasks_submitted_.store(0, std::memory_order_relaxed);
        tasks_enqueued_.store(0, std::memory_order_relaxed);
        tasks_executed_.store(0, std::memory_order_relaxed);
        tasks_failed_.store(0, std::memory_order_relaxed);
        total_busy_time_ns_.store(0, std::memory_order_relaxed);
        total_idle_time_ns_.store(0, std::memory_order_relaxed);
    }

    [[nodiscard]] Snapshot snapshot() const {
        return Snapshot{
            tasks_submitted_.load(std::memory_order_relaxed),
            tasks_enqueued_.load(std::memory_order_relaxed),
            tasks_executed_.load(std::memory_order_relaxed),
            tasks_failed_.load(std::memory_order_relaxed),
            total_busy_time_ns_.load(std::memory_order_relaxed),
            total_idle_time_ns_.load(std::memory_order_relaxed),
        };
    }

private:
    std::atomic<std::uint64_t> tasks_submitted_{0};
    std::atomic<std::uint64_t> tasks_enqueued_{0};
    std::atomic<std::uint64_t> tasks_executed_{0};
    std::atomic<std::uint64_t> tasks_failed_{0};
    std::atomic<std::uint64_t> total_busy_time_ns_{0};
    std::atomic<std::uint64_t> total_idle_time_ns_{0};
};

} // namespace kcenon::thread::metrics
