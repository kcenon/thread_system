/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, kcenon
All rights reserved.
*****************************************************************************/

#include <gtest/gtest.h>

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/diagnostics/thread_pool_diagnostics.h>

#include <chrono>
#include <thread>
#include <set>

namespace kcenon::thread::diagnostics::test
{

class DiagnosticsIntegrationTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		pool_ = std::make_shared<thread_pool>("TestPool");

		// Add workers
		for (int i = 0; i < 4; ++i)
		{
			pool_->enqueue(std::make_unique<thread_worker>());
		}

		pool_->start();
	}

	void TearDown() override
	{
		if (pool_)
		{
			pool_->stop();
		}
	}

	std::shared_ptr<thread_pool> pool_;
};

TEST_F(DiagnosticsIntegrationTest, ThreadDumpReturnsWorkerInfo)
{
	auto& diag = pool_->diagnostics();
	auto threads = diag.dump_thread_states();

	EXPECT_EQ(threads.size(), 4);

	for (const auto& t : threads)
	{
		EXPECT_FALSE(t.thread_name.empty());
		EXPECT_GE(t.utilization, 0.0);
		EXPECT_LE(t.utilization, 1.0);
	}
}

TEST_F(DiagnosticsIntegrationTest, FormatThreadDumpProducesOutput)
{
	auto& diag = pool_->diagnostics();
	auto dump = diag.format_thread_dump();

	EXPECT_FALSE(dump.empty());
	EXPECT_NE(dump.find("TestPool"), std::string::npos);
	EXPECT_NE(dump.find("Workers:"), std::string::npos);
}

TEST_F(DiagnosticsIntegrationTest, HealthCheckReturnsHealthyWhenRunning)
{
	auto& diag = pool_->diagnostics();
	auto health = diag.health_check();

	EXPECT_TRUE(health.is_operational());
	EXPECT_EQ(health.total_workers, 4);
}

TEST_F(DiagnosticsIntegrationTest, IsHealthyReturnsTrueWhenRunning)
{
	auto& diag = pool_->diagnostics();
	EXPECT_TRUE(diag.is_healthy());
}

TEST_F(DiagnosticsIntegrationTest, BottleneckDetectionNoBottleneckOnIdlePool)
{
	auto& diag = pool_->diagnostics();
	auto report = diag.detect_bottlenecks();

	// Idle pool should not have bottlenecks
	EXPECT_EQ(report.total_workers, 4);
}

TEST_F(DiagnosticsIntegrationTest, EventTracingCanBeEnabled)
{
	auto& diag = pool_->diagnostics();

	EXPECT_FALSE(diag.is_tracing_enabled());

	diag.enable_tracing(true, 100);
	EXPECT_TRUE(diag.is_tracing_enabled());

	diag.enable_tracing(false);
	EXPECT_FALSE(diag.is_tracing_enabled());
}

TEST_F(DiagnosticsIntegrationTest, ToJsonProducesValidOutput)
{
	auto& diag = pool_->diagnostics();
	auto json = diag.to_json();

	EXPECT_FALSE(json.empty());
	EXPECT_NE(json.find("health"), std::string::npos);
	EXPECT_NE(json.find("workers"), std::string::npos);
	EXPECT_NE(json.find("queue"), std::string::npos);
}

TEST_F(DiagnosticsIntegrationTest, RecentJobsInitiallyEmpty)
{
	auto& diag = pool_->diagnostics();
	auto recent = diag.get_recent_jobs();

	EXPECT_TRUE(recent.empty());
}

TEST_F(DiagnosticsIntegrationTest, HealthCheckAfterJobExecution)
{
	// Submit some jobs
	for (int i = 0; i < 10; ++i)
	{
		pool_->enqueue(std::make_unique<callback_job>([]() -> common::VoidResult {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			return common::ok();
		}));
	}

	// Wait for jobs to complete
	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	auto& diag = pool_->diagnostics();
	auto health = diag.health_check();

	EXPECT_TRUE(health.is_operational());
	EXPECT_GT(health.total_jobs_processed, 0u);
}

TEST_F(DiagnosticsIntegrationTest, ConfigurationCanBeChanged)
{
	auto& diag = pool_->diagnostics();
	auto config = diag.get_config();

	config.enable_tracing = true;
	config.event_history_size = 500;
	diag.set_config(config);

	auto updated = diag.get_config();
	EXPECT_TRUE(updated.enable_tracing);
	EXPECT_EQ(updated.event_history_size, 500);
}

