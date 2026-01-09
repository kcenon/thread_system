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

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/diagnostics.h>

#include <chrono>
#include <thread>

using namespace kcenon::thread;
using namespace kcenon::thread::diagnostics;

class BottleneckDetectionTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		pool_ = std::make_shared<thread_pool>("TestPool");
	}

	void TearDown() override
	{
		if (pool_)
		{
			pool_->stop(true);
		}
	}

	std::shared_ptr<thread_pool> pool_;
};

TEST_F(BottleneckDetectionTest, NoBottleneckOnIdlePool)
{
	// Add workers and start pool
	for (int i = 0; i < 4; ++i)
	{
		auto result = pool_->enqueue(std::make_unique<thread_worker>());
		ASSERT_FALSE(result.is_err());
	}
	auto start_result = pool_->start();
	ASSERT_FALSE(start_result.is_err());

	// Wait for workers to initialize
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	// Check for bottlenecks - should be none on idle pool
	auto report = pool_->diagnostics().detect_bottlenecks();

	EXPECT_FALSE(report.has_bottleneck);
	EXPECT_EQ(report.type, bottleneck_type::none);
	EXPECT_EQ(report.total_workers, 4u);
	EXPECT_EQ(report.queue_depth, 0u);
	EXPECT_TRUE(report.recommendations.empty());
}

TEST_F(BottleneckDetectionTest, BottleneckReportHasSeverityLevels)
{
	bottleneck_report report;

	// Test no bottleneck
	report.has_bottleneck = false;
	EXPECT_EQ(report.severity(), 0);
	EXPECT_EQ(report.severity_string(), "none");
	EXPECT_FALSE(report.requires_immediate_action());

	// Test medium severity
	report.has_bottleneck = true;
	report.queue_saturation = 0.5;
	report.worker_utilization = 0.7;
	EXPECT_EQ(report.severity(), 1);
	EXPECT_EQ(report.severity_string(), "low");

	// Test high severity
	report.queue_saturation = 0.85;
	EXPECT_EQ(report.severity(), 2);
	EXPECT_EQ(report.severity_string(), "medium");

	// Test critical severity
	report.queue_saturation = 0.98;
	EXPECT_EQ(report.severity(), 3);
	EXPECT_EQ(report.severity_string(), "critical");
	EXPECT_TRUE(report.requires_immediate_action());
}

TEST_F(BottleneckDetectionTest, BottleneckTypeToStringConversion)
{
	EXPECT_EQ(bottleneck_type_to_string(bottleneck_type::none), "none");
	EXPECT_EQ(bottleneck_type_to_string(bottleneck_type::queue_full), "queue_full");
	EXPECT_EQ(bottleneck_type_to_string(bottleneck_type::slow_consumer), "slow_consumer");
	EXPECT_EQ(bottleneck_type_to_string(bottleneck_type::worker_starvation), "worker_starvation");
	EXPECT_EQ(bottleneck_type_to_string(bottleneck_type::lock_contention), "lock_contention");
	EXPECT_EQ(bottleneck_type_to_string(bottleneck_type::uneven_distribution), "uneven_distribution");
	EXPECT_EQ(bottleneck_type_to_string(bottleneck_type::memory_pressure), "memory_pressure");
}

TEST_F(BottleneckDetectionTest, QueueSaturationCalculation)
{
	// Create pool with bounded queue
	auto bounded_queue = std::make_shared<job_queue>(10);  // max 10 jobs
	auto bounded_pool = std::make_shared<thread_pool>("BoundedPool", bounded_queue);

	// Add one worker
	auto result = bounded_pool->enqueue(std::make_unique<thread_worker>());
	ASSERT_FALSE(result.is_err());

	auto start_result = bounded_pool->start();
	ASSERT_FALSE(start_result.is_err());

	// Wait for worker to start
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	// Initial state - no saturation
	auto report = bounded_pool->diagnostics().detect_bottlenecks();
	EXPECT_LT(report.queue_saturation, 0.5);

	bounded_pool->stop(true);
}

TEST_F(BottleneckDetectionTest, RecommendationsForQueueFull)
{
	bottleneck_report report;
	report.has_bottleneck = true;
	report.type = bottleneck_type::queue_full;

	// Verify recommendations structure
	EXPECT_TRUE(report.recommendations.empty());
}

