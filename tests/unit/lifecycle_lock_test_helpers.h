// BSD 3-Clause License
// See the LICENSE file in the project root for full license information.

#pragma once

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

namespace kcenon::thread::test {

inline auto wait_for_flag(const std::atomic<bool>& flag) -> bool {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!flag.load(std::memory_order_acquire)) {
        if (std::chrono::steady_clock::now() >= deadline) {
            return false;
        }
        std::this_thread::yield();
    }
    return true;
}

struct lifecycle_lock_scenario {
    std::mutex application_mutex;
    std::atomic<bool> entered{false};
    std::atomic<bool> acquired{false};
    std::atomic<bool> allow_completion{false};
    bool completion_timed_out{false};
    int completed{0};

    auto run_work() -> void {
        entered.store(true, std::memory_order_release);
        {
            // Before #762, do_work() holds the lifecycle mutex here.
            std::scoped_lock lock(application_mutex);
        }
        acquired.store(true, std::memory_order_release);

        // Do not need application_mutex again: stop() synchronously joins us.
        completion_timed_out = !wait_for_flag(allow_completion);
        ++completed;
    }
};

template <typename Worker>
auto verify_stop_with_application_lock(Worker& worker, lifecycle_lock_scenario& scenario) -> void {
    std::unique_lock application_lock(scenario.application_mutex);
    ASSERT_TRUE(worker.start().is_ok());

    const bool entered = wait_for_flag(scenario.entered);
    application_lock.unlock();
    const bool acquired = entered && wait_for_flag(scenario.acquired);
    if (acquired) {
        application_lock.lock();
    }

    // Always open the gate, including on a failed handshake, before joining.
    scenario.allow_completion.store(true, std::memory_order_release);
    // With the original loop, this adds application -> lifecycle ordering,
    // reversing the ordering recorded by run_work(). TSan detects the cycle.
    EXPECT_TRUE(worker.stop().is_ok());

    EXPECT_TRUE(entered);
    EXPECT_TRUE(acquired);
    EXPECT_FALSE(scenario.completion_timed_out);
    EXPECT_EQ(scenario.completed, 1);
}

}  // namespace kcenon::thread::test