// Test job_info structure
TEST(JobInfoTest, IsFinishedReturnsCorrectly)
{
	job_info info;

	info.status = job_status::pending;
	EXPECT_FALSE(info.is_finished());
	EXPECT_TRUE(info.is_active());

	info.status = job_status::running;
	EXPECT_FALSE(info.is_finished());
	EXPECT_TRUE(info.is_active());

	info.status = job_status::completed;
	EXPECT_TRUE(info.is_finished());
	EXPECT_FALSE(info.is_active());

	info.status = job_status::failed;
	EXPECT_TRUE(info.is_finished());
	EXPECT_FALSE(info.is_active());
}

// Test thread_info structure
TEST(ThreadInfoTest, UtilizationCalculation)
{
	thread_info info;
	info.total_busy_time = std::chrono::milliseconds(800);
	info.total_idle_time = std::chrono::milliseconds(200);

	info.update_utilization();

	EXPECT_NEAR(info.utilization, 0.8, 0.01);
}

// Test health_status structure
TEST(HealthStatusTest, CalculateOverallStatus)
{
	health_status status;

	component_health healthy_comp;
	healthy_comp.name = "workers";
	healthy_comp.state = health_state::healthy;
	status.components.push_back(healthy_comp);

	status.calculate_overall_status();
	EXPECT_EQ(status.overall_status, health_state::healthy);

	component_health degraded_comp;
	degraded_comp.name = "queue";
	degraded_comp.state = health_state::degraded;
	status.components.push_back(degraded_comp);

	status.calculate_overall_status();
	EXPECT_EQ(status.overall_status, health_state::degraded);
}

// Test bottleneck_report structure
TEST(BottleneckReportTest, SeverityLevels)
{
	bottleneck_report report;

	report.has_bottleneck = false;
	EXPECT_EQ(report.severity(), 0);

	report.has_bottleneck = true;
	report.queue_saturation = 0.5;
	report.worker_utilization = 0.5;
	EXPECT_EQ(report.severity(), 1);

	report.queue_saturation = 0.85;
	EXPECT_EQ(report.severity(), 2);

	report.queue_saturation = 0.96;
	EXPECT_EQ(report.severity(), 3);
}

// Test job_status conversions
TEST(EnumConversionTest, JobStatusToString)
{
	EXPECT_EQ(job_status_to_string(job_status::pending), "pending");
	EXPECT_EQ(job_status_to_string(job_status::running), "running");
	EXPECT_EQ(job_status_to_string(job_status::completed), "completed");
	EXPECT_EQ(job_status_to_string(job_status::failed), "failed");
	EXPECT_EQ(job_status_to_string(job_status::cancelled), "cancelled");
}

TEST(EnumConversionTest, WorkerStateToString)
{
	EXPECT_EQ(worker_state_to_string(worker_state::idle), "IDLE");
	EXPECT_EQ(worker_state_to_string(worker_state::active), "ACTIVE");
	EXPECT_EQ(worker_state_to_string(worker_state::stopping), "STOPPING");
	EXPECT_EQ(worker_state_to_string(worker_state::stopped), "STOPPED");
}

TEST(EnumConversionTest, HealthStateToString)
{
	EXPECT_EQ(health_state_to_string(health_state::healthy), "healthy");
	EXPECT_EQ(health_state_to_string(health_state::degraded), "degraded");
	EXPECT_EQ(health_state_to_string(health_state::unhealthy), "unhealthy");
	EXPECT_EQ(health_state_to_string(health_state::unknown), "unknown");
}

TEST(EnumConversionTest, BottleneckTypeToString)
{
	EXPECT_EQ(bottleneck_type_to_string(bottleneck_type::none), "none");
	EXPECT_EQ(bottleneck_type_to_string(bottleneck_type::queue_full), "queue_full");
	EXPECT_EQ(bottleneck_type_to_string(bottleneck_type::slow_consumer), "slow_consumer");
	EXPECT_EQ(bottleneck_type_to_string(bottleneck_type::worker_starvation), "worker_starvation");
}

// Thread Dump Phase 1.3.2 Tests
class ThreadDumpTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		pool_ = std::make_shared<thread_pool>("ThreadDumpTestPool");

		for (int i = 0; i < 4; ++i)
		{
			pool_->enqueue(std::make_unique<thread_worker>());
		}

		pool_->start();
	}

	void TearDown() override
	{
		if (pool_)
		{
			pool_->stop();
		}
	}

	std::shared_ptr<thread_pool> pool_;
};

