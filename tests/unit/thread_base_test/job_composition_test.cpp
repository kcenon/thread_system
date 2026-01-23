/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, DongCheol Shin
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
 * @file job_composition_test.cpp
 * @brief Unit tests for job composition pattern (with_*() methods)
 *
 * Tests the new composition-based job configuration pattern introduced
 * as part of the "Composition over Inheritance" refactoring initiative.
 */

#include <gtest/gtest.h>
#include <kcenon/thread/core/job.h>
#include <kcenon/thread/core/callback_job.h>
#include <atomic>
#include <string>

using namespace kcenon::thread;

/**
 * @class JobCompositionTest
 * @brief Test fixture for job composition functionality
 */
class JobCompositionTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		callback_invoked = false;
		error_callback_invoked = false;
		received_result_ok = false;
		received_error_code = 0;
		received_error_message.clear();
	}

	bool callback_invoked{false};
	bool error_callback_invoked{false};
	bool received_result_ok{false};
	int received_error_code{0};
	std::string received_error_message;
};

// ============================================================================
// Basic Composition Tests
// ============================================================================

TEST_F(JobCompositionTest, JobHasNoComponentsByDefault)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	EXPECT_FALSE(job->has_components());
}

TEST_F(JobCompositionTest, JobHasComponentsAfterWithOnComplete)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	job->with_on_complete([](auto) {});

	EXPECT_TRUE(job->has_components());
}

TEST_F(JobCompositionTest, JobHasComponentsAfterWithOnError)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	job->with_on_error([](const auto&) {});

	EXPECT_TRUE(job->has_components());
}

TEST_F(JobCompositionTest, JobHasComponentsAfterWithPriority)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	job->with_priority(job_priority::high);

	EXPECT_TRUE(job->has_components());
}

// ============================================================================
// Priority Tests
// ============================================================================

TEST_F(JobCompositionTest, DefaultPriorityIsNormal)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	EXPECT_EQ(job->get_priority(), job_priority::normal);
}

TEST_F(JobCompositionTest, PriorityCanBeSetToHigh)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	job->with_priority(job_priority::high);

	EXPECT_EQ(job->get_priority(), job_priority::high);
}

TEST_F(JobCompositionTest, PriorityCanBeSetToLowest)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	job->with_priority(job_priority::lowest);

	EXPECT_EQ(job->get_priority(), job_priority::lowest);
}

TEST_F(JobCompositionTest, PriorityCanBeSetToRealtime)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	job->with_priority(job_priority::realtime);

	EXPECT_EQ(job->get_priority(), job_priority::realtime);
}

// ============================================================================
// Fluent Interface Tests
// ============================================================================

TEST_F(JobCompositionTest, WithOnCompleteReturnsJobReference)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	auto& ref = job->with_on_complete([](auto) {});

	EXPECT_EQ(&ref, job.get());
}

TEST_F(JobCompositionTest, WithOnErrorReturnsJobReference)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	auto& ref = job->with_on_error([](const auto&) {});

	EXPECT_EQ(&ref, job.get());
}

TEST_F(JobCompositionTest, WithPriorityReturnsJobReference)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	auto& ref = job->with_priority(job_priority::high);

	EXPECT_EQ(&ref, job.get());
}

TEST_F(JobCompositionTest, MethodChainingWorks)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	job->with_priority(job_priority::high)
		.with_on_complete([](auto) {})
		.with_on_error([](const auto&) {});

	EXPECT_TRUE(job->has_components());
	EXPECT_EQ(job->get_priority(), job_priority::high);
}

// ============================================================================
// Callback Invocation Tests (via invoke_callbacks)
// ============================================================================

/**
 * @class TestableJob
 * @brief Job that exposes invoke_callbacks for testing
 */
class TestableJob : public job
{
public:
	explicit TestableJob(const std::string& name = "testable_job")
		: job(name)
	{
	}

	[[nodiscard]] auto do_work() -> kcenon::common::VoidResult override
	{
		auto result = work_function_();
		invoke_callbacks(result);
		return result;
	}

