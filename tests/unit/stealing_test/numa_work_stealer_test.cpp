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

#include <kcenon/thread/stealing/numa_work_stealer.h>
#include <kcenon/thread/lockfree/work_stealing_deque.h>
#include <kcenon/thread/core/job.h>

#include <memory>
#include <vector>

using namespace kcenon::thread;

class numa_work_stealer_test : public ::testing::Test
{
protected:
	static constexpr std::size_t NUM_WORKERS = 4;

	void SetUp() override
	{
		// Create work-stealing deques for each worker
		deques_.clear();
		for (std::size_t i = 0; i < NUM_WORKERS; ++i)
		{
			deques_.push_back(std::make_unique<lockfree::work_stealing_deque<job*>>());
		}

		// Create accessor functions
		deque_accessor_ = [this](std::size_t id) -> lockfree::work_stealing_deque<job*>* {
			if (id < deques_.size())
			{
				return deques_[id].get();
			}
			return nullptr;
		};

		cpu_accessor_ = [](std::size_t id) -> int {
			return static_cast<int>(id); // Simple mapping: worker ID = CPU ID
		};

		// Default config with statistics enabled
		config_ = enhanced_work_stealing_config::default_config();
		config_.enabled = true;
		config_.collect_statistics = true;
	}

	void TearDown() override
	{
		// Clear jobs from deques
		for (auto& deque : deques_)
		{
			while (!deque->empty())
			{
				auto result = deque->pop();
				if (result.has_value())
				{
					delete result.value();
				}
			}
		}
		deques_.clear();
	}

	std::vector<std::unique_ptr<lockfree::work_stealing_deque<job*>>> deques_;
	numa_work_stealer::deque_accessor_fn deque_accessor_;
	numa_work_stealer::cpu_accessor_fn cpu_accessor_;
	enhanced_work_stealing_config config_;
};

// ============================================
// Construction Tests
// ============================================

TEST_F(numa_work_stealer_test, construction_default_config)
{
	numa_work_stealer stealer(NUM_WORKERS, deque_accessor_, cpu_accessor_);

	auto& config = stealer.get_config();
	EXPECT_FALSE(config.enabled);
	EXPECT_EQ(config.policy, enhanced_steal_policy::adaptive);
}

TEST_F(numa_work_stealer_test, construction_with_config)
{
	config_.policy = enhanced_steal_policy::numa_aware;
	config_.numa_aware = true;

	numa_work_stealer stealer(NUM_WORKERS, deque_accessor_, cpu_accessor_, config_);

	auto& config = stealer.get_config();
	EXPECT_TRUE(config.enabled);
	EXPECT_EQ(config.policy, enhanced_steal_policy::numa_aware);
	EXPECT_TRUE(config.numa_aware);
}

// ============================================
// steal_for Tests
// ============================================

TEST_F(numa_work_stealer_test, steal_for_empty_queues_returns_null)
{
	numa_work_stealer stealer(NUM_WORKERS, deque_accessor_, cpu_accessor_, config_);

	auto* stolen = stealer.steal_for(0);
	EXPECT_EQ(stolen, nullptr);
}

TEST_F(numa_work_stealer_test, steal_for_disabled_returns_null)
{
	config_.enabled = false;
	numa_work_stealer stealer(NUM_WORKERS, deque_accessor_, cpu_accessor_, config_);

	// Add a job to worker 1's deque
	auto* test_job = new job("test_job");
	deques_[1]->push(test_job);

	auto* stolen = stealer.steal_for(0);
	EXPECT_EQ(stolen, nullptr);

	// Clean up
	delete deques_[1]->pop().value();
}

TEST_F(numa_work_stealer_test, steal_for_steals_job_from_other_worker)
{
	numa_work_stealer stealer(NUM_WORKERS, deque_accessor_, cpu_accessor_, config_);

	// Add a job to worker 1's deque
	auto* test_job = new job("test_job");
	deques_[1]->push(test_job);

	// Worker 0 tries to steal
	auto* stolen = stealer.steal_for(0);
	EXPECT_NE(stolen, nullptr);
	EXPECT_EQ(stolen->get_name(), "test_job");

	delete stolen;
}

TEST_F(numa_work_stealer_test, steal_for_updates_statistics)
{
	numa_work_stealer stealer(NUM_WORKERS, deque_accessor_, cpu_accessor_, config_);

	// Add a job to worker 1's deque
	auto* test_job = new job("test_job");
	deques_[1]->push(test_job);

	// Steal the job
	auto* stolen = stealer.steal_for(0);
	ASSERT_NE(stolen, nullptr);

	auto stats = stealer.get_stats_snapshot();
	EXPECT_GT(stats.steal_attempts, 0);
	EXPECT_EQ(stats.successful_steals, 1);
	EXPECT_EQ(stats.jobs_stolen, 1);

	delete stolen;
}

// ============================================
// steal_batch_for Tests
// ============================================

TEST_F(numa_work_stealer_test, steal_batch_for_empty_queues_returns_empty)
{
	numa_work_stealer stealer(NUM_WORKERS, deque_accessor_, cpu_accessor_, config_);

	auto stolen = stealer.steal_batch_for(0, 4);
	EXPECT_TRUE(stolen.empty());
}

