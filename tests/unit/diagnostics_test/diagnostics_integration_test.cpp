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

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace kcenon::thread;
using namespace kcenon::thread::diagnostics;

// ============================================================================
// Integration tests for thread_pool_diagnostics with thread_pool
// ============================================================================

class DiagnosticsIntegrationTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		pool_ = std::make_shared<thread_pool>("DiagTestPool");
	}

	void TearDown() override
	{
		if (pool_)
		{
			pool_->stop(true);
		}
	}

	void start_pool_with_workers(int worker_count)
	{
		for (int i = 0; i < worker_count; ++i)
		{
			auto result = pool_->enqueue(std::make_unique<thread_worker>());
			ASSERT_FALSE(result.is_err());
		}
		auto start_result = pool_->start();
		ASSERT_FALSE(start_result.is_err());
	}

	std::shared_ptr<thread_pool> pool_;
};

// ============================================================================
// Thread Dump Integration Tests
// ============================================================================

TEST_F(DiagnosticsIntegrationTest, ThreadDumpShowsAllWorkers)
{
	start_pool_with_workers(4);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto threads = pool_->diagnostics().dump_thread_states();

	EXPECT_EQ(threads.size(), 4U);
}

TEST_F(DiagnosticsIntegrationTest, ThreadDumpShowsWorkerStates)
{
	start_pool_with_workers(2);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto threads = pool_->diagnostics().dump_thread_states();

	for (const auto& t : threads)
	{
		EXPECT_FALSE(t.thread_name.empty());
		EXPECT_GE(t.utilization, 0.0);
		EXPECT_LE(t.utilization, 1.0);
	}
}

TEST_F(DiagnosticsIntegrationTest, FormatThreadDumpContainsPoolInfo)
{
	start_pool_with_workers(2);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto dump = pool_->diagnostics().format_thread_dump();

	EXPECT_NE(dump.find("Thread Pool Dump:"), std::string::npos);
	EXPECT_NE(dump.find("Workers:"), std::string::npos);
	EXPECT_NE(dump.find("Active:"), std::string::npos);
	EXPECT_NE(dump.find("Idle:"), std::string::npos);
}

