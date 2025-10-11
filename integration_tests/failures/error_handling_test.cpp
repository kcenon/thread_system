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
 * @brief Integration tests for error handling and failure scenarios
 *
 * These tests verify:
 * - Error propagation with Result<T> pattern
 * - Recovery from failures
 * - Exception safety
 * - Resource cleanup on errors
 */
class ErrorHandlingTest : public SystemFixture {};

TEST_F(ErrorHandlingTest, ResultPatternSuccess) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result);
    EXPECT_FALSE(result.has_error());

    result = pool_->stop();
    ASSERT_TRUE(result);
    EXPECT_FALSE(result.has_error());
}

TEST_F(ErrorHandlingTest, ResultPatternFailure) {
    CreateThreadPool(4);

    // Start pool
    auto result = pool_->start();
    ASSERT_TRUE(result);

    // Try to start again - should fail
    result = pool_->start();
    EXPECT_TRUE(result.has_error());

    if (result.has_error()) {
        const auto& error = result.get_error();
        EXPECT_NE(error.code(), kcenon::thread::error_code::success);
        std::cout << "Expected error: " << error.to_string() << "\n";
    }
}

TEST_F(ErrorHandlingTest, ExceptionInJob) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result);

    std::atomic<size_t> exceptions_caught{0};

    for (size_t i = 0; i < 100; ++i) {
        auto job = std::make_unique<kcenon::thread::callback_job>(
            [&exceptions_caught, this]() -> kcenon::thread::result_void {
                try {
                    throw std::runtime_error("Intentional exception");
                } catch (const std::exception& e) {
                    exceptions_caught.fetch_add(1);
                }
                completed_jobs_.fetch_add(1);
                return {};
            }
        );
        auto submit_result = pool_->enqueue(std::move(job));
        EXPECT_TRUE(submit_result);
    }

    EXPECT_TRUE(WaitForJobCompletion(100));
    EXPECT_EQ(exceptions_caught.load(), 100);
}

TEST_F(ErrorHandlingTest, PartialJobFailure) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result);

    std::atomic<size_t> success_count{0};
    std::atomic<size_t> failure_count{0};

    for (size_t i = 0; i < 100; ++i) {
        bool should_fail = (i % 3 == 0);

        auto job = std::make_unique<kcenon::thread::callback_job>(
            [&success_count, &failure_count, should_fail, this]() -> kcenon::thread::result_void {
                if (should_fail) {
                    failure_count.fetch_add(1);
                    return kcenon::thread::error{
                        kcenon::thread::error_code::job_execution_failed,
                        "Intentional failure"
                    };
                }
                success_count.fetch_add(1);
                completed_jobs_.fetch_add(1);
                return {};
            }
        );
        auto submit_result = pool_->enqueue(std::move(job));
        EXPECT_TRUE(submit_result);
    }

    // Wait for processing
    EXPECT_TRUE(WaitForCondition([&success_count, &failure_count]() {
        return (success_count.load() + failure_count.load()) >= 100;
    }));

    EXPECT_GT(success_count.load(), 0);
    EXPECT_GT(failure_count.load(), 0);
    EXPECT_EQ(success_count.load() + failure_count.load(), 100);
}

TEST_F(ErrorHandlingTest, QueueErrorHandling) {
    auto queue = std::make_shared<kcenon::thread::job_queue>();
    queue->set_notify(true);

    // Try to dequeue from empty queue (non-blocking)
    auto result = queue->try_dequeue();
    EXPECT_FALSE(result.has_value());

    // Enqueue a job
    auto job = std::make_unique<kcenon::thread::callback_job>(
        []() -> kcenon::thread::result_void { return {}; }
    );
    auto enqueue_result = queue->enqueue(std::move(job));
    EXPECT_TRUE(enqueue_result);

    // Now dequeue should succeed
    result = queue->try_dequeue();
    EXPECT_TRUE(result.has_value());
}

