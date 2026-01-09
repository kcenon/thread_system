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

#include <kcenon/thread/stealing/numa_topology.h>

#include <algorithm>
#include <thread>

#ifdef __linux__
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
#endif

namespace kcenon::thread
{

auto numa_topology::detect() -> numa_topology
{
#ifdef __linux__
	return detect_linux();
#else
	// macOS, Windows, and other platforms: fallback to single-node topology
	return create_fallback();
#endif
}

auto numa_topology::get_node_for_cpu(int cpu_id) const -> int
{
	if (cpu_id < 0 || static_cast<std::size_t>(cpu_id) >= cpu_to_node_.size())
	{
		return -1;
	}
	return cpu_to_node_[static_cast<std::size_t>(cpu_id)];
}

auto numa_topology::get_distance(int node1, int node2) const -> int
{
	if (node1 < 0 || node2 < 0 ||
	    static_cast<std::size_t>(node1) >= distances_.size() ||
	    static_cast<std::size_t>(node2) >= distances_.size())
	{
		return -1;
	}

	const auto& row = distances_[static_cast<std::size_t>(node1)];
	if (static_cast<std::size_t>(node2) >= row.size())
	{
		return -1;
	}

	return row[static_cast<std::size_t>(node2)];
}

auto numa_topology::is_same_node(int cpu1, int cpu2) const -> bool
{
	int node1 = get_node_for_cpu(cpu1);
	int node2 = get_node_for_cpu(cpu2);

	if (node1 < 0 || node2 < 0)
	{
		return false;
	}

	return node1 == node2;
}

auto numa_topology::is_numa_available() const -> bool
{
	return nodes_.size() > 1;
}

auto numa_topology::node_count() const -> std::size_t
{
	return nodes_.size();
}

auto numa_topology::cpu_count() const -> std::size_t
{
	return total_cpus_;
}

auto numa_topology::get_nodes() const -> const std::vector<numa_node>&
{
	return nodes_;
}

auto numa_topology::get_cpus_for_node(int node_id) const -> std::vector<int>
{
	for (const auto& node : nodes_)
	{
		if (node.node_id == node_id)
		{
			return node.cpu_ids;
		}
	}
	return {};
}

#ifdef __linux__
auto numa_topology::detect_linux() -> numa_topology
{
	numa_topology topology;

	// Check if /sys/devices/system/node exists
	DIR* node_dir = opendir("/sys/devices/system/node");
	if (!node_dir)
	{
		return create_fallback();
	}

	// Find all NUMA nodes
	std::vector<int> node_ids;
	struct dirent* entry = nullptr;
	while ((entry = readdir(node_dir)) != nullptr)
	{
		std::string name = entry->d_name;
		if (name.substr(0, 4) == "node" && name.length() > 4)
		{
			try
			{
				int node_id = std::stoi(name.substr(4));
				node_ids.push_back(node_id);
			}
			catch (...)
			{
				// Skip invalid entries
			}
		}
	}
	closedir(node_dir);

	if (node_ids.empty())
	{
		return create_fallback();
	}

	std::sort(node_ids.begin(), node_ids.end());

	// Determine maximum CPU ID for cpu_to_node_ sizing
	unsigned int hw_concurrency = std::thread::hardware_concurrency();
	if (hw_concurrency == 0)
	{
		hw_concurrency = 1;
	}
	topology.cpu_to_node_.resize(hw_concurrency, -1);

	// Parse each node's information
	for (int node_id : node_ids)
	{
		numa_node node;
		node.node_id = node_id;

		// Read CPUs for this node
		std::string cpulist_path = "/sys/devices/system/node/node" +
		                           std::to_string(node_id) + "/cpulist";
		std::ifstream cpulist_file(cpulist_path);
		if (cpulist_file.is_open())
		{
			std::string line;
			if (std::getline(cpulist_file, line))
			{
				// Parse CPU list format: "0-3,8-11" or "0,1,2,3"
				std::istringstream iss(line);
				std::string token;
				while (std::getline(iss, token, ','))
				{
					// Check for range (e.g., "0-3")
					auto dash_pos = token.find('-');
					if (dash_pos != std::string::npos)
					{
						try
						{
							int start = std::stoi(token.substr(0, dash_pos));
							int end = std::stoi(token.substr(dash_pos + 1));
							for (int cpu = start; cpu <= end; ++cpu)
							{
								node.cpu_ids.push_back(cpu);
								if (static_cast<std::size_t>(cpu) < topology.cpu_to_node_.size())
								{
									topology.cpu_to_node_[static_cast<std::size_t>(cpu)] = node_id;
								}
							}
						}
						catch (...)
						{
							// Skip invalid entries
						}
					}
					else
					{
						try
						{
							int cpu = std::stoi(token);
							node.cpu_ids.push_back(cpu);
							if (static_cast<std::size_t>(cpu) < topology.cpu_to_node_.size())
							{
								topology.cpu_to_node_[static_cast<std::size_t>(cpu)] = node_id;
							}
						}
						catch (...)
						{
							// Skip invalid entries
						}
					}
				}
			}
		}

		// Read memory size for this node
		std::string meminfo_path = "/sys/devices/system/node/node" +
		                           std::to_string(node_id) + "/meminfo";
		std::ifstream meminfo_file(meminfo_path);
		if (meminfo_file.is_open())
		{
			std::string line;
			while (std::getline(meminfo_file, line))
			{
				if (line.find("MemTotal:") != std::string::npos)
				{
					std::istringstream iss(line);
					std::string dummy;
					std::size_t mem_kb = 0;
					iss >> dummy >> dummy >> mem_kb;
					node.memory_size_bytes = mem_kb * 1024;
					break;
				}
			}
		}

		topology.nodes_.push_back(std::move(node));
		topology.total_cpus_ += topology.nodes_.back().cpu_ids.size();
	}

	// Read inter-node distances
	topology.distances_.resize(node_ids.size());
	for (std::size_t i = 0; i < node_ids.size(); ++i)
	{
		topology.distances_[i].resize(node_ids.size(), 10);

		std::string distance_path = "/sys/devices/system/node/node" +
		                            std::to_string(node_ids[i]) + "/distance";
		std::ifstream distance_file(distance_path);
		if (distance_file.is_open())
		{
			std::string line;
			if (std::getline(distance_file, line))
			{
				std::istringstream iss(line);
				for (std::size_t j = 0; j < node_ids.size(); ++j)
				{
					int dist = 10;
					if (iss >> dist)
					{
						topology.distances_[i][j] = dist;
					}
				}
			}
		}
	}

	if (topology.nodes_.empty())
	{
		return create_fallback();
	}

	return topology;
}
#endif

auto numa_topology::create_fallback() -> numa_topology
{
	numa_topology topology;

	unsigned int hw_concurrency = std::thread::hardware_concurrency();
	if (hw_concurrency == 0)
	{
		hw_concurrency = 1;
	}

	// Create single NUMA node with all CPUs
	numa_node node;
	node.node_id = 0;
	for (unsigned int i = 0; i < hw_concurrency; ++i)
	{
		node.cpu_ids.push_back(static_cast<int>(i));
	}
	node.memory_size_bytes = 0;  // Unknown

	topology.nodes_.push_back(std::move(node));
	topology.total_cpus_ = hw_concurrency;

	// Initialize cpu_to_node mapping
	topology.cpu_to_node_.resize(hw_concurrency, 0);

	// Single node has distance 10 to itself
	topology.distances_ = {{10}};

	return topology;
}

} // namespace kcenon::thread
