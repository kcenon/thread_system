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

#include "enhanced_work_stealing_config.h"
#include "numa_topology.h"
#include "steal_backoff_strategy.h"
#include "work_affinity_tracker.h"
#include "work_stealing_stats.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <random>
#include <vector>

namespace kcenon::thread
{

// Forward declarations
class thread_worker;
class job;

namespace lockfree
{
template <typename T>
class work_stealing_deque;
} // namespace lockfree

/**
 * @class numa_work_stealer
 * @brief NUMA-aware work stealer with enhanced victim selection policies
 *
 * This class implements advanced work-stealing strategies with NUMA awareness,
 * locality tracking, batch stealing, and comprehensive statistics collection.
 * It coordinates stealing across multiple workers using configurable policies.
 *
 * ### Design Goals
 * - Minimize cross-NUMA node memory access
 * - Maximize cache locality through affinity tracking
 * - Reduce contention through intelligent victim selection
 * - Provide detailed statistics for performance analysis
 *
 * ### Thread Safety
 * All public methods are thread-safe and can be called concurrently from
 * multiple worker threads. Statistics updates use atomic operations.
 *
 * ### Memory Model
 * - Victim selection: sequential consistency for correctness
 * - Statistics: relaxed ordering for performance
 * - Topology access: read-only after construction
 *
 * ### Usage Example
 * @code
 * // Create workers
 * std::vector<std::unique_ptr<thread_worker>> workers;
 * // ... initialize workers ...
 *
 * // Configure NUMA-aware stealing
 * auto config = enhanced_work_stealing_config::numa_optimized();
 *
 * // Create accessor function
 * auto get_worker_deque = [&](std::size_t id) {
 *     return workers[id]->get_local_deque();
 * };
 * auto get_worker_cpu = [&](std::size_t id) {
 *     return workers[id]->get_policy().preferred_cpu;
 * };
 *
 * // Create work stealer
 * numa_work_stealer stealer(workers.size(), get_worker_deque, get_worker_cpu, config);
 *
 * // Steal work for worker 0
 * if (auto* stolen_job = stealer.steal_for(0)) {
 *     // Process stolen job
 * }
 *
 * // Batch steal
 * auto batch = stealer.steal_batch_for(0, 4);
 * for (auto* j : batch) {
 *     // Process jobs
 * }
 *
 * // Get statistics
 * auto stats = stealer.get_stats();
 * std::cout << "Success rate: " << stats.steal_success_rate() * 100 << "%\n";
 * @endcode
 */
class numa_work_stealer
{
public:
	/**
	 * @brief Function type for accessing worker's local deque
	 * @param worker_id The worker ID
	 * @return Pointer to the worker's local deque, or nullptr if not available
	 */
	using deque_accessor_fn = std::function<lockfree::work_stealing_deque<job*>*(std::size_t)>;

	/**
	 * @brief Function type for getting worker's CPU affinity
	 * @param worker_id The worker ID
	 * @return Preferred CPU for the worker, or -1 if no preference
	 */
	using cpu_accessor_fn = std::function<int(std::size_t)>;

	/**
	 * @brief Construct a NUMA-aware work stealer
	 * @param worker_count Number of workers in the pool
	 * @param deque_accessor Function to access worker deques
	 * @param cpu_accessor Function to get worker CPU affinity
	 * @param config Configuration for work stealing
	 *
	 * @note The accessor functions must remain valid for the lifetime of this object.
	 */
	numa_work_stealer(std::size_t worker_count,
	                  deque_accessor_fn deque_accessor,
	                  cpu_accessor_fn cpu_accessor,
	                  enhanced_work_stealing_config config = {});

	/**
	 * @brief Destructor
	 */
	~numa_work_stealer() = default;

	// Non-copyable, non-movable (contains atomics)
	numa_work_stealer(const numa_work_stealer&) = delete;
	numa_work_stealer& operator=(const numa_work_stealer&) = delete;
	numa_work_stealer(numa_work_stealer&&) = delete;
	numa_work_stealer& operator=(numa_work_stealer&&) = delete;

	/**
	 * @brief Attempt to steal work for a worker
	 * @param worker_id The worker requesting work
	 * @return Stolen job pointer, or nullptr if no work available
	 *
	 * This method selects victims based on the configured policy and attempts
	 * to steal a single job. NUMA awareness and affinity are considered
	 * when selecting victims.
	 *
	 * Thread Safety:
	 * - Safe to call concurrently from multiple workers
	 * - Statistics are updated atomically
	 */
	[[nodiscard]] auto steal_for(std::size_t worker_id) -> job*;

