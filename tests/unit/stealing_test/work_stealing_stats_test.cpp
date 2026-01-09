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

#include <kcenon/thread/stealing/work_stealing_stats.h>

#include <thread>
#include <vector>

using namespace kcenon::thread;

class work_stealing_stats_test : public ::testing::Test
{
protected:
	void SetUp() override
	{
		stats_.reset();
	}

	work_stealing_stats stats_;
};

// ============================================
// Initialization Tests
// ============================================

TEST_F(work_stealing_stats_test, default_values_are_zero)
{
	work_stealing_stats new_stats;
	EXPECT_EQ(new_stats.steal_attempts.load(), 0);
	EXPECT_EQ(new_stats.successful_steals.load(), 0);
	EXPECT_EQ(new_stats.failed_steals.load(), 0);
	EXPECT_EQ(new_stats.jobs_stolen.load(), 0);
	EXPECT_EQ(new_stats.same_node_steals.load(), 0);
	EXPECT_EQ(new_stats.cross_node_steals.load(), 0);
	EXPECT_EQ(new_stats.batch_steals.load(), 0);
	EXPECT_EQ(new_stats.total_batch_size.load(), 0);
	EXPECT_EQ(new_stats.total_steal_time_ns.load(), 0);
	EXPECT_EQ(new_stats.total_backoff_time_ns.load(), 0);
}

// ============================================
// Computed Metrics Tests
// ============================================

TEST_F(work_stealing_stats_test, steal_success_rate_zero_attempts)
{
	EXPECT_DOUBLE_EQ(stats_.steal_success_rate(), 0.0);
}

TEST_F(work_stealing_stats_test, steal_success_rate_all_successful)
{
	stats_.steal_attempts.store(100);
	stats_.successful_steals.store(100);
	EXPECT_DOUBLE_EQ(stats_.steal_success_rate(), 1.0);
}

TEST_F(work_stealing_stats_test, steal_success_rate_partial)
{
	stats_.steal_attempts.store(100);
	stats_.successful_steals.store(75);
	EXPECT_DOUBLE_EQ(stats_.steal_success_rate(), 0.75);
}

TEST_F(work_stealing_stats_test, avg_batch_size_zero_batches)
{
	EXPECT_DOUBLE_EQ(stats_.avg_batch_size(), 0.0);
}

TEST_F(work_stealing_stats_test, avg_batch_size_calculation)
{
	stats_.batch_steals.store(10);
	stats_.total_batch_size.store(40);
	EXPECT_DOUBLE_EQ(stats_.avg_batch_size(), 4.0);
}

TEST_F(work_stealing_stats_test, cross_node_ratio_zero_steals)
{
	EXPECT_DOUBLE_EQ(stats_.cross_node_ratio(), 0.0);
}

TEST_F(work_stealing_stats_test, cross_node_ratio_all_same_node)
{
	stats_.same_node_steals.store(100);
	stats_.cross_node_steals.store(0);
	EXPECT_DOUBLE_EQ(stats_.cross_node_ratio(), 0.0);
}

TEST_F(work_stealing_stats_test, cross_node_ratio_all_cross_node)
{
	stats_.same_node_steals.store(0);
	stats_.cross_node_steals.store(100);
	EXPECT_DOUBLE_EQ(stats_.cross_node_ratio(), 1.0);
}

TEST_F(work_stealing_stats_test, cross_node_ratio_mixed)
{
	stats_.same_node_steals.store(75);
	stats_.cross_node_steals.store(25);
	EXPECT_DOUBLE_EQ(stats_.cross_node_ratio(), 0.25);
}

TEST_F(work_stealing_stats_test, avg_steal_time_zero_attempts)
{
	EXPECT_DOUBLE_EQ(stats_.avg_steal_time_ns(), 0.0);
}

TEST_F(work_stealing_stats_test, avg_steal_time_calculation)
{
	stats_.steal_attempts.store(10);
	stats_.total_steal_time_ns.store(10000);
	EXPECT_DOUBLE_EQ(stats_.avg_steal_time_ns(), 1000.0);
}

// ============================================
// Reset Tests
// ============================================

TEST_F(work_stealing_stats_test, reset_clears_all_counters)
{
	stats_.steal_attempts.store(100);
	stats_.successful_steals.store(75);
	stats_.failed_steals.store(25);
	stats_.jobs_stolen.store(200);
	stats_.same_node_steals.store(50);
	stats_.cross_node_steals.store(25);
	stats_.batch_steals.store(10);
	stats_.total_batch_size.store(40);
	stats_.total_steal_time_ns.store(10000);
	stats_.total_backoff_time_ns.store(5000);

	stats_.reset();

	EXPECT_EQ(stats_.steal_attempts.load(), 0);
	EXPECT_EQ(stats_.successful_steals.load(), 0);
	EXPECT_EQ(stats_.failed_steals.load(), 0);
	EXPECT_EQ(stats_.jobs_stolen.load(), 0);
	EXPECT_EQ(stats_.same_node_steals.load(), 0);
	EXPECT_EQ(stats_.cross_node_steals.load(), 0);
	EXPECT_EQ(stats_.batch_steals.load(), 0);
	EXPECT_EQ(stats_.total_batch_size.load(), 0);
	EXPECT_EQ(stats_.total_steal_time_ns.load(), 0);
	EXPECT_EQ(stats_.total_backoff_time_ns.load(), 0);
}

// ============================================
// Snapshot Tests
// ============================================

TEST_F(work_stealing_stats_test, snapshot_captures_current_values)
{
	stats_.steal_attempts.store(100);
	stats_.successful_steals.store(75);
	stats_.failed_steals.store(25);
	stats_.jobs_stolen.store(200);

	auto snap = stats_.snapshot();

	EXPECT_EQ(snap.steal_attempts, 100);
	EXPECT_EQ(snap.successful_steals, 75);
	EXPECT_EQ(snap.failed_steals, 25);
	EXPECT_EQ(snap.jobs_stolen, 200);
}

TEST_F(work_stealing_stats_test, snapshot_computed_metrics)
{
	stats_.steal_attempts.store(100);
	stats_.successful_steals.store(75);
	stats_.batch_steals.store(10);
	stats_.total_batch_size.store(40);
	stats_.same_node_steals.store(60);
	stats_.cross_node_steals.store(15);

	auto snap = stats_.snapshot();

	EXPECT_DOUBLE_EQ(snap.steal_success_rate(), 0.75);
	EXPECT_DOUBLE_EQ(snap.avg_batch_size(), 4.0);
	EXPECT_DOUBLE_EQ(snap.cross_node_ratio(), 0.2);
}

// ============================================
// Thread Safety Tests
// ============================================

TEST_F(work_stealing_stats_test, concurrent_increments)
{
	constexpr int num_threads = 4;
	constexpr int increments_per_thread = 10000;

	std::vector<std::thread> threads;
	threads.reserve(num_threads);

	for (int i = 0; i < num_threads; ++i)
	{
		threads.emplace_back([this]() {
			for (int j = 0; j < increments_per_thread; ++j)
			{
				stats_.steal_attempts.fetch_add(1, std::memory_order_relaxed);
				stats_.successful_steals.fetch_add(1, std::memory_order_relaxed);
			}
		});
	}

	for (auto& t : threads)
	{
		t.join();
	}

	EXPECT_EQ(stats_.steal_attempts.load(), num_threads * increments_per_thread);
	EXPECT_EQ(stats_.successful_steals.load(), num_threads * increments_per_thread);
}
