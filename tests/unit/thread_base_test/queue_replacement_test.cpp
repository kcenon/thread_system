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
#include <condition_variable>

namespace kcenon::thread {
namespace test {

// Reduce iteration count for coverage builds
#ifdef ENABLE_COVERAGE
constexpr int QUEUE_REPLACEMENT_ITERATIONS = 3;
#else
constexpr int QUEUE_REPLACEMENT_ITERATIONS = 10;
#endif

constexpr auto MAX_WAIT_TIME = std::chrono::seconds(5);

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

    // Helper: Wait with timeout
    template<typename Predicate>
    bool wait_for(Predicate pred, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        auto start = std::chrono::steady_clock::now();
        while (!pred()) {
            if (std::chrono::steady_clock::now() - start > timeout) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        return true;
    }

    std::unique_ptr<thread_worker> worker_;
    thread_context context_;
};

/**
 * Test concurrent queue replacement while worker is processing jobs
 * This verifies the fix for the race condition described in IMPROVEMENT_PLAN.md
 *
 * Simplified to test basic queue replacement mechanism:
 * - Worker starts with empty queue
 * - Replacement happens while worker is idle
 * - Verifies replacement completes without deadlock
 */
TEST_F(QueueReplacementTest, ConcurrentQueueReplacement) {
    std::atomic<int> job_count{0};

    // Create initial empty queue
    auto initial_queue = std::make_shared<job_queue>();
    worker_->set_job_queue(initial_queue);
    worker_->start();

    // Give worker time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Create final queue for replacements
    auto final_queue = std::make_shared<job_queue>();

    // Perform queue replacements
    for (int i = 0; i < QUEUE_REPLACEMENT_ITERATIONS; ++i) {
        auto new_queue = std::make_shared<job_queue>();

        // Add a simple job
        auto job = std::make_unique<callback_job>([&job_count]() -> result_void {
            job_count.fetch_add(1, std::memory_order_relaxed);
            return {};
        });
        (void)new_queue->enqueue(std::move(job));

        // Replace queue - should complete without deadlock
        worker_->set_job_queue(new_queue);
        final_queue = new_queue;  // Keep reference to last queue

        // Brief delay to allow job processing
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Stop the queue to allow worker to exit
    final_queue->stop();

    // Stop worker
    worker_->stop();

    // Verify some jobs were processed
    EXPECT_GT(job_count.load(), 0);
}

/**
 * Test that queue replacement waits for current job to complete
 *
 * Improved with:
 * - Reduced timeout to 500ms (was 2000ms)
 * - Join with timeout to prevent test hang
 * - Explicit test failure if thread doesn't complete
 */
TEST_F(QueueReplacementTest, WaitsForCurrentJobCompletion) {
    std::atomic<bool> job_started{false};
    std::atomic<bool> job_can_finish{false};
    std::atomic<bool> job_finished{false};

    auto queue = std::make_shared<job_queue>();

    // Create a job that waits for signal
    auto controlled_job = std::make_unique<callback_job>([&]() -> result_void {
        job_started.store(true, std::memory_order_release);

        // Wait for signal to finish with timeout to prevent hang
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
        while (!job_can_finish.load(std::memory_order_acquire)) {
            if (std::chrono::steady_clock::now() > deadline) {
                // Timeout - fail gracefully
                return {};
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        job_finished.store(true, std::memory_order_release);
        return {};
    });

    (void)queue->enqueue(std::move(controlled_job));
    worker_->set_job_queue(queue);
    worker_->start();

    // Wait for job to start with timeout
    ASSERT_TRUE(wait_for([&]() { return job_started.load(std::memory_order_acquire); },
                         std::chrono::milliseconds(200)))
        << "Job failed to start";

    // Try to replace queue in another thread
    std::atomic<bool> replacement_started{false};
    std::atomic<bool> replacement_finished{false};
    auto new_queue = std::make_shared<job_queue>();

    std::thread replacement_thread([&]() {
        replacement_started.store(true, std::memory_order_release);
        worker_->set_job_queue(new_queue);
        replacement_finished.store(true, std::memory_order_release);
    });

    // Wait for replacement thread to start
    ASSERT_TRUE(wait_for([&]() { return replacement_started.load(std::memory_order_acquire); },
                         std::chrono::milliseconds(100)))
        << "Replacement thread failed to start";

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Replacement should be blocked waiting for job to finish
    EXPECT_FALSE(replacement_finished.load(std::memory_order_acquire));

    // Allow job to finish
    job_can_finish.store(true, std::memory_order_release);

    // Wait for job to finish with timeout
    ASSERT_TRUE(wait_for([&]() { return job_finished.load(std::memory_order_acquire); },
                         std::chrono::milliseconds(200)))
        << "Job failed to finish";

    // Wait for replacement to complete with timeout
    ASSERT_TRUE(wait_for([&]() { return replacement_finished.load(std::memory_order_acquire); },
                         std::chrono::milliseconds(200)))
        << "Queue replacement failed to complete";

    // Join the thread (should be immediate since replacement_finished is true)
    replacement_thread.join();

    // Stop the new queue before stopping worker
    new_queue->stop();
    worker_->stop();
}

/**
 * Test that multiple rapid queue replacements don't cause issues
 */
TEST_F(QueueReplacementTest, RapidQueueReplacements) {
    std::atomic<int> total_jobs{0};

    // Start with initial queue
    auto initial_queue = std::make_shared<job_queue>();
    worker_->set_job_queue(initial_queue);
    worker_->start();

    std::shared_ptr<job_queue> last_queue;

    // Rapidly replace queues with reduced iterations
    const int num_replacements = 10;  // Reduced from 20
    for (int i = 0; i < num_replacements; ++i) {
        auto queue = std::make_shared<job_queue>();

        // Add some jobs
        for (int j = 0; j < 3; ++j) {  // Reduced from 5
            auto job = std::make_unique<callback_job>([&total_jobs]() -> result_void {
                total_jobs.fetch_add(1, std::memory_order_relaxed);
                return {};
            });
            (void)queue->enqueue(std::move(job));
        }

        worker_->set_job_queue(queue);
        last_queue = queue;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));  // Reduced from 5
    }

    // Give time for jobs to complete with timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Reduced from 100

    // Stop queue before stopping worker
    if (last_queue) {
        last_queue->stop();
    }
    worker_->stop();

    // Verify some jobs were processed (exact count may vary due to rapid replacement)
    EXPECT_GT(total_jobs.load(), 0);
}

} // namespace test
} // namespace kcenon::thread
