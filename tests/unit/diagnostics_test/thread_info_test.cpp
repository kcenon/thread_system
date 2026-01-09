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

#include <kcenon/thread/diagnostics/thread_info.h>
#include <kcenon/thread/diagnostics/job_info.h>

#include <chrono>
#include <thread>

using namespace kcenon::thread::diagnostics;

// ============================================================================
// worker_state enum tests
// ============================================================================

TEST(WorkerStateTest, WorkerStateToStringConversion)
{
	EXPECT_EQ(worker_state_to_string(worker_state::idle), "IDLE");
	EXPECT_EQ(worker_state_to_string(worker_state::active), "ACTIVE");
	EXPECT_EQ(worker_state_to_string(worker_state::stopping), "STOPPING");
	EXPECT_EQ(worker_state_to_string(worker_state::stopped), "STOPPED");
}

TEST(WorkerStateTest, InvalidWorkerStateReturnsUnknown)
{
	auto invalid_state = static_cast<worker_state>(999);
	EXPECT_EQ(worker_state_to_string(invalid_state), "UNKNOWN");
}

// ============================================================================
// thread_info struct tests
// ============================================================================

class ThreadInfoTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		info_.thread_id = std::this_thread::get_id();
		info_.thread_name = "TestWorker-0";
		info_.worker_id = 0;
		info_.state = worker_state::idle;
		info_.state_since = std::chrono::steady_clock::now();
		info_.jobs_completed = 100;
		info_.jobs_failed = 5;
		info_.total_busy_time = std::chrono::milliseconds(5000);
		info_.total_idle_time = std::chrono::milliseconds(1000);
		info_.utilization = 0.833;
	}

	thread_info info_;
};

TEST_F(ThreadInfoTest, DefaultConstruction)
{
	thread_info default_info;

	EXPECT_EQ(default_info.worker_id, 0u);
	EXPECT_EQ(default_info.state, worker_state::idle);
	EXPECT_EQ(default_info.jobs_completed, 0u);
	EXPECT_EQ(default_info.jobs_failed, 0u);
	EXPECT_EQ(default_info.total_busy_time.count(), 0);
	EXPECT_EQ(default_info.total_idle_time.count(), 0);
	EXPECT_DOUBLE_EQ(default_info.utilization, 0.0);
	EXPECT_FALSE(default_info.current_job.has_value());
}

TEST_F(ThreadInfoTest, TotalJobsCalculation)
{
	EXPECT_EQ(info_.total_jobs(), 105u);
}

TEST_F(ThreadInfoTest, SuccessRateCalculation)
{
	EXPECT_NEAR(info_.success_rate(), 100.0 / 105.0, 0.001);
}

TEST_F(ThreadInfoTest, SuccessRateWithNoJobs)
{
	thread_info empty_info;
	EXPECT_DOUBLE_EQ(empty_info.success_rate(), 1.0);
}

TEST_F(ThreadInfoTest, IsBusyWhenActive)
{
	info_.state = worker_state::active;
	EXPECT_TRUE(info_.is_busy());
	EXPECT_FALSE(info_.is_available());
}

TEST_F(ThreadInfoTest, IsAvailableWhenIdle)
{
	info_.state = worker_state::idle;
	EXPECT_FALSE(info_.is_busy());
	EXPECT_TRUE(info_.is_available());
}

TEST_F(ThreadInfoTest, IsNotAvailableWhenStopping)
{
	info_.state = worker_state::stopping;
	EXPECT_FALSE(info_.is_busy());
	EXPECT_FALSE(info_.is_available());
}

TEST_F(ThreadInfoTest, UpdateUtilizationCalculation)
{
	thread_info test_info;
	test_info.total_busy_time = std::chrono::milliseconds(800);
	test_info.total_idle_time = std::chrono::milliseconds(200);
	test_info.update_utilization();

	EXPECT_DOUBLE_EQ(test_info.utilization, 0.8);
}

TEST_F(ThreadInfoTest, UpdateUtilizationWithZeroTime)
{
	thread_info test_info;
	test_info.update_utilization();

	EXPECT_DOUBLE_EQ(test_info.utilization, 0.0);
}

TEST_F(ThreadInfoTest, BusyTimeMsConversion)
{
	EXPECT_DOUBLE_EQ(info_.busy_time_ms(), 5000.0);
}

TEST_F(ThreadInfoTest, IdleTimeMsConversion)
{
	EXPECT_DOUBLE_EQ(info_.idle_time_ms(), 1000.0);
}

