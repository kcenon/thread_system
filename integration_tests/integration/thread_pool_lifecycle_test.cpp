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

#include "../framework/system_fixture.h"
#include "../framework/test_helpers.h"
#include <gtest/gtest.h>

using namespace integration_tests;

/**
 * @brief Integration tests for thread pool lifecycle management
 *
 * These tests verify:
 * - Pool creation and initialization
 * - Starting and stopping pools
 * - Worker management
 * - Job submission and execution
 * - Resource cleanup
 */
class ThreadPoolLifecycleTest : public SystemFixture {};

TEST_F(ThreadPoolLifecycleTest, CreateAndDestroyEmptyPool) {
    auto pool = std::make_shared<kcenon::thread::thread_pool>("empty_pool", context_);
    EXPECT_NE(pool, nullptr);
    EXPECT_FALSE(pool->is_running());
    EXPECT_EQ(pool->get_pending_task_count(), 0);
}

TEST_F(ThreadPoolLifecycleTest, StartAndStopPool) {
    CreateThreadPool(4, "lifecycle_pool");

    auto result = pool_->start();
    ASSERT_TRUE(result) << "Failed to start pool: " << result.get_error().to_string();
    EXPECT_TRUE(pool_->is_running());

    result = pool_->stop();
    ASSERT_TRUE(result) << "Failed to stop pool: " << result.get_error().to_string();
    EXPECT_FALSE(pool_->is_running());
}

TEST_F(ThreadPoolLifecycleTest, SubmitJobsAfterStart) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result);

    const size_t job_count = 100;
    for (size_t i = 0; i < job_count; ++i) {
        SubmitCountingJob();
    }

    EXPECT_TRUE(WaitForJobCompletion(job_count));
    EXPECT_EQ(completed_jobs_.load(), job_count);
}

TEST_F(ThreadPoolLifecycleTest, SubmitJobsBeforeStart) {
    CreateThreadPool(4);

    // Submit jobs before starting
    const size_t job_count = 50;
    for (size_t i = 0; i < job_count; ++i) {
        SubmitCountingJob();
    }

    // Now start the pool
    auto result = pool_->start();
    ASSERT_TRUE(result);

    EXPECT_TRUE(WaitForJobCompletion(job_count));
    EXPECT_EQ(completed_jobs_.load(), job_count);
}

TEST_F(ThreadPoolLifecycleTest, ImmediateShutdown) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result);

    // Submit many jobs
    for (size_t i = 0; i < 1000; ++i) {
        SubmitJob([]() {
            WorkSimulator::simulate_work(std::chrono::microseconds(100));
        });
    }

    // Immediate shutdown should succeed
    result = pool_->stop(true);
    EXPECT_TRUE(result);
    EXPECT_FALSE(pool_->is_running());
}

TEST_F(ThreadPoolLifecycleTest, GracefulShutdown) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result);

    const size_t job_count = 100;
    for (size_t i = 0; i < job_count; ++i) {
        SubmitCountingJob();
    }

    // Wait for jobs to complete then shutdown
    EXPECT_TRUE(WaitForJobCompletion(job_count));

    result = pool_->stop(false);
    EXPECT_TRUE(result);
    EXPECT_FALSE(pool_->is_running());
}

TEST_F(ThreadPoolLifecycleTest, AddWorkersAfterCreation) {
    pool_ = std::make_shared<kcenon::thread::thread_pool>("dynamic_pool", context_);
    job_queue_ = pool_->get_job_queue();

    // Start with 2 workers
    for (size_t i = 0; i < 2; ++i) {
        auto worker = std::make_unique<kcenon::thread::thread_worker>();
        auto result = pool_->enqueue(std::move(worker));
        ASSERT_TRUE(result);
    }

    auto result = pool_->start();
    ASSERT_TRUE(result);

    // Add 2 more workers dynamically
    for (size_t i = 0; i < 2; ++i) {
        auto worker = std::make_unique<kcenon::thread::thread_worker>();
        result = pool_->enqueue(std::move(worker));
        EXPECT_TRUE(result);
    }

    EXPECT_EQ(pool_->get_thread_count(), 4);
}

TEST_F(ThreadPoolLifecycleTest, MultipleStartStopCycles) {
    CreateThreadPool(4);

    for (int cycle = 0; cycle < 3; ++cycle) {
        auto result = pool_->start();
        ASSERT_TRUE(result) << "Failed to start in cycle " << cycle;

        for (size_t i = 0; i < 50; ++i) {
            SubmitCountingJob();
        }

        size_t expected = (cycle + 1) * 50;
        EXPECT_TRUE(WaitForJobCompletion(expected));

        result = pool_->stop();
        ASSERT_TRUE(result) << "Failed to stop in cycle " << cycle;
    }

    EXPECT_EQ(completed_jobs_.load(), 150);
}

