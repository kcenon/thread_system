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

#include "aging_typed_job.h"
#include "priority_aging_config.h"
#include "typed_job.h"
#include "job_types.h"
#include <kcenon/thread/policies/policy_queue.h>
#include <kcenon/thread/policies/sync_policies.h>
#include <kcenon/thread/policies/bound_policies.h>
#include <kcenon/thread/utils/span.h>

#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <unordered_map>
#include <optional>
#include <sstream>

namespace kcenon::thread
{
	/**
	 * @struct aging_stats
	 * @brief Statistics about priority aging behavior.
	 */
	struct aging_stats
	{
		std::size_t total_boosts_applied{0};
		std::size_t jobs_reached_max_boost{0};
		std::size_t starvation_alerts{0};
		std::chrono::milliseconds max_wait_time{0};
		std::chrono::milliseconds avg_wait_time{0};
		double boost_rate{0.0};  // Boosts per second
	};

	/**
	 * @class aging_typed_job_queue_t
	 * @brief A typed job queue with priority aging support, based on policy_queue.
	 *
	 * This class provides priority aging functionality using policy_queue internally
	 * instead of inheriting from typed_job_queue_t. It maintains API compatibility
	 * while leveraging the modern policy-based queue design.
	 *
	 * @tparam job_type The type used to represent priority levels.
	 *
	 * ### Features
	 * - Background aging thread for periodic priority updates
	 * - Configurable aging curves (linear, exponential, logarithmic)
	 * - Starvation detection and alerting
	 * - Statistics tracking
	 * - Uses policy_queue internally for efficient queue operations
	 *
	 * ### Thread Safety
	 * All public methods are thread-safe.
	 *
	 * ### Example Usage
	 * @code{.cpp}
	 * priority_aging_config config{
	 *     .enabled = true,
	 *     .aging_interval = std::chrono::seconds{1},
	 *     .max_priority_boost = 3
	 * };
	 *
	 * auto queue = std::make_shared<aging_typed_job_queue_t<job_types>>(config);
	 * queue->start_aging();
	 *
	 * // Enqueue jobs...
	 * queue->enqueue(std::make_unique<aging_typed_job_t<job_types>>(...));
	 *
	 * // When done
	 * queue->stop_aging();
	 * @endcode
	 */
	template <typename job_type = job_types>
	class aging_typed_job_queue_t
	{
	public:
		using queue_type = policy_queue<policies::mutex_sync_policy,
		                                policies::unbounded_policy,
		                                policies::overflow_reject_policy>;

		/**
		 * @brief Constructs an aging typed job queue.
		 *
		 * @param config The aging configuration.
		 */
		explicit aging_typed_job_queue_t(priority_aging_config config = {});

		/**
		 * @brief Destroys the aging typed job queue.
		 *
		 * Stops the aging thread if running.
		 */
		~aging_typed_job_queue_t();

		// Non-copyable
		aging_typed_job_queue_t(const aging_typed_job_queue_t&) = delete;
		auto operator=(const aging_typed_job_queue_t&) -> aging_typed_job_queue_t& = delete;

		// Non-movable (due to thread)
		aging_typed_job_queue_t(aging_typed_job_queue_t&&) = delete;
		auto operator=(aging_typed_job_queue_t&&) -> aging_typed_job_queue_t& = delete;

		// ============================================
		// Aging Control
		// ============================================

		/**
		 * @brief Starts the background aging thread.
		 *
		 * The aging thread periodically applies priority boosts to waiting jobs.
		 */
		auto start_aging() -> void;

		/**
		 * @brief Stops the background aging thread.
		 */
		auto stop_aging() -> void;

		/**
		 * @brief Checks if aging is currently running.
		 *
		 * @return true if the aging thread is active.
		 */
		[[nodiscard]] auto is_aging_running() const -> bool;

		/**
		 * @brief Manually applies aging to all queued jobs.
		 *
		 * This is useful for testing or when you want to trigger
		 * aging on demand without waiting for the background thread.
		 *
		 * @return The number of jobs that received priority boosts.
		 */
		auto apply_aging() -> std::size_t;

