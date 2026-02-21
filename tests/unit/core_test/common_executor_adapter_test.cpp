/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, kcenon
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

/**
 * @file common_executor_adapter_test.cpp
 * @brief Unit tests for thread_pool_executor_adapter and common_executor_factory
 * @since 4.0.0
 */

#include <gtest/gtest.h>

#include <kcenon/thread/adapters/common_executor_adapter.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_pool_builder.h>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <thread>

using namespace kcenon::thread;
namespace adapters = kcenon::thread::adapters;
using namespace std::chrono_literals;

// =============================================================================
// Test fixture with a running thread pool
// =============================================================================

class ExecutorAdapterTest : public ::testing::Test {
protected:
	void SetUp() override {
		pool_ = thread_pool_builder("adapter_test")
			.with_workers(2)
			.build_and_start();
		adapter_ = std::make_shared<adapters::thread_pool_executor_adapter>(pool_);
	}

	void TearDown() override {
		adapter_.reset();
		if (pool_ && pool_->is_running()) {
			pool_->stop(false);
		}
	}

	std::shared_ptr<thread_pool> pool_;
	std::shared_ptr<adapters::thread_pool_executor_adapter> adapter_;
};

// Simple IJob implementation for testing
class test_job : public kcenon::common::interfaces::IJob {
public:
	explicit test_job(std::atomic<bool>& executed)
		: executed_(executed) {}

	auto execute() -> kcenon::common::VoidResult override {
		executed_.store(true);
		return kcenon::common::ok();
	}

	auto get_name() const -> std::string override {
		return "test_job";
	}

private:
	std::atomic<bool>& executed_;
};

class failing_test_job : public kcenon::common::interfaces::IJob {
public:
	auto execute() -> kcenon::common::VoidResult override {
		return kcenon::common::VoidResult(
			kcenon::common::error_info{-1, "intentional failure", "test"});
	}

	auto get_name() const -> std::string override {
		return "failing_test_job";
	}
};

// =============================================================================
// Construction tests
// =============================================================================

TEST_F(ExecutorAdapterTest, ConstructionWithPool) {
	EXPECT_NE(adapter_, nullptr);
}

TEST_F(ExecutorAdapterTest, GetThreadPoolReturnsWrappedPool) {
	EXPECT_EQ(adapter_->get_thread_pool(), pool_);
}

// =============================================================================
// submit() tests
// =============================================================================

TEST_F(ExecutorAdapterTest, SubmitExecutesTask) {
	std::atomic<bool> executed{false};

	auto future = adapter_->submit([&executed]() {
		executed.store(true);
	});

	future.get();
	EXPECT_TRUE(executed.load());
}

TEST_F(ExecutorAdapterTest, SubmitMultipleTasks) {
	std::atomic<int> counter{0};
	std::vector<std::future<void>> futures;

	for (int i = 0; i < 10; ++i) {
		futures.push_back(adapter_->submit([&counter]() {
			counter.fetch_add(1);
		}));
	}

	for (auto& f : futures) {
		f.get();
	}
	EXPECT_EQ(counter.load(), 10);
}

// =============================================================================
// execute() with IJob tests
// =============================================================================

TEST_F(ExecutorAdapterTest, ExecuteRunsJob) {
	std::atomic<bool> executed{false};
	auto job = std::make_unique<test_job>(executed);

	auto result = adapter_->execute(std::move(job));
	ASSERT_TRUE(result.is_ok());

	auto future = std::move(result.unwrap());
	future.get();
	EXPECT_TRUE(executed.load());
}

TEST_F(ExecutorAdapterTest, ExecuteFailingJobReportsError) {
	auto job = std::make_unique<failing_test_job>();

	auto result = adapter_->execute(std::move(job));
	ASSERT_TRUE(result.is_ok());

	auto future = std::move(result.unwrap());
	EXPECT_THROW(future.get(), std::runtime_error);
}

// =============================================================================
// State query tests
// =============================================================================

TEST_F(ExecutorAdapterTest, WorkerCountMatchesPool) {
	EXPECT_EQ(adapter_->worker_count(), pool_->get_active_worker_count());
}

TEST_F(ExecutorAdapterTest, IsRunningReflectsPool) {
	EXPECT_TRUE(adapter_->is_running());
}

TEST_F(ExecutorAdapterTest, PendingTasksInitiallyZero) {
	// After setup (no tasks submitted), pending should be 0
	EXPECT_EQ(adapter_->pending_tasks(), 0u);
}

// =============================================================================
// shutdown() tests
// =============================================================================

TEST_F(ExecutorAdapterTest, ShutdownStopsPool) {
	adapter_->shutdown(true);
	EXPECT_FALSE(adapter_->is_running());
}

// =============================================================================
// Factory tests
// =============================================================================

TEST_F(ExecutorAdapterTest, FactoryCreatesAdapter) {
	auto executor = adapters::common_executor_factory::create_from_thread_pool(pool_);
	EXPECT_NE(executor, nullptr);
	EXPECT_TRUE(executor->is_running());
}

TEST_F(ExecutorAdapterTest, FactoryAdapterExecutesJob) {
	auto executor = adapters::common_executor_factory::create_from_thread_pool(pool_);

	std::atomic<bool> executed{false};
	auto job = std::make_unique<test_job>(executed);

	auto result = executor->execute(std::move(job));
	ASSERT_TRUE(result.is_ok());

	auto future = std::move(result.unwrap());
	future.get();
	EXPECT_TRUE(executed.load());
}

// =============================================================================
// Null pool handling tests
// =============================================================================

TEST(ExecutorAdapterNullTest, NullPoolWorkerCountZero) {
	adapters::thread_pool_executor_adapter adapter(nullptr);
	EXPECT_EQ(adapter.worker_count(), 0u);
}

TEST(ExecutorAdapterNullTest, NullPoolNotRunning) {
	adapters::thread_pool_executor_adapter adapter(nullptr);
	EXPECT_FALSE(adapter.is_running());
}

TEST(ExecutorAdapterNullTest, NullPoolPendingZero) {
	adapters::thread_pool_executor_adapter adapter(nullptr);
	EXPECT_EQ(adapter.pending_tasks(), 0u);
}