TEST_F(ThreadPoolLifecycleTest, VerifyWorkerCount) {
    const size_t worker_count = 8;
    CreateThreadPool(worker_count);

    EXPECT_EQ(pool_->get_thread_count(), worker_count);

    auto result = pool_->start();
    ASSERT_TRUE(result);

    EXPECT_EQ(pool_->get_thread_count(), worker_count);
}

TEST_F(ThreadPoolLifecycleTest, SubmitBatchJobs) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result);

    std::vector<std::unique_ptr<kcenon::thread::job>> jobs;
    const size_t batch_size = 100;

    for (size_t i = 0; i < batch_size; ++i) {
        jobs.push_back(std::make_unique<kcenon::thread::callback_job>(
            [this]() -> kcenon::thread::result_void {
                completed_jobs_.fetch_add(1);
                return {};
            }
        ));
    }

    result = pool_->enqueue_batch(std::move(jobs));
    EXPECT_TRUE(result);

    EXPECT_TRUE(WaitForJobCompletion(batch_size));
    EXPECT_EQ(completed_jobs_.load(), batch_size);
}

TEST_F(ThreadPoolLifecycleTest, QueueSizeTracking) {
    CreateThreadPool(1); // Single worker to control execution

    auto result = pool_->start();
    ASSERT_TRUE(result);

    // Submit jobs faster than they can be processed
    const size_t job_count = 100;
    for (size_t i = 0; i < job_count; ++i) {
        SubmitJob([]() {
            WorkSimulator::simulate_work(std::chrono::microseconds(100));
        });
    }

    // Queue should have pending tasks
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    size_t pending = pool_->get_pending_task_count();
    EXPECT_GT(pending, 0);

    EXPECT_TRUE(WaitForJobCompletion(job_count, std::chrono::seconds(30)));
}

TEST_F(ThreadPoolLifecycleTest, ConcurrentJobSubmission) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result);

    const size_t threads = 4;
    const size_t jobs_per_thread = 100;
    std::vector<std::thread> producers;

    for (size_t t = 0; t < threads; ++t) {
        producers.emplace_back([this, jobs_per_thread]() {
            for (size_t i = 0; i < jobs_per_thread; ++i) {
                SubmitCountingJob();
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }

    EXPECT_TRUE(WaitForJobCompletion(threads * jobs_per_thread));
    EXPECT_EQ(completed_jobs_.load(), threads * jobs_per_thread);
}

TEST_F(ThreadPoolLifecycleTest, ErrorHandlingInJobs) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result);

    std::atomic<size_t> exceptions_caught{0};

    for (size_t i = 0; i < 50; ++i) {
        auto job = std::make_unique<kcenon::thread::callback_job>(
            [this, &exceptions_caught]() -> kcenon::thread::result_void {
                try {
                    throw std::runtime_error("Test exception");
                } catch (...) {
                    exceptions_caught.fetch_add(1);
                }
                completed_jobs_.fetch_add(1);
                return {};
            }
        );
        auto submit_result = pool_->enqueue(std::move(job));
        EXPECT_TRUE(submit_result);
    }

    EXPECT_TRUE(WaitForJobCompletion(50));
    EXPECT_EQ(exceptions_caught.load(), 50);
}

TEST_F(ThreadPoolLifecycleTest, PoolResourceCleanup) {
    {
        auto pool = std::make_shared<kcenon::thread::thread_pool>("cleanup_pool", context_);

        for (size_t i = 0; i < 4; ++i) {
            auto worker = std::make_unique<kcenon::thread::thread_worker>();
            auto result = pool->enqueue(std::move(worker));
            ASSERT_TRUE(result);
        }

        auto result = pool->start();
        ASSERT_TRUE(result);

        for (size_t i = 0; i < 100; ++i) {
            auto job = std::make_unique<kcenon::thread::callback_job>(
                []() -> kcenon::thread::result_void {
                    return {};
                }
            );
            pool->enqueue(std::move(job));
        }

        // Pool will be destroyed when scope exits
    }

    // If we reach here without crashes, cleanup was successful
    SUCCEED();
}

TEST_F(ThreadPoolLifecycleTest, StressTestStartStop) {
    CreateThreadPool(4);

    for (int i = 0; i < 10; ++i) {
        auto result = pool_->start();
        EXPECT_TRUE(result);

        for (size_t j = 0; j < 10; ++j) {
            SubmitCountingJob();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        result = pool_->stop(true);
        EXPECT_TRUE(result);
    }
}