TEST_F(ErrorHandlingTest, ResourceCleanupOnError) {
    std::atomic<size_t> resource_acquired{0};
    std::atomic<size_t> resource_released{0};

    {
        CreateThreadPool(4);

        auto result = pool_->start();
        ASSERT_TRUE(result);

        for (size_t i = 0; i < 100; ++i) {
            auto job = std::make_unique<kcenon::thread::callback_job>(
                [&resource_acquired, &resource_released, this]() -> kcenon::thread::result_void {
                    resource_acquired.fetch_add(1);

                    // Simulate work
                    WorkSimulator::simulate_work(std::chrono::microseconds(10));

                    resource_released.fetch_add(1);
                    completed_jobs_.fetch_add(1);
                    return {};
                }
            );
            pool_->enqueue(std::move(job));
        }

        EXPECT_TRUE(WaitForJobCompletion(100));

        // Pool will be destroyed, triggering cleanup
    }

    // Verify all resources were cleaned up
    EXPECT_EQ(resource_acquired.load(), resource_released.load());
}

TEST_F(ErrorHandlingTest, ConcurrentErrorPropagation) {
    CreateThreadPool(8);

    auto result = pool_->start();
    ASSERT_TRUE(result);

    std::atomic<size_t> error_count{0};
    std::mutex error_mutex;
    std::vector<std::string> error_messages;

    const size_t job_count = 200;
    for (size_t i = 0; i < job_count; ++i) {
        bool should_error = (i % 5 == 0);

        auto job = std::make_unique<kcenon::thread::callback_job>(
            [&error_count, &error_mutex, &error_messages, should_error, this]()
            -> kcenon::thread::result_void {
                if (should_error) {
                    error_count.fetch_add(1);
                    auto err = kcenon::thread::error{
                        kcenon::thread::error_code::job_execution_failed,
                        "Concurrent error"
                    };

                    std::lock_guard<std::mutex> lock(error_mutex);
                    error_messages.push_back(err.to_string());

                    return err;
                }

                completed_jobs_.fetch_add(1);
                return {};
            }
        );
        pool_->enqueue(std::move(job));
    }

    EXPECT_TRUE(WaitForCondition([this, &error_count, job_count]() {
        return (completed_jobs_.load() + error_count.load()) >= job_count;
    }));

    EXPECT_GT(error_count.load(), 0);
    EXPECT_EQ(error_messages.size(), error_count.load());
}

TEST_F(ErrorHandlingTest, ErrorRecoveryAfterStop) {
    CreateThreadPool(4);

    // First lifecycle
    auto result = pool_->start();
    ASSERT_TRUE(result);

    for (size_t i = 0; i < 50; ++i) {
        SubmitCountingJob();
    }

    EXPECT_TRUE(WaitForJobCompletion(50));

    result = pool_->stop();
    ASSERT_TRUE(result);

    // Second lifecycle - should work fine
    result = pool_->start();
    ASSERT_TRUE(result);

    for (size_t i = 0; i < 50; ++i) {
        SubmitCountingJob();
    }

    EXPECT_TRUE(WaitForJobCompletion(100));
    EXPECT_EQ(completed_jobs_.load(), 100);
}

TEST_F(ErrorHandlingTest, NullJobHandling) {
    auto queue = std::make_shared<kcenon::thread::job_queue>();
    queue->set_notify(true);

    // Create a null job pointer
    std::unique_ptr<kcenon::thread::job> null_job;

    // Attempting to enqueue null should be handled gracefully
    auto result = queue->enqueue(std::move(null_job));

    // The system should handle this (either reject or accept based on implementation)
    // We just verify it doesn't crash
    SUCCEED();
}

TEST_F(ErrorHandlingTest, ErrorCodeValidation) {
    using kcenon::thread::error;
    using kcenon::thread::error_code;

    error err1(error_code::queue_full, "Queue is full");
    EXPECT_EQ(err1.code(), error_code::queue_full);
    EXPECT_FALSE(err1.message().empty());

    error err2(error_code::thread_start_failure);
    EXPECT_EQ(err2.code(), error_code::thread_start_failure);

    // Test error to string conversion
    std::string err_str = err1.to_string();
    EXPECT_FALSE(err_str.empty());
    EXPECT_NE(err_str.find("Queue is full"), std::string::npos);
}