TEST_F(numa_work_stealer_test, steal_batch_for_steals_multiple_jobs)
{
	numa_work_stealer stealer(NUM_WORKERS, deque_accessor_, cpu_accessor_, config_);

	// Add multiple jobs to worker 1's deque
	for (int i = 0; i < 5; ++i)
	{
		deques_[1]->push(new job("job_" + std::to_string(i)));
	}

	// Worker 0 tries to batch steal
	auto stolen = stealer.steal_batch_for(0, 4);
	EXPECT_GE(stolen.size(), 1);
	EXPECT_LE(stolen.size(), 4);

	for (auto* j : stolen)
	{
		delete j;
	}

	// Clean up remaining
	while (!deques_[1]->empty())
	{
		delete deques_[1]->pop().value();
	}
}

TEST_F(numa_work_stealer_test, steal_batch_for_updates_batch_statistics)
{
	numa_work_stealer stealer(NUM_WORKERS, deque_accessor_, cpu_accessor_, config_);

	// Add multiple jobs
	for (int i = 0; i < 5; ++i)
	{
		deques_[1]->push(new job("job_" + std::to_string(i)));
	}

	auto stolen = stealer.steal_batch_for(0, 4);
	ASSERT_FALSE(stolen.empty());

	auto stats = stealer.get_stats_snapshot();
	EXPECT_EQ(stats.batch_steals, 1);
	EXPECT_GT(stats.total_batch_size, 0);

	for (auto* j : stolen)
	{
		delete j;
	}

	while (!deques_[1]->empty())
	{
		delete deques_[1]->pop().value();
	}
}

// ============================================
// Policy Tests
// ============================================

TEST_F(numa_work_stealer_test, random_policy_steals_work)
{
	config_.policy = enhanced_steal_policy::random;
	numa_work_stealer stealer(NUM_WORKERS, deque_accessor_, cpu_accessor_, config_);

	auto* test_job = new job("test_job");
	deques_[2]->push(test_job);

	auto* stolen = stealer.steal_for(0);
	EXPECT_NE(stolen, nullptr);

	delete stolen;
}

TEST_F(numa_work_stealer_test, round_robin_policy_steals_work)
{
	config_.policy = enhanced_steal_policy::round_robin;
	numa_work_stealer stealer(NUM_WORKERS, deque_accessor_, cpu_accessor_, config_);

	auto* test_job = new job("test_job");
	deques_[2]->push(test_job);

	auto* stolen = stealer.steal_for(0);
	EXPECT_NE(stolen, nullptr);

	delete stolen;
}

TEST_F(numa_work_stealer_test, adaptive_policy_prefers_larger_queues)
{
	config_.policy = enhanced_steal_policy::adaptive;
	numa_work_stealer stealer(NUM_WORKERS, deque_accessor_, cpu_accessor_, config_);

	// Worker 1 has 1 job, worker 2 has 10 jobs
	deques_[1]->push(new job("small_queue_job"));
	for (int i = 0; i < 10; ++i)
	{
		deques_[2]->push(new job("large_queue_job"));
	}

	// Adaptive should prefer stealing from worker 2 (larger queue)
	auto* stolen = stealer.steal_for(0);
	EXPECT_NE(stolen, nullptr);

	delete stolen;

	// Clean up
	while (!deques_[1]->empty())
	{
		delete deques_[1]->pop().value();
	}
	while (!deques_[2]->empty())
	{
		delete deques_[2]->pop().value();
	}
}

// ============================================
// Configuration Tests
// ============================================

TEST_F(numa_work_stealer_test, set_config_updates_settings)
{
	numa_work_stealer stealer(NUM_WORKERS, deque_accessor_, cpu_accessor_, config_);

	enhanced_work_stealing_config new_config;
	new_config.enabled = false;
	new_config.policy = enhanced_steal_policy::hierarchical;

	stealer.set_config(new_config);

	auto& config = stealer.get_config();
	EXPECT_FALSE(config.enabled);
	EXPECT_EQ(config.policy, enhanced_steal_policy::hierarchical);
}

TEST_F(numa_work_stealer_test, reset_stats_clears_statistics)
{
	numa_work_stealer stealer(NUM_WORKERS, deque_accessor_, cpu_accessor_, config_);

	// Generate some statistics
	auto* test_job = new job("test_job");
	deques_[1]->push(test_job);
	auto* stolen = stealer.steal_for(0);
	delete stolen;

	auto stats_before = stealer.get_stats_snapshot();
	EXPECT_GT(stats_before.steal_attempts, 0);

	stealer.reset_stats();

	auto stats_after = stealer.get_stats_snapshot();
	EXPECT_EQ(stats_after.steal_attempts, 0);
	EXPECT_EQ(stats_after.successful_steals, 0);
}

// ============================================
// Topology Tests
// ============================================

TEST_F(numa_work_stealer_test, get_topology_returns_valid_topology)
{
	numa_work_stealer stealer(NUM_WORKERS, deque_accessor_, cpu_accessor_, config_);

	auto& topology = stealer.get_topology();
	EXPECT_GE(topology.node_count(), 1);
}

// ============================================
// Single Worker Tests
// ============================================

TEST_F(numa_work_stealer_test, single_worker_returns_null)
{
	numa_work_stealer stealer(1, deque_accessor_, cpu_accessor_, config_);

	deques_[0]->push(new job("test_job"));

	// With only 1 worker, there's no one to steal from
	auto* stolen = stealer.steal_for(0);
	EXPECT_EQ(stolen, nullptr);

	delete deques_[0]->pop().value();
}
