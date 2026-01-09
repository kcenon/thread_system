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
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace kcenon::thread
{

/**
 * @class work_affinity_tracker
 * @brief Tracks cooperation patterns between workers for locality-aware stealing
 *
 * This class maintains a cooperation matrix that records successful work-stealing
 * interactions between worker threads. Workers that frequently exchange work
 * develop higher affinity scores, making them preferred victims for future steals.
 *
 * ### Design Rationale
 * When workers frequently cooperate (steal from each other successfully), they
 * likely share related work that benefits from cache locality. By preferring
 * to steal from high-affinity workers, we improve cache utilization and reduce
 * memory access latency.
 *
 * ### Thread Safety
 * All methods are thread-safe. The cooperation matrix uses atomic operations
 * for lock-free updates and reads.
 *
 * ### Memory Model
 * - `record_cooperation()`: Uses relaxed ordering (eventual consistency is acceptable)
 * - `get_affinity()`: Uses relaxed ordering (snapshot view)
 * - `get_preferred_victims()`: Uses acquire ordering for consistent ranking
 *
 * ### Usage Example
 * @code
 * // Create tracker for 8 workers with history size of 16
 * work_affinity_tracker tracker(8, 16);
 *
 * // Record successful steal: worker 2 stole from worker 5
 * tracker.record_cooperation(2, 5);
 *
 * // Get preferred victims for worker 2
 * auto victims = tracker.get_preferred_victims(2, 3);
 * // Returns up to 3 workers sorted by affinity with worker 2
 *
 * // Check specific affinity
 * double affinity = tracker.get_affinity(2, 5);
 * // Higher values indicate stronger cooperation history
 * @endcode
 *
 * ### Decay Mechanism
 * The tracker implements a simple decay mechanism: new interactions are weighted
 * more heavily than older ones. This ensures the affinity scores reflect recent
 * behavior rather than historical patterns that may no longer be relevant.
 */
class work_affinity_tracker
{
public:
	/**
	 * @brief Construct a work affinity tracker
	 * @param worker_count Number of workers to track
	 * @param history_size Size of history to consider for affinity calculations
	 *
	 * @note The history_size affects memory usage: O(worker_count^2 * history_size)
	 */
	explicit work_affinity_tracker(std::size_t worker_count,
	                               std::size_t history_size = 16);

	/**
	 * @brief Default constructor - creates an empty tracker
	 */
	work_affinity_tracker() = default;

	/**
	 * @brief Move constructor
	 */
	work_affinity_tracker(work_affinity_tracker&& other) noexcept;

	/**
	 * @brief Move assignment operator
	 */
	auto operator=(work_affinity_tracker&& other) noexcept -> work_affinity_tracker&;

	// Deleted copy operations (contains atomics)
	work_affinity_tracker(const work_affinity_tracker&) = delete;
	auto operator=(const work_affinity_tracker&) -> work_affinity_tracker& = delete;

	/**
	 * @brief Destructor
	 */
	~work_affinity_tracker() = default;

	/**
	 * @brief Record a cooperation event between two workers
	 * @param thief_id Worker that stole work (the one receiving work)
	 * @param victim_id Worker that provided work (the one being stolen from)
	 *
	 * This method is typically called after a successful steal operation.
	 * It updates the cooperation matrix to reflect the interaction.
	 *
	 * @note Thread-safe, can be called concurrently from multiple workers
	 */
	void record_cooperation(std::size_t thief_id, std::size_t victim_id);

	/**
	 * @brief Get the affinity score between two workers
	 * @param worker_a First worker ID
	 * @param worker_b Second worker ID
	 * @return Affinity score (0.0 = no cooperation, higher = more cooperation)
	 *
	 * The affinity is symmetric: get_affinity(a, b) == get_affinity(b, a)
	 *
	 * The returned score is normalized based on the history size, making it
	 * comparable across different tracker configurations.
	 */
	[[nodiscard]] auto get_affinity(std::size_t worker_a,
	                                std::size_t worker_b) const -> double;

	/**
	 * @brief Get preferred victims for a worker, sorted by affinity
	 * @param worker_id The worker seeking victims
	 * @param max_count Maximum number of victims to return
	 * @return Vector of worker IDs sorted by descending affinity
	 *
	 * Returns workers with the highest affinity scores, excluding the
	 * requesting worker itself. Workers with zero affinity may be included
	 * if there aren't enough high-affinity workers.
	 */
	[[nodiscard]] auto get_preferred_victims(std::size_t worker_id,
	                                         std::size_t max_count) const
		-> std::vector<std::size_t>;

	/**
	 * @brief Reset all affinity data
	 *
	 * Clears all cooperation history, resetting all affinities to zero.
	 * Useful when worker roles change significantly.
	 */
	void reset();

	/**
	 * @brief Get the number of workers being tracked
	 * @return Worker count
	 */
	[[nodiscard]] auto worker_count() const -> std::size_t;

	/**
	 * @brief Get the configured history size
	 * @return History size
	 */
	[[nodiscard]] auto history_size() const -> std::size_t;

	/**
	 * @brief Get total cooperation events recorded
	 * @return Total cooperation count
	 */
	[[nodiscard]] auto total_cooperations() const -> std::uint64_t;

private:
	/**
	 * @brief Get the matrix index for a worker pair
	 * @note Uses upper triangular matrix indexing (worker_a < worker_b)
	 */
	[[nodiscard]] auto get_matrix_index(std::size_t worker_a,
	                                    std::size_t worker_b) const -> std::size_t;

	/**
	 * @brief Normalize worker IDs to ensure a < b
	 */
	static auto normalize_pair(std::size_t a, std::size_t b)
		-> std::pair<std::size_t, std::size_t>;

	std::size_t worker_count_{0};
	std::size_t history_size_{16};
	std::size_t matrix_size_{0};

	// Cooperation matrix stored as flattened upper triangular matrix
	// Entry (i,j) where i < j is at index: i * worker_count - i*(i+1)/2 + j - i - 1
	std::unique_ptr<std::atomic<std::uint64_t>[]> cooperation_matrix_;

	// Total cooperation events for normalization
	std::atomic<std::uint64_t> total_cooperations_{0};
};

} // namespace kcenon::thread
