/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, DongCheol Shin
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
#include <kcenon/thread/core/error_handling.h>
#include <kcenon/thread/core/job_queue.h>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/core/thread_base.h>
#include <thread>
#include <chrono>

namespace kcenon::thread {
namespace test {

class ErrorHandlingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test environment
    }

    void TearDown() override {
        // Clean up
    }
};

TEST_F(ErrorHandlingTest, ErrorCodeToString) {
    // Test all error codes have string representations
    EXPECT_FALSE(error_code_to_string(error_code::success).empty());
    EXPECT_FALSE(error_code_to_string(error_code::unknown_error).empty());
    EXPECT_FALSE(error_code_to_string(error_code::thread_already_running).empty());
    EXPECT_FALSE(error_code_to_string(error_code::queue_full).empty());
    EXPECT_FALSE(error_code_to_string(error_code::job_creation_failed).empty());
    EXPECT_FALSE(error_code_to_string(error_code::resource_allocation_failed).empty());
    EXPECT_FALSE(error_code_to_string(error_code::mutex_error).empty());
    EXPECT_FALSE(error_code_to_string(error_code::io_error).empty());

    // Test unknown error code
    auto unknown_code = static_cast<error_code>(9999);
    EXPECT_FALSE(error_code_to_string(unknown_code).empty());
}

TEST_F(ErrorHandlingTest, VoidResultSuccess) {
    common::VoidResult success_result = common::ok();

    EXPECT_TRUE(success_result.is_ok());
    EXPECT_FALSE(success_result.is_err());
}

TEST_F(ErrorHandlingTest, VoidResultError) {
    common::VoidResult error_result = make_error_result(error_code::unknown_error, "Test error message");

    EXPECT_TRUE(error_result.is_err());
    EXPECT_FALSE(error_result.is_ok());
    EXPECT_EQ(get_error_code(error_result.error()), error_code::unknown_error);
    EXPECT_EQ(error_result.error().message, "Test error message");
}

TEST_F(ErrorHandlingTest, VoidResultAPICompatibility) {
    // Test that VoidResult API is consistent
    common::VoidResult success = common::ok();
    common::VoidResult failure = make_error_result(error_code::unknown_error, "Error");

    // Verify methods are consistent for success case
    EXPECT_TRUE(success.is_ok());
    EXPECT_FALSE(success.is_err());

    // Verify methods are consistent for error case
    EXPECT_FALSE(failure.is_ok());
    EXPECT_TRUE(failure.is_err());
}

TEST_F(ErrorHandlingTest, ResultWithValue) {
    // Success case
    common::Result<int> success_result = common::Result<int>::ok(42);

    EXPECT_TRUE(success_result.is_ok());
    EXPECT_FALSE(success_result.is_err());
    EXPECT_EQ(success_result.value(), 42);

    // Error case
    common::Result<int> error_result = make_error_result<int>(error_code::invalid_argument, "Invalid value");

    EXPECT_FALSE(error_result.is_ok());
    EXPECT_TRUE(error_result.is_err());
    EXPECT_EQ(get_error_code(error_result.error()), error_code::invalid_argument);
}

TEST_F(ErrorHandlingTest, ResultValueOr) {
    common::Result<int> success_result = common::Result<int>::ok(42);
    EXPECT_EQ(success_result.value_or(0), 42);

    common::Result<int> error_result = make_error_result<int>(error_code::unknown_error, "Error");
    EXPECT_EQ(error_result.value_or(99), 99);
}

TEST_F(ErrorHandlingTest, ToErrorInfoConversion) {
    // Test to_error_info helper function
    auto info = to_error_info(error_code::queue_full, "Queue is at capacity");
    EXPECT_EQ(info.code, static_cast<int>(error_code::queue_full));
    EXPECT_EQ(info.message, "Queue is at capacity");
    EXPECT_EQ(info.module, "thread_system");

    // Test with default message
    auto info_default = to_error_info(error_code::queue_empty);
    EXPECT_EQ(info_default.code, static_cast<int>(error_code::queue_empty));
    EXPECT_EQ(info_default.message, error_code_to_string(error_code::queue_empty));
}

TEST_F(ErrorHandlingTest, GetErrorCodeFromInfo) {
    common::error_info info{
        static_cast<int>(error_code::job_execution_failed),
        "Job failed",
        "thread_system"
    };

    EXPECT_EQ(get_error_code(info), error_code::job_execution_failed);
}

TEST_F(ErrorHandlingTest, JobQueueErrorStates) {
    // Test queue operations
    auto queue = std::make_unique<job_queue>();

    // Enqueue a job
    auto job = std::make_unique<callback_job>([]() -> common::VoidResult { return common::ok(); });
    auto result = queue->enqueue(std::move(job));
    EXPECT_TRUE(result.is_ok());

    // Test dequeue
    auto dequeue_result = queue->dequeue();
    EXPECT_TRUE(dequeue_result.is_ok());
    EXPECT_NE(dequeue_result.value(), nullptr);
}