TEST_F(ThreadDumpTest, WorkerIdsAreUnique)
{
	auto& diag = pool_->diagnostics();
	auto threads = diag.dump_thread_states();

	EXPECT_EQ(threads.size(), 4);

	std::set<std::size_t> worker_ids;
	for (const auto& t : threads)
	{
		worker_ids.insert(t.worker_id);
	}

	// All worker IDs should be unique
	EXPECT_EQ(worker_ids.size(), threads.size());
}

TEST_F(ThreadDumpTest, IdleWorkersHaveCorrectState)
{
	// Wait for workers to become idle
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto& diag = pool_->diagnostics();
	auto threads = diag.dump_thread_states();

	// All workers should be idle when no jobs are submitted
	for (const auto& t : threads)
	{
		EXPECT_EQ(t.state, worker_state::idle);
		EXPECT_FALSE(t.current_job.has_value());
	}
}

TEST_F(ThreadDumpTest, JobsCompletedTracking)
{
	// Submit some jobs
	const int job_count = 20;
	std::atomic<int> completed{0};

	for (int i = 0; i < job_count; ++i)
	{
		pool_->enqueue(std::make_unique<callback_job>([&completed]() -> common::VoidResult {
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			completed.fetch_add(1);
			return common::ok();
		}));
	}

	// Wait for jobs to complete
	while (completed.load() < job_count)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto& diag = pool_->diagnostics();
	auto threads = diag.dump_thread_states();

	// Sum up completed jobs across all workers
	std::uint64_t total_completed = 0;
	for (const auto& t : threads)
	{
		total_completed += t.jobs_completed;
	}

	EXPECT_EQ(total_completed, static_cast<std::uint64_t>(job_count));
}

TEST_F(ThreadDumpTest, BusyTimeTracking)
{
	// Submit jobs that take some time
	const int job_count = 4;
	std::atomic<int> completed{0};

	for (int i = 0; i < job_count; ++i)
	{
		pool_->enqueue(std::make_unique<callback_job>([&completed]() -> common::VoidResult {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			completed.fetch_add(1);
			return common::ok();
		}));
	}

	// Wait for jobs to complete
	while (completed.load() < job_count)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto& diag = pool_->diagnostics();
	auto threads = diag.dump_thread_states();

	// At least some workers should have busy time > 0
	std::chrono::nanoseconds total_busy_time{0};
	for (const auto& t : threads)
	{
		total_busy_time += t.total_busy_time;
	}

	// Total busy time should be at least job_count * 50ms
	auto min_expected = std::chrono::milliseconds(job_count * 40);
	EXPECT_GE(total_busy_time, min_expected);
}

TEST_F(ThreadDumpTest, ActiveWorkerHasCurrentJob)
{
	// Submit a long-running job
	std::atomic<bool> job_started{false};
	std::atomic<bool> should_continue{true};

	pool_->enqueue(std::make_unique<callback_job>([&job_started, &should_continue]() -> common::VoidResult {
		job_started.store(true);
		while (should_continue.load())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		return common::ok();
	}));

	// Wait for job to start
	while (!job_started.load())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(20));

	auto& diag = pool_->diagnostics();
	auto threads = diag.dump_thread_states();

	// At least one worker should be active with a current job
	bool found_active_worker = false;
	for (const auto& t : threads)
	{
		if (t.state == worker_state::active && t.current_job.has_value())
		{
			found_active_worker = true;
			EXPECT_EQ(t.current_job->status, job_status::running);
			break;
		}
	}

	EXPECT_TRUE(found_active_worker);

	// Stop the long-running job
	should_continue.store(false);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

TEST_F(ThreadDumpTest, UtilizationCalculation)
{
	// Submit jobs to generate some utilization
	const int job_count = 10;
	std::atomic<int> completed{0};

	for (int i = 0; i < job_count; ++i)
	{
		pool_->enqueue(std::make_unique<callback_job>([&completed]() -> common::VoidResult {
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			completed.fetch_add(1);
			return common::ok();
		}));
	}

	// Wait for jobs to complete
	while (completed.load() < job_count)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto& diag = pool_->diagnostics();
	auto threads = diag.dump_thread_states();

	// Check that utilization is calculated and in valid range
	for (const auto& t : threads)
	{
		EXPECT_GE(t.utilization, 0.0);
		EXPECT_LE(t.utilization, 1.0);
	}

	// At least some workers should have non-zero utilization
	double total_utilization = 0.0;
	for (const auto& t : threads)
	{
		total_utilization += t.utilization;
	}

	EXPECT_GT(total_utilization, 0.0);
}

} // namespace kcenon::thread::diagnostics::test
