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
 * @file protected_job_test.cpp
 * @brief Unit tests for protected_job with circuit breaker integration
 * @since 4.0.0
 */

#include <gtest/gtest.h>

#include <kcenon/thread/resilience/protected_job.h>
#include <kcenon/thread/core/job_builder.h>
#include <kcenon/common/resilience/circuit_breaker.h>
#include <kcenon/common/resilience/circuit_breaker_config.h>
#include <kcenon/common/resilience/circuit_state.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>

using namespace kcenon::thread;
namespace common = kcenon::common;
namespace resilience = kcenon::common::resilience;
using namespace std::chrono_literals;

// Helper: create a circuit breaker with fast test-friendly config
static auto make_test_cb(std::size_t fail_threshold = 2,
                         std::size_t success_threshold = 1,
                         std::chrono::milliseconds timeout = 100ms)
	-> std::shared_ptr<resilience::circuit_breaker> {
	resilience::circuit_breaker_config config;
	config.failure_threshold = fail_threshold;
	config.success_threshold = success_threshold;
	config.timeout = timeout;
	config.failure_window = std::chrono::seconds(60);
	config.half_open_max_requests = 3;
	return std::make_shared<resilience::circuit_breaker>(config);
}

// Helper: create a simple inner job via job_builder
static auto make_success_job(bool& executed)
	-> std::unique_ptr<job> {
	return job_builder()
		.name("success_job")
		.work([&executed]() -> common::VoidResult {
			executed = true;
			return common::ok();
		})
		.build();
}

static auto make_failing_job(bool& executed)
	-> std::unique_ptr<job> {
	return job_builder()
		.name("failing_job")
		.work([&executed]() -> common::VoidResult {
			executed = true;
			return common::VoidResult(
				common::error_info{-1, "simulated failure", "test"});
		})
		.build();
}

// =============================================================================
// Construction tests
// =============================================================================

TEST(ProtectedJobTest, Construction) {
	auto cb = make_test_cb();
	bool executed = false;
	auto inner = make_success_job(executed);
	protected_job pj(std::move(inner), cb);
	// Should construct without error
}

// =============================================================================
// Successful execution tests
// =============================================================================

TEST(ProtectedJobTest, SuccessfulExecutionWhenCircuitClosed) {
	auto cb = make_test_cb();
	EXPECT_EQ(cb->get_state(), resilience::circuit_state::CLOSED);

	bool executed = false;
	auto inner = make_success_job(executed);
	protected_job pj(std::move(inner), cb);

	auto result = pj.do_work();
	EXPECT_TRUE(result.is_ok());
	EXPECT_TRUE(executed);
	EXPECT_EQ(cb->get_state(), resilience::circuit_state::CLOSED);
}

TEST(ProtectedJobTest, GetNameIncludesInnerJobName) {
	auto cb = make_test_cb();
	bool executed = false;
	auto inner = make_success_job(executed);
	protected_job pj(std::move(inner), cb);

	auto name = pj.get_name();
	EXPECT_FALSE(name.empty());
	// Should contain the inner job name
	EXPECT_NE(name.find("success_job"), std::string::npos);
}

// =============================================================================
// Circuit breaker rejection tests
// =============================================================================

TEST(ProtectedJobTest, RejectedWhenCircuitOpen) {
	auto cb = make_test_cb(2);

	// Trip the circuit by recording failures directly
	cb->record_failure();
	cb->record_failure();
	EXPECT_EQ(cb->get_state(), resilience::circuit_state::OPEN);

	bool executed = false;
	auto inner = make_success_job(executed);
	protected_job pj(std::move(inner), cb);

	auto result = pj.do_work();
	EXPECT_TRUE(result.is_err());
	EXPECT_FALSE(executed);
}

// =============================================================================
// State transition through protected_job tests
// =============================================================================

TEST(ProtectedJobTest, FailureRecordedToCircuitBreaker) {
	auto cb = make_test_cb(3);
	EXPECT_EQ(cb->get_state(), resilience::circuit_state::CLOSED);

	// Execute a failing job
	bool executed = false;
	{
		auto inner = make_failing_job(executed);
		protected_job pj(std::move(inner), cb);
		auto result = pj.do_work();
		EXPECT_TRUE(result.is_err());
		EXPECT_TRUE(executed);
	}

	// Circuit should still be CLOSED after 1 failure (threshold=3)
	EXPECT_EQ(cb->get_state(), resilience::circuit_state::CLOSED);
}

TEST(ProtectedJobTest, CircuitOpensAfterThresholdFailures) {
	auto cb = make_test_cb(2);

	// Execute failing jobs until circuit opens
	for (int i = 0; i < 2; ++i) {
		bool executed = false;
		auto inner = make_failing_job(executed);
		protected_job pj(std::move(inner), cb);
		pj.do_work();
	}

	EXPECT_EQ(cb->get_state(), resilience::circuit_state::OPEN);
}

TEST(ProtectedJobTest, SuccessKeepsCircuitClosed) {
	auto cb = make_test_cb(3);

	for (int i = 0; i < 5; ++i) {
		bool executed = false;
		auto inner = make_success_job(executed);
		protected_job pj(std::move(inner), cb);
		auto result = pj.do_work();
		EXPECT_TRUE(result.is_ok());
	}

	EXPECT_EQ(cb->get_state(), resilience::circuit_state::CLOSED);
}

// =============================================================================
// Half-open recovery tests
// =============================================================================

TEST(ProtectedJobTest, HalfOpenAllowsLimitedRequests) {
	auto cb = make_test_cb(2, 1, 50ms);

	// Trip the circuit
	cb->record_failure();
	cb->record_failure();
	EXPECT_EQ(cb->get_state(), resilience::circuit_state::OPEN);

	// Wait for timeout to transition to HALF_OPEN
	std::this_thread::sleep_for(100ms);

	// Should now be HALF_OPEN and allow a request
	EXPECT_TRUE(cb->allow_request());

	bool executed = false;
	auto inner = make_success_job(executed);
	protected_job pj(std::move(inner), cb);
	auto result = pj.do_work();

	EXPECT_TRUE(result.is_ok());
	EXPECT_TRUE(executed);
	// After success_threshold (1) successes, should close
	EXPECT_EQ(cb->get_state(), resilience::circuit_state::CLOSED);
}

// =============================================================================
// Circuit breaker standalone tests
// =============================================================================

TEST(CircuitBreakerTest, InitialStateClosed) {
	auto cb = make_test_cb();
	EXPECT_EQ(cb->get_state(), resilience::circuit_state::CLOSED);
	EXPECT_TRUE(cb->allow_request());
}

TEST(CircuitBreakerTest, AllowRequestReturnsFalseWhenOpen) {
	auto cb = make_test_cb(2);
	cb->record_failure();
	cb->record_failure();
	EXPECT_EQ(cb->get_state(), resilience::circuit_state::OPEN);
	EXPECT_FALSE(cb->allow_request());
}

TEST(CircuitBreakerTest, RecordSuccessKeepsClosed) {
	auto cb = make_test_cb();
	cb->record_success();
	cb->record_success();
	EXPECT_EQ(cb->get_state(), resilience::circuit_state::CLOSED);
}

TEST(CircuitBreakerTest, ConfigDefaults) {
	resilience::circuit_breaker_config config;
	EXPECT_EQ(config.failure_threshold, 5u);
	EXPECT_EQ(config.success_threshold, 2u);
	EXPECT_EQ(config.half_open_max_requests, 3u);
	EXPECT_EQ(config.timeout, std::chrono::seconds(30));
	EXPECT_EQ(config.failure_window, std::chrono::seconds(60));
}
