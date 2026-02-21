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
 * @file numa_thread_pool_test.cpp
 * @brief Unit tests for numa_thread_pool and related NUMA utilities
 * @since 4.0.0
 */

#include <gtest/gtest.h>

#include <kcenon/thread/core/numa_thread_pool.h>
#include <kcenon/thread/stealing/enhanced_work_stealing_config.h>
#include <kcenon/thread/stealing/work_stealing_stats.h>
#include <kcenon/thread/stealing/numa_topology.h>

using namespace kcenon::thread;

// =============================================================================
// numa_topology standalone tests
// =============================================================================

TEST(NumaTopologyTest, DetectReturnsValidTopology) {
	auto topology = numa_topology::detect();
	// All platforms should have at least one node
	EXPECT_GE(topology.node_count(), 1u);
	EXPECT_GE(topology.cpu_count(), 1u);
}

TEST(NumaTopologyTest, GetNodesNotEmpty) {
	auto topology = numa_topology::detect();
	const auto& nodes = topology.get_nodes();
	EXPECT_FALSE(nodes.empty());
	EXPECT_EQ(nodes.size(), topology.node_count());
}

TEST(NumaTopologyTest, CpuToNodeMappingValid) {
	auto topology = numa_topology::detect();
	if (topology.cpu_count() > 0) {
		int node = topology.get_node_for_cpu(0);
		EXPECT_GE(node, 0);
	}
}

TEST(NumaTopologyTest, InvalidCpuReturnsNegative) {
	auto topology = numa_topology::detect();
	int node = topology.get_node_for_cpu(999999);
	EXPECT_EQ(node, -1);
}

TEST(NumaTopologyTest, SameNodeReflexive) {
	auto topology = numa_topology::detect();
	if (topology.cpu_count() > 0) {
		EXPECT_TRUE(topology.is_same_node(0, 0));
	}
}

TEST(NumaTopologyTest, LocalDistanceIsTen) {
	auto topology = numa_topology::detect();
	if (topology.node_count() > 0) {
		int dist = topology.get_distance(0, 0);
		EXPECT_EQ(dist, 10);
	}
}

TEST(NumaTopologyTest, GetCpusForNodeNotEmpty) {
	auto topology = numa_topology::detect();
	if (topology.node_count() > 0) {
		auto cpus = topology.get_cpus_for_node(0);
		EXPECT_FALSE(cpus.empty());
	}
}

TEST(NumaTopologyTest, GetCpusForInvalidNodeEmpty) {
	auto topology = numa_topology::detect();
	auto cpus = topology.get_cpus_for_node(999999);
	EXPECT_TRUE(cpus.empty());
}

TEST(NumaTopologyTest, DefaultConstructedIsEmpty) {
	numa_topology topology;
	EXPECT_EQ(topology.node_count(), 0u);
	EXPECT_EQ(topology.cpu_count(), 0u);
	EXPECT_FALSE(topology.is_numa_available());
}

// =============================================================================
// enhanced_work_stealing_config factory tests
// =============================================================================

TEST(EnhancedWorkStealingConfigTest, DefaultConfigDisabled) {
	auto config = enhanced_work_stealing_config::default_config();
	EXPECT_FALSE(config.enabled);
	EXPECT_FALSE(config.numa_aware);
	EXPECT_FALSE(config.collect_statistics);
}

TEST(EnhancedWorkStealingConfigTest, NumaOptimized) {
	auto config = enhanced_work_stealing_config::numa_optimized();
	EXPECT_TRUE(config.enabled);
	EXPECT_EQ(config.policy, enhanced_steal_policy::numa_aware);
	EXPECT_TRUE(config.numa_aware);
	EXPECT_TRUE(config.prefer_same_node);
	EXPECT_DOUBLE_EQ(config.numa_penalty_factor, 2.0);
	EXPECT_TRUE(config.collect_statistics);
}

TEST(EnhancedWorkStealingConfigTest, LocalityOptimized) {
	auto config = enhanced_work_stealing_config::locality_optimized();
	EXPECT_TRUE(config.enabled);
	EXPECT_EQ(config.policy, enhanced_steal_policy::locality_aware);
	EXPECT_TRUE(config.track_locality);
	EXPECT_EQ(config.locality_history_size, 32u);
	EXPECT_TRUE(config.collect_statistics);
}

TEST(EnhancedWorkStealingConfigTest, BatchOptimized) {
	auto config = enhanced_work_stealing_config::batch_optimized();
	EXPECT_TRUE(config.enabled);
	EXPECT_EQ(config.min_steal_batch, 2u);
	EXPECT_EQ(config.max_steal_batch, 8u);
	EXPECT_TRUE(config.adaptive_batch_size);
}

TEST(EnhancedWorkStealingConfigTest, HierarchicalNuma) {
	auto config = enhanced_work_stealing_config::hierarchical_numa();
	EXPECT_TRUE(config.enabled);
	EXPECT_EQ(config.policy, enhanced_steal_policy::hierarchical);
	EXPECT_TRUE(config.numa_aware);
	EXPECT_TRUE(config.prefer_same_node);
	EXPECT_DOUBLE_EQ(config.numa_penalty_factor, 3.0);
	EXPECT_TRUE(config.track_locality);
	EXPECT_TRUE(config.collect_statistics);
}

// =============================================================================
// work_stealing_stats_snapshot computed metrics tests
// =============================================================================

