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
