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
    ASSERT_TRUE(r.has_error());
    EXPECT_EQ(r.get_error().code(), error_code::invalid_argument);
}

TEST(job_queue_error, enqueue_batch_empty)
{
    job_queue q;
    std::vector<std::unique_ptr<job>> batch;
    auto r = q.enqueue_batch(std::move(batch));
    ASSERT_TRUE(r.has_error());
    EXPECT_EQ(r.get_error().code(), error_code::invalid_argument);
}

TEST(job_queue_error, enqueue_batch_contains_null)
{
    job_queue q;
    std::vector<std::unique_ptr<job>> batch;
    batch.push_back(std::unique_ptr<job>{});
    auto r = q.enqueue_batch(std::move(batch));
    ASSERT_TRUE(r.has_error());
    EXPECT_EQ(r.get_error().code(), error_code::invalid_argument);
}

TEST(job_queue_error, dequeue_after_stop)
{
    job_queue q;
    q.stop_waiting_dequeue();
    auto r = q.dequeue();
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.get_error().code(), error_code::queue_empty);
}

// Test new stop() method (consistent API)
TEST(job_queue_error, dequeue_after_stop_new_api)
{
    job_queue q;
    q.stop();  // New consistent API
    auto r = q.dequeue();
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.get_error().code(), error_code::queue_empty);
}

// Test that deprecated and new methods behave identically
TEST(job_queue_error, stop_methods_equivalence)
{
    // Test deprecated method
    job_queue q1;
    q1.stop_waiting_dequeue();
    EXPECT_TRUE(q1.is_stopped());

    // Test new method
    job_queue q2;
    q2.stop();
    EXPECT_TRUE(q2.is_stopped());

    // Both should prevent enqueue
    auto job1 = std::make_unique<callback_job>([]() -> result_void { return {}; });
    auto job2 = std::make_unique<callback_job>([]() -> result_void { return {}; });

    auto r1 = q1.enqueue(std::move(job1));
    auto r2 = q2.enqueue(std::move(job2));

    ASSERT_TRUE(r1.has_error());
    ASSERT_TRUE(r2.has_error());
    EXPECT_EQ(r1.get_error().code(), error_code::queue_stopped);
    EXPECT_EQ(r2.get_error().code(), error_code::queue_stopped);
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
