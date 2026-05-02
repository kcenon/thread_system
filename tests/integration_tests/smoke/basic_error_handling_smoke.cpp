// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#include "../framework/system_fixture.h"
#include <gtest/gtest.h>

using namespace integration_tests;

/**
 * @brief Smoke tests for basic error handling
 *
 * Goal: Verify that error handling works fundamentally
 * Expected time: < 5 seconds
 */
class BasicErrorHandlingSmoke : public SystemFixture {};

TEST_F(BasicErrorHandlingSmoke, ReturnsSuccessOnValidOperations) {
    CreateThreadPool(2);

    auto result = pool_->start();
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.is_err());

    result = pool_->stop();
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.is_err());
}

TEST_F(BasicErrorHandlingSmoke, ReturnsErrorOnInvalidOperations) {
    CreateThreadPool(2);

    auto result = pool_->start();
    ASSERT_TRUE(result.is_ok());

    // Try to start again - should return error
    result = pool_->start();
    EXPECT_TRUE(result.is_err());
}

TEST_F(BasicErrorHandlingSmoke, HandlesJobExceptionGracefully) {
    CreateThreadPool(2);

    auto result = pool_->start();
    ASSERT_TRUE(result.is_ok());

    std::atomic<bool> exception_caught{false};

    auto job = std::make_unique<kcenon::thread::callback_job>(
        [&exception_caught, this]() -> kcenon::common::VoidResult {
            try {
                throw std::runtime_error("Test exception");
            } catch (...) {
                exception_caught.store(true);
            }
            completed_jobs_.fetch_add(1);
            return kcenon::common::ok();
        }
    );

    auto submit_result = pool_->enqueue(std::move(job));
    EXPECT_TRUE(submit_result.is_ok());

    EXPECT_TRUE(WaitForJobCompletion(1, std::chrono::seconds(2)));
    EXPECT_TRUE(exception_caught.load());
}