TEST_F(ThreadInfoTest, StateDurationIsPositive)
{
	// state_since is set in SetUp() to now()
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	auto duration = info_.state_duration();

	EXPECT_GE(duration.count(), 0);
}

TEST_F(ThreadInfoTest, ToJsonContainsRequiredFields)
{
	auto json = info_.to_json();

	EXPECT_NE(json.find("\"worker_id\""), std::string::npos);
	EXPECT_NE(json.find("\"thread_name\""), std::string::npos);
	EXPECT_NE(json.find("\"thread_id\""), std::string::npos);
	EXPECT_NE(json.find("\"state\""), std::string::npos);
	EXPECT_NE(json.find("\"jobs_completed\""), std::string::npos);
	EXPECT_NE(json.find("\"jobs_failed\""), std::string::npos);
	EXPECT_NE(json.find("\"success_rate\""), std::string::npos);
	EXPECT_NE(json.find("\"utilization\""), std::string::npos);
	EXPECT_NE(json.find("\"busy_time_ms\""), std::string::npos);
	EXPECT_NE(json.find("\"idle_time_ms\""), std::string::npos);
	EXPECT_NE(json.find("\"current_job\": null"), std::string::npos);
}

TEST_F(ThreadInfoTest, ToJsonWithCurrentJob)
{
	job_info job;
	job.job_id = 123;
	job.job_name = "TestJob";
	job.status = job_status::running;
	info_.current_job = job;

	auto json = info_.to_json();

	EXPECT_NE(json.find("\"current_job\":"), std::string::npos);
	EXPECT_EQ(json.find("\"current_job\": null"), std::string::npos);
	EXPECT_NE(json.find("\"job_id\": 123"), std::string::npos);
}

TEST_F(ThreadInfoTest, ToStringContainsWorkerInfo)
{
	auto str = info_.to_string();

	EXPECT_NE(str.find("TestWorker-0"), std::string::npos);
	EXPECT_NE(str.find("IDLE"), std::string::npos);
	EXPECT_NE(str.find("Jobs:"), std::string::npos);
	EXPECT_NE(str.find("Utilization:"), std::string::npos);
}

TEST_F(ThreadInfoTest, ToStringWithCurrentJob)
{
	job_info job;
	job.job_id = 456;
	job.job_name = "RunningJob";
	job.status = job_status::running;
	job.execution_time = std::chrono::milliseconds(150);
	info_.current_job = job;
	info_.state = worker_state::active;

	auto str = info_.to_string();

	EXPECT_NE(str.find("Current Job:"), std::string::npos);
	EXPECT_NE(str.find("RunningJob"), std::string::npos);
}

// ============================================================================
// job_status enum tests
// ============================================================================

TEST(JobStatusTest, JobStatusToStringConversion)
{
	EXPECT_EQ(job_status_to_string(job_status::pending), "pending");
	EXPECT_EQ(job_status_to_string(job_status::running), "running");
	EXPECT_EQ(job_status_to_string(job_status::completed), "completed");
	EXPECT_EQ(job_status_to_string(job_status::failed), "failed");
	EXPECT_EQ(job_status_to_string(job_status::cancelled), "cancelled");
	EXPECT_EQ(job_status_to_string(job_status::timed_out), "timed_out");
}

TEST(JobStatusTest, InvalidJobStatusReturnsUnknown)
{
	auto invalid_status = static_cast<job_status>(999);
	EXPECT_EQ(job_status_to_string(invalid_status), "unknown");
}

// ============================================================================
// job_info struct tests
// ============================================================================

class JobInfoTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		info_.job_id = 12345;
		info_.job_name = "ProcessOrder";
		info_.enqueue_time = std::chrono::steady_clock::now();
		info_.start_time = info_.enqueue_time + std::chrono::milliseconds(10);
		info_.wait_time = std::chrono::milliseconds(10);
		info_.execution_time = std::chrono::milliseconds(100);
		info_.status = job_status::completed;
		info_.executed_by = std::this_thread::get_id();
	}

	job_info info_;
};

