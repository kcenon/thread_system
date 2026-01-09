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

#include <kcenon/thread/diagnostics/health_status.h>

using namespace kcenon::thread::diagnostics;

// ============================================================================
// health_state enum tests
// ============================================================================

TEST(HealthStateTest, HealthStateToStringConversion)
{
	EXPECT_EQ(health_state_to_string(health_state::healthy), "healthy");
	EXPECT_EQ(health_state_to_string(health_state::degraded), "degraded");
	EXPECT_EQ(health_state_to_string(health_state::unhealthy), "unhealthy");
	EXPECT_EQ(health_state_to_string(health_state::unknown), "unknown");
}

TEST(HealthStateTest, InvalidHealthStateReturnsUnknown)
{
	auto invalid_state = static_cast<health_state>(999);
	EXPECT_EQ(health_state_to_string(invalid_state), "unknown");
}

TEST(HealthStateTest, HealthStateToHttpCode)
{
	EXPECT_EQ(health_state_to_http_code(health_state::healthy), 200);
	EXPECT_EQ(health_state_to_http_code(health_state::degraded), 200);
	EXPECT_EQ(health_state_to_http_code(health_state::unhealthy), 503);
	EXPECT_EQ(health_state_to_http_code(health_state::unknown), 503);
}

// ============================================================================
// health_thresholds struct tests
// ============================================================================

TEST(HealthThresholdsTest, DefaultValues)
{
	health_thresholds thresholds;

	EXPECT_DOUBLE_EQ(thresholds.min_success_rate, 0.95);
	EXPECT_DOUBLE_EQ(thresholds.unhealthy_success_rate, 0.8);
	EXPECT_DOUBLE_EQ(thresholds.max_healthy_latency_ms, 100.0);
	EXPECT_DOUBLE_EQ(thresholds.degraded_latency_ms, 500.0);
	EXPECT_DOUBLE_EQ(thresholds.queue_saturation_warning, 0.8);
	EXPECT_DOUBLE_EQ(thresholds.queue_saturation_critical, 0.95);
	EXPECT_DOUBLE_EQ(thresholds.worker_utilization_warning, 0.9);
	EXPECT_EQ(thresholds.min_idle_workers, 0u);
}

// ============================================================================
// component_health struct tests
// ============================================================================

TEST(ComponentHealthTest, IsOperationalWhenHealthy)
{
	component_health comp;
	comp.state = health_state::healthy;

	EXPECT_TRUE(comp.is_operational());
}

TEST(ComponentHealthTest, IsOperationalWhenDegraded)
{
	component_health comp;
	comp.state = health_state::degraded;

	EXPECT_TRUE(comp.is_operational());
}

TEST(ComponentHealthTest, NotOperationalWhenUnhealthy)
{
	component_health comp;
	comp.state = health_state::unhealthy;

	EXPECT_FALSE(comp.is_operational());
}

TEST(ComponentHealthTest, NotOperationalWhenUnknown)
{
	component_health comp;
	comp.state = health_state::unknown;

	EXPECT_FALSE(comp.is_operational());
}

// ============================================================================
// health_status struct tests
// ============================================================================

class HealthStatusTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		status_.overall_status = health_state::healthy;
		status_.status_message = "All systems operational";
		status_.check_time = std::chrono::steady_clock::now();
		status_.uptime_seconds = 3600.0;
		status_.total_jobs_processed = 10000;
		status_.success_rate = 0.99;
		status_.avg_latency_ms = 5.5;
		status_.active_workers = 3;
		status_.total_workers = 4;
		status_.queue_depth = 10;
		status_.queue_capacity = 100;
	}

	health_status status_;
};