	void set_work(std::function<kcenon::common::VoidResult()> fn)
	{
		work_function_ = std::move(fn);
	}

private:
	std::function<kcenon::common::VoidResult()> work_function_ = []() {
		return kcenon::common::ok();
	};
};

TEST_F(JobCompositionTest, OnCompleteCallbackInvokedOnSuccess)
{
	auto job = std::make_unique<TestableJob>("success_job");
	job->set_work([]() { return kcenon::common::ok(); });

	job->with_on_complete([this](auto result) {
		callback_invoked = true;
		received_result_ok = result.is_ok();
	});

	auto result = job->do_work();

	EXPECT_TRUE(result.is_ok());
	EXPECT_TRUE(callback_invoked);
	EXPECT_TRUE(received_result_ok);
}

TEST_F(JobCompositionTest, OnCompleteCallbackInvokedOnError)
{
	auto job = std::make_unique<TestableJob>("error_job");
	job->set_work([]() {
		return kcenon::common::error_info{-100, "Test error", "test"};
	});

	job->with_on_complete([this](auto result) {
		callback_invoked = true;
		received_result_ok = result.is_ok();
		if (result.is_err())
		{
			received_error_code = result.error().code;
			received_error_message = result.error().message;
		}
	});

	auto result = job->do_work();

	EXPECT_TRUE(result.is_err());
	EXPECT_TRUE(callback_invoked);
	EXPECT_FALSE(received_result_ok);
	EXPECT_EQ(received_error_code, -100);
	EXPECT_EQ(received_error_message, "Test error");
}

TEST_F(JobCompositionTest, OnErrorCallbackNotInvokedOnSuccess)
{
	auto job = std::make_unique<TestableJob>("success_job");
	job->set_work([]() { return kcenon::common::ok(); });

	job->with_on_error([this](const auto&) {
		error_callback_invoked = true;
	});

	auto result = job->do_work();

	EXPECT_TRUE(result.is_ok());
	EXPECT_FALSE(error_callback_invoked);
}

TEST_F(JobCompositionTest, OnErrorCallbackInvokedOnError)
{
	auto job = std::make_unique<TestableJob>("error_job");
	job->set_work([]() {
		return kcenon::common::error_info{-200, "Error message", "test"};
	});

	job->with_on_error([this](const auto& err) {
		error_callback_invoked = true;
		received_error_code = err.code;
		received_error_message = err.message;
	});

	auto result = job->do_work();

	EXPECT_TRUE(result.is_err());
	EXPECT_TRUE(error_callback_invoked);
	EXPECT_EQ(received_error_code, -200);
	EXPECT_EQ(received_error_message, "Error message");
}

TEST_F(JobCompositionTest, BothCallbacksInvokedOnError)
{
	auto job = std::make_unique<TestableJob>("error_job");
	job->set_work([]() {
		return kcenon::common::error_info{-300, "Both callbacks", "test"};
	});

	job->with_on_error([this](const auto&) {
		error_callback_invoked = true;
	});

	job->with_on_complete([this](auto) {
		callback_invoked = true;
	});

	auto result = job->do_work();

	EXPECT_TRUE(result.is_err());
	EXPECT_TRUE(error_callback_invoked);
	EXPECT_TRUE(callback_invoked);
}

// ============================================================================
// Memory Efficiency Tests
// ============================================================================

TEST_F(JobCompositionTest, NoMemoryAllocatedWithoutComposition)
{
	auto job1 = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"job1"
	);

	auto job2 = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"job2"
	);

	// Both jobs should have no components (lazy initialization)
	EXPECT_FALSE(job1->has_components());
	EXPECT_FALSE(job2->has_components());
}

TEST_F(JobCompositionTest, ComponentsAllocatedOnlyWhenNeeded)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	EXPECT_FALSE(job->has_components());

	// This should trigger allocation
	job->with_priority(job_priority::high);

	EXPECT_TRUE(job->has_components());
}

