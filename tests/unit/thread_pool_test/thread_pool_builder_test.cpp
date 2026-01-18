/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, kcenon
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

#include "gtest/gtest.h"

#include <kcenon/thread/core/thread_pool_builder.h>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/pool_policies/circuit_breaker_policy.h>
#include <kcenon/thread/pool_policies/work_stealing_pool_policy.h>

#include <atomic>
#include <chrono>
#include <thread>

using namespace kcenon::thread;

class ThreadPoolBuilderTest : public ::testing::Test
{
protected:
	void TearDown() override
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
};

TEST_F(ThreadPoolBuilderTest, BasicConstruction)
{
	auto pool = thread_pool_builder("test_pool")
		.with_workers(2)
		.build();

	ASSERT_NE(pool, nullptr);
	EXPECT_FALSE(pool->is_running());
}

TEST_F(ThreadPoolBuilderTest, BuildAndStart)
{
	auto pool = thread_pool_builder("test_pool")
		.with_workers(2)
		.build_and_start();

	ASSERT_NE(pool, nullptr);
	EXPECT_TRUE(pool->is_running());

	pool->stop();
}

TEST_F(ThreadPoolBuilderTest, DefaultWorkerCount)
{
	auto pool = thread_pool_builder("test_pool")
		.build();

	ASSERT_NE(pool, nullptr);

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.is_err());
	EXPECT_TRUE(pool->is_running());

	pool->stop();
}

TEST_F(ThreadPoolBuilderTest, WithCircuitBreaker)
{
	circuit_breaker_config config;
	config.failure_threshold = 3;
	config.open_duration = std::chrono::seconds{10};

	auto pool = thread_pool_builder("cb_pool")
		.with_workers(2)
		.with_circuit_breaker(config)
		.build();

	ASSERT_NE(pool, nullptr);

	auto* cb_policy = pool->find_policy<circuit_breaker_policy>("circuit_breaker_policy");
	ASSERT_NE(cb_policy, nullptr);
	EXPECT_TRUE(cb_policy->is_accepting_work());
	EXPECT_EQ(cb_policy->get_state(), circuit_state::closed);

	pool->stop();
}

TEST_F(ThreadPoolBuilderTest, WithWorkStealing)
{
	auto pool = thread_pool_builder("ws_pool")
		.with_workers(4)
		.with_work_stealing()
		.build();

	ASSERT_NE(pool, nullptr);

	auto* ws_policy = pool->find_policy<work_stealing_pool_policy>("work_stealing_pool_policy");
	ASSERT_NE(ws_policy, nullptr);
	EXPECT_TRUE(ws_policy->is_enabled());

	pool->stop();
}

TEST_F(ThreadPoolBuilderTest, WithWorkStealingCustomConfig)
{
	worker_policy config;
	config.enable_work_stealing = true;
	config.victim_selection = steal_policy::adaptive;
	config.max_steal_attempts = 5;

	auto pool = thread_pool_builder("ws_custom_pool")
		.with_workers(4)
		.with_work_stealing(config)
		.build();

	ASSERT_NE(pool, nullptr);

	auto* ws_policy = pool->find_policy<work_stealing_pool_policy>("work_stealing_pool_policy");
	ASSERT_NE(ws_policy, nullptr);
	EXPECT_TRUE(ws_policy->is_enabled());
	EXPECT_EQ(ws_policy->get_steal_policy(), steal_policy::adaptive);
	EXPECT_EQ(ws_policy->get_max_steal_attempts(), 5u);

	pool->stop();
}

TEST_F(ThreadPoolBuilderTest, WithEnhancedMetrics)
{
	auto pool = thread_pool_builder("metrics_pool")
		.with_workers(2)
		.with_enhanced_metrics()
		.build();

	ASSERT_NE(pool, nullptr);
	EXPECT_TRUE(pool->is_enhanced_metrics_enabled());

	pool->stop();
}

TEST_F(ThreadPoolBuilderTest, WithDiagnostics)
{
	auto pool = thread_pool_builder("diag_pool")
		.with_workers(2)
		.with_diagnostics()
		.build();

	ASSERT_NE(pool, nullptr);

	auto& diag = pool->diagnostics();
	(void)diag;

	pool->stop();
}

TEST_F(ThreadPoolBuilderTest, MultiplePolicies)
{
	circuit_breaker_config cb_config;
	cb_config.failure_threshold = 5;

	auto pool = thread_pool_builder("multi_policy_pool")
		.with_workers(4)
		.with_circuit_breaker(cb_config)
		.with_work_stealing()
		.with_enhanced_metrics()
		.with_diagnostics()
		.build();

	ASSERT_NE(pool, nullptr);

	auto* cb_policy = pool->find_policy<circuit_breaker_policy>("circuit_breaker_policy");
	ASSERT_NE(cb_policy, nullptr);

	auto* ws_policy = pool->find_policy<work_stealing_pool_policy>("work_stealing_pool_policy");
	ASSERT_NE(ws_policy, nullptr);

	EXPECT_TRUE(pool->is_enhanced_metrics_enabled());

	pool->stop();
}

TEST_F(ThreadPoolBuilderTest, JobExecution)
{
	std::atomic<int> counter{0};

	auto pool = thread_pool_builder("exec_pool")
		.with_workers(2)
		.build_and_start();

	ASSERT_NE(pool, nullptr);

	for (int i = 0; i < 10; ++i)
	{
		auto job = std::make_unique<callback_job>(
			[&counter]() -> std::optional<std::string> {
				counter.fetch_add(1, std::memory_order_relaxed);
				return std::nullopt;
			});
		auto result = pool->enqueue(std::move(job));
		EXPECT_FALSE(result.is_err());
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	EXPECT_EQ(counter.load(), 10);

	pool->stop();
}

TEST_F(ThreadPoolBuilderTest, BuilderReuse)
{
	thread_pool_builder builder("reuse_pool");

	auto pool1 = builder
		.with_workers(2)
		.build();

	ASSERT_NE(pool1, nullptr);

	auto pool2 = builder
		.with_workers(4)
		.with_circuit_breaker()
		.build();

	ASSERT_NE(pool2, nullptr);

	EXPECT_NE(pool1.get(), pool2.get());

	pool1->stop();
	pool2->stop();
}

TEST_F(ThreadPoolBuilderTest, FluentInterface)
{
	circuit_breaker_config cb_config;
	worker_policy ws_config;
	ws_config.enable_work_stealing = true;

	auto pool = thread_pool_builder("fluent_pool")
		.with_workers(8)
		.with_circuit_breaker(cb_config)
		.with_work_stealing(ws_config)
		.with_enhanced_metrics()
		.with_diagnostics()
		.build_and_start();

	ASSERT_NE(pool, nullptr);
	EXPECT_TRUE(pool->is_running());

	pool->stop();
}

TEST_F(ThreadPoolBuilderTest, SubmitWithBuilder)
{
	auto pool = thread_pool_builder("submit_pool")
		.with_workers(2)
		.build_and_start();

	ASSERT_NE(pool, nullptr);

	auto future = pool->submit([]() { return 42; });
	EXPECT_EQ(future.get(), 42);

	pool->stop();
}
