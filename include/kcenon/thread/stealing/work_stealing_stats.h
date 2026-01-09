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

#include <atomic>
#include <cstdint>

namespace kcenon::thread
{

/**
 * @struct work_stealing_stats_snapshot
 * @brief Non-atomic snapshot of work-stealing statistics
 *
 * This structure provides a non-atomic copy of work_stealing_stats
 * for safe reading without synchronization concerns.
 */
struct work_stealing_stats_snapshot
{
	std::uint64_t steal_attempts{0};
	std::uint64_t successful_steals{0};
	std::uint64_t failed_steals{0};
	std::uint64_t jobs_stolen{0};
	std::uint64_t same_node_steals{0};
	std::uint64_t cross_node_steals{0};
	std::uint64_t batch_steals{0};
	std::uint64_t total_batch_size{0};
	std::uint64_t total_steal_time_ns{0};
	std::uint64_t total_backoff_time_ns{0};

	[[nodiscard]] auto steal_success_rate() const -> double
	{
		if (steal_attempts == 0)
		{
			return 0.0;
		}
		return static_cast<double>(successful_steals) / static_cast<double>(steal_attempts);
	}

	[[nodiscard]] auto avg_batch_size() const -> double
	{
		if (batch_steals == 0)
		{
			return 0.0;
		}
		return static_cast<double>(total_batch_size) / static_cast<double>(batch_steals);
	}

	[[nodiscard]] auto cross_node_ratio() const -> double
	{
		auto total = same_node_steals + cross_node_steals;
		if (total == 0)
		{
			return 0.0;
		}
		return static_cast<double>(cross_node_steals) / static_cast<double>(total);
	}

	[[nodiscard]] auto avg_steal_time_ns() const -> double
	{
		if (steal_attempts == 0)
		{
			return 0.0;
		}
		return static_cast<double>(total_steal_time_ns) / static_cast<double>(steal_attempts);
	}
};

/**
 * @struct work_stealing_stats
 * @brief Statistics for work-stealing operations
 *
 * This structure tracks various metrics about work-stealing performance,
 * including success rates, NUMA locality, batch efficiency, and timing.
 * All counters are atomic for thread-safe updates from multiple workers.
 *
 * ### Thread Safety
 * All members use atomic operations for lock-free concurrent updates.
 * Statistics may show momentary inconsistencies during rapid updates
 * but will eventually converge to accurate totals.
 *
 * ### Memory Ordering
 * - Counter updates: relaxed ordering (eventual consistency acceptable)
 * - Snapshot reads: acquire ordering for consistent view
 *
 * ### Usage Example
 * @code
 * work_stealing_stats stats;
 *
 * // After stealing operations
 * stats.steal_attempts.fetch_add(1, std::memory_order_relaxed);
 * stats.successful_steals.fetch_add(1, std::memory_order_relaxed);
 * stats.jobs_stolen.fetch_add(batch_size, std::memory_order_relaxed);
 *
 * // Query statistics
 * double success_rate = stats.steal_success_rate();
 * double avg_batch = stats.avg_batch_size();
 * double cross_numa = stats.cross_node_ratio();
 * @endcode
 */
struct work_stealing_stats
{
	// ========================================================================
	// Steal Counts
	// ========================================================================

	/// Total number of steal attempts
	std::atomic<std::uint64_t> steal_attempts{0};

	/// Number of successful steal operations
	std::atomic<std::uint64_t> successful_steals{0};

	/// Number of failed steal operations
	std::atomic<std::uint64_t> failed_steals{0};

	/// Total number of jobs successfully stolen
	std::atomic<std::uint64_t> jobs_stolen{0};

	// ========================================================================
	// NUMA Statistics
	// ========================================================================

	/// Steals from workers on the same NUMA node
	std::atomic<std::uint64_t> same_node_steals{0};

	/// Steals from workers on different NUMA nodes
	std::atomic<std::uint64_t> cross_node_steals{0};

	// ========================================================================
	// Batch Statistics
	// ========================================================================

	/// Number of batch steal operations (stealing multiple jobs)
	std::atomic<std::uint64_t> batch_steals{0};

	/// Total size of all batch steals (for averaging)
	std::atomic<std::uint64_t> total_batch_size{0};

	// ========================================================================
	// Timing Statistics
	// ========================================================================

	/// Total time spent in steal operations (nanoseconds)
	std::atomic<std::uint64_t> total_steal_time_ns{0};

	/// Total time spent in backoff delays (nanoseconds)
	std::atomic<std::uint64_t> total_backoff_time_ns{0};