TEST_F(HealthStatusTest, DefaultConstruction)
{
	health_status default_status;

	EXPECT_EQ(default_status.overall_status, health_state::unknown);
	EXPECT_TRUE(default_status.status_message.empty());
	EXPECT_DOUBLE_EQ(default_status.uptime_seconds, 0.0);
	EXPECT_EQ(default_status.total_jobs_processed, 0u);
	EXPECT_DOUBLE_EQ(default_status.success_rate, 1.0);
	EXPECT_DOUBLE_EQ(default_status.avg_latency_ms, 0.0);
	EXPECT_EQ(default_status.active_workers, 0u);
	EXPECT_EQ(default_status.total_workers, 0u);
	EXPECT_EQ(default_status.queue_depth, 0u);
	EXPECT_EQ(default_status.queue_capacity, 0u);
	EXPECT_TRUE(default_status.components.empty());
}

TEST_F(HealthStatusTest, IsOperationalWhenHealthy)
{
	status_.overall_status = health_state::healthy;
	EXPECT_TRUE(status_.is_operational());
}

TEST_F(HealthStatusTest, IsOperationalWhenDegraded)
{
	status_.overall_status = health_state::degraded;
	EXPECT_TRUE(status_.is_operational());
}

TEST_F(HealthStatusTest, NotOperationalWhenUnhealthy)
{
	status_.overall_status = health_state::unhealthy;
	EXPECT_FALSE(status_.is_operational());
}

TEST_F(HealthStatusTest, IsHealthyCheck)
{
	status_.overall_status = health_state::healthy;
	EXPECT_TRUE(status_.is_healthy());

	status_.overall_status = health_state::degraded;
	EXPECT_FALSE(status_.is_healthy());
}

TEST_F(HealthStatusTest, HttpStatusCodeMapping)
{
	status_.overall_status = health_state::healthy;
	EXPECT_EQ(status_.http_status_code(), 200);

	status_.overall_status = health_state::degraded;
	EXPECT_EQ(status_.http_status_code(), 200);

	status_.overall_status = health_state::unhealthy;
	EXPECT_EQ(status_.http_status_code(), 503);
}

TEST_F(HealthStatusTest, FindComponentByName)
{
	component_health workers_comp;
	workers_comp.name = "workers";
	workers_comp.state = health_state::healthy;
	status_.components.push_back(workers_comp);

	component_health queue_comp;
	queue_comp.name = "queue";
	queue_comp.state = health_state::degraded;
	status_.components.push_back(queue_comp);

	auto workers = status_.find_component("workers");
	ASSERT_NE(workers, nullptr);
	EXPECT_EQ(workers->state, health_state::healthy);

	auto queue = status_.find_component("queue");
	ASSERT_NE(queue, nullptr);
	EXPECT_EQ(queue->state, health_state::degraded);

	auto nonexistent = status_.find_component("nonexistent");
	EXPECT_EQ(nonexistent, nullptr);
}

TEST_F(HealthStatusTest, CalculateOverallStatusWithNoComponents)
{
	status_.components.clear();
	status_.calculate_overall_status();

	EXPECT_EQ(status_.overall_status, health_state::unknown);
	EXPECT_EQ(status_.status_message, "No components registered");
}

TEST_F(HealthStatusTest, CalculateOverallStatusAllHealthy)
{
	component_health comp1;
	comp1.name = "workers";
	comp1.state = health_state::healthy;
	status_.components.push_back(comp1);

	component_health comp2;
	comp2.name = "queue";
	comp2.state = health_state::healthy;
	status_.components.push_back(comp2);

	status_.calculate_overall_status();

	EXPECT_EQ(status_.overall_status, health_state::healthy);
	EXPECT_EQ(status_.status_message, "All components are healthy");
}

TEST_F(HealthStatusTest, CalculateOverallStatusWithDegraded)
{
	component_health comp1;
	comp1.name = "workers";
	comp1.state = health_state::healthy;
	status_.components.push_back(comp1);

	component_health comp2;
	comp2.name = "queue";
	comp2.state = health_state::degraded;
	status_.components.push_back(comp2);

	status_.calculate_overall_status();

	EXPECT_EQ(status_.overall_status, health_state::degraded);
	EXPECT_EQ(status_.status_message, "One or more components are degraded");
}