	/**
	 * @brief Attempt to steal multiple jobs for a worker
	 * @param worker_id The worker requesting work
	 * @param max_count Maximum number of jobs to steal
	 * @return Vector of stolen job pointers (may be empty or smaller than max_count)
	 *
	 * Batch stealing is more efficient when multiple jobs need to be transferred.
	 * The actual batch size is determined by configuration and victim queue depth.
	 *
	 * Thread Safety:
	 * - Safe to call concurrently from multiple workers
	 * - Statistics are updated atomically
	 */
	[[nodiscard]] auto steal_batch_for(std::size_t worker_id, std::size_t max_count)
		-> std::vector<job*>;

	/**
	 * @brief Get the current statistics
	 * @return Reference to the work-stealing statistics
	 */
	[[nodiscard]] auto get_stats() const -> const work_stealing_stats&;

	/**
	 * @brief Get a snapshot of current statistics
	 * @return Non-atomic copy of statistics for safe reading
	 */
	[[nodiscard]] auto get_stats_snapshot() const -> work_stealing_stats_snapshot;

	/**
	 * @brief Reset all statistics to zero
	 */
	void reset_stats();

	/**
	 * @brief Get the NUMA topology information
	 * @return Reference to the detected NUMA topology
	 */
	[[nodiscard]] auto get_topology() const -> const numa_topology&;

	/**
	 * @brief Get the current configuration
	 * @return Reference to the work-stealing configuration
	 */
	[[nodiscard]] auto get_config() const -> const enhanced_work_stealing_config&;

	/**
	 * @brief Update the configuration
	 * @param config New configuration to use
	 *
	 * @note Changes take effect immediately. Be cautious when changing
	 *       configuration while workers are actively stealing.
	 */
	void set_config(const enhanced_work_stealing_config& config);

private:
	/**
	 * @brief Select victim workers based on the configured policy
	 * @param requester_id Worker requesting victims
	 * @param count Maximum number of victims to select
	 * @return Vector of worker IDs to attempt stealing from
	 */
	[[nodiscard]] auto select_victims(std::size_t requester_id, std::size_t count)
		-> std::vector<std::size_t>;

	/**
	 * @brief Select victims using random policy
	 */
	[[nodiscard]] auto select_victims_random(std::size_t requester_id, std::size_t count)
		-> std::vector<std::size_t>;

	/**
	 * @brief Select victims using round-robin policy
	 */
	[[nodiscard]] auto select_victims_round_robin(std::size_t requester_id, std::size_t count)
		-> std::vector<std::size_t>;

	/**
	 * @brief Select victims using adaptive (queue-size based) policy
	 */
	[[nodiscard]] auto select_victims_adaptive(std::size_t requester_id, std::size_t count)
		-> std::vector<std::size_t>;

	/**
	 * @brief Select victims using NUMA-aware policy
	 */
	[[nodiscard]] auto select_victims_numa_aware(std::size_t requester_id, std::size_t count)
		-> std::vector<std::size_t>;

	/**
	 * @brief Select victims using locality-aware policy
	 */
	[[nodiscard]] auto select_victims_locality_aware(std::size_t requester_id, std::size_t count)
		-> std::vector<std::size_t>;

	/**
	 * @brief Select victims using hierarchical policy
	 */
	[[nodiscard]] auto select_victims_hierarchical(std::size_t requester_id, std::size_t count)
		-> std::vector<std::size_t>;

	/**
	 * @brief Calculate batch size based on configuration and victim queue depth
	 */
	[[nodiscard]] auto calculate_batch_size(std::size_t victim_queue_size) const -> std::size_t;

	/**
	 * @brief Get the CPU ID for a worker
	 */
	[[nodiscard]] auto get_worker_cpu(std::size_t worker_id) const -> int;

	/**
	 * @brief Check if two workers are on the same NUMA node
	 */
	[[nodiscard]] auto workers_on_same_node(std::size_t worker_a, std::size_t worker_b) const
		-> bool;

	/**
	 * @brief Record a successful steal for affinity tracking
	 */
	void record_steal(std::size_t thief_id, std::size_t victim_id);

	std::size_t worker_count_;
	deque_accessor_fn deque_accessor_;
	cpu_accessor_fn cpu_accessor_;
	enhanced_work_stealing_config config_;
	numa_topology topology_;
	work_stealing_stats stats_;
	std::unique_ptr<work_affinity_tracker> affinity_tracker_;
	std::unique_ptr<backoff_calculator> backoff_calculator_;

	// Per-thread random generators for random victim selection
	mutable std::mt19937_64 rng_;

	// Round-robin state
	mutable std::atomic<std::size_t> round_robin_index_{0};
};

} // namespace kcenon::thread
