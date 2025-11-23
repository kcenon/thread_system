// BSD 3-Clause License
// Copyright (c) 2024, kcenon

#pragma once

#include <chrono>
#include <cstdint>

namespace kcenon::thread::stress {

/**
 * @brief Configuration for stress tests
 */
struct stress_test_config {
    std::chrono::seconds duration{60};
    uint32_t worker_threads{0};  // 0 = hardware_concurrency
    uint32_t task_rate{1000};    // tasks per second
    bool monitor_memory{true};
    bool monitor_threads{true};
    bool verbose{false};
};

/**
 * @brief Results from a stress test run
 */
struct stress_test_results {
    uint64_t tasks_completed{0};
    uint64_t tasks_failed{0};
    size_t peak_memory_mb{0};
    int peak_thread_count{0};
    double avg_latency_us{0.0};
    double p99_latency_us{0.0};
    std::chrono::milliseconds total_duration{0};
};

/**
 * @brief Stress test scenarios
 */
enum class stress_scenario {
    sustained_load,     // Continuous task submission
    burst_load,         // Periodic bursts
    memory_stress,      // Large payload handling
    thread_churn,       // Rapid pool creation/destruction
    mixed_workload      // Combination of above
};

} // namespace kcenon::thread::stress