TEST_F(HealthStatusTest, CalculateOverallStatusWithUnhealthy)
{
	component_health comp1;
	comp1.name = "workers";
	comp1.state = health_state::unhealthy;
	status_.components.push_back(comp1);

	component_health comp2;
	comp2.name = "queue";
	comp2.state = health_state::healthy;
	status_.components.push_back(comp2);

	status_.calculate_overall_status();

	EXPECT_EQ(status_.overall_status, health_state::unhealthy);
	EXPECT_EQ(status_.status_message, "One or more components are unhealthy");
}

TEST_F(HealthStatusTest, CalculateOverallStatusWithUnknown)
{
	component_health comp1;
	comp1.name = "workers";
	comp1.state = health_state::healthy;
	status_.components.push_back(comp1);

	component_health comp2;
	comp2.name = "queue";
	comp2.state = health_state::unknown;
	status_.components.push_back(comp2);

	status_.calculate_overall_status();

	EXPECT_EQ(status_.overall_status, health_state::degraded);
	EXPECT_EQ(status_.status_message, "One or more components have unknown status");
}

TEST_F(HealthStatusTest, UnhealthyTakesPrecedenceOverDegraded)
{
	component_health comp1;
	comp1.name = "workers";
	comp1.state = health_state::unhealthy;
	status_.components.push_back(comp1);

	component_health comp2;
	comp2.name = "queue";
	comp2.state = health_state::degraded;
	status_.components.push_back(comp2);

	status_.calculate_overall_status();

	EXPECT_EQ(status_.overall_status, health_state::unhealthy);
}

TEST_F(HealthStatusTest, ToJsonContainsRequiredFields)
{
	component_health comp;
	comp.name = "workers";
	comp.state = health_state::healthy;
	comp.message = "Workers operational";
	status_.components.push_back(comp);

	auto json = status_.to_json();

	EXPECT_NE(json.find("\"status\": \"healthy\""), std::string::npos);
	EXPECT_NE(json.find("\"message\""), std::string::npos);
	EXPECT_NE(json.find("\"http_code\": 200"), std::string::npos);
	EXPECT_NE(json.find("\"metrics\""), std::string::npos);
	EXPECT_NE(json.find("\"uptime_seconds\""), std::string::npos);
	EXPECT_NE(json.find("\"total_jobs_processed\""), std::string::npos);
	EXPECT_NE(json.find("\"success_rate\""), std::string::npos);
	EXPECT_NE(json.find("\"workers\""), std::string::npos);
	EXPECT_NE(json.find("\"queue\""), std::string::npos);
	EXPECT_NE(json.find("\"components\""), std::string::npos);
}

TEST_F(HealthStatusTest, ToJsonWithComponentDetails)
{
	component_health comp;
	comp.name = "workers";
	comp.state = health_state::healthy;
	comp.message = "Workers operational";
	comp.details["count"] = "4";
	comp.details["active"] = "3";
	status_.components.push_back(comp);

	auto json = status_.to_json();

	EXPECT_NE(json.find("\"details\""), std::string::npos);
	EXPECT_NE(json.find("\"count\": \"4\""), std::string::npos);
	EXPECT_NE(json.find("\"active\": \"3\""), std::string::npos);
}

TEST_F(HealthStatusTest, ToStringContainsHealthInfo)
{
	component_health comp;
	comp.name = "workers";
	comp.state = health_state::healthy;
	comp.message = "4 workers ready";
	status_.components.push_back(comp);

	auto str = status_.to_string();

	EXPECT_NE(str.find("Health Status:"), std::string::npos);
	EXPECT_NE(str.find("healthy"), std::string::npos);
	EXPECT_NE(str.find("Metrics:"), std::string::npos);
	EXPECT_NE(str.find("Uptime:"), std::string::npos);
	EXPECT_NE(str.find("Jobs processed:"), std::string::npos);
	EXPECT_NE(str.find("Success rate:"), std::string::npos);
	EXPECT_NE(str.find("Workers:"), std::string::npos);
	EXPECT_NE(str.find("Queue:"), std::string::npos);
	EXPECT_NE(str.find("Components:"), std::string::npos);
}