TEST(WorkStealingStatsSnapshotTest, ZeroStatsReturnZeroRates) {
	work_stealing_stats_snapshot snap{};
	EXPECT_DOUBLE_EQ(snap.steal_success_rate(), 0.0);
	EXPECT_DOUBLE_EQ(snap.avg_batch_size(), 0.0);
	EXPECT_DOUBLE_EQ(snap.cross_node_ratio(), 0.0);
	EXPECT_DOUBLE_EQ(snap.avg_steal_time_ns(), 0.0);
}

TEST(WorkStealingStatsSnapshotTest, SuccessRateComputed) {
	work_stealing_stats_snapshot snap{};
	snap.steal_attempts = 10;
	snap.successful_steals = 7;
	EXPECT_DOUBLE_EQ(snap.steal_success_rate(), 0.7);
}

TEST(WorkStealingStatsSnapshotTest, CrossNodeRatioComputed) {
	work_stealing_stats_snapshot snap{};
	snap.same_node_steals = 8;
	snap.cross_node_steals = 2;
	EXPECT_DOUBLE_EQ(snap.cross_node_ratio(), 0.2);
}

TEST(WorkStealingStatsSnapshotTest, AvgBatchSizeComputed) {
	work_stealing_stats_snapshot snap{};
	snap.batch_steals = 4;
	snap.total_batch_size = 12;
	EXPECT_DOUBLE_EQ(snap.avg_batch_size(), 3.0);
}

// =============================================================================
// work_stealing_stats atomic tests
// =============================================================================

TEST(WorkStealingStatsTest, InitiallyZero) {
	work_stealing_stats stats;
	EXPECT_EQ(stats.steal_attempts.load(), 0u);
	EXPECT_EQ(stats.successful_steals.load(), 0u);
	EXPECT_EQ(stats.failed_steals.load(), 0u);
	EXPECT_EQ(stats.jobs_stolen.load(), 0u);
}

TEST(WorkStealingStatsTest, ResetClearsAll) {
	work_stealing_stats stats;
	stats.steal_attempts.store(10);
	stats.successful_steals.store(5);
	stats.jobs_stolen.store(20);

	stats.reset();

	EXPECT_EQ(stats.steal_attempts.load(), 0u);
	EXPECT_EQ(stats.successful_steals.load(), 0u);
	EXPECT_EQ(stats.jobs_stolen.load(), 0u);
}

TEST(WorkStealingStatsTest, SnapshotCopiesValues) {
	work_stealing_stats stats;
	stats.steal_attempts.store(100);
	stats.successful_steals.store(80);
	stats.same_node_steals.store(60);
	stats.cross_node_steals.store(20);

	auto snap = stats.snapshot();
	EXPECT_EQ(snap.steal_attempts, 100u);
	EXPECT_EQ(snap.successful_steals, 80u);
	EXPECT_EQ(snap.same_node_steals, 60u);
	EXPECT_EQ(snap.cross_node_steals, 20u);
}

// =============================================================================
// numa_thread_pool construction tests
// =============================================================================

TEST(NumaThreadPoolTest, DefaultConstruction) {
	numa_thread_pool pool;
	// Should construct without error
}

TEST(NumaThreadPoolTest, ConstructionWithName) {
	numa_thread_pool pool("test_numa_pool");
	// Should construct with custom name
}

// =============================================================================
// numa_thread_pool NUMA API tests
// =============================================================================

TEST(NumaThreadPoolTest, IsNumaSystemReturnsValue) {
	numa_thread_pool pool("test");
	// On macOS/non-NUMA systems, should return false without crashing
	auto is_numa = pool.is_numa_system();
	(void)is_numa;
}

TEST(NumaThreadPoolTest, NumaTopologyAccessible) {
	numa_thread_pool pool("test");
	const auto& topology = pool.numa_topology_info();
	EXPECT_GE(topology.node_count(), 1u);
}

TEST(NumaThreadPoolTest, DefaultConfigIsDisabled) {
	numa_thread_pool pool("test");
	const auto& config = pool.numa_work_stealing_config();
	EXPECT_FALSE(config.enabled);
}

TEST(NumaThreadPoolTest, ConfigureNumaWorkStealing) {
	numa_thread_pool pool("test");
	auto config = enhanced_work_stealing_config::numa_optimized();
	pool.configure_numa_work_stealing(config);

	const auto& stored = pool.numa_work_stealing_config();
	EXPECT_TRUE(stored.enabled);
	EXPECT_TRUE(stored.numa_aware);
	EXPECT_TRUE(stored.prefer_same_node);
}

TEST(NumaThreadPoolTest, EnableNumaWorkStealing) {
	numa_thread_pool pool("test");
	pool.enable_numa_work_stealing();
	EXPECT_TRUE(pool.is_numa_work_stealing_enabled());
}

TEST(NumaThreadPoolTest, DisableNumaWorkStealing) {
	numa_thread_pool pool("test");
	pool.enable_numa_work_stealing();
	EXPECT_TRUE(pool.is_numa_work_stealing_enabled());

	pool.disable_numa_work_stealing();
	EXPECT_FALSE(pool.is_numa_work_stealing_enabled());
}

TEST(NumaThreadPoolTest, InitiallyNotEnabled) {
	numa_thread_pool pool("test");
	EXPECT_FALSE(pool.is_numa_work_stealing_enabled());
}

TEST(NumaThreadPoolTest, StatsInitiallyZero) {
	numa_thread_pool pool("test");
	auto stats = pool.numa_work_stealing_stats();
	EXPECT_EQ(stats.steal_attempts, 0u);
	EXPECT_EQ(stats.successful_steals, 0u);
	EXPECT_EQ(stats.failed_steals, 0u);
	EXPECT_EQ(stats.jobs_stolen, 0u);
}
