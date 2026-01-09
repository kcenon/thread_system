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
