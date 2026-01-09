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

#include <kcenon/thread/stealing/numa_topology.h>

#include <thread>

using namespace kcenon::thread;

class numa_topology_test : public ::testing::Test
{
protected:
	void SetUp() override
	{
		topology_ = numa_topology::detect();
	}

	numa_topology topology_;
};

// ============================================
// Basic Detection Tests
// ============================================

TEST_F(numa_topology_test, detect_returns_valid_topology)
{
	// Topology should always return at least one node
	EXPECT_GE(topology_.node_count(), 1);
	EXPECT_GT(topology_.cpu_count(), 0);
}

TEST_F(numa_topology_test, node_count_matches_nodes)
{
	auto nodes = topology_.get_nodes();
	EXPECT_EQ(topology_.node_count(), nodes.size());
}

TEST_F(numa_topology_test, cpu_count_matches_hardware_concurrency)
{
	unsigned int hw = std::thread::hardware_concurrency();
	if (hw > 0)
	{
		// CPU count should be at least as much as hardware_concurrency reports
		EXPECT_GE(topology_.cpu_count(), static_cast<std::size_t>(hw));
	}
}

// ============================================
// Node Query Tests
// ============================================

TEST_F(numa_topology_test, get_node_for_cpu_valid)
{
	// CPU 0 should be on some node
	int node = topology_.get_node_for_cpu(0);
	EXPECT_GE(node, 0);
}

TEST_F(numa_topology_test, get_node_for_cpu_invalid)
{
	// Invalid CPU should return -1
	int node = topology_.get_node_for_cpu(-1);
	EXPECT_EQ(node, -1);

	// Very large CPU ID should return -1
	node = topology_.get_node_for_cpu(99999);
	EXPECT_EQ(node, -1);
}

TEST_F(numa_topology_test, all_cpus_on_valid_node)
{
	for (std::size_t cpu = 0; cpu < topology_.cpu_count(); ++cpu)
	{
		int node = topology_.get_node_for_cpu(static_cast<int>(cpu));
		EXPECT_GE(node, 0) << "CPU " << cpu << " should be on a valid node";
	}
}

// ============================================
// Distance Tests
// ============================================

TEST_F(numa_topology_test, same_node_distance)
{
	// Distance from node 0 to itself should be 10 (local)
	int dist = topology_.get_distance(0, 0);
	EXPECT_EQ(dist, 10);
}

TEST_F(numa_topology_test, invalid_node_distance)
{
	// Invalid nodes should return -1
	int dist = topology_.get_distance(-1, 0);
	EXPECT_EQ(dist, -1);

	dist = topology_.get_distance(0, -1);
	EXPECT_EQ(dist, -1);

	dist = topology_.get_distance(99999, 0);
	EXPECT_EQ(dist, -1);
}

// ============================================
// Same Node Tests
// ============================================

TEST_F(numa_topology_test, is_same_node_same_cpu)
{
	// CPU 0 should be on the same node as itself
	EXPECT_TRUE(topology_.is_same_node(0, 0));
}

TEST_F(numa_topology_test, is_same_node_invalid_cpu)
{
	// Invalid CPUs should return false
	EXPECT_FALSE(topology_.is_same_node(-1, 0));
	EXPECT_FALSE(topology_.is_same_node(0, -1));
	EXPECT_FALSE(topology_.is_same_node(99999, 0));
}

TEST_F(numa_topology_test, is_same_node_consistency)
{
	// All CPUs on node 0 should report same node
	auto cpus = topology_.get_cpus_for_node(0);
	for (std::size_t i = 0; i < cpus.size(); ++i)
	{
		for (std::size_t j = i; j < cpus.size(); ++j)
		{
			EXPECT_TRUE(topology_.is_same_node(cpus[i], cpus[j]))
			    << "CPUs " << cpus[i] << " and " << cpus[j] << " should be on the same node";
		}
	}
}

// ============================================
// NUMA Availability Tests
// ============================================

TEST_F(numa_topology_test, numa_availability_consistency)
{
	// is_numa_available should be true only if more than 1 node
	bool available = topology_.is_numa_available();
	if (available)
	{
		EXPECT_GT(topology_.node_count(), 1);
	}
	else
	{
		EXPECT_EQ(topology_.node_count(), 1);
	}
}

// ============================================
// Get CPUs for Node Tests
// ============================================

TEST_F(numa_topology_test, get_cpus_for_node_valid)
{
	auto cpus = topology_.get_cpus_for_node(0);
	EXPECT_FALSE(cpus.empty()) << "Node 0 should have at least one CPU";
}

TEST_F(numa_topology_test, get_cpus_for_node_invalid)
{
	auto cpus = topology_.get_cpus_for_node(-1);
	EXPECT_TRUE(cpus.empty());

	cpus = topology_.get_cpus_for_node(99999);
	EXPECT_TRUE(cpus.empty());
}

TEST_F(numa_topology_test, all_cpus_covered)
{
	// All CPUs should be in exactly one node
	std::vector<bool> cpu_seen(topology_.cpu_count(), false);

	for (const auto& node : topology_.get_nodes())
	{
		for (int cpu : node.cpu_ids)
		{
			std::size_t cpu_idx = static_cast<std::size_t>(cpu);
			if (cpu_idx < cpu_seen.size())
			{
				EXPECT_FALSE(cpu_seen[cpu_idx])
				    << "CPU " << cpu << " appears in multiple nodes";
				cpu_seen[cpu_idx] = true;
			}
		}
	}

	for (std::size_t i = 0; i < cpu_seen.size(); ++i)
	{
		EXPECT_TRUE(cpu_seen[i]) << "CPU " << i << " is not assigned to any node";
	}
}

// ============================================
// Fallback Tests (for non-NUMA systems)
// ============================================

TEST_F(numa_topology_test, fallback_single_node)
{
	// On non-NUMA systems (like macOS), should have exactly one node
	// This test verifies the fallback behavior works correctly
#ifndef __linux__
	EXPECT_EQ(topology_.node_count(), 1);
	EXPECT_FALSE(topology_.is_numa_available());
#endif
}

TEST_F(numa_topology_test, fallback_all_cpus_on_node_zero)
{
	// On non-NUMA systems, all CPUs should be on node 0
#ifndef __linux__
	auto cpus = topology_.get_cpus_for_node(0);
	EXPECT_EQ(cpus.size(), topology_.cpu_count());
#endif
}
