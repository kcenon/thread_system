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

#include <kcenon/thread/stealing/work_affinity_tracker.h>

#include <thread>
#include <vector>

using namespace kcenon::thread;

// ============================================
// Construction Tests
// ============================================

TEST(work_affinity_tracker_test, default_construction)
{
	work_affinity_tracker tracker;

	EXPECT_EQ(tracker.worker_count(), 0);
	EXPECT_EQ(tracker.history_size(), 16);
	EXPECT_EQ(tracker.total_cooperations(), 0);
}

TEST(work_affinity_tracker_test, parameterized_construction)
{
	work_affinity_tracker tracker(8, 32);

	EXPECT_EQ(tracker.worker_count(), 8);
	EXPECT_EQ(tracker.history_size(), 32);
	EXPECT_EQ(tracker.total_cooperations(), 0);
}

TEST(work_affinity_tracker_test, single_worker)
{
	work_affinity_tracker tracker(1, 16);

	EXPECT_EQ(tracker.worker_count(), 1);
	// Single worker means no cooperation possible
	EXPECT_EQ(tracker.total_cooperations(), 0);
}

TEST(work_affinity_tracker_test, move_construction)
{
	work_affinity_tracker original(4, 16);
	original.record_cooperation(0, 1);
	original.record_cooperation(0, 1);

	work_affinity_tracker moved(std::move(original));

	EXPECT_EQ(moved.worker_count(), 4);
	EXPECT_EQ(moved.history_size(), 16);
	EXPECT_EQ(moved.total_cooperations(), 2);
}

TEST(work_affinity_tracker_test, move_assignment)
{
	work_affinity_tracker original(4, 16);
	original.record_cooperation(0, 1);

	work_affinity_tracker other;
	other = std::move(original);

	EXPECT_EQ(other.worker_count(), 4);
	EXPECT_EQ(other.total_cooperations(), 1);
}

// ============================================
// Record Cooperation Tests
// ============================================

class work_affinity_tracker_cooperation_test : public ::testing::Test
{
protected:
	void SetUp() override
	{
		tracker_ = std::make_unique<work_affinity_tracker>(4, 16);
	}

	std::unique_ptr<work_affinity_tracker> tracker_;
};

TEST_F(work_affinity_tracker_cooperation_test, record_increases_count)
{
	EXPECT_EQ(tracker_->total_cooperations(), 0);

	tracker_->record_cooperation(0, 1);
	EXPECT_EQ(tracker_->total_cooperations(), 1);

	tracker_->record_cooperation(0, 2);
	EXPECT_EQ(tracker_->total_cooperations(), 2);

	tracker_->record_cooperation(1, 2);
	EXPECT_EQ(tracker_->total_cooperations(), 3);
}

TEST_F(work_affinity_tracker_cooperation_test, record_same_pair_accumulates)
{
	tracker_->record_cooperation(0, 1);
	tracker_->record_cooperation(0, 1);
	tracker_->record_cooperation(0, 1);

	EXPECT_EQ(tracker_->total_cooperations(), 3);
	// Affinity should reflect 3 cooperations
	double affinity = tracker_->get_affinity(0, 1);
	EXPECT_GT(affinity, 0.0);
}

TEST_F(work_affinity_tracker_cooperation_test, ignore_self_cooperation)
{
	tracker_->record_cooperation(0, 0);
	tracker_->record_cooperation(1, 1);

	EXPECT_EQ(tracker_->total_cooperations(), 0);
}

TEST_F(work_affinity_tracker_cooperation_test, ignore_invalid_workers)
{
	tracker_->record_cooperation(0, 10);  // Worker 10 doesn't exist
	tracker_->record_cooperation(10, 0);
	tracker_->record_cooperation(99, 100);

	EXPECT_EQ(tracker_->total_cooperations(), 0);
}

// ============================================
// Get Affinity Tests
// ============================================