		// ============================================
		// Starvation Detection
		// ============================================

		/**
		 * @brief Gets jobs that are approaching starvation.
		 *
		 * @return A vector of job_info for jobs waiting longer than the threshold.
		 */
		[[nodiscard]] auto get_starving_jobs() const -> std::vector<job_info>;

		// ============================================
		// Statistics
		// ============================================

		/**
		 * @brief Gets aging statistics.
		 *
		 * @return The current aging statistics.
		 */
		[[nodiscard]] auto get_aging_stats() const -> aging_stats;

		/**
		 * @brief Resets the aging statistics.
		 */
		auto reset_aging_stats() -> void;

		// ============================================
		// Configuration
		// ============================================

		/**
		 * @brief Sets the aging configuration.
		 *
		 * @param config The new configuration.
		 */
		auto set_aging_config(priority_aging_config config) -> void;

		/**
		 * @brief Gets the current aging configuration.
		 *
		 * @return A const reference to the configuration.
		 */
		[[nodiscard]] auto get_aging_config() const -> const priority_aging_config&;

		// ============================================
		// Job Tracking
		// ============================================

		/**
		 * @brief Enqueues an aging typed job.
		 *
		 * @param value The job to enqueue.
		 * @return Result indicating success or failure.
		 */
		[[nodiscard]] auto enqueue(std::unique_ptr<aging_typed_job_t<job_type>>&& value)
			-> common::VoidResult;

		/**
		 * @brief Registers an aging job for tracking.
		 *
		 * @param job Pointer to the job to track.
		 */
		auto register_aging_job(aging_typed_job_t<job_type>* job) -> void;

		/**
		 * @brief Unregisters an aging job from tracking.
		 *
		 * @param job Pointer to the job to unregister.
		 */
		auto unregister_aging_job(aging_typed_job_t<job_type>* job) -> void;

		// ============================================
		// typed_job_queue_t compatible API
		// ============================================

		/**
		 * @brief Enqueues a base job into the queue.
		 *
		 * @param value A unique pointer to the base job to enqueue.
		 * @return Result indicating success or failure.
		 */
		[[nodiscard]] auto enqueue(std::unique_ptr<job>&& value) -> common::VoidResult;

		/**
		 * @brief Enqueues a typed job into the appropriate priority queue.
		 *
		 * @param value A unique pointer to the typed job to enqueue.
		 * @return Result indicating success or failure.
		 */
		[[nodiscard]] auto enqueue(std::unique_ptr<typed_job_t<job_type>>&& value)
			-> common::VoidResult;

		/**
		 * @brief Enqueues a derived typed job.
		 *
		 * @tparam DerivedJob A type that derives from typed_job_t<job_type>
		 * @param value A unique pointer to the derived job to enqueue.
		 * @return Result indicating success or failure.
		 */
		template<typename DerivedJob, typename = std::enable_if_t<std::is_base_of_v<typed_job_t<job_type>, DerivedJob>>>
		[[nodiscard]] auto enqueue(std::unique_ptr<DerivedJob>&& value) -> common::VoidResult
		{
			return enqueue(std::unique_ptr<typed_job_t<job_type>>(std::move(value)));
		}

		/**
		 * @brief Enqueues a batch of jobs.
		 *
		 * @param jobs A vector of unique pointers to jobs to enqueue.
		 * @return Result indicating success or failure.
		 */
		[[nodiscard]] auto enqueue_batch(std::vector<std::unique_ptr<typed_job_t<job_type>>>&& jobs)
			-> common::VoidResult;

		/**
		 * @brief Dequeues the next available job.
		 *
		 * @return Result containing the dequeued job or error.
		 */
		[[nodiscard]] auto dequeue() -> common::Result<std::unique_ptr<job>>;

		/**
		 * @brief Dequeues a job with one of the specified types.
		 *
		 * @param types A list of priority levels from which to attempt a dequeue.
		 * @return Result containing the dequeued job or error.
		 */
		[[nodiscard]] auto dequeue(const std::vector<job_type>& types)
			-> common::Result<std::unique_ptr<typed_job_t<job_type>>>;

