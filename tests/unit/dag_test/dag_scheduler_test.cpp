/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
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

#include <kcenon/thread/dag/dag_scheduler.h>
#include <kcenon/thread/dag/dag_job_builder.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/interfaces/thread_context.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace kcenon::thread;
using namespace kcenon::common;

class dag_scheduler_test : public ::testing::Test
{
protected:
	void SetUp() override
	{
		pool_ = std::make_shared<thread_pool>("dag_test_pool");

		// Add workers to the pool (required before start)
		auto worker_count = std::thread::hardware_concurrency();
		if (worker_count == 0)
		{
			worker_count = 4;
		}

		thread_context context;
		for (unsigned int i = 0; i < worker_count; ++i)
		{
			auto worker = std::make_unique<thread_worker>(false, context);
			pool_->enqueue(std::move(worker));
		}

		auto start_result = pool_->start();
		ASSERT_TRUE(start_result.is_ok()) << "Failed to start thread pool: " << start_result.error().message;
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

// ============================================
// dag_job Tests
// ============================================

TEST_F(dag_scheduler_test, dag_job_creation)
{
	auto job = std::make_unique<dag_job>("test_job");

	EXPECT_NE(job->get_dag_id(), INVALID_JOB_ID);
	EXPECT_EQ(job->get_name(), "test_job");
	EXPECT_EQ(job->get_state(), dag_job_state::pending);
	EXPECT_TRUE(job->get_dependencies().empty());
}

TEST_F(dag_scheduler_test, dag_job_state_transitions)
{
	auto job = std::make_unique<dag_job>("test_job");

	EXPECT_EQ(job->get_state(), dag_job_state::pending);

	job->set_state(dag_job_state::ready);
	EXPECT_EQ(job->get_state(), dag_job_state::ready);

	EXPECT_TRUE(job->try_transition_state(dag_job_state::ready, dag_job_state::running));
	EXPECT_EQ(job->get_state(), dag_job_state::running);

	EXPECT_FALSE(job->try_transition_state(dag_job_state::ready, dag_job_state::completed));
	EXPECT_EQ(job->get_state(), dag_job_state::running);
}

TEST_F(dag_scheduler_test, dag_job_dependencies)
{
	auto job = std::make_unique<dag_job>("test_job");

	job->add_dependency(1);
	job->add_dependency(2);
	job->add_dependency(3);

	const auto& deps = job->get_dependencies();
	EXPECT_EQ(deps.size(), 3);
	EXPECT_EQ(deps[0], 1);
	EXPECT_EQ(deps[1], 2);
	EXPECT_EQ(deps[2], 3);
}

TEST_F(dag_scheduler_test, dag_job_result)
{
	auto job = std::make_unique<dag_job>("test_job");

	job->set_result(42);
	EXPECT_TRUE(job->has_result());
	EXPECT_EQ(job->get_result<int>(), 42);
}

// ============================================
// dag_job_builder Tests
// ============================================

TEST_F(dag_scheduler_test, dag_job_builder_basic)
{
	auto job = dag_job_builder("builder_test")
		.work([]() -> kcenon::common::VoidResult {
			return kcenon::common::ok();
		})
		.build();

	EXPECT_NE(job, nullptr);
	EXPECT_EQ(job->get_name(), "builder_test");
}

TEST_F(dag_scheduler_test, dag_job_builder_with_dependencies)
{
	auto job = dag_job_builder("dependent_job")
		.depends_on(1)
		.depends_on({2, 3})
		.work([]() -> kcenon::common::VoidResult {
			return kcenon::common::ok();
		})
		.build();

	const auto& deps = job->get_dependencies();
	EXPECT_EQ(deps.size(), 3);
}

TEST_F(dag_scheduler_test, dag_job_builder_validation_no_work)
{
	dag_job_builder builder("no_work_job");

	// Without work() set, is_valid() should return false
	EXPECT_FALSE(builder.is_valid());

	// get_validation_error() should return an error message
	auto error = builder.get_validation_error();
	EXPECT_TRUE(error.has_value());
	EXPECT_FALSE(error->empty());

	// build() should return nullptr for invalid configuration
	auto job = builder.build();
	EXPECT_EQ(job, nullptr);
}

TEST_F(dag_scheduler_test, dag_job_builder_validation_with_work)
{
	dag_job_builder builder("valid_job");
	builder.work([]() -> kcenon::common::VoidResult {
		return kcenon::common::ok();
	});

	// With work() set, is_valid() should return true
	EXPECT_TRUE(builder.is_valid());

	// get_validation_error() should return nullopt
	EXPECT_FALSE(builder.get_validation_error().has_value());
}

TEST_F(dag_scheduler_test, dag_job_builder_reusability)
{
	dag_job_builder builder("reusable_job");

	// Build first job
	auto job1 = builder
		.work([]() -> kcenon::common::VoidResult {
			return kcenon::common::ok();
		})
		.depends_on(1)
		.build();

	EXPECT_NE(job1, nullptr);
	EXPECT_EQ(job1->get_dependencies().size(), 1);

	// After build(), builder should be reset and reusable
	// Dependencies should be cleared
	auto job2 = builder
		.work([]() -> kcenon::common::VoidResult {
			return kcenon::common::ok();
		})
		.depends_on({2, 3, 4})
		.build();

	EXPECT_NE(job2, nullptr);
	EXPECT_EQ(job2->get_dependencies().size(), 3);

	// Job IDs should be different
	EXPECT_NE(job1->get_dag_id(), job2->get_dag_id());
}

TEST_F(dag_scheduler_test, dag_job_builder_reset)
{
	dag_job_builder builder("reset_test");

	builder
		.work([]() -> kcenon::common::VoidResult {
			return kcenon::common::ok();
		})
		.depends_on(1)
		.on_failure([]() -> kcenon::common::VoidResult {
			return kcenon::common::ok();
		});

	// Manually reset
	builder.reset();

	// After reset, builder should be invalid (no work function)
	EXPECT_FALSE(builder.is_valid());

	// build() should return nullptr
	auto job = builder.build();
	EXPECT_EQ(job, nullptr);
}

TEST_F(dag_scheduler_test, dag_job_builder_returns_method)
{
	auto job = dag_job_builder("returns_test")
		.returns<int>()
		.work([]() -> kcenon::common::VoidResult {
			return kcenon::common::ok();
		})
		.build();

	EXPECT_NE(job, nullptr);
	EXPECT_EQ(job->get_name(), "returns_test");
}

TEST_F(dag_scheduler_test, dag_job_builder_work_with_result)
{
	dag_scheduler scheduler(pool_);

	auto job_id = scheduler.add_job(
		dag_job_builder("compute_job")
			.work_with_result<int>([]() -> kcenon::common::Result<int> {
				return kcenon::common::Result<int>::ok(42);
			})
			.build()
	);

	auto future = scheduler.execute_all();
	auto result = future.get();

	EXPECT_TRUE(result.is_ok());

	auto info = scheduler.get_job_info(job_id);
	EXPECT_EQ(info->state, dag_job_state::completed);
	EXPECT_TRUE(info->result.has_value());
}

// ============================================
// dag_scheduler Core Tests
// ============================================

TEST_F(dag_scheduler_test, scheduler_add_job)
{
	dag_scheduler scheduler(pool_);

	auto job_id = scheduler.add_job(
		dag_job_builder("test_job")
			.work([]() -> kcenon::common::VoidResult { return kcenon::common::ok(); })
			.build()
	);

	EXPECT_NE(job_id, INVALID_JOB_ID);

	auto info = scheduler.get_job_info(job_id);
	EXPECT_TRUE(info.has_value());
	EXPECT_EQ(info->name, "test_job");
}

TEST_F(dag_scheduler_test, scheduler_simple_execution)
{
	dag_scheduler scheduler(pool_);

	std::atomic<int> counter{0};

	auto job_a = scheduler.add_job(
		dag_job_builder("job_a")
			.work([&counter]() -> kcenon::common::VoidResult {
				counter.fetch_add(1);
				return kcenon::common::ok();
			})
			.build()
	);

	auto future = scheduler.execute_all();
	auto result = future.get();

	EXPECT_TRUE(result.is_ok());
	EXPECT_EQ(counter.load(), 1);

	auto info = scheduler.get_job_info(job_a);
	EXPECT_EQ(info->state, dag_job_state::completed);
}

TEST_F(dag_scheduler_test, scheduler_dependency_chain)
{
	dag_scheduler scheduler(pool_);

	std::vector<int> execution_order;
	std::mutex order_mutex;

	auto job_a = scheduler.add_job(
		dag_job_builder("job_a")
			.work([&]() -> kcenon::common::VoidResult {
				std::lock_guard lock(order_mutex);
				execution_order.push_back(1);
				return kcenon::common::ok();
			})
			.build()
	);

	auto job_b = scheduler.add_job(
		dag_job_builder("job_b")
			.depends_on(job_a)
			.work([&]() -> kcenon::common::VoidResult {
				std::lock_guard lock(order_mutex);
				execution_order.push_back(2);
				return kcenon::common::ok();
			})
			.build()
	);

	auto job_c = scheduler.add_job(
		dag_job_builder("job_c")
			.depends_on(job_b)
			.work([&]() -> kcenon::common::VoidResult {
				std::lock_guard lock(order_mutex);
				execution_order.push_back(3);
				return kcenon::common::ok();
			})
			.build()
	);

	auto future = scheduler.execute_all();
	auto result = future.get();

	EXPECT_TRUE(result.is_ok());
	EXPECT_EQ(execution_order.size(), 3);
	EXPECT_EQ(execution_order[0], 1);
	EXPECT_EQ(execution_order[1], 2);
	EXPECT_EQ(execution_order[2], 3);
}

TEST_F(dag_scheduler_test, scheduler_parallel_execution)
{
	dag_scheduler scheduler(pool_);

	std::atomic<int> concurrent_count{0};
	std::atomic<int> max_concurrent{0};

	auto create_parallel_job = [&](const std::string& name) {
		return dag_job_builder(name)
			.work([&]() -> kcenon::common::VoidResult {
				int current = concurrent_count.fetch_add(1) + 1;
				int expected = max_concurrent.load();
				while (current > expected && !max_concurrent.compare_exchange_weak(expected, current))
				{
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				concurrent_count.fetch_sub(1);
				return kcenon::common::ok();
			})
			.build();
	};

	(void)scheduler.add_job(create_parallel_job("parallel_1"));
	(void)scheduler.add_job(create_parallel_job("parallel_2"));
	(void)scheduler.add_job(create_parallel_job("parallel_3"));

	auto future = scheduler.execute_all();
	auto result = future.get();

	EXPECT_TRUE(result.is_ok());
	EXPECT_GT(max_concurrent.load(), 1);
}

// ============================================
// Cycle Detection Tests
// ============================================

TEST_F(dag_scheduler_test, scheduler_cycle_detection)
{
	dag_scheduler scheduler(pool_);

	auto job_a = scheduler.add_job(
		dag_job_builder("job_a")
			.work([]() -> kcenon::common::VoidResult { return kcenon::common::ok(); })
			.build()
	);

	auto job_b = scheduler.add_job(
		dag_job_builder("job_b")
			.depends_on(job_a)
			.work([]() -> kcenon::common::VoidResult { return kcenon::common::ok(); })
			.build()
	);

	// Try to add dependency that would create a cycle
	auto result = scheduler.add_dependency(job_a, job_b);
	EXPECT_TRUE(result.is_err());

	EXPECT_FALSE(scheduler.has_cycles());
}

// ============================================
// Failure Handling Tests
// ============================================

TEST_F(dag_scheduler_test, scheduler_fail_fast_policy)
{
	dag_config config;
	config.failure_policy = dag_failure_policy::fail_fast;

	dag_scheduler scheduler(pool_, config);

	auto job_a = scheduler.add_job(
		dag_job_builder("failing_job")
			.work([]() -> kcenon::common::VoidResult {
				return make_error_result(kcenon::thread::error_code::job_execution_failed, "Intentional failure");
			})
			.build()
	);

	auto job_b = scheduler.add_job(
		dag_job_builder("dependent_job")
			.depends_on(job_a)
			.work([]() -> kcenon::common::VoidResult {
				return kcenon::common::ok();
			})
			.build()
	);

	auto future = scheduler.execute_all();
	auto result = future.get();

	EXPECT_TRUE(result.is_err());

	auto info_b = scheduler.get_job_info(job_b);
	EXPECT_EQ(info_b->state, dag_job_state::cancelled);
}

TEST_F(dag_scheduler_test, scheduler_continue_others_policy)
{
	dag_config config;
	config.failure_policy = dag_failure_policy::continue_others;

	dag_scheduler scheduler(pool_, config);

	std::atomic<bool> unrelated_executed{false};

	auto job_a = scheduler.add_job(
		dag_job_builder("failing_job")
			.work([]() -> kcenon::common::VoidResult {
				return make_error_result(kcenon::thread::error_code::job_execution_failed, "Intentional failure");
			})
			.build()
	);

	auto job_b = scheduler.add_job(
		dag_job_builder("dependent_job")
			.depends_on(job_a)
			.work([]() -> kcenon::common::VoidResult {
				return kcenon::common::ok();
			})
			.build()
	);

	auto job_c = scheduler.add_job(
		dag_job_builder("unrelated_job")
			.work([&]() -> kcenon::common::VoidResult {
				unrelated_executed.store(true);
				return kcenon::common::ok();
			})
			.build()
	);

	auto future = scheduler.execute_all();
	future.get();

	EXPECT_TRUE(unrelated_executed.load());

	auto info_b = scheduler.get_job_info(job_b);
	EXPECT_EQ(info_b->state, dag_job_state::skipped);
}

// ============================================
// Visualization Tests
// ============================================

TEST_F(dag_scheduler_test, scheduler_to_dot)
{
	dag_scheduler scheduler(pool_);

	auto job_a = scheduler.add_job(
		dag_job_builder("job_a")
			.work([]() -> kcenon::common::VoidResult { return kcenon::common::ok(); })
			.build()
	);

	(void)scheduler.add_job(
		dag_job_builder("job_b")
			.depends_on(job_a)
			.work([]() -> kcenon::common::VoidResult { return kcenon::common::ok(); })
			.build()
	);

	auto dot = scheduler.to_dot();

	EXPECT_TRUE(dot.find("digraph DAG") != std::string::npos);
	EXPECT_TRUE(dot.find("job_a") != std::string::npos);
	EXPECT_TRUE(dot.find("job_b") != std::string::npos);
	EXPECT_TRUE(dot.find("->") != std::string::npos);
}

TEST_F(dag_scheduler_test, scheduler_to_json)
{
	dag_scheduler scheduler(pool_);

	(void)scheduler.add_job(
		dag_job_builder("test_job")
			.work([]() -> kcenon::common::VoidResult { return kcenon::common::ok(); })
			.build()
	);

	auto json = scheduler.to_json();

	EXPECT_TRUE(json.find("\"jobs\"") != std::string::npos);
	EXPECT_TRUE(json.find("\"test_job\"") != std::string::npos);
	EXPECT_TRUE(json.find("\"stats\"") != std::string::npos);
}

// ============================================
// Statistics Tests
// ============================================

TEST_F(dag_scheduler_test, scheduler_statistics)
{
	dag_scheduler scheduler(pool_);

	(void)scheduler.add_job(
		dag_job_builder("job_1")
			.work([]() -> kcenon::common::VoidResult { return kcenon::common::ok(); })
			.build()
	);

	(void)scheduler.add_job(
		dag_job_builder("job_2")
			.work([]() -> kcenon::common::VoidResult { return kcenon::common::ok(); })
			.build()
	);

	auto stats = scheduler.get_stats();
	EXPECT_EQ(stats.total_jobs, 2);
	EXPECT_EQ(stats.pending_jobs, 2);
	EXPECT_EQ(stats.completed_jobs, 0);

	auto future = scheduler.execute_all();
	future.get();

	stats = scheduler.get_stats();
	EXPECT_EQ(stats.completed_jobs, 2);
	EXPECT_TRUE(stats.all_succeeded());
}

// ============================================
// Execution Order Tests
// ============================================

TEST_F(dag_scheduler_test, scheduler_topological_order)
{
	dag_scheduler scheduler(pool_);

	auto job_a = scheduler.add_job(
		dag_job_builder("job_a")
			.work([]() -> kcenon::common::VoidResult { return kcenon::common::ok(); })
			.build()
	);

	auto job_b = scheduler.add_job(
		dag_job_builder("job_b")
			.depends_on(job_a)
			.work([]() -> kcenon::common::VoidResult { return kcenon::common::ok(); })
			.build()
	);

	auto job_c = scheduler.add_job(
		dag_job_builder("job_c")
			.depends_on(job_a)
			.work([]() -> kcenon::common::VoidResult { return kcenon::common::ok(); })
			.build()
	);

	auto job_d = scheduler.add_job(
		dag_job_builder("job_d")
			.depends_on({job_b, job_c})
			.work([]() -> kcenon::common::VoidResult { return kcenon::common::ok(); })
			.build()
	);

	auto order = scheduler.get_execution_order();

	EXPECT_EQ(order.size(), 4);

	// job_a must come before job_b and job_c
	auto pos_a = std::find(order.begin(), order.end(), job_a);
	auto pos_b = std::find(order.begin(), order.end(), job_b);
	auto pos_c = std::find(order.begin(), order.end(), job_c);
	auto pos_d = std::find(order.begin(), order.end(), job_d);

	EXPECT_LT(pos_a, pos_b);
	EXPECT_LT(pos_a, pos_c);
	EXPECT_LT(pos_b, pos_d);
	EXPECT_LT(pos_c, pos_d);
}
