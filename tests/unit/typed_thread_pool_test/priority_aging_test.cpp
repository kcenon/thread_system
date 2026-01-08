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

#include <kcenon/thread/impl/typed_pool/priority_aging_config.h>
#include <kcenon/thread/impl/typed_pool/aging_typed_job.h>
#include <kcenon/thread/impl/typed_pool/aging_typed_job_queue.h>
#include <kcenon/thread/impl/typed_pool/typed_thread_pool.h>
#include <kcenon/thread/impl/typed_pool/typed_thread_worker.h>

#include <atomic>
#include <chrono>
#include <thread>

using namespace kcenon::thread;
using namespace kcenon::common;

// ============================================================================
// aged_priority tests
// ============================================================================

TEST(aged_priority_test, effective_priority_no_boost)
{
	aged_priority<job_types> ap{job_types::Background, 0, std::chrono::steady_clock::now()};

	EXPECT_EQ(ap.effective_priority(), job_types::Background);
	EXPECT_EQ(ap.boost, 0);
}

TEST(aged_priority_test, effective_priority_with_boost)
{
	aged_priority<job_types> ap{job_types::Background, 0, std::chrono::steady_clock::now()};

	ap.apply_boost(1, 3);
	EXPECT_EQ(ap.boost, 1);
	// Background (2) - 1 = 1 (Batch)
	EXPECT_EQ(ap.effective_priority(), job_types::Batch);

	ap.apply_boost(1, 3);
	EXPECT_EQ(ap.boost, 2);
	// Background (2) - 2 = 0 (RealTime)
	EXPECT_EQ(ap.effective_priority(), job_types::RealTime);
}

TEST(aged_priority_test, max_boost_cap)
{
	aged_priority<job_types> ap{job_types::Background, 0, std::chrono::steady_clock::now()};

	ap.apply_boost(5, 3);  // Request 5, but max is 3
	EXPECT_EQ(ap.boost, 3);
	EXPECT_TRUE(ap.is_max_boosted(3));
}

TEST(aged_priority_test, reset_boost)
{
	aged_priority<job_types> ap{job_types::Background, 0, std::chrono::steady_clock::now()};

	ap.apply_boost(2, 3);
	EXPECT_EQ(ap.boost, 2);

	ap.reset_boost();
	EXPECT_EQ(ap.boost, 0);
	EXPECT_EQ(ap.effective_priority(), job_types::Background);
}

TEST(aged_priority_test, wait_time)
{
	auto enqueue_time = std::chrono::steady_clock::now();
	aged_priority<job_types> ap{job_types::Background, 0, enqueue_time};

	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto wait = ap.wait_time();
	EXPECT_GE(wait.count(), 50);
}

// ============================================================================
// aging_typed_job tests
// ============================================================================

TEST(aging_typed_job_test, construction)
{
	auto job = std::make_unique<aging_typed_job_t<job_types>>(
		job_types::Background,
		[]() -> VoidResult { return ok(); },
		"test_job"
	);

	EXPECT_EQ(job->priority(), job_types::Background);
	EXPECT_EQ(job->effective_priority(), job_types::Background);
	EXPECT_EQ(job->get_max_boost(), 3);
}

TEST(aging_typed_job_test, apply_boost)
{
	auto job = std::make_unique<aging_typed_job_t<job_types>>(
		job_types::Background,
		[]() -> VoidResult { return ok(); }
	);

	job->apply_boost(1);
	EXPECT_EQ(job->effective_priority(), job_types::Batch);

	job->apply_boost(1);
	EXPECT_EQ(job->effective_priority(), job_types::RealTime);
}

TEST(aging_typed_job_test, set_max_boost)
{
	auto job = std::make_unique<aging_typed_job_t<job_types>>(
		job_types::Background,
		[]() -> VoidResult { return ok(); }
	);

	job->set_max_boost(1);
	EXPECT_EQ(job->get_max_boost(), 1);

	job->apply_boost(5);
	EXPECT_EQ(job->get_aged_priority().boost, 1);
}

TEST(aging_typed_job_test, execute)
{
	std::atomic<bool> executed{false};

	auto job = std::make_unique<aging_typed_job_t<job_types>>(
		job_types::Background,
		[&executed]() -> VoidResult {
			executed = true;
			return ok();
		}
	);

	auto result = job->do_work();
	EXPECT_FALSE(result.is_err());
	EXPECT_TRUE(executed.load());
}

TEST(aging_typed_job_test, to_job_info)
{
	auto job = std::make_unique<aging_typed_job_t<job_types>>(
		job_types::Background,
		[]() -> VoidResult { return ok(); },
		"info_test_job"
	);

	job->apply_boost(2);

	auto info = job->to_job_info();
	EXPECT_EQ(info.job_name, "info_test_job");
	EXPECT_EQ(info.priority_boost, 2);
}

// ============================================================================
// aging_typed_job_queue tests
// ============================================================================

TEST(aging_typed_job_queue_test, construction)
{
	priority_aging_config config{
		.enabled = true,
		.aging_interval = std::chrono::milliseconds{100},
		.priority_boost_per_interval = 1,
		.max_priority_boost = 3
	};

	auto queue = std::make_shared<aging_typed_job_queue_t<job_types>>(config);
	EXPECT_FALSE(queue->is_aging_running());
}

TEST(aging_typed_job_queue_test, start_stop_aging)
{
	priority_aging_config config{
		.enabled = true,
		.aging_interval = std::chrono::milliseconds{100}
	};

	auto queue = std::make_shared<aging_typed_job_queue_t<job_types>>(config);

	queue->start_aging();
	EXPECT_TRUE(queue->is_aging_running());

	queue->stop_aging();
	EXPECT_FALSE(queue->is_aging_running());
}

