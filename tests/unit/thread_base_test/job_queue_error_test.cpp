// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/*****************************************************************************
BSD 3-Clause License
*****************************************************************************/

#include <gtest/gtest.h>

#include <kcenon/thread/core/job_queue.h>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/core/error_handling.h>

using namespace kcenon::thread;

TEST(job_queue_error, enqueue_null)
{
    job_queue q;
    std::unique_ptr<job> j{};
    auto r = q.enqueue(std::move(j));
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error().code, static_cast<int>(error_code::invalid_argument));
}

TEST(job_queue_error, enqueue_batch_empty)
{
    job_queue q;
    std::vector<std::unique_ptr<job>> batch;
    auto r = q.enqueue_batch(std::move(batch));
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error().code, static_cast<int>(error_code::invalid_argument));
}

TEST(job_queue_error, enqueue_batch_contains_null)
{
    job_queue q;
    std::vector<std::unique_ptr<job>> batch;
    batch.push_back(std::unique_ptr<job>{});
    auto r = q.enqueue_batch(std::move(batch));
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error().code, static_cast<int>(error_code::invalid_argument));
}

TEST(job_queue_error, dequeue_after_stop)
{
    job_queue q;
    q.stop();
    auto r = q.dequeue();
    ASSERT_FALSE(r.is_ok());
    EXPECT_EQ(r.error().code, static_cast<int>(error_code::queue_empty));
}

// Test new stop() method (consistent API)
TEST(job_queue_error, dequeue_after_stop_new_api)
{
    job_queue q;
    q.stop();  // New consistent API
    auto r = q.dequeue();
    ASSERT_FALSE(r.is_ok());
    EXPECT_EQ(r.error().code, static_cast<int>(error_code::queue_empty));
}

// Test that stop() method works correctly
TEST(job_queue_error, stop_methods_equivalence)
{
    // Test stop method
    job_queue q1;
    q1.stop();
    EXPECT_TRUE(q1.is_stopped());

    // Test new method
    job_queue q2;
    q2.stop();
    EXPECT_TRUE(q2.is_stopped());

    // Both should prevent enqueue
    auto job1 = std::make_unique<callback_job>([]() -> kcenon::common::VoidResult { return kcenon::common::ok(); });
    auto job2 = std::make_unique<callback_job>([]() -> kcenon::common::VoidResult { return kcenon::common::ok(); });

    auto r1 = q1.enqueue(std::move(job1));
    auto r2 = q2.enqueue(std::move(job2));

    ASSERT_TRUE(r1.is_err());
    ASSERT_TRUE(r2.is_err());
    EXPECT_EQ(r1.error().code, static_cast<int>(error_code::queue_stopped));
    EXPECT_EQ(r2.error().code, static_cast<int>(error_code::queue_stopped));
}

// Test stop() is idempotent
TEST(job_queue_error, stop_is_idempotent)
{
    job_queue q;

    // Multiple stop calls should be safe
    q.stop();
    EXPECT_TRUE(q.is_stopped());

    q.stop();
    EXPECT_TRUE(q.is_stopped());

    q.stop();
    EXPECT_TRUE(q.is_stopped());
}
