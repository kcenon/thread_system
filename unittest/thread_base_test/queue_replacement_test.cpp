/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, 🍀☀🌕🌥 🌊
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

#include <gtest/gtest.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/core/job_queue.h>
#include <kcenon/thread/core/callback_job.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

namespace kcenon::thread {
namespace test {

/**
 * Test for Sprint 1, Task 1.2: Worker queue replacement synchronization
 * Verifies that set_job_queue() safely coordinates with concurrent job execution
 */
class QueueReplacementTest : public ::testing::Test {
protected:
    void SetUp() override {
        worker_ = std::make_unique<thread_worker>();
        context_ = thread_context{};
        worker_->set_context(context_);
    }

    void TearDown() override {
        if (worker_ && worker_->is_running()) {
            worker_->stop();
        }
    }

    std::unique_ptr<thread_worker> worker_;
    thread_context context_;
};

/**
 * Test concurrent queue replacement while worker is processing jobs
 * This verifies the fix for the race condition described in IMPROVEMENT_PLAN.md
 */
TEST_F(QueueReplacementTest, ConcurrentQueueReplacement) {
    std::atomic<int> job_count{0};
    std::atomic<bool> stop_replacement{false};

    // Create initial queue with jobs
    auto initial_queue = std::make_shared<job_queue>();
    for (int i = 0; i < 100; ++i) {
        auto job = std::make_unique<callback_job>([&job_count]() -> result_void {
            job_count.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            return {};
        });
        (void)initial_queue->enqueue(std::move(job));
    }

    worker_->set_job_queue(initial_queue);
    worker_->start();

    // Thread that continuously replaces the queue
    std::thread replacement_thread([this, &job_count, &stop_replacement]() {
        int replacement_count = 0;
        while (!stop_replacement.load(std::memory_order_acquire) && replacement_count < 10) {
            // Create new queue
            auto new_queue = std::make_shared<job_queue>();
            for (int i = 0; i < 10; ++i) {
                auto job = std::make_unique<callback_job>([&job_count]() -> result_void {
                    job_count.fetch_add(1, std::memory_order_relaxed);
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    return {};
                });
                (void)new_queue->enqueue(std::move(job));
            }

            // Replace queue (should coordinate with worker)
            worker_->set_job_queue(new_queue);
            replacement_count++;

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    // Let it run for a while
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Stop replacement
    stop_replacement.store(true, std::memory_order_release);
    replacement_thread.join();

    // Stop worker
    worker_->stop();

    // Verify jobs were processed (no crashes means race condition is prevented)
    EXPECT_GT(job_count.load(), 0);
}

/**
 * Test that queue replacement waits for current job to complete
 */
TEST_F(QueueReplacementTest, WaitsForCurrentJobCompletion) {
    std::atomic<bool> job_started{false};
    std::atomic<bool> job_can_finish{false};
    std::atomic<bool> job_finished{false};

    auto queue = std::make_shared<job_queue>();

    // Create a long-running job
    auto long_job = std::make_unique<callback_job>([&]() -> result_void {
        job_started.store(true, std::memory_order_release);

        // Wait for signal to finish
        while (!job_can_finish.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        job_finished.store(true, std::memory_order_release);
        return {};
    });

    (void)queue->enqueue(std::move(long_job));
    worker_->set_job_queue(queue);
    worker_->start();

    // Wait for job to start
    while (!job_started.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Try to replace queue in another thread
    std::atomic<bool> replacement_started{false};
    std::atomic<bool> replacement_finished{false};

    std::thread replacement_thread([&]() {
        replacement_started.store(true, std::memory_order_release);
        auto new_queue = std::make_shared<job_queue>();
        worker_->set_job_queue(new_queue);
        replacement_finished.store(true, std::memory_order_release);
    });

    // Give replacement thread time to start
    while (!replacement_started.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Replacement should be blocked waiting for job to finish
    EXPECT_FALSE(replacement_finished.load(std::memory_order_acquire));

    // Allow job to finish
    job_can_finish.store(true, std::memory_order_release);

    // Wait for job to finish
    while (!job_finished.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Replacement should now complete
    replacement_thread.join();
    EXPECT_TRUE(replacement_finished.load(std::memory_order_acquire));

    worker_->stop();
}

/**
 * Test that multiple rapid queue replacements don't cause issues
 */
TEST_F(QueueReplacementTest, RapidQueueReplacements) {
    std::atomic<int> total_jobs{0};

    worker_->start();

    // Rapidly replace queues
    for (int i = 0; i < 20; ++i) {
        auto queue = std::make_shared<job_queue>();

        // Add some jobs
        for (int j = 0; j < 5; ++j) {
            auto job = std::make_unique<callback_job>([&total_jobs]() -> result_void {
                total_jobs.fetch_add(1, std::memory_order_relaxed);
                return {};
            });
            (void)queue->enqueue(std::move(job));
        }

        worker_->set_job_queue(queue);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // Give time for jobs to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    worker_->stop();

    // Verify some jobs were processed (exact count may vary due to rapid replacement)
    EXPECT_GT(total_jobs.load(), 0);
}

} // namespace test
} // namespace kcenon::thread