// ============================================================================
// Priority Enum Tests
// ============================================================================

TEST_F(JobCompositionTest, PriorityEnumValuesAreOrdered)
{
	EXPECT_LT(static_cast<int>(job_priority::lowest), static_cast<int>(job_priority::low));
	EXPECT_LT(static_cast<int>(job_priority::low), static_cast<int>(job_priority::normal));
	EXPECT_LT(static_cast<int>(job_priority::normal), static_cast<int>(job_priority::high));
	EXPECT_LT(static_cast<int>(job_priority::high), static_cast<int>(job_priority::highest));
	EXPECT_LT(static_cast<int>(job_priority::highest), static_cast<int>(job_priority::realtime));
}

TEST_F(JobCompositionTest, AllPriorityLevelsCanBeSet)
{
	const job_priority priorities[] = {
		job_priority::lowest,
		job_priority::low,
		job_priority::normal,
		job_priority::high,
		job_priority::highest,
		job_priority::realtime
	};

	for (auto priority : priorities)
	{
		auto job = std::make_unique<callback_job>(
			[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
			"test_job"
		);
		job->with_priority(priority);
		EXPECT_EQ(job->get_priority(), priority);
	}
}

// ============================================================================
// Retry Policy Tests
// ============================================================================

TEST_F(JobCompositionTest, NoRetryPolicyByDefault)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	EXPECT_FALSE(job->get_retry_policy().has_value());
}

TEST_F(JobCompositionTest, RetryPolicyCanBeSet)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	auto policy = retry_policy::fixed(3, std::chrono::milliseconds(100));
	job->with_retry(policy);

	EXPECT_TRUE(job->get_retry_policy().has_value());
	EXPECT_EQ(job->get_retry_policy()->get_max_attempts(), 3);
	EXPECT_EQ(job->get_retry_policy()->get_initial_delay(), std::chrono::milliseconds(100));
}

TEST_F(JobCompositionTest, WithRetryReturnsJobReference)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	auto& ref = job->with_retry(retry_policy::no_retry());

	EXPECT_EQ(&ref, job.get());
}

TEST_F(JobCompositionTest, ExponentialBackoffRetryPolicy)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	auto policy = retry_policy::exponential_backoff(5, std::chrono::milliseconds(50), 2.0);
	job->with_retry(policy);

	auto retrieved = job->get_retry_policy();
	EXPECT_TRUE(retrieved.has_value());
	EXPECT_EQ(retrieved->get_strategy(), retry_strategy::exponential_backoff);
	EXPECT_EQ(retrieved->get_max_attempts(), 5);
	EXPECT_EQ(retrieved->get_initial_delay(), std::chrono::milliseconds(50));
	EXPECT_DOUBLE_EQ(retrieved->get_multiplier(), 2.0);
}

// ============================================================================
// Timeout Tests
// ============================================================================

TEST_F(JobCompositionTest, NoTimeoutByDefault)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	EXPECT_FALSE(job->get_timeout().has_value());
}

TEST_F(JobCompositionTest, TimeoutCanBeSet)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	job->with_timeout(std::chrono::milliseconds(5000));

	EXPECT_TRUE(job->get_timeout().has_value());
	EXPECT_EQ(job->get_timeout().value(), std::chrono::milliseconds(5000));
}

TEST_F(JobCompositionTest, WithTimeoutReturnsJobReference)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	auto& ref = job->with_timeout(std::chrono::seconds(30));

	EXPECT_EQ(&ref, job.get());
}

// ============================================================================
// Cancellation Composition Tests
// ============================================================================

TEST_F(JobCompositionTest, NoExplicitCancellationByDefault)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	EXPECT_FALSE(job->has_explicit_cancellation());
}

TEST_F(JobCompositionTest, WithCancellationSetsToken)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	cancellation_token token;
	job->with_cancellation(token);

	EXPECT_TRUE(job->has_explicit_cancellation());
	EXPECT_TRUE(job->has_components());
}