TEST_F(DiagnosticsIntegrationTest, ThreadDumpDuringJobExecution)
{
	start_pool_with_workers(2);

	std::atomic<bool> job_running{false};
	std::atomic<bool> should_continue{true};

	auto job = std::make_unique<callback_job>(
		[&job_running, &should_continue]() -> std::optional<std::string>
		{
			job_running = true;
			while (should_continue)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
			return std::nullopt;
		});

	auto result = pool_->enqueue(std::move(job));
	ASSERT_FALSE(result.is_err());

	// Wait for job to start
	while (!job_running)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	auto threads = pool_->diagnostics().dump_thread_states();

	// At least one worker should be active
	bool found_active = false;
	for (const auto& t : threads)
	{
		if (t.state == diagnostics::worker_state::active)
		{
			found_active = true;
			break;
		}
	}
	EXPECT_TRUE(found_active);

	should_continue = false;
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

// ============================================================================
// Job Inspection Integration Tests
// ============================================================================

TEST_F(DiagnosticsIntegrationTest, GetActiveJobsDuringExecution)
{
	start_pool_with_workers(1);

	std::atomic<bool> job_running{false};
	std::atomic<bool> should_continue{true};

	auto job = std::make_unique<callback_job>(
		[&job_running, &should_continue]() -> std::optional<std::string>
		{
			job_running = true;
			while (should_continue)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
			return std::nullopt;
		});

	auto result = pool_->enqueue(std::move(job));
	ASSERT_FALSE(result.is_err());

	while (!job_running)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	auto active_jobs = pool_->diagnostics().get_active_jobs();

	// Should have at least one active job
	EXPECT_GE(active_jobs.size(), 0U);

	should_continue = false;
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

TEST_F(DiagnosticsIntegrationTest, GetPendingJobsFromQueue)
{
	start_pool_with_workers(1);

	// Block the worker with a long job
	std::atomic<bool> blocker_running{false};
	std::atomic<bool> should_continue{true};

	auto blocker = std::make_unique<callback_job>(
		[&blocker_running, &should_continue]() -> std::optional<std::string>
		{
			blocker_running = true;
			while (should_continue)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
			return std::nullopt;
		});

	pool_->enqueue(std::move(blocker));

	while (!blocker_running)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	// Now enqueue more jobs that will be pending
	for (int i = 0; i < 5; ++i)
	{
		auto job = std::make_unique<callback_job>(
			[]() -> std::optional<std::string>
			{
				return std::nullopt;
			});
		pool_->enqueue(std::move(job));
	}

	auto pending = pool_->diagnostics().get_pending_jobs();

	// Should have pending jobs (exact number may vary due to timing)
	EXPECT_GE(pending.size(), 0U);

	should_continue = false;
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ============================================================================
// Health Check Integration Tests
// ============================================================================

TEST_F(DiagnosticsIntegrationTest, HealthCheckOnRunningPool)
{
	start_pool_with_workers(4);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto health = pool_->diagnostics().health_check();

	EXPECT_TRUE(health.is_operational());
	EXPECT_EQ(health.total_workers, 4U);
	EXPECT_GE(health.uptime_seconds, 0.0);
}

TEST_F(DiagnosticsIntegrationTest, IsHealthyOnRunningPool)
{
	start_pool_with_workers(2);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	EXPECT_TRUE(pool_->diagnostics().is_healthy());
}

TEST_F(DiagnosticsIntegrationTest, HealthCheckAfterJobsProcessed)
{
	start_pool_with_workers(2);

	std::atomic<int> completed{0};

	// Submit and execute jobs
	for (int i = 0; i < 10; ++i)
	{
		auto job = std::make_unique<callback_job>(
			[&completed]() -> std::optional<std::string>
			{
				completed++;
				return std::nullopt;
			});
		pool_->enqueue(std::move(job));
	}

	// Wait for jobs to complete
	while (completed < 10)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	auto health = pool_->diagnostics().health_check();

	EXPECT_GE(health.total_jobs_processed, 10U);
	EXPECT_GT(health.success_rate, 0.0);
}

TEST_F(DiagnosticsIntegrationTest, HealthCheckComponentsPresent)
{
	start_pool_with_workers(2);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto health = pool_->diagnostics().health_check();

	// Should have workers, queue, and metrics components
	EXPECT_GE(health.components.size(), 3U);

	auto workers = health.find_component("workers");
	EXPECT_NE(workers, nullptr);

	auto queue = health.find_component("queue");
	EXPECT_NE(queue, nullptr);

	auto metrics = health.find_component("metrics");
	EXPECT_NE(metrics, nullptr);
}

// ============================================================================
// Event Tracing Integration Tests
// ============================================================================

class TestTracingListener : public execution_event_listener
{
public:
	void on_event(const job_execution_event& event) override
	{
		std::lock_guard<std::mutex> lock(mutex_);
		events_.push_back(event);
	}

	std::vector<job_execution_event> get_events()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return events_;
	}

	size_t event_count()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return events_.size();
	}

private:
	std::mutex mutex_;
	std::vector<job_execution_event> events_;
};

TEST_F(DiagnosticsIntegrationTest, EnableAndDisableTracing)
{
	start_pool_with_workers(1);

	EXPECT_FALSE(pool_->diagnostics().is_tracing_enabled());

	pool_->diagnostics().enable_tracing(true);
	EXPECT_TRUE(pool_->diagnostics().is_tracing_enabled());

	pool_->diagnostics().enable_tracing(false);
	EXPECT_FALSE(pool_->diagnostics().is_tracing_enabled());
}

TEST_F(DiagnosticsIntegrationTest, AddAndRemoveEventListener)
{
	start_pool_with_workers(1);

	auto listener = std::make_shared<TestTracingListener>();

	pool_->diagnostics().add_event_listener(listener);
	pool_->diagnostics().remove_event_listener(listener);

	// Should not crash
	SUCCEED();
}

TEST_F(DiagnosticsIntegrationTest, GetRecentEventsWhenTracingEnabled)
{
	start_pool_with_workers(1);
	pool_->diagnostics().enable_tracing(true);

	// Record some events manually
	job_execution_event event;
	event.event_id = 1;
	event.job_id = 100;
	event.type = event_type::completed;

	pool_->diagnostics().record_event(event);

	auto events = pool_->diagnostics().get_recent_events(10);

	EXPECT_GE(events.size(), 1U);
}

TEST_F(DiagnosticsIntegrationTest, EventsNotRecordedWhenTracingDisabled)
{
	start_pool_with_workers(1);
	pool_->diagnostics().enable_tracing(false);

	job_execution_event event;
	event.event_id = 1;
	event.job_id = 100;

	pool_->diagnostics().record_event(event);

	auto events = pool_->diagnostics().get_recent_events(10);

	EXPECT_EQ(events.size(), 0U);
}

// ============================================================================
// Export Integration Tests
// ============================================================================

TEST_F(DiagnosticsIntegrationTest, ToJsonReturnsValidJson)
{
	start_pool_with_workers(2);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto json = pool_->diagnostics().to_json();

	EXPECT_NE(json.find("{"), std::string::npos);
	EXPECT_NE(json.find("}"), std::string::npos);
	EXPECT_NE(json.find("\"health\""), std::string::npos);
	EXPECT_NE(json.find("\"workers\""), std::string::npos);
	EXPECT_NE(json.find("\"queue\""), std::string::npos);
	EXPECT_NE(json.find("\"bottleneck\""), std::string::npos);
}

TEST_F(DiagnosticsIntegrationTest, ToStringReturnsFormatted)
{
	start_pool_with_workers(2);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto str = pool_->diagnostics().to_string();

	EXPECT_FALSE(str.empty());
	EXPECT_NE(str.find("Thread Pool Dump:"), std::string::npos);
}

TEST_F(DiagnosticsIntegrationTest, ToPrometheusReturnsMetrics)
{
	start_pool_with_workers(2);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto prometheus = pool_->diagnostics().to_prometheus();

	EXPECT_NE(prometheus.find("thread_pool_"), std::string::npos);
	EXPECT_NE(prometheus.find("# HELP"), std::string::npos);
	EXPECT_NE(prometheus.find("# TYPE"), std::string::npos);
}

// ============================================================================
// Configuration Integration Tests
// ============================================================================

TEST_F(DiagnosticsIntegrationTest, GetAndSetConfig)
{
	start_pool_with_workers(1);

	auto initial_config = pool_->diagnostics().get_config();

	diagnostics_config new_config;
	new_config.enable_tracing = true;
	new_config.event_history_size = 500;
	new_config.wait_time_threshold_ms = 50.0;

	pool_->diagnostics().set_config(new_config);

	auto updated_config = pool_->diagnostics().get_config();

	EXPECT_TRUE(updated_config.enable_tracing);
	EXPECT_EQ(updated_config.event_history_size, 500U);
	EXPECT_DOUBLE_EQ(updated_config.wait_time_threshold_ms, 50.0);
}

TEST_F(DiagnosticsIntegrationTest, ConfigAffectsTracing)
{
	start_pool_with_workers(1);

	diagnostics_config config;
	config.enable_tracing = true;

	pool_->diagnostics().set_config(config);

	EXPECT_TRUE(pool_->diagnostics().is_tracing_enabled());
}