	// ========================================================================
	// Computed Metrics
	// ========================================================================

	/**
	 * @brief Calculate the steal success rate
	 * @return Success rate as a ratio (0.0 to 1.0)
	 *
	 * Returns 0.0 if no steal attempts have been made.
	 */
	[[nodiscard]] auto steal_success_rate() const -> double
	{
		auto attempts = steal_attempts.load(std::memory_order_relaxed);
		if (attempts == 0)
		{
			return 0.0;
		}
		auto successes = successful_steals.load(std::memory_order_relaxed);
		return static_cast<double>(successes) / static_cast<double>(attempts);
	}

	/**
	 * @brief Calculate the average batch size
	 * @return Average number of jobs per batch steal
	 *
	 * Returns 0.0 if no batch steals have been made.
	 */
	[[nodiscard]] auto avg_batch_size() const -> double
	{
		auto batches = batch_steals.load(std::memory_order_relaxed);
		if (batches == 0)
		{
			return 0.0;
		}
		auto total = total_batch_size.load(std::memory_order_relaxed);
		return static_cast<double>(total) / static_cast<double>(batches);
	}

	/**
	 * @brief Calculate the cross-NUMA node steal ratio
	 * @return Ratio of cross-node steals to total steals (0.0 to 1.0)
	 *
	 * Returns 0.0 if no successful steals have been made.
	 * Lower values indicate better NUMA locality.
	 */
	[[nodiscard]] auto cross_node_ratio() const -> double
	{
		auto same = same_node_steals.load(std::memory_order_relaxed);
		auto cross = cross_node_steals.load(std::memory_order_relaxed);
		auto total = same + cross;
		if (total == 0)
		{
			return 0.0;
		}
		return static_cast<double>(cross) / static_cast<double>(total);
	}

	/**
	 * @brief Calculate average steal operation time
	 * @return Average time per steal attempt in nanoseconds
	 *
	 * Returns 0.0 if no steal attempts have been made.
	 */
	[[nodiscard]] auto avg_steal_time_ns() const -> double
	{
		auto attempts = steal_attempts.load(std::memory_order_relaxed);
		if (attempts == 0)
		{
			return 0.0;
		}
		auto total_time = total_steal_time_ns.load(std::memory_order_relaxed);
		return static_cast<double>(total_time) / static_cast<double>(attempts);
	}

	/**
	 * @brief Reset all statistics to zero
	 *
	 * Thread Safety:
	 * This operation is not atomic across all counters. During reset,
	 * some counters may be zero while others retain old values.
	 * For accurate snapshots during operation, avoid calling reset()
	 * while steal operations are in progress.
	 */
	void reset()
	{
		steal_attempts.store(0, std::memory_order_relaxed);
		successful_steals.store(0, std::memory_order_relaxed);
		failed_steals.store(0, std::memory_order_relaxed);
		jobs_stolen.store(0, std::memory_order_relaxed);
		same_node_steals.store(0, std::memory_order_relaxed);
		cross_node_steals.store(0, std::memory_order_relaxed);
		batch_steals.store(0, std::memory_order_relaxed);
		total_batch_size.store(0, std::memory_order_relaxed);
		total_steal_time_ns.store(0, std::memory_order_relaxed);
		total_backoff_time_ns.store(0, std::memory_order_relaxed);
	}

	/**
	 * @brief Create a snapshot of current statistics
	 * @return A non-atomic copy of the current statistics
	 *
	 * This creates a non-atomic snapshot that can be safely read
	 * without worrying about concurrent updates.
	 */
	[[nodiscard]] auto snapshot() const -> work_stealing_stats_snapshot
	{
		work_stealing_stats_snapshot snap;
		snap.steal_attempts = steal_attempts.load(std::memory_order_acquire);
		snap.successful_steals = successful_steals.load(std::memory_order_acquire);
		snap.failed_steals = failed_steals.load(std::memory_order_acquire);
		snap.jobs_stolen = jobs_stolen.load(std::memory_order_acquire);
		snap.same_node_steals = same_node_steals.load(std::memory_order_acquire);
		snap.cross_node_steals = cross_node_steals.load(std::memory_order_acquire);
		snap.batch_steals = batch_steals.load(std::memory_order_acquire);
		snap.total_batch_size = total_batch_size.load(std::memory_order_acquire);
		snap.total_steal_time_ns = total_steal_time_ns.load(std::memory_order_acquire);
		snap.total_backoff_time_ns = total_backoff_time_ns.load(std::memory_order_acquire);
		return snap;
	}
};

} // namespace kcenon::thread
