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

#include <cstddef>
#include <vector>

namespace kcenon::thread
{

/**
 * @struct numa_node
 * @brief Information about a single NUMA node
 */
struct numa_node
{
	int node_id{-1};                   ///< NUMA node identifier
	std::vector<int> cpu_ids;          ///< CPUs belonging to this node
	std::size_t memory_size_bytes{0};  ///< Total memory on this node
};

/**
 * @class numa_topology
 * @brief NUMA (Non-Uniform Memory Access) topology information
 *
 * This class provides information about the system's NUMA topology,
 * including the number of NUMA nodes, which CPUs belong to which nodes,
 * and the distances between nodes.
 *
 * ### Thread Safety
 * All methods are thread-safe after construction. The topology is
 * detected once during construction and remains immutable.
 *
 * ### Platform Support
 * - Linux: Full support via /sys/devices/system/node
 * - macOS: Falls back to single-node topology (no NUMA on macOS)
 * - Windows: Falls back to single-node topology (could be extended)
 *
 * ### Usage Example
 * @code
 * auto topology = numa_topology::detect();
 *
 * // Check if system has NUMA
 * if (topology.is_numa_available()) {
 *     std::cout << "NUMA nodes: " << topology.node_count() << "\n";
 *
 *     // Find which node a CPU belongs to
 *     int cpu_id = 0;
 *     int node = topology.get_node_for_cpu(cpu_id);
 *
 *     // Check if two CPUs are on the same node
 *     if (topology.is_same_node(0, 1)) {
 *         std::cout << "CPU 0 and 1 are on the same NUMA node\n";
 *     }
 * }
 * @endcode
 */
class numa_topology
{
public:
	/**
	 * @brief Default constructor - creates an empty topology
	 */
	numa_topology() = default;

	/**
	 * @brief Detect and return the system's NUMA topology
	 * @return numa_topology object with detected information
	 *
	 * This static method detects the NUMA topology of the current system.
	 * On non-NUMA systems or unsupported platforms, it returns a single-node
	 * topology with all CPUs.
	 */
	[[nodiscard]] static auto detect() -> numa_topology;

	/**
	 * @brief Get the NUMA node for a given CPU
	 * @param cpu_id The CPU identifier
	 * @return NUMA node ID, or -1 if CPU not found
	 */
	[[nodiscard]] auto get_node_for_cpu(int cpu_id) const -> int;

	/**
	 * @brief Get the distance between two NUMA nodes
	 * @param node1 First NUMA node ID
	 * @param node2 Second NUMA node ID
	 * @return Distance value (10 = local, higher = farther), or -1 if invalid
	 *
	 * The distance is a relative measure where:
	 * - 10 typically means local (same node)
	 * - Higher values indicate greater latency/bandwidth cost
	 */
	[[nodiscard]] auto get_distance(int node1, int node2) const -> int;

	/**
	 * @brief Check if two CPUs are on the same NUMA node
	 * @param cpu1 First CPU ID
	 * @param cpu2 Second CPU ID
	 * @return true if both CPUs are on the same node
	 */
	[[nodiscard]] auto is_same_node(int cpu1, int cpu2) const -> bool;

	/**
	 * @brief Check if NUMA is available on this system
	 * @return true if system has multiple NUMA nodes
	 */
	[[nodiscard]] auto is_numa_available() const -> bool;

	/**
	 * @brief Get the number of NUMA nodes
	 * @return Number of NUMA nodes
	 */
	[[nodiscard]] auto node_count() const -> std::size_t;

	/**
	 * @brief Get the total number of CPUs
	 * @return Total CPU count
	 */
	[[nodiscard]] auto cpu_count() const -> std::size_t;

	/**
	 * @brief Get all NUMA nodes
	 * @return Vector of numa_node structures
	 */
	[[nodiscard]] auto get_nodes() const -> const std::vector<numa_node>&;

	/**
	 * @brief Get CPUs belonging to a specific node
	 * @param node_id NUMA node ID
	 * @return Vector of CPU IDs, empty if node not found
	 */
	[[nodiscard]] auto get_cpus_for_node(int node_id) const -> std::vector<int>;

private:
	std::vector<numa_node> nodes_;              ///< All NUMA nodes
	std::vector<int> cpu_to_node_;              ///< CPU ID -> NUMA node mapping
	std::vector<std::vector<int>> distances_;   ///< Inter-node distances
	std::size_t total_cpus_{0};                 ///< Total CPU count

	/**
	 * @brief Detect topology on Linux using sysfs
	 */
	static auto detect_linux() -> numa_topology;

	/**
	 * @brief Create fallback single-node topology
	 * @return Single-node topology with all CPUs
	 */
	static auto create_fallback() -> numa_topology;
};

} // namespace kcenon::thread