TEST(aging_typed_job_queue_test, enqueue_job)
{
	priority_aging_config config{
		.enabled = true,
		.aging_interval = std::chrono::milliseconds{100}
	};

	auto queue = std::make_shared<aging_typed_job_queue_t<job_types>>(config);

	auto job = std::make_unique<aging_typed_job_t<job_types>>(
		job_types::Background,
		[]() -> VoidResult { return ok(); }
	);

	auto result = queue->enqueue(std::move(job));
	EXPECT_FALSE(result.is_err());
}

TEST(aging_typed_job_queue_test, aging_stats)
{
	priority_aging_config config{
		.enabled = true,
		.aging_interval = std::chrono::milliseconds{50},
		.priority_boost_per_interval = 1,
		.max_priority_boost = 3
	};

	auto queue = std::make_shared<aging_typed_job_queue_t<job_types>>(config);

	auto stats = queue->get_aging_stats();
	EXPECT_EQ(stats.total_boosts_applied, 0);
	EXPECT_EQ(stats.starvation_alerts, 0);
}

TEST(aging_typed_job_queue_test, starvation_detection)
{
	std::atomic<int> starvation_count{0};

	priority_aging_config config{
		.enabled = true,
		.aging_interval = std::chrono::milliseconds{10},
		.starvation_threshold = std::chrono::seconds{0},  // Immediate detection
		.starvation_callback = [&starvation_count](const job_info&) {
			++starvation_count;
		}
	};

	auto queue = std::make_shared<aging_typed_job_queue_t<job_types>>(config);

	auto job = std::make_unique<aging_typed_job_t<job_types>>(
		job_types::Background,
		[]() -> VoidResult { return ok(); }
	);

	auto result = queue->enqueue(std::move(job));
	EXPECT_FALSE(result.is_err());

	queue->start_aging();
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	queue->stop_aging();

	EXPECT_GT(starvation_count.load(), 0);
}

// ============================================================================
// typed_thread_pool priority aging integration tests
// ============================================================================

TEST(typed_thread_pool_aging_test, enable_disable_aging)
{
	auto pool = std::make_shared<typed_thread_pool>();

	auto worker = std::make_unique<typed_thread_worker>();
	auto enqueue_result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(enqueue_result.is_err());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.is_err());

	priority_aging_config config{
		.enabled = true,
		.aging_interval = std::chrono::milliseconds{100},
		.max_priority_boost = 3
	};

	pool->enable_priority_aging(config);
	EXPECT_TRUE(pool->is_priority_aging_enabled());

	pool->disable_priority_aging();
	EXPECT_FALSE(pool->is_priority_aging_enabled());

	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.is_err());
}

TEST(typed_thread_pool_aging_test, get_aging_stats)
{
	auto pool = std::make_shared<typed_thread_pool>();

	auto worker = std::make_unique<typed_thread_worker>();
	auto enqueue_result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(enqueue_result.is_err());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.is_err());

	pool->enable_priority_aging({
		.enabled = true,
		.aging_interval = std::chrono::milliseconds{100}
	});

	auto stats = pool->get_aging_stats();
	EXPECT_EQ(stats.total_boosts_applied, 0);

	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.is_err());
}

// TODO: Fix this test - there's an issue with job queue replacement after pool start
TEST(typed_thread_pool_aging_test, DISABLED_enqueue_aging_job)
{
	auto pool = std::make_shared<typed_thread_pool>();

	auto worker = std::make_unique<typed_thread_worker>();
	auto enqueue_result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(enqueue_result.is_err());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.is_err());

	pool->enable_priority_aging({
		.enabled = true,
		.aging_interval = std::chrono::milliseconds{100}
	});

	std::atomic<bool> executed{false};
	auto job = std::make_unique<aging_typed_job_t<job_types>>(
		job_types::Background,
		[&executed]() -> VoidResult {
			executed = true;
			return ok();
		}
	);

	auto job_result = pool->enqueue(std::move(job));
	EXPECT_FALSE(job_result.is_err());

	// Wait for job to execute
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	EXPECT_TRUE(executed.load());

	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.is_err());
}

// ============================================================================
// Aging curve tests
// ============================================================================

TEST(aging_curve_test, linear_curve)
{
	priority_aging_config config{
		.enabled = true,
		.aging_interval = std::chrono::milliseconds{100},
		.priority_boost_per_interval = 1,
		.max_priority_boost = 5,
		.curve = aging_curve::linear
	};

	auto queue = std::make_shared<aging_typed_job_queue_t<job_types>>(config);
	EXPECT_EQ(queue->get_aging_config().curve, aging_curve::linear);
}

TEST(aging_curve_test, exponential_curve)
{
	priority_aging_config config{
		.enabled = true,
		.aging_interval = std::chrono::milliseconds{100},
		.curve = aging_curve::exponential,
		.exponential_factor = 2.0
	};

	auto queue = std::make_shared<aging_typed_job_queue_t<job_types>>(config);
	EXPECT_EQ(queue->get_aging_config().curve, aging_curve::exponential);
	EXPECT_EQ(queue->get_aging_config().exponential_factor, 2.0);
}

TEST(aging_curve_test, logarithmic_curve)
{
	priority_aging_config config{
		.enabled = true,
		.aging_interval = std::chrono::milliseconds{100},
		.curve = aging_curve::logarithmic
	};

	auto queue = std::make_shared<aging_typed_job_queue_t<job_types>>(config);
	EXPECT_EQ(queue->get_aging_config().curve, aging_curve::logarithmic);
}
