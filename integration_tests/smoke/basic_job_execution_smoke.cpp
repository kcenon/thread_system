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
 * @brief Smoke tests for basic job execution
 *
 * Goal: Verify that jobs execute correctly
 * Expected time: < 5 seconds
 */
class BasicJobExecutionSmoke : public SystemFixture {};

TEST_F(BasicJobExecutionSmoke, CanExecuteMultipleJobs) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result);

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
    ASSERT_TRUE(result);

    EXPECT_TRUE(WaitForJobCompletion(job_count, std::chrono::seconds(2)));
    EXPECT_EQ(completed_jobs_.load(), job_count);
}

TEST_F(BasicJobExecutionSmoke, CanEnqueueAndDequeueFromQueue) {
    auto queue = std::make_shared<kcenon::thread::job_queue>();
    queue->set_notify(true);

    auto job = std::make_unique<kcenon::thread::callback_job>(
        []() -> kcenon::thread::result_void { return {}; }
    );

    auto result = queue->enqueue(std::move(job));
    EXPECT_TRUE(result);
    EXPECT_EQ(queue->size(), 1);

    auto dequeued = queue->try_dequeue();
    EXPECT_TRUE(dequeued.has_value());
    EXPECT_TRUE(queue->empty());
}
