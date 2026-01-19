/**
 * @file future_promise_test.cpp
 * @brief Unit tests for Future/Promise Integration feature (Issue #377)
 *
 * Tests cover:
 * - future_job template class
 * - submit_async method
 * - Batch operations (submit_batch_async, submit_all, submit_any)
 * - when_all/when_any helpers
 * - cancellable_future
 */

#include <gtest/gtest.h>

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/core/future_job.h>
#include <kcenon/thread/core/cancellable_future.h>
#include <kcenon/thread/core/submit_options.h>
#include <kcenon/thread/utils/when_helpers.h>

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace kcenon::thread::test {

class FuturePromiseTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool_ = std::make_shared<thread_pool>("test_pool");

        // Add workers to the pool
        for (int i = 0; i < 4; ++i) {
            auto worker = std::make_unique<thread_worker>();
            pool_->enqueue(std::move(worker));
        }

        pool_->start();
    }

    void TearDown() override {
        if (pool_) {
            pool_->stop();
        }
    }

    std::shared_ptr<thread_pool> pool_;
};

// ============================================================================
// future_job tests
// ============================================================================

TEST_F(FuturePromiseTest, FutureJobReturnsIntResult) {
    auto [job_ptr, future] = make_future_job([]{ return 42; });
    pool_->enqueue(std::move(job_ptr));

    auto result = future.get();
    EXPECT_EQ(result, 42);
}

TEST_F(FuturePromiseTest, FutureJobReturnsStringResult) {
    auto [job_ptr, future] = make_future_job([]{ return std::string("hello"); });
    pool_->enqueue(std::move(job_ptr));

    auto result = future.get();
    EXPECT_EQ(result, "hello");
}

TEST_F(FuturePromiseTest, FutureJobHandlesVoidReturn) {
    std::atomic<int> counter{0};

    auto [job_ptr, future] = make_future_job([&counter]{
        counter.fetch_add(1);
    });
    pool_->enqueue(std::move(job_ptr));

    future.get();  // Should not throw
    EXPECT_EQ(counter.load(), 1);
}

TEST_F(FuturePromiseTest, FutureJobPropagatesException) {
    auto [job_ptr, future] = make_future_job([]() -> int {
        throw std::runtime_error("test error");
    });
    pool_->enqueue(std::move(job_ptr));

    EXPECT_THROW(future.get(), std::runtime_error);
}

// ============================================================================
// submit tests
// ============================================================================

TEST_F(FuturePromiseTest, SubmitReturnsCorrectResult) {
    auto future = pool_->submit([]{ return 100; });
    EXPECT_EQ(future.get(), 100);
}

TEST_F(FuturePromiseTest, SubmitWithNamedJob) {
    auto future = pool_->submit([]{ return 200; }, submit_options::named("named_job"));
    EXPECT_EQ(future.get(), 200);
}

TEST_F(FuturePromiseTest, SubmitMultipleConcurrent) {
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool_->submit([i]{ return i * i; }));
    }

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(futures[i].get(), i * i);
    }
}

// ============================================================================
// Batch operation tests
// ============================================================================

TEST_F(FuturePromiseTest, SubmitBatchReturnsFutures) {
    std::vector<std::function<int()>> tasks;
    for (int i = 0; i < 5; ++i) {
        tasks.push_back([i]{ return i + 1; });
    }

    auto futures = pool_->submit(std::move(tasks));

    EXPECT_EQ(futures.size(), 5u);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(futures[i].get(), i + 1);
    }
}

TEST_F(FuturePromiseTest, SubmitWaitAllBlocksAndReturnsResults) {
    std::vector<std::function<int()>> tasks;
    for (int i = 0; i < 5; ++i) {
        tasks.push_back([i]{ return i * 2; });
    }

    auto results = pool_->submit_wait_all(std::move(tasks));

    EXPECT_EQ(results.size(), 5u);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(results[i], i * 2);
    }
}

TEST_F(FuturePromiseTest, SubmitWaitAnyReturnsFirstResult) {
    std::vector<std::function<int()>> tasks;

    // Add a slow task
    tasks.push_back([]{
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 1;
    });

    // Add a fast task
    tasks.push_back([]{ return 2; });

    auto result = pool_->submit_wait_any(std::move(tasks));

    // Should return quickly (the fast task)
    EXPECT_TRUE(result == 1 || result == 2);
}