		/**
		 * @brief Dequeues a job with one of the specified types using span.
		 *
		 * @param types A span of priority levels from which to attempt a dequeue.
		 * @return Result containing the dequeued job or error.
		 */
		[[nodiscard]] auto dequeue(utility_module::span<const job_type> types)
			-> common::Result<std::unique_ptr<typed_job_t<job_type>>>;

		/**
		 * @brief Removes all jobs from all priority queues.
		 */
		auto clear() -> void;

		/**
		 * @brief Checks if there are no jobs in any of the specified priority queues.
		 *
		 * @param types A list of priority levels to check.
		 * @return true if all specified priority queues are empty.
		 */
		[[nodiscard]] auto empty(const std::vector<job_type>& types) const -> bool;

		/**
		 * @brief Checks if there are no jobs in any of the specified priority queues using span.
		 *
		 * @param types A span of priority levels to check.
		 * @return true if all specified priority queues are empty.
		 */
		[[nodiscard]] auto empty(utility_module::span<const job_type> types) const -> bool;

		/**
		 * @brief Returns a string representation of the queue.
		 *
		 * @return A string describing the current state of the queue.
		 */
		[[nodiscard]] auto to_string() const -> std::string;

		/**
		 * @brief Stops accepting new jobs and marks the queue as stopped.
		 */
		auto stop() -> void;

		/**
		 * @brief Gets the total number of jobs in all queues.
		 *
		 * @return The total number of jobs.
		 */
		[[nodiscard]] auto size() const -> std::size_t;

	private:
		priority_aging_config config_;
		std::unique_ptr<std::thread> aging_thread_;
		std::atomic<bool> aging_running_{false};
		std::atomic<bool> stopped_{false};
		std::condition_variable aging_cv_;
		mutable std::mutex aging_mutex_;

		// Type-based job storage using policy_queue for each type
		std::unordered_map<job_type, std::unique_ptr<queue_type>> job_queues_;
		mutable std::shared_mutex queues_mutex_;

		// Tracked aging jobs
		std::vector<aging_typed_job_t<job_type>*> aging_jobs_;
		mutable std::mutex jobs_mutex_;

		// Statistics
		aging_stats stats_;
		mutable std::mutex stats_mutex_;
		std::chrono::steady_clock::time_point stats_start_time_;

		/**
		 * @brief The main aging loop running in the background thread.
		 */
		auto aging_loop() -> void;

		/**
		 * @brief Calculates the priority boost for a given wait time.
		 *
		 * @param wait_time The time the job has been waiting.
		 * @return The calculated boost amount.
		 */
		[[nodiscard]] auto calculate_boost(std::chrono::milliseconds wait_time) const -> int;

		/**
		 * @brief Checks for starving jobs and invokes callbacks.
		 */
		auto check_starvation() -> void;

		/**
		 * @brief Updates statistics after applying aging.
		 *
		 * @param boosts_applied Number of boosts applied in this cycle.
		 * @param max_wait Maximum wait time observed.
		 * @param total_wait Total wait time for average calculation.
		 * @param job_count Number of jobs processed.
		 */
		auto update_stats(std::size_t boosts_applied,
		                  std::chrono::milliseconds max_wait,
		                  std::chrono::milliseconds total_wait,
		                  std::size_t job_count) -> void;

		/**
		 * @brief Get or create a queue for the specified type.
		 *
		 * @param type The job type.
		 * @return Pointer to the queue.
		 */
		auto get_or_create_queue(const job_type& type) -> queue_type*;

		/**
		 * @brief Attempts to dequeue a single job from the queue for a given priority.
		 *
		 * @param priority The priority level from which to dequeue.
		 * @return The dequeued job, or std::nullopt if empty.
		 */
		[[nodiscard]] auto try_dequeue_from_priority(const job_type& priority)
			-> std::optional<std::unique_ptr<typed_job_t<job_type>>>;
	};

	/**
	 * @typedef aging_typed_job_queue
	 * @brief A convenient alias for aging_typed_job_queue_t using the job_types type.
	 */
	using aging_typed_job_queue = aging_typed_job_queue_t<job_types>;

} // namespace kcenon::thread