TEST_F(work_affinity_tracker_cooperation_test, affinity_zero_initially)
{
	EXPECT_DOUBLE_EQ(tracker_->get_affinity(0, 1), 0.0);
	EXPECT_DOUBLE_EQ(tracker_->get_affinity(0, 2), 0.0);
	EXPECT_DOUBLE_EQ(tracker_->get_affinity(1, 2), 0.0);
}

TEST_F(work_affinity_tracker_cooperation_test, affinity_increases_with_cooperation)
{
	tracker_->record_cooperation(0, 1);
	double affinity1 = tracker_->get_affinity(0, 1);

	tracker_->record_cooperation(0, 1);
	double affinity2 = tracker_->get_affinity(0, 1);

	EXPECT_GT(affinity1, 0.0);
	EXPECT_GT(affinity2, affinity1);
}

TEST_F(work_affinity_tracker_cooperation_test, affinity_is_symmetric)
{
	tracker_->record_cooperation(0, 1);
	tracker_->record_cooperation(1, 0);

	// Both directions should contribute to the same affinity
	double affinity_01 = tracker_->get_affinity(0, 1);
	double affinity_10 = tracker_->get_affinity(1, 0);

	EXPECT_DOUBLE_EQ(affinity_01, affinity_10);
}

TEST_F(work_affinity_tracker_cooperation_test, affinity_normalized_by_history)
{
	// With history_size = 16, each cooperation adds 1/16 to affinity
	for (int i = 0; i < 16; ++i)
	{
		tracker_->record_cooperation(0, 1);
	}

	double affinity = tracker_->get_affinity(0, 1);
	EXPECT_DOUBLE_EQ(affinity, 1.0);  // 16/16 = 1.0
}

TEST_F(work_affinity_tracker_cooperation_test, affinity_self_is_zero)
{
	EXPECT_DOUBLE_EQ(tracker_->get_affinity(0, 0), 0.0);
}

TEST_F(work_affinity_tracker_cooperation_test, affinity_invalid_worker_is_zero)
{
	EXPECT_DOUBLE_EQ(tracker_->get_affinity(0, 10), 0.0);
	EXPECT_DOUBLE_EQ(tracker_->get_affinity(10, 0), 0.0);
	EXPECT_DOUBLE_EQ(tracker_->get_affinity(99, 100), 0.0);
}

// ============================================
// Get Preferred Victims Tests
// ============================================

TEST_F(work_affinity_tracker_cooperation_test, preferred_victims_empty_initially)
{
	auto victims = tracker_->get_preferred_victims(0, 3);

	// Should return all other workers (1, 2, 3) even with zero affinity
	EXPECT_EQ(victims.size(), 3);
}

TEST_F(work_affinity_tracker_cooperation_test, preferred_victims_excludes_self)
{
	auto victims = tracker_->get_preferred_victims(0, 10);

	// Should never include worker 0 itself
	for (auto victim : victims)
	{
		EXPECT_NE(victim, 0);
	}
}

TEST_F(work_affinity_tracker_cooperation_test, preferred_victims_sorted_by_affinity)
{
	// Build different affinity levels
	tracker_->record_cooperation(0, 1);  // 1 cooperation with worker 1
	tracker_->record_cooperation(0, 2);  // 2 cooperations with worker 2
	tracker_->record_cooperation(0, 2);
	tracker_->record_cooperation(0, 3);  // 3 cooperations with worker 3
	tracker_->record_cooperation(0, 3);
	tracker_->record_cooperation(0, 3);

	auto victims = tracker_->get_preferred_victims(0, 3);

	// Should be sorted by descending affinity: 3, 2, 1
	ASSERT_EQ(victims.size(), 3);
	EXPECT_EQ(victims[0], 3);
	EXPECT_EQ(victims[1], 2);
	EXPECT_EQ(victims[2], 1);
}

TEST_F(work_affinity_tracker_cooperation_test, preferred_victims_respects_max_count)
{
	auto victims = tracker_->get_preferred_victims(0, 2);

	EXPECT_LE(victims.size(), 2);
}

