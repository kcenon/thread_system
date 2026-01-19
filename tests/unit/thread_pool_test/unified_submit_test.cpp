/**
 * @file unified_submit_test.cpp
 * @brief Unit tests for Unified Submit API (Issue #492)
 *
 * Tests cover:
 * - submit_options struct
 * - Unified submit() method for single tasks
 * - Unified submit() method for batch tasks
 * - submit_wait_all() method
 * - submit_wait_any() method
 */

#include <gtest/gtest.h>

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/core/submit_options.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace kcenon::thread::test {

class UnifiedSubmitTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool_ = std::make_shared<thread_pool>("test_pool");

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
// submit_options tests
// ============================================================================

TEST(SubmitOptionsTest, DefaultConstructor) {
    submit_options opts;
    EXPECT_TRUE(opts.name.empty());
    EXPECT_FALSE(opts.wait_all);
    EXPECT_FALSE(opts.wait_any);
}

TEST(SubmitOptionsTest, ExplicitNameConstructor) {
    submit_options opts("my_job");
    EXPECT_EQ(opts.name, "my_job");
    EXPECT_FALSE(opts.wait_all);
    EXPECT_FALSE(opts.wait_any);
}

TEST(SubmitOptionsTest, NamedFactory) {
    auto opts = submit_options::named("task_name");
    EXPECT_EQ(opts.name, "task_name");
    EXPECT_FALSE(opts.wait_all);
    EXPECT_FALSE(opts.wait_any);
}

TEST(SubmitOptionsTest, AllFactory) {
    auto opts = submit_options::all();
    EXPECT_TRUE(opts.name.empty());
    EXPECT_TRUE(opts.wait_all);
    EXPECT_FALSE(opts.wait_any);
}

TEST(SubmitOptionsTest, AnyFactory) {
    auto opts = submit_options::any();
    EXPECT_TRUE(opts.name.empty());
    EXPECT_FALSE(opts.wait_all);
    EXPECT_TRUE(opts.wait_any);
}

// ============================================================================
// Unified submit() single task tests
// ============================================================================

TEST_F(UnifiedSubmitTest, SubmitSingleReturnsCorrectResult) {
    auto future = pool_->submit([] { return 42; });
    EXPECT_EQ(future.get(), 42);
}

TEST_F(UnifiedSubmitTest, SubmitSingleWithDefaultOptions) {
    auto future = pool_->submit([] { return 100; }, submit_options{});
    EXPECT_EQ(future.get(), 100);
}

TEST_F(UnifiedSubmitTest, SubmitSingleWithNamedJob) {
    auto future = pool_->submit([] { return 200; }, submit_options::named("compute_task"));
    EXPECT_EQ(future.get(), 200);
}

TEST_F(UnifiedSubmitTest, SubmitSingleWithDesignatedInitializer) {
    submit_options opts;
    opts.name = "designated_task";
    auto future = pool_->submit([] { return 300; }, opts);
    EXPECT_EQ(future.get(), 300);
}

TEST_F(UnifiedSubmitTest, SubmitSingleMultipleConcurrent) {
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool_->submit([i] { return i * i; }));
    }

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(futures[i].get(), i * i);
    }
}

TEST_F(UnifiedSubmitTest, SubmitSinglePropagatesException) {
    auto future = pool_->submit([]() -> int { throw std::logic_error("test exception"); });

    EXPECT_THROW(future.get(), std::logic_error);
}

TEST_F(UnifiedSubmitTest, SubmitSingleWithVoidReturn) {
    std::atomic<int> counter{0};

    auto future = pool_->submit([&counter] { counter.fetch_add(1); });

    future.get();
    EXPECT_EQ(counter.load(), 1);
}

// ============================================================================
// Unified submit() batch tests
// ============================================================================

TEST_F(UnifiedSubmitTest, SubmitBatchReturnsFutures) {
    std::vector<std::function<int()>> tasks;
    for (int i = 0; i < 5; ++i) {
        tasks.push_back([i] { return i + 1; });
    }

    auto futures = pool_->submit(std::move(tasks));

    EXPECT_EQ(futures.size(), 5u);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(futures[i].get(), i + 1);
    }
}

TEST_F(UnifiedSubmitTest, SubmitBatchWithOptions) {
    std::vector<std::function<int()>> tasks;
    for (int i = 0; i < 3; ++i) {
        tasks.push_back([i] { return i * 10; });
    }

    auto futures = pool_->submit(std::move(tasks), submit_options::named("batch_job"));

    EXPECT_EQ(futures.size(), 3u);
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(futures[i].get(), i * 10);
    }
}

// ============================================================================
// submit_wait_all() tests
// ============================================================================

TEST_F(UnifiedSubmitTest, SubmitWaitAllBlocksAndReturnsResults) {
    std::vector<std::function<int()>> tasks;
    for (int i = 0; i < 5; ++i) {
        tasks.push_back([i] { return i * 2; });
    }

    auto results = pool_->submit_wait_all(std::move(tasks));

    EXPECT_EQ(results.size(), 5u);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(results[i], i * 2);
    }
}

TEST_F(UnifiedSubmitTest, SubmitWaitAllWithNamedOptions) {
    std::vector<std::function<int()>> tasks;
    for (int i = 0; i < 3; ++i) {
        tasks.push_back([i] { return i + 100; });
    }

    auto results = pool_->submit_wait_all(std::move(tasks), submit_options::named("wait_all_job"));

    EXPECT_EQ(results.size(), 3u);
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(results[i], i + 100);
    }
}

TEST_F(UnifiedSubmitTest, SubmitWaitAllEmptyVector) {
    std::vector<std::function<int()>> empty_tasks;
    auto results = pool_->submit_wait_all(std::move(empty_tasks));
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// submit_wait_any() tests
// ============================================================================

TEST_F(UnifiedSubmitTest, SubmitWaitAnyReturnsFirstResult) {
    std::vector<std::function<int()>> tasks;

    tasks.push_back([] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 1;
    });

    tasks.push_back([] { return 2; });

    auto result = pool_->submit_wait_any(std::move(tasks));

    EXPECT_TRUE(result == 1 || result == 2);
}

TEST_F(UnifiedSubmitTest, SubmitWaitAnyThrowsOnEmptyVector) {
    std::vector<std::function<int()>> empty_tasks;
    EXPECT_THROW(pool_->submit_wait_any(std::move(empty_tasks)), std::invalid_argument);
}

TEST_F(UnifiedSubmitTest, SubmitWaitAnyWithOptions) {
    std::vector<std::function<int()>> tasks;
    tasks.push_back([] { return 10; });
    tasks.push_back([] { return 20; });

    auto result = pool_->submit_wait_any(std::move(tasks), submit_options::named("any_job"));

    EXPECT_TRUE(result == 10 || result == 20);
}

}  // namespace kcenon::thread::test
