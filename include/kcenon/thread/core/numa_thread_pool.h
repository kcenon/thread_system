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

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/stealing/enhanced_work_stealing_config.h>
#include <kcenon/thread/stealing/work_stealing_stats.h>
#include <kcenon/thread/stealing/numa_topology.h>

namespace kcenon::thread
{

/**
 * @class numa_thread_pool
 * @brief A NUMA-aware thread pool optimized for Non-Uniform Memory Access architectures
 *
 * @ingroup thread_pools
 *
 * The @c numa_thread_pool class extends @c thread_pool with specialized support for
 * NUMA (Non-Uniform Memory Access) architectures. It provides:
 * - NUMA topology detection and awareness
 * - NUMA-optimized work stealing (prefer same-node steals)
 * - Cross-node steal penalty configuration
 * - NUMA-specific statistics collection
 *
 * This class is designed for systems where memory access latency varies based on
 * the physical location of CPUs and memory. By preferring work stealing from
 * workers on the same NUMA node, it can significantly improve cache locality
 * and reduce cross-node memory traffic.
 *
 * ### When to Use
 * Use @c numa_thread_pool when:
 * - Running on multi-socket servers with NUMA architecture
 * - Memory-intensive workloads where cache locality matters
 * - You need to monitor NUMA-specific performance metrics
 *
 * For single-socket systems or NUMA-unaware workloads, use the base @c thread_pool
 * class for simpler API and lower overhead.
 *
 * ### Usage Example
 * @code
 * #include <kcenon/thread/core/numa_thread_pool.h>
 *
 * // Create NUMA-aware pool with default settings
 * auto pool = std::make_shared<numa_thread_pool>("numa_workers");
 *
 * // Configure NUMA-optimized work stealing
 * pool->configure_numa_work_stealing(enhanced_work_stealing_config::numa_optimized());
 *
 * // Add workers and start
 * pool->enqueue(std::make_unique<thread_worker>(true));
 * pool->start();
 *
 * // Check NUMA topology
 * const auto& topology = pool->numa_topology();
 * std::cout << "NUMA nodes: " << topology.node_count() << "\n";
 *
 * // Monitor NUMA performance
 * auto stats = pool->numa_work_stealing_stats();
 * std::cout << "Cross-node ratio: " << stats.cross_node_ratio() << "\n";
 * @endcode
 *
 * ### Migration from thread_pool NUMA methods
 * If you were using NUMA methods directly on thread_pool, migrate as follows:
 * @code
 * // Old (deprecated):
 * auto pool = std::make_shared<thread_pool>("pool");
 * pool->set_work_stealing_config(config);  // Deprecated
 * auto stats = pool->get_work_stealing_stats();  // Deprecated
 *
 * // New (recommended):
 * auto pool = std::make_shared<numa_thread_pool>("pool");
 * pool->configure_numa_work_stealing(config);
 * auto stats = pool->numa_work_stealing_stats();
 * @endcode
 *
 * @see thread_pool Base class with general thread pool functionality
 * @see enhanced_work_stealing_config Configuration for NUMA-aware work stealing
 * @see numa_topology System NUMA topology information
 */
class numa_thread_pool : public thread_pool
{
public:
	/**
	 * @brief Constructs a new @c numa_thread_pool instance.
	 * @param thread_title An optional title or identifier for the thread pool
	 *        (defaults to "numa_thread_pool").
	 * @param context Optional thread context for logging and monitoring
	 *        (defaults to empty context).
	 *
	 * The pool automatically detects the system's NUMA topology on construction.
	 */
	explicit numa_thread_pool(const std::string& thread_title = "numa_thread_pool",
	                          const thread_context& context = thread_context());

	/**
	 * @brief Constructs a new @c numa_thread_pool instance with a custom job queue.
	 * @param thread_title A title or identifier for the thread pool.
	 * @param custom_queue A custom job queue implementation.
	 * @param context Optional thread context for logging and monitoring.
	 */
	numa_thread_pool(const std::string& thread_title,
	                 std::shared_ptr<job_queue> custom_queue,
	                 const thread_context& context = thread_context());

	/**
	 * @brief Constructs a new @c numa_thread_pool instance with a policy_queue adapter.
	 * @param thread_title A title or identifier for the thread pool.
	 * @param queue_adapter A queue adapter wrapping a policy_queue.
	 * @param context Optional thread context for logging and monitoring.
	 */
	numa_thread_pool(const std::string& thread_title,
	                 std::unique_ptr<pool_queue_adapter_interface> queue_adapter,
	                 const thread_context& context = thread_context());

	/**
	 * @brief Virtual destructor.
	 */
	~numa_thread_pool() override = default;

	// =========================================================================
	// NUMA-specific Methods
	// =========================================================================

	/**
	 * @brief Configure NUMA-aware work stealing.
	 * @param config The enhanced work-stealing configuration.
	 *
	 * This is the primary method for enabling and configuring NUMA-aware
	 * work stealing. Use factory methods on enhanced_work_stealing_config
	 * for common configurations:
	 * @code
	 * pool->configure_numa_work_stealing(enhanced_work_stealing_config::numa_optimized());
	 * @endcode
	 */
	void configure_numa_work_stealing(const enhanced_work_stealing_config& config);

	/**
	 * @brief Get the current NUMA work-stealing configuration.
	 * @return Reference to the current configuration.
	 */
	[[nodiscard]] const enhanced_work_stealing_config& numa_work_stealing_config() const;

	/**
	 * @brief Get a snapshot of NUMA work-stealing statistics.
	 * @return Non-atomic snapshot of current statistics.
	 *
	 * Statistics include:
	 * - Steal attempts and success rate
	 * - Same-node vs cross-node steals
	 * - Batch stealing metrics
	 * - Timing information
	 */
	[[nodiscard]] work_stealing_stats_snapshot numa_work_stealing_stats() const;

	/**
	 * @brief Get the detected NUMA topology.
	 * @return Reference to the NUMA topology information.
	 *
	 * The topology is detected once and cached. It includes:
	 * - Number of NUMA nodes
	 * - CPU-to-node mapping
	 * - Inter-node distances
	 */
	[[nodiscard]] const numa_topology& numa_topology_info() const;

	/**
	 * @brief Check if the system has NUMA architecture.
	 * @return true if system has multiple NUMA nodes, false otherwise.
	 */
	[[nodiscard]] bool is_numa_system() const;

	/**
	 * @brief Enable NUMA-optimized work stealing with default settings.
	 *
	 * Convenience method equivalent to:
	 * @code
	 * configure_numa_work_stealing(enhanced_work_stealing_config::numa_optimized());
	 * @endcode
	 */
	void enable_numa_work_stealing();

	/**
	 * @brief Disable NUMA-aware work stealing.
	 *
	 * Reverts to basic work stealing without NUMA awareness.
	 */
	void disable_numa_work_stealing();

	/**
	 * @brief Check if NUMA work stealing is currently enabled.
	 * @return true if NUMA work stealing is enabled.
	 */
	[[nodiscard]] bool is_numa_work_stealing_enabled() const;

private:
	/// Cached NUMA topology (detected on construction)
	mutable numa_topology cached_topology_;

	/// Flag indicating if topology has been detected
	mutable bool topology_detected_{false};

	/// Ensure topology is detected
	void ensure_topology_detected() const;
};

} // namespace kcenon::thread