TEST_F(work_affinity_tracker_cooperation_test, preferred_victims_invalid_worker)
{
	auto victims = tracker_->get_preferred_victims(10, 3);

	EXPECT_TRUE(victims.empty());
}

TEST_F(work_affinity_tracker_cooperation_test, preferred_victims_zero_count)
{
	auto victims = tracker_->get_preferred_victims(0, 0);

	EXPECT_TRUE(victims.empty());
}

// ============================================
// Reset Tests
// ============================================

TEST_F(work_affinity_tracker_cooperation_test, reset_clears_all)
{
	tracker_->record_cooperation(0, 1);
	tracker_->record_cooperation(0, 2);
	tracker_->record_cooperation(1, 2);

	EXPECT_GT(tracker_->total_cooperations(), 0);
	EXPECT_GT(tracker_->get_affinity(0, 1), 0.0);

	tracker_->reset();

	EXPECT_EQ(tracker_->total_cooperations(), 0);
	EXPECT_DOUBLE_EQ(tracker_->get_affinity(0, 1), 0.0);
	EXPECT_DOUBLE_EQ(tracker_->get_affinity(0, 2), 0.0);
	EXPECT_DOUBLE_EQ(tracker_->get_affinity(1, 2), 0.0);
}

// ============================================
// Thread Safety Tests
// ============================================

TEST(work_affinity_tracker_thread_safety_test, concurrent_record_cooperation)
{
	work_affinity_tracker tracker(8, 1000);

	constexpr int num_threads = 4;
	constexpr int ops_per_thread = 1000;

	std::vector<std::thread> threads;

	for (int t = 0; t < num_threads; ++t)
	{
		threads.emplace_back([&tracker, t]() {
			for (int i = 0; i < ops_per_thread; ++i)
			{
				std::size_t victim = static_cast<std::size_t>((t + 1) % 8);
				tracker.record_cooperation(static_cast<std::size_t>(t), victim);
			}
		});
	}

	for (auto& thread : threads)
	{
		thread.join();
	}

	// Should have recorded all cooperations
	EXPECT_EQ(tracker.total_cooperations(),
	          static_cast<std::uint64_t>(num_threads * ops_per_thread));
}

TEST(work_affinity_tracker_thread_safety_test, concurrent_read_write)
{
	work_affinity_tracker tracker(4, 100);

	std::atomic<bool> stop{false};

	// Writer thread
	std::thread writer([&tracker, &stop]() {
		while (!stop)
		{
			tracker.record_cooperation(0, 1);
			tracker.record_cooperation(1, 2);
		}
	});

	// Reader threads
	std::vector<std::thread> readers;
	for (int i = 0; i < 3; ++i)
	{
		readers.emplace_back([&tracker, &stop]() {
			while (!stop)
			{
				[[maybe_unused]] auto affinity = tracker.get_affinity(0, 1);
				[[maybe_unused]] auto victims = tracker.get_preferred_victims(0, 3);
				[[maybe_unused]] auto total = tracker.total_cooperations();
			}
		});
	}

	std::this_thread::sleep_for(std::chrono::milliseconds{100});
	stop = true;

	writer.join();
	for (auto& reader : readers)
	{
		reader.join();
	}

	// Should complete without crashes or deadlocks
	EXPECT_GT(tracker.total_cooperations(), 0);
}

// ============================================
// Edge Cases
// ============================================

TEST(work_affinity_tracker_test, large_worker_count)
{
	work_affinity_tracker tracker(100, 16);

	tracker.record_cooperation(0, 99);
	tracker.record_cooperation(50, 75);

	EXPECT_GT(tracker.get_affinity(0, 99), 0.0);
	EXPECT_GT(tracker.get_affinity(50, 75), 0.0);
}

TEST(work_affinity_tracker_test, two_workers)
{
	work_affinity_tracker tracker(2, 16);

	tracker.record_cooperation(0, 1);

	auto victims = tracker.get_preferred_victims(0, 10);
	ASSERT_EQ(victims.size(), 1);
	EXPECT_EQ(victims[0], 1);
}
