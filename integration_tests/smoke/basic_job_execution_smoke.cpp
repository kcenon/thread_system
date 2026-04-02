// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#include "../framework/system_fixture.h"
#include "../framework/test_helpers.h"
#include <gtest/gtest.h>

using namespace integration_tests;

/**
 * @brief Smoke tests for basic job execution
 *
 * Goal: Verify that jobs execute correctly
 * Expected time: < 5 seconds
 */
class BasicJobExecutionSmoke : public SystemFixture {};

TEST_F(BasicJobExecutionSmoke, CanExecuteMultipleJobs) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result.is_ok());

    const size_t job_count = 10;
    for (size_t i = 0; i < job_count; ++i) {
        SubmitCountingJob();
    }

    EXPECT_TRUE(WaitForJobCompletion(job_count, std::chrono::seconds(2)));
    EXPECT_EQ(completed_jobs_.load(), job_count);
}

TEST_F(BasicJobExecutionSmoke, CanSubmitJobsBeforeStart) {
    CreateThreadPool(4);

    // Submit jobs before starting pool
    const size_t job_count = 5;
    for (size_t i = 0; i < job_count; ++i) {
        SubmitCountingJob();
    }

    // Now start pool - jobs should execute
    auto result = pool_->start();
    ASSERT_TRUE(result.is_ok());

    EXPECT_TRUE(WaitForJobCompletion(job_count, std::chrono::seconds(2)));
    EXPECT_EQ(completed_jobs_.load(), job_count);
}

TEST_F(BasicJobExecutionSmoke, CanEnqueueAndDequeueFromQueue) {
    auto queue = std::make_shared<kcenon::thread::job_queue>();
    queue->set_notify(true);

    auto job = std::make_unique<kcenon::thread::callback_job>(
        []() -> kcenon::common::VoidResult { return kcenon::common::ok(); }
    );

    auto result = queue->enqueue(std::move(job));
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(queue->size(), 1);

    auto dequeued = queue->try_dequeue();
    EXPECT_TRUE(dequeued.is_ok());
    EXPECT_TRUE(queue->empty());
}
