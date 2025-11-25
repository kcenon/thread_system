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

// =============================================================================
// CRITICAL SECURITY WARNING - TICKET-001
// =============================================================================
// This lock-free queue implementation has a Thread-Local Storage (TLS)
// initialization order bug that causes:
//   - Use-After-Free (CWE-416, CVSS 9.8 Critical)
//   - Data corruption and segmentation faults during thread destruction
//   - Unpredictable crashes in production environments
//
// DO NOT USE THIS IN PRODUCTION CODE.
// Use typed_job_queue (mutex-based) or adaptive_typed_job_queue_t with
// FORCE_LEGACY strategy instead.
//
// To enable for testing/debugging ONLY, define TYPED_LOCKFREE_QUEUE_FORCE_ENABLE
// before including this header.
//
// See: docs/tickets/TICKET-001-LOCKFREE-QUEUE-TLS-BUG.md
// =============================================================================
#ifndef TYPED_LOCKFREE_QUEUE_FORCE_ENABLE
    #error "CRITICAL: typed_lockfree_job_queue has TLS initialization bug (TICKET-001, CVSS 9.8). " \
           "DO NOT USE IN PRODUCTION. Use typed_job_queue or adaptive_typed_job_queue_t with FORCE_LEGACY instead. " \
           "Define TYPED_LOCKFREE_QUEUE_FORCE_ENABLE only for testing/debugging."
#endif

#include <kcenon/thread/core/job_queue.h>
#include "typed_job.h"
#include "job_types.h"
#include <kcenon/thread/utils/formatter.h>
#include <kcenon/thread/utils/span.h>

#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include <vector>
#include <optional>
#include <algorithm>

namespace kcenon::thread
{
	/**
	 * @struct typed_queue_statistics
	 * @brief Statistics for typed lock-free job queue performance
	 */
	template<typename job_type = job_types>
	struct typed_queue_statistics_t
	{
		uint64_t total_enqueues{0};
		uint64_t total_dequeues{0};
		uint64_t type_switch_count{0};
		uint64_t enqueue_latency_ns{0};
		uint64_t dequeue_latency_ns{0};
		std::unordered_map<job_type, uint64_t> per_type_enqueues;
		std::unordered_map<job_type, uint64_t> per_type_dequeues;
		
		[[nodiscard]] auto get_average_enqueue_latency_ns() const -> uint64_t
		{
			return total_enqueues > 0 ? enqueue_latency_ns / total_enqueues : 0;
		}
		
		[[nodiscard]] auto get_average_dequeue_latency_ns() const -> uint64_t
		{
			return total_dequeues > 0 ? dequeue_latency_ns / total_dequeues : 0;
		}
		
		[[nodiscard]] auto get_busiest_type() const -> std::optional<job_type>
		{
			if (per_type_dequeues.empty()) return std::nullopt;
			
			auto it = std::max_element(per_type_dequeues.begin(), per_type_dequeues.end(),
				[](const auto& a, const auto& b) { return a.second < b.second; });
			
			return it->first;
		}
	};
	
