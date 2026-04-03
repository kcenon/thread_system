// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/**
 * @file enhanced_steal_policy.h
 * @brief Enhanced policies for selecting work-stealing victims.
 *
 * @see numa_work_stealer
 */

#pragma once

#include <cstdint>

namespace kcenon::thread
{

/**
 * @enum enhanced_steal_policy
 * @brief Enhanced policies for selecting work-stealing victims
 *
 * These policies extend the basic steal_policy with NUMA awareness and
 * locality optimizations. Choose based on your workload characteristics
 * and system architecture.
 *
 * ### Policy Comparison
 * | Policy | Description | Best For |
 * |--------|-------------|----------|
 * | random | Random victim selection | General use, good load distribution |
 * | round_robin | Sequential selection | Deterministic, fair distribution |
 * | adaptive | Queue size based | Uneven workloads |
 * | numa_aware | Prefer same NUMA node | NUMA systems with memory locality |
 * | locality_aware | Prefer recently cooperated | Cache-sensitive workloads |
 * | hierarchical | NUMA node -> random | Large NUMA systems |
 */
enum class enhanced_steal_policy : std::uint8_t
{
	random,           ///< Random victim selection (baseline, good distribution)
	round_robin,      ///< Sequential victim selection (deterministic, fair)
	adaptive,         ///< Select based on queue sizes (best for uneven loads)
	numa_aware,       ///< Prefer workers on the same NUMA node (reduces cross-node traffic)
	locality_aware,   ///< Prefer workers with recent cooperation history (cache affinity)
	hierarchical      ///< NUMA node first, then random within node (large NUMA systems)
};

/**
 * @brief Convert enhanced_steal_policy to string representation
 * @param policy The policy to convert
 * @return String name of the policy
 */
constexpr const char* to_string(enhanced_steal_policy policy)
{
	switch (policy)
	{
	case enhanced_steal_policy::random:
		return "random";
	case enhanced_steal_policy::round_robin:
		return "round_robin";
	case enhanced_steal_policy::adaptive:
		return "adaptive";
	case enhanced_steal_policy::numa_aware:
		return "numa_aware";
	case enhanced_steal_policy::locality_aware:
		return "locality_aware";
	case enhanced_steal_policy::hierarchical:
		return "hierarchical";
	default:
		return "unknown";
	}
}

} // namespace kcenon::thread