TEST_F(ErrorHandlingTest, JobExecutionErrors) {
    auto queue = std::make_unique<job_queue>();

    // Job that returns an error
    auto error_job = std::make_unique<callback_job>([]() -> common::VoidResult {
        return make_error_result(error_code::job_execution_failed, "Simulated failure");
    });

    auto enqueue_result = queue->enqueue(std::move(error_job));
    EXPECT_TRUE(enqueue_result.is_ok());

    // Dequeue and execute
    auto dequeue_result = queue->dequeue();
    ASSERT_TRUE(dequeue_result.is_ok());
    auto dequeued = std::move(dequeue_result.value());
    ASSERT_NE(dequeued, nullptr);

    auto result = dequeued->do_work();
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, static_cast<int>(error_code::job_execution_failed));
}

TEST_F(ErrorHandlingTest, ThreadBaseStartStop) {
    // Create a custom thread that counts work cycles
    class test_thread : public thread_base {
    public:
        test_thread() : thread_base("test_thread") {}
        std::atomic<int> work_count{0};
        std::atomic<bool> error_occurred{false};

    protected:
        common::VoidResult do_work() override {
            work_count.fetch_add(1);
            if (work_count.load() >= 3) {
                error_occurred.store(true);
                return make_error_result(error_code::unknown_error, "Test error");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            return common::ok();
        }
    };

    auto worker = std::make_unique<test_thread>();
    worker->set_wake_interval(std::chrono::milliseconds(10));

    // Start the thread
    worker->start();

    // Let it run for a bit - ensure enough time for at least 3 iterations
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Stop the thread
    worker->stop();

    // Verify it executed multiple times
    EXPECT_GT(worker->work_count.load(), 0);
    EXPECT_GE(worker->work_count.load(), 3);  // Should have run at least 3 times
    EXPECT_TRUE(worker->error_occurred.load());
}

TEST_F(ErrorHandlingTest, ConcurrentErrorHandling) {
    const int thread_count = 4;
    const int errors_per_thread = 10;

    std::atomic<int> total_errors{0};
    std::atomic<int> total_successes{0};

    std::vector<std::thread> threads;

    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([&total_errors, &total_successes]() {
            for (int i = 0; i < errors_per_thread * 2; ++i) {
                // Alternate between success and error
                common::Result<int> res = (i % 2 == 0)
                    ? common::Result<int>::ok(i)
                    : make_error_result<int>(error_code::unknown_error, "Error");

                if (res.is_err()) {
                    total_errors.fetch_add(1);
                } else {
                    total_successes.fetch_add(1);
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(total_errors.load(), thread_count * errors_per_thread);
    EXPECT_EQ(total_successes.load(), thread_count * errors_per_thread);
}

TEST_F(ErrorHandlingTest, ResourceAllocationErrors) {
    // Simulate resource allocation failure scenarios
    std::vector<common::Result<std::unique_ptr<int>>> allocations;

    for (int i = 0; i < 10; ++i) {
        if (i == 5) {
            // Simulate allocation failure
            allocations.push_back(
                make_error_result<std::unique_ptr<int>>(error_code::resource_allocation_failed, "Out of memory")
            );
        } else {
            allocations.push_back(
                common::Result<std::unique_ptr<int>>::ok(std::make_unique<int>(i))
            );
        }
    }

    int success_count = 0;
    int error_count = 0;

    for (const auto& alloc : allocations) {
        if (alloc.is_ok()) {
            success_count++;
        } else {
            error_count++;
            EXPECT_EQ(get_error_code(alloc.error()), error_code::resource_allocation_failed);
        }
    }

    EXPECT_EQ(success_count, 9);
    EXPECT_EQ(error_count, 1);
}

TEST_F(ErrorHandlingTest, StdErrorCodeIntegration) {
    // Test std::error_code integration
    std::error_code ec = make_error_code(error_code::queue_full);
    EXPECT_EQ(ec.value(), static_cast<int>(error_code::queue_full));
    EXPECT_EQ(ec.category().name(), std::string("thread_system"));
    EXPECT_FALSE(ec.message().empty());

    // Test error_condition equivalence
    std::error_code timeout_ec = make_error_code(error_code::operation_timeout);
    EXPECT_TRUE(timeout_ec == std::errc::timed_out);

    std::error_code arg_ec = make_error_code(error_code::invalid_argument);
    EXPECT_TRUE(arg_ec == std::errc::invalid_argument);
}

} // namespace test
} // namespace kcenon::thread