	// Convenience alias
	using typed_queue_statistics = typed_queue_statistics_t<job_types>;
	/**
	 * @class typed_lockfree_job_queue_t
	 * @brief High-performance lock-free priority-based job queue
	 *
	 * This class provides a lock-free implementation of a typed job queue that
	 * manages jobs with distinct priority levels. It maintains separate lock-free
	 * queues for each job type/priority, ensuring thread-safe operations with
	 * minimal contention.
	 *
	 * @tparam job_type The type used to represent job priority levels. Defaults to job_types.
	 *
	 * @warning PRODUCTION UNSAFE - DO NOT USE IN PRODUCTION
	 *
	 * This implementation has a CRITICAL thread-local storage (TLS) destructor ordering bug
	 * that causes segmentation faults during shutdown. The issue occurs when:
	 * 1. TLS destructors run after the node pool is freed
	 * 2. TLS destructors attempt to return nodes to the already-freed pool
	 * 3. Access to freed memory causes segfault
	 *
	 * Symptoms:
	 * - Tests pass individually but fail when run in sequence
	 * - Random segfaults during application shutdown
	 * - Crashes in TLS destructor cleanup
	 *
	 * This bug is tracked in KNOWN_ISSUES.md. Use mutex-based job_queue instead
	 * until hazard pointers or epoch-based reclamation is implemented.
	 *
	 * @see KNOWN_ISSUES.md for detailed analysis and proposed solutions
	 * @see job_queue.h for production-safe alternative
	 *
	 * @note This implementation is optimized for high-concurrency scenarios where
	 *       traditional mutex-based queues would become a bottleneck.
	 *
	 * @warning ABA Problem Mitigation:
	 *          This lock-free implementation currently relies on the following strategies
	 *          to mitigate the ABA problem:
	 *          1. Node pooling: Reduces memory reuse frequency
	 *          2. Unique pointer semantics: Prevents direct pointer reuse
	 *          3. C++ memory model guarantees: std::atomic operations
	 *
	 *          Future enhancements may include:
	 *          - Hazard pointers (for complete ABA protection)
	 *          - Epoch-based reclamation
	 *          - Tagged pointers with version counters
	 *
	 *          For production use in extremely high-contention scenarios (>1M ops/s),
	 *          consider enabling ThreadSanitizer during testing to detect data races.
	 */
	template <typename job_type = job_types>
	class typed_lockfree_job_queue_t : public job_queue
	{
	public:
		/**
		 * @brief Constructs a typed lock-free job queue
		 * @param max_threads Maximum number of threads that will access the queue
		 */
		explicit typed_lockfree_job_queue_t(size_t max_threads = 128);

		/**
		 * @brief Destructor
		 */
		virtual ~typed_lockfree_job_queue_t();

		// Delete copy operations
		typed_lockfree_job_queue_t(const typed_lockfree_job_queue_t&) = delete;
		typed_lockfree_job_queue_t& operator=(const typed_lockfree_job_queue_t&) = delete;

		// Delete move operations for now
		typed_lockfree_job_queue_t(typed_lockfree_job_queue_t&&) = delete;
		typed_lockfree_job_queue_t& operator=(typed_lockfree_job_queue_t&&) = delete;

		/**
		 * @brief Enqueues a typed job with priority
		 * @param value Unique pointer to a typed job
		 * @return Success or error result
		 */
		[[nodiscard]] auto enqueue(std::unique_ptr<typed_job_t<job_type>>&& value)
			-> result_void;

		/**
		 * @brief Enqueues a generic job (attempts to cast to typed_job)
		 * @param value Unique pointer to a job
		 * @return Success or error result
		 */
		[[nodiscard]] auto enqueue(std::unique_ptr<job>&& value)
			-> result_void override;

		/**
		 * @brief Enqueues multiple typed jobs
		 * @param jobs Vector of typed jobs to enqueue
		 * @return Success or error result
		 */
		[[nodiscard]] auto enqueue_batch(std::vector<std::unique_ptr<typed_job_t<job_type>>>&& jobs)
			-> result_void;

		/**
		 * @brief Enqueues multiple generic jobs
		 * @param jobs Vector of jobs to enqueue
		 * @return Success or error result
		 */
		[[nodiscard]] auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs)
			-> result_void override;

		/**
		 * @brief Dequeues a job with highest priority
		 * @return The dequeued job or error
		 */
		[[nodiscard]] auto dequeue()
			-> result<std::unique_ptr<job>> override;

		/**
		 * @brief Dequeues a job of specific type
		 * @param type The job type to dequeue
		 * @return The dequeued job or error
		 */
		[[nodiscard]] auto dequeue(const job_type& type)
			-> result<std::unique_ptr<typed_job_t<job_type>>>;

		/**
		 * @brief Dequeues jobs from multiple types
		 * @param types Span of job types to consider
		 * @return The dequeued job or error
		 */
		[[nodiscard]] auto dequeue(utility_module::span<const job_type> types)
			-> result<std::unique_ptr<typed_job_t<job_type>>>;