TEST_F(FuturePromiseTest, SubmitWaitAnyThrowsOnEmptyVector) {
    std::vector<std::function<int()>> empty_tasks;
    EXPECT_THROW(pool_->submit_wait_any(std::move(empty_tasks)), std::invalid_argument);
}

// ============================================================================
// when_all tests
// ============================================================================

TEST_F(FuturePromiseTest, WhenAllCombinesMultipleFutures) {
    auto f1 = pool_->submit([]{ return 1; });
    auto f2 = pool_->submit([]{ return 2; });
    auto f3 = pool_->submit([]{ return 3; });

    auto combined = when_all(std::move(f1), std::move(f2), std::move(f3));
    auto [a, b, c] = combined.get();

    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 2);
    EXPECT_EQ(c, 3);
}

TEST_F(FuturePromiseTest, WhenAllWithDifferentTypes) {
    auto f1 = pool_->submit([]{ return 42; });
    auto f2 = pool_->submit([]{ return std::string("hello"); });

    auto combined = when_all(std::move(f1), std::move(f2));
    auto [num, str] = combined.get();

    EXPECT_EQ(num, 42);
    EXPECT_EQ(str, "hello");
}

// ============================================================================
// when_any tests
// ============================================================================

TEST_F(FuturePromiseTest, WhenAnyReturnsFirstCompleted) {
    std::vector<std::future<int>> futures;

    futures.push_back(pool_->submit([]{
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 1;
    }));

    futures.push_back(pool_->submit([]{
        return 2;  // Fast
    }));

    auto result = when_any(std::move(futures));
    auto value = result.get();

    EXPECT_TRUE(value == 1 || value == 2);
}

TEST_F(FuturePromiseTest, WhenAnyWithIndexReturnsCorrectIndex) {
    std::vector<std::future<int>> futures;

    futures.push_back(pool_->submit([]{
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 100;
    }));

    futures.push_back(pool_->submit([]{
        return 200;  // Fast - should complete first
    }));

    auto result = when_any_with_index(std::move(futures));
    auto [idx, value] = result.get();

    // The second task (index 1) should complete first
    // But due to scheduling, either could complete first
    EXPECT_TRUE((idx == 0 && value == 100) || (idx == 1 && value == 200));
}

// ============================================================================
// cancellable_future tests
// ============================================================================

TEST_F(FuturePromiseTest, CancellableFutureBasicUsage) {
    auto token = cancellation_token::create();
    auto future = pool_->submit([]{
        return 42;
    });

    cancellable_future<int> cf(std::move(future), token);
    EXPECT_FALSE(cf.is_cancelled());
    EXPECT_EQ(cf.get(), 42);
}

TEST_F(FuturePromiseTest, CancellableFutureCancel) {
    auto token = cancellation_token::create();
    auto future = pool_->submit([]{
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return 42;
    });

    cancellable_future<int> cf(std::move(future), token);
    cf.cancel();

    EXPECT_TRUE(cf.is_cancelled());
}

TEST_F(FuturePromiseTest, CancellableFutureGetForWithTimeout) {
    auto token = cancellation_token::create();
    auto future = pool_->submit([]{
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return 42;
    });

    cancellable_future<int> cf(std::move(future), token);

    // Short timeout - should return nullopt
    auto result = cf.get_for(std::chrono::milliseconds(10));
    EXPECT_FALSE(result.has_value());
}

TEST_F(FuturePromiseTest, CancellableFutureIsReady) {
    auto token = cancellation_token::create();
    auto future = pool_->submit([]{
        return 42;
    });

    cancellable_future<int> cf(std::move(future), token);

    // Wait for completion
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(cf.is_ready());
}

// ============================================================================
// Exception propagation tests
// ============================================================================

TEST_F(FuturePromiseTest, SubmitPropagatesException) {
    auto future = pool_->submit([]() -> int {
        throw std::logic_error("test exception");
    });

    EXPECT_THROW(future.get(), std::logic_error);
}

TEST_F(FuturePromiseTest, WhenAllPropagatesException) {
    auto f1 = pool_->submit([]{ return 1; });
    auto f2 = pool_->submit([]() -> int {
        throw std::runtime_error("error in f2");
    });

    auto combined = when_all(std::move(f1), std::move(f2));
    EXPECT_THROW(combined.get(), std::runtime_error);
}

} // namespace kcenon::thread::test