TEST_F(JobCompositionTest, WithCancellationReturnsJobReference)
{
	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	cancellation_token token;
	auto& ref = job->with_cancellation(token);

	EXPECT_EQ(&ref, job.get());
}

// ============================================================================
// Combined Composition Tests
// ============================================================================

TEST_F(JobCompositionTest, AllCompositionMethodsCanBeChained)
{
	cancellation_token token;

	auto job = std::make_unique<callback_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test_job"
	);

	job->with_priority(job_priority::high)
		.with_cancellation(token)
		.with_retry(retry_policy::fixed(3, std::chrono::milliseconds(100)))
		.with_timeout(std::chrono::seconds(30))
		.with_on_complete([](auto) {})
		.with_on_error([](const auto&) {});

	EXPECT_TRUE(job->has_components());
	EXPECT_EQ(job->get_priority(), job_priority::high);
	EXPECT_TRUE(job->has_explicit_cancellation());
	EXPECT_TRUE(job->get_retry_policy().has_value());
	EXPECT_TRUE(job->get_timeout().has_value());
}

// ============================================================================
// Retry Policy Class Tests
// ============================================================================

TEST(RetryPolicyTest, NoRetryPolicyHasCorrectDefaults)
{
	auto policy = retry_policy::no_retry();

	EXPECT_EQ(policy.get_strategy(), retry_strategy::none);
	EXPECT_EQ(policy.get_max_attempts(), 1);
	EXPECT_FALSE(policy.is_retry_enabled());
}

TEST(RetryPolicyTest, FixedPolicyConfiguration)
{
	auto policy = retry_policy::fixed(5, std::chrono::milliseconds(200));

	EXPECT_EQ(policy.get_strategy(), retry_strategy::fixed);
	EXPECT_EQ(policy.get_max_attempts(), 5);
	EXPECT_EQ(policy.get_initial_delay(), std::chrono::milliseconds(200));
	EXPECT_TRUE(policy.is_retry_enabled());
}

TEST(RetryPolicyTest, LinearPolicyConfiguration)
{
	auto policy = retry_policy::linear(4, std::chrono::milliseconds(100));

	EXPECT_EQ(policy.get_strategy(), retry_strategy::linear);
	EXPECT_EQ(policy.get_max_attempts(), 4);
	EXPECT_EQ(policy.get_initial_delay(), std::chrono::milliseconds(100));
	EXPECT_TRUE(policy.is_retry_enabled());
}

TEST(RetryPolicyTest, ExponentialBackoffPolicyConfiguration)
{
	auto policy = retry_policy::exponential_backoff(
		6,
		std::chrono::milliseconds(50),
		3.0,
		std::chrono::milliseconds(5000),
		true
	);

	EXPECT_EQ(policy.get_strategy(), retry_strategy::exponential_backoff);
	EXPECT_EQ(policy.get_max_attempts(), 6);
	EXPECT_EQ(policy.get_initial_delay(), std::chrono::milliseconds(50));
	EXPECT_DOUBLE_EQ(policy.get_multiplier(), 3.0);
	EXPECT_EQ(policy.get_max_delay(), std::chrono::milliseconds(5000));
	EXPECT_TRUE(policy.uses_jitter());
	EXPECT_TRUE(policy.is_retry_enabled());
}

TEST(RetryPolicyTest, AttemptTracking)
{
	auto policy = retry_policy::fixed(3, std::chrono::milliseconds(100));

	EXPECT_EQ(policy.get_current_attempt(), 0);
	EXPECT_TRUE(policy.has_attempts_remaining());

	policy.record_attempt();
	EXPECT_EQ(policy.get_current_attempt(), 1);
	EXPECT_TRUE(policy.has_attempts_remaining());

	policy.record_attempt();
	EXPECT_EQ(policy.get_current_attempt(), 2);
	EXPECT_FALSE(policy.has_attempts_remaining());
}