		/**
		 * @brief Dequeues all available jobs
		 * @return Deque containing all dequeued jobs
		 */
		[[nodiscard]] auto dequeue_batch()
			-> std::deque<std::unique_ptr<job>> override;
		
		/**
		 * @brief Clears all jobs from all priority queues
		 */
		auto clear() -> void override;
		
		/**
		 * @brief Checks if all queues are empty
		 * @return true if empty, false otherwise
		 */
		[[nodiscard]] auto empty() const -> bool;
		
		/**
		 * @brief Checks if specific types are empty
		 * @param types Vector of job types to check
		 * @return True if all specified types have no jobs
		 */
		[[nodiscard]] auto empty(const std::vector<job_type>& types) const -> bool;
		
		/**
		 * @brief Gets the total number of jobs across all queues
		 * @return Total number of jobs
		 */
		[[nodiscard]] auto size() const -> std::size_t;
		
		/**
		 * @brief Gets the number of jobs for a specific type
		 * @param type The job type to query
		 * @return Number of jobs of that type
		 */
		[[nodiscard]] auto size(const job_type& type) const -> std::size_t;
		
		/**
		 * @brief Gets sizes for all job types
		 * @return Map of job types to their queue sizes
		 */
		[[nodiscard]] auto get_sizes() const -> std::unordered_map<job_type, std::size_t>;
		
		/**
		 * @brief Gets string representation of the queue
		 * @return String describing the queue state
		 */
		[[nodiscard]] auto to_string() const -> std::string override;
		
		/**
		 * @brief Extended statistics for typed queues
		 */
		struct typed_queue_statistics
		{
			std::size_t total_enqueued = 0;
			std::size_t total_dequeued = 0;
			std::size_t current_size = 0;
			std::unordered_map<job_type, std::size_t> per_type_stats;
			uint64_t type_switch_count{0};
			
			[[nodiscard]] auto get_busiest_type() const -> std::optional<job_type>
			{
				if (per_type_stats.empty()) return std::nullopt;

				auto it = std::max_element(per_type_stats.begin(), per_type_stats.end(),
					[](const auto& a, const auto& b) {
						return a.second < b.second;
					});

				return it->first;
			}
		};
		
		/**
		 * @brief Get detailed statistics including per-type metrics
		 * @return Extended statistics
		 */
		[[nodiscard]] auto get_typed_statistics() const -> typed_queue_statistics_t<job_type>;
		
		/**
		 * @brief Reset all statistics
		 */
		auto reset_statistics() -> void;
		
		/**
		 * @brief Stop the queue (for interface compatibility)
		 */
		auto stop() -> void;
		
	private:
		// Type aliases
		using job_queue_ptr = std::unique_ptr<job_queue>;
		using lockfree_queue_ptr = std::unique_ptr<job_queue>;
		using queue_map = std::unordered_map<job_type, lockfree_queue_ptr>;
		
		// Per-type queues
		mutable std::shared_mutex queues_mutex_;
		queue_map typed_queues_;
		
		// Priority order (cached for performance)
		mutable std::shared_mutex priority_mutex_;
		std::vector<job_type> priority_order_;
		
		// Configuration
		size_t max_threads_;
		
		// Statistics
		mutable std::atomic<uint64_t> type_switch_count_{0};
		mutable std::atomic<job_type> last_dequeue_type_{};
		
		// Helper methods
		auto get_or_create_queue(const job_type& type) -> job_queue*;
		auto get_queue(const job_type& type) const -> job_queue*;
		auto update_priority_order() -> void;
		auto should_update_priority_order() const -> bool;
	};
	
	// Convenience type aliases
	using typed_lockfree_job_queue = typed_lockfree_job_queue_t<job_types>;
	
} // namespace kcenon::thread


// Explicit instantiation for common types
namespace kcenon::thread {
    extern template class typed_lockfree_job_queue_t<job_types>;
}