TEST_F(HealthStatusTest, ToPrometheusContainsMetrics)
{
	status_.components.clear();
	component_health comp;
	comp.name = "workers";
	comp.state = health_state::healthy;
	status_.components.push_back(comp);

	auto prometheus = status_.to_prometheus("TestPool");

	EXPECT_NE(prometheus.find("thread_pool_health_status"), std::string::npos);
	EXPECT_NE(prometheus.find("thread_pool_uptime_seconds"), std::string::npos);
	EXPECT_NE(prometheus.find("thread_pool_jobs_total"), std::string::npos);
	EXPECT_NE(prometheus.find("thread_pool_success_rate"), std::string::npos);
	EXPECT_NE(prometheus.find("thread_pool_latency_avg_ms"), std::string::npos);
	EXPECT_NE(prometheus.find("thread_pool_workers_total"), std::string::npos);
	EXPECT_NE(prometheus.find("thread_pool_workers_active"), std::string::npos);
	EXPECT_NE(prometheus.find("thread_pool_workers_idle"), std::string::npos);
	EXPECT_NE(prometheus.find("thread_pool_queue_depth"), std::string::npos);
	EXPECT_NE(prometheus.find("pool=\"TestPool\""), std::string::npos);
}

TEST_F(HealthStatusTest, ToPrometheusHealthValues)
{
	status_.components.clear();
	component_health comp;
	comp.name = "workers";
	comp.state = health_state::healthy;
	status_.components.push_back(comp);

	// Test healthy status
	status_.overall_status = health_state::healthy;
	auto prometheus_healthy = status_.to_prometheus();
	EXPECT_NE(prometheus_healthy.find("thread_pool_health_status{pool=\"default\"} 1"), std::string::npos);

	// Test degraded status
	status_.overall_status = health_state::degraded;
	auto prometheus_degraded = status_.to_prometheus();
	EXPECT_NE(prometheus_degraded.find("thread_pool_health_status{pool=\"default\"} 0.5"), std::string::npos);

	// Test unhealthy status
	status_.overall_status = health_state::unhealthy;
	auto prometheus_unhealthy = status_.to_prometheus();
	EXPECT_NE(prometheus_unhealthy.find("thread_pool_health_status{pool=\"default\"} 0"), std::string::npos);
}

TEST_F(HealthStatusTest, ToPrometheusWithQueueCapacity)
{
	status_.queue_capacity = 100;
	status_.queue_depth = 50;
	status_.components.clear();

	auto prometheus = status_.to_prometheus();

	EXPECT_NE(prometheus.find("thread_pool_queue_capacity"), std::string::npos);
	EXPECT_NE(prometheus.find("thread_pool_queue_saturation"), std::string::npos);
}

TEST_F(HealthStatusTest, ToPrometheusComponentHealth)
{
	status_.components.clear();

	component_health workers_comp;
	workers_comp.name = "workers";
	workers_comp.state = health_state::healthy;
	status_.components.push_back(workers_comp);

	component_health queue_comp;
	queue_comp.name = "queue";
	queue_comp.state = health_state::degraded;
	status_.components.push_back(queue_comp);

	auto prometheus = status_.to_prometheus("MyPool");

	EXPECT_NE(prometheus.find("thread_pool_component_health{pool=\"MyPool\",component=\"workers\"} 1"), std::string::npos);
	EXPECT_NE(prometheus.find("thread_pool_component_health{pool=\"MyPool\",component=\"queue\"} 0.5"), std::string::npos);
}