TEST(RetryPolicyTest, ResetClearsAttemptCounter)
{
	auto policy = retry_policy::fixed(3, std::chrono::milliseconds(100));

	policy.record_attempt();
	policy.record_attempt();
	EXPECT_EQ(policy.get_current_attempt(), 2);

	policy.reset();
	EXPECT_EQ(policy.get_current_attempt(), 0);
	EXPECT_TRUE(policy.has_attempts_remaining());
}

TEST(RetryPolicyTest, FixedDelayCalculation)
{
	auto policy = retry_policy::fixed(3, std::chrono::milliseconds(100));

	// Initial attempt (0) has no delay
	EXPECT_EQ(policy.get_delay_for_current_attempt(), std::chrono::milliseconds(0));

	policy.record_attempt();
	EXPECT_EQ(policy.get_delay_for_current_attempt(), std::chrono::milliseconds(100));

	policy.record_attempt();
	EXPECT_EQ(policy.get_delay_for_current_attempt(), std::chrono::milliseconds(100));
}

TEST(RetryPolicyTest, LinearDelayCalculation)
{
	auto policy = retry_policy::linear(5, std::chrono::milliseconds(100));

	// Initial attempt (0) has no delay
	EXPECT_EQ(policy.get_delay_for_current_attempt(), std::chrono::milliseconds(0));

	policy.record_attempt();  // attempt 1
	EXPECT_EQ(policy.get_delay_for_current_attempt(), std::chrono::milliseconds(100));

	policy.record_attempt();  // attempt 2
	EXPECT_EQ(policy.get_delay_for_current_attempt(), std::chrono::milliseconds(200));

	policy.record_attempt();  // attempt 3
	EXPECT_EQ(policy.get_delay_for_current_attempt(), std::chrono::milliseconds(300));
}

TEST(RetryPolicyTest, ExponentialDelayCalculation)
{
	auto policy = retry_policy::exponential_backoff(
		5,
		std::chrono::milliseconds(100),
		2.0,
		std::chrono::milliseconds(10000),
		false
	);

	// Initial attempt (0) has no delay
	EXPECT_EQ(policy.get_delay_for_current_attempt(), std::chrono::milliseconds(0));

	policy.record_attempt();  // attempt 1: 100 * 2^0 = 100
	EXPECT_EQ(policy.get_delay_for_current_attempt(), std::chrono::milliseconds(100));

	policy.record_attempt();  // attempt 2: 100 * 2^1 = 200
	EXPECT_EQ(policy.get_delay_for_current_attempt(), std::chrono::milliseconds(200));

	policy.record_attempt();  // attempt 3: 100 * 2^2 = 400
	EXPECT_EQ(policy.get_delay_for_current_attempt(), std::chrono::milliseconds(400));

	policy.record_attempt();  // attempt 4: 100 * 2^3 = 800
	EXPECT_EQ(policy.get_delay_for_current_attempt(), std::chrono::milliseconds(800));
}

TEST(RetryPolicyTest, MaxDelayIsCapped)
{
	auto policy = retry_policy::exponential_backoff(
		10,
		std::chrono::milliseconds(100),
		2.0,
		std::chrono::milliseconds(500),  // Cap at 500ms
		false
	);

	// Record enough attempts to exceed cap
	for (int i = 0; i < 5; ++i)
	{
		policy.record_attempt();
	}

	// Delay should be capped at 500ms
	EXPECT_LE(policy.get_delay_for_current_attempt(), std::chrono::milliseconds(500));
}

TEST(RetryPolicyTest, ToStringOutput)
{
	auto none_policy = retry_policy::no_retry();
	EXPECT_EQ(none_policy.to_string(), "retry_policy(none)");

	auto fixed_policy = retry_policy::fixed(3, std::chrono::milliseconds(100));
	EXPECT_NE(fixed_policy.to_string().find("fixed"), std::string::npos);
	EXPECT_NE(fixed_policy.to_string().find("attempts=3"), std::string::npos);

	auto exp_policy = retry_policy::exponential_backoff(5);
	EXPECT_NE(exp_policy.to_string().find("exponential"), std::string::npos);
}
