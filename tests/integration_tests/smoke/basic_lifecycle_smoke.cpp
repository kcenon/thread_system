// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#include "../framework/system_fixture.h"
#include <gtest/gtest.h>

using namespace integration_tests;

/**
 * @brief Smoke tests for basic thread pool lifecycle
 *
 * Goal: Verify that the most fundamental operations work
 * Expected time: < 5 seconds
 */
class BasicLifecycleSmoke : public SystemFixture {};

TEST_F(BasicLifecycleSmoke, CanCreateThreadPool) {
    CreateThreadPool(4);
    EXPECT_NE(pool_, nullptr);
    EXPECT_EQ(pool_->get_active_worker_count(), 0);  // Not started yet
}

TEST_F(BasicLifecycleSmoke, CanStartAndStopPool) {
    CreateThreadPool(2);

    auto result = pool_->start();
    ASSERT_TRUE(result.is_ok()) << "Failed to start pool";
    EXPECT_TRUE(pool_->is_running());

    result = pool_->stop();
    ASSERT_TRUE(result.is_ok()) << "Failed to stop pool";
    EXPECT_FALSE(pool_->is_running());
}

TEST_F(BasicLifecycleSmoke, CanSubmitSingleJob) {
    CreateThreadPool(2);

    auto result = pool_->start();
    ASSERT_TRUE(result.is_ok());

    SubmitCountingJob();

    EXPECT_TRUE(WaitForJobCompletion(1, std::chrono::seconds(2)));
    EXPECT_EQ(completed_jobs_.load(), 1);
}