TEST_F(JobInfoTest, DefaultConstruction)
{
	job_info default_info;

	EXPECT_EQ(default_info.job_id, 0u);
	EXPECT_TRUE(default_info.job_name.empty());
	EXPECT_EQ(default_info.status, job_status::pending);
	EXPECT_EQ(default_info.wait_time.count(), 0);
	EXPECT_EQ(default_info.execution_time.count(), 0);
	EXPECT_FALSE(default_info.end_time.has_value());
	EXPECT_FALSE(default_info.error_message.has_value());
	EXPECT_FALSE(default_info.stack_trace.has_value());
}

TEST_F(JobInfoTest, TotalLatencyCalculation)
{
	auto total = info_.total_latency();

	EXPECT_EQ(std::chrono::duration_cast<std::chrono::milliseconds>(total).count(), 110);
}

TEST_F(JobInfoTest, IsFinishedForCompletedJob)
{
	info_.status = job_status::completed;
	EXPECT_TRUE(info_.is_finished());
	EXPECT_FALSE(info_.is_active());
}

TEST_F(JobInfoTest, IsFinishedForFailedJob)
{
	info_.status = job_status::failed;
	EXPECT_TRUE(info_.is_finished());
}

TEST_F(JobInfoTest, IsFinishedForCancelledJob)
{
	info_.status = job_status::cancelled;
	EXPECT_TRUE(info_.is_finished());
}

TEST_F(JobInfoTest, IsFinishedForTimedOutJob)
{
	info_.status = job_status::timed_out;
	EXPECT_TRUE(info_.is_finished());
}

TEST_F(JobInfoTest, IsActiveForPendingJob)
{
	info_.status = job_status::pending;
	EXPECT_TRUE(info_.is_active());
	EXPECT_FALSE(info_.is_finished());
}

TEST_F(JobInfoTest, IsActiveForRunningJob)
{
	info_.status = job_status::running;
	EXPECT_TRUE(info_.is_active());
	EXPECT_FALSE(info_.is_finished());
}

TEST_F(JobInfoTest, WaitTimeMsConversion)
{
	EXPECT_DOUBLE_EQ(info_.wait_time_ms(), 10.0);
}

TEST_F(JobInfoTest, ExecutionTimeMsConversion)
{
	EXPECT_DOUBLE_EQ(info_.execution_time_ms(), 100.0);
}

TEST_F(JobInfoTest, ToJsonContainsRequiredFields)
{
	auto json = info_.to_json();

	EXPECT_NE(json.find("\"job_id\": 12345"), std::string::npos);
	EXPECT_NE(json.find("\"job_name\": \"ProcessOrder\""), std::string::npos);
	EXPECT_NE(json.find("\"status\": \"completed\""), std::string::npos);
	EXPECT_NE(json.find("\"wait_time_ms\""), std::string::npos);
	EXPECT_NE(json.find("\"execution_time_ms\""), std::string::npos);
	EXPECT_NE(json.find("\"total_latency_ms\""), std::string::npos);
	EXPECT_NE(json.find("\"thread_id\""), std::string::npos);
	EXPECT_NE(json.find("\"error_message\": null"), std::string::npos);
}

TEST_F(JobInfoTest, ToJsonWithErrorMessage)
{
	info_.status = job_status::failed;
	info_.error_message = "Connection timeout";

	auto json = info_.to_json();

	EXPECT_NE(json.find("\"error_message\": \"Connection timeout\""), std::string::npos);
}

TEST_F(JobInfoTest, ToJsonWithStackTrace)
{
	info_.status = job_status::failed;
	info_.error_message = "NullPointerException";
	info_.stack_trace = "at main.cpp:42";

	auto json = info_.to_json();

	EXPECT_NE(json.find("\"stack_trace\""), std::string::npos);
}

TEST_F(JobInfoTest, ToStringContainsJobInfo)
{
	auto str = info_.to_string();

	EXPECT_NE(str.find("Job#12345"), std::string::npos);
	EXPECT_NE(str.find("ProcessOrder"), std::string::npos);
	EXPECT_NE(str.find("completed"), std::string::npos);
	EXPECT_NE(str.find("Wait:"), std::string::npos);
	EXPECT_NE(str.find("Exec:"), std::string::npos);
	EXPECT_NE(str.find("Total:"), std::string::npos);
	EXPECT_NE(str.find("Thread:"), std::string::npos);
}

TEST_F(JobInfoTest, ToStringWithError)
{
	info_.status = job_status::failed;
	info_.error_message = "Database connection failed";

	auto str = info_.to_string();

	EXPECT_NE(str.find("Error:"), std::string::npos);
	EXPECT_NE(str.find("Database connection failed"), std::string::npos);
}