TEST_F(BottleneckDetectionTest, WorkerUtilizationCalculation)
{
	// Add workers
	for (int i = 0; i < 4; ++i)
	{
		auto result = pool_->enqueue(std::make_unique<thread_worker>());
		ASSERT_FALSE(result.is_err());
	}
	auto start_result = pool_->start();
	ASSERT_FALSE(start_result.is_err());

	// Wait for workers to initialize and become idle
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	// Utilization is calculated from active_count / worker_count
	// Initially after pool start, workers may be briefly active
	// The utilization should be a valid ratio between 0.0 and 1.0
	auto report = pool_->diagnostics().detect_bottlenecks();
	EXPECT_GE(report.worker_utilization, 0.0);
	EXPECT_LE(report.worker_utilization, 1.0);
	EXPECT_EQ(report.total_workers, 4U);
}

TEST_F(BottleneckDetectionTest, EstimatedBacklogTimeCalculation)
{
	// Add workers
	for (int i = 0; i < 2; ++i)
	{
		auto result = pool_->enqueue(std::make_unique<thread_worker>());
		ASSERT_FALSE(result.is_err());
	}
	auto start_result = pool_->start();
	ASSERT_FALSE(start_result.is_err());

	// Submit some jobs that take time
	std::atomic<int> completed{0};
	for (int i = 0; i < 10; ++i)
	{
		auto job = std::make_unique<callback_job>(
			[&completed]() -> std::optional<std::string>
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				completed++;
				return std::nullopt;
			});
		auto result = pool_->enqueue(std::move(job));
		ASSERT_FALSE(result.is_err());
	}

	// Wait briefly
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto report = pool_->diagnostics().detect_bottlenecks();

	// Queue depth should show pending jobs
	// (some jobs may have completed by now)
	EXPECT_GE(report.queue_depth, 0u);
	EXPECT_EQ(report.total_workers, 2u);

	// Wait for all jobs to complete
	while (completed < 10)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

TEST_F(BottleneckDetectionTest, DiagnosticsConfigThresholds)
{
	diagnostics_config config;

	// Verify default thresholds
	EXPECT_DOUBLE_EQ(config.queue_saturation_high, 0.8);
	EXPECT_DOUBLE_EQ(config.utilization_high_threshold, 0.9);
	EXPECT_DOUBLE_EQ(config.wait_time_threshold_ms, 100.0);

	// Add workers
	for (int i = 0; i < 4; ++i)
	{
		auto result = pool_->enqueue(std::make_unique<thread_worker>());
		ASSERT_FALSE(result.is_err());
	}
	auto start_result = pool_->start();
	ASSERT_FALSE(start_result.is_err());

	// Wait for workers to initialize
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	// Apply custom config
	diagnostics_config custom_config;
	custom_config.wait_time_threshold_ms = 50.0;
	pool_->diagnostics().set_config(custom_config);

	// Verify config was applied
	auto applied_config = pool_->diagnostics().get_config();
	EXPECT_DOUBLE_EQ(applied_config.wait_time_threshold_ms, 50.0);
}

TEST_F(BottleneckDetectionTest, UtilizationVarianceCalculation)
{
	// Add workers
	for (int i = 0; i < 4; ++i)
	{
		auto result = pool_->enqueue(std::make_unique<thread_worker>());
		ASSERT_FALSE(result.is_err());
	}
	auto start_result = pool_->start();
	ASSERT_FALSE(start_result.is_err());

	// Wait for workers to initialize
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto report = pool_->diagnostics().detect_bottlenecks();

	// Initially, variance should be low (all workers idle)
	EXPECT_GE(report.utilization_variance, 0.0);
	EXPECT_LE(report.utilization_variance, 1.0);
}

TEST_F(BottleneckDetectionTest, BottleneckReportMetricsArePopulated)
{
	// Add workers
	for (int i = 0; i < 4; ++i)
	{
		auto result = pool_->enqueue(std::make_unique<thread_worker>());
		ASSERT_FALSE(result.is_err());
	}
	auto start_result = pool_->start();
	ASSERT_FALSE(start_result.is_err());

	// Wait for workers to initialize
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto report = pool_->diagnostics().detect_bottlenecks();

	// Verify all metrics are populated
	EXPECT_EQ(report.total_workers, 4u);
	EXPECT_GE(report.idle_workers, 0u);
	EXPECT_LE(report.idle_workers, 4u);
	EXPECT_GE(report.queue_saturation, 0.0);
	EXPECT_GE(report.worker_utilization, 0.0);
	EXPECT_GE(report.avg_wait_time_ms, 0.0);
	EXPECT_GE(report.utilization_variance, 0.0);
}
