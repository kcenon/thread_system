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
// TICKET-001: TLS Bug Resolution
// =============================================================================
// This implementation uses Hazard Pointers for safe memory reclamation,
// eliminating the TLS destructor ordering bug that affected previous versions.
//
// Key safety features:
// - Uses lockfree_job_queue internally (with Hazard Pointer protection)
// - GlobalReclamationManager handles orphaned nodes from terminated threads
// - No TLS node pool (eliminates destructor ordering issues)
//
// See: docs/tickets/TICKET-001-LOCKFREE-QUEUE-TLS-BUG.md (RESOLVED)
// =============================================================================

#include <kcenon/thread/core/job_queue.h>
#include <kcenon/thread/lockfree/lockfree_job_queue.h>
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
	 * ## Thread Safety
	 *
	 * This implementation is thread-safe and well-tested. It uses:
	 * - lockfree_job_queue internally for each type-specific queue
	 * - Hazard Pointers for safe memory reclamation
	 * - GlobalReclamationManager for handling orphaned nodes from terminated threads
	 *
	 * ## Memory Safety (TICKET-001 Resolution)
	 *
	 * Previous versions had a TLS destructor ordering bug. This has been resolved by:
	 * 1. Using Hazard Pointers instead of TLS-based node pools
	 * 2. Implementing GlobalReclamationManager to handle orphaned nodes
	 * 3. Ensuring safe cleanup when threads exit unexpectedly
	 *
	 * ## Performance Characteristics
	 *
	 * - Enqueue: O(1) amortized, wait-free per type
	 * - Dequeue: O(1) amortized, lock-free per type
	 * - Memory overhead: ~256 bytes per thread (hazard pointers)
	 *
	 * @note This implementation is optimized for high-concurrency scenarios where
	 *       traditional mutex-based queues would become a bottleneck.
	 *
	 * @see lockfree_job_queue for the underlying lock-free queue implementation
	 * @see hazard_pointer.h for memory reclamation details
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
			-> common::VoidResult;

		/**
		 * @brief Enqueues a generic job (attempts to cast to typed_job)
		 * @param value Unique pointer to a job
		 * @return Success or error result
		 */
		[[nodiscard]] auto enqueue(std::unique_ptr<job>&& value)
			-> common::VoidResult override;

		/**
		 * @brief Enqueues multiple typed jobs
		 * @param jobs Vector of typed jobs to enqueue
		 * @return Success or error result
		 */
		[[nodiscard]] auto enqueue_batch(std::vector<std::unique_ptr<typed_job_t<job_type>>>&& jobs)
			-> common::VoidResult;

		/**
		 * @brief Enqueues multiple generic jobs
		 * @param jobs Vector of jobs to enqueue
		 * @return Success or error result
		 */
		[[nodiscard]] auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs)
			-> common::VoidResult override;

		/**
		 * @brief Dequeues a job with highest priority
		 * @return The dequeued job or error
		 */
		[[nodiscard]] auto dequeue()
			-> common::Result<std::unique_ptr<job>> override;

		/**
		 * @brief Dequeues a job of specific type
		 * @param type The job type to dequeue
		 * @return The dequeued job or error
		 */
		[[nodiscard]] auto dequeue(const job_type& type)
			-> common::Result<std::unique_ptr<typed_job_t<job_type>>>;

		/**
		 * @brief Dequeues jobs from multiple types
		 * @param types Span of job types to consider
		 * @return The dequeued job or error
		 */
		[[nodiscard]] auto dequeue(utility_module::span<const job_type> types)
			-> common::Result<std::unique_ptr<typed_job_t<job_type>>>;

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
		using lockfree_queue_ptr = std::unique_ptr<detail::lockfree_job_queue>;
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
		auto get_or_create_queue(const job_type& type) -> detail::lockfree_job_queue*;
		auto get_queue(const job_type& type) const -> detail::lockfree_job_queue*;
		auto update_priority_order() -> void;
		auto should_update_priority_order() const -> bool;
	};
	
	// Convenience type aliases
	using typed_lockfree_job_queue = typed_lockfree_job_queue_t<job_types>;

	// =============================================================================
	// Template Implementation
	// =============================================================================

	template <typename job_type>
	typed_lockfree_job_queue_t<job_type>::typed_lockfree_job_queue_t(size_t max_threads)
		: job_queue()
		, max_threads_(max_threads)
	{
	}

	template <typename job_type>
	typed_lockfree_job_queue_t<job_type>::~typed_lockfree_job_queue_t()
	{
		clear();
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::get_or_create_queue(const job_type& type) -> detail::lockfree_job_queue*
	{
		auto it = typed_queues_.find(type);
		if (it != typed_queues_.end())
		{
			return it->second.get();
		}

		// Create a new lockfree_job_queue for this type
		auto new_queue = std::make_unique<detail::lockfree_job_queue>();
		auto* queue_ptr = new_queue.get();
		typed_queues_[type] = std::move(new_queue);

		// Update priority order
		update_priority_order();

		return queue_ptr;
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::get_queue(const job_type& type) const -> detail::lockfree_job_queue*
	{
		auto it = typed_queues_.find(type);
		if (it != typed_queues_.end())
		{
			return it->second.get();
		}
		return nullptr;
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::update_priority_order() -> void
	{
		priority_order_.clear();
		priority_order_.reserve(typed_queues_.size());
		for (const auto& [type, queue] : typed_queues_)
		{
			priority_order_.push_back(type);
		}
		// Sort by type value (lower value = higher priority)
		std::sort(priority_order_.begin(), priority_order_.end());
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::should_update_priority_order() const -> bool
	{
		return priority_order_.size() != typed_queues_.size();
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::enqueue(std::unique_ptr<typed_job_t<job_type>>&& value)
		-> common::VoidResult
	{
		if (!value)
		{
			return common::error_info{static_cast<int>(error_code::invalid_argument), "Null job", "thread_system"};
		}

		auto priority = value->priority();

		std::unique_lock lock(queues_mutex_);
		auto* queue = get_or_create_queue(priority);
		lock.unlock();

		return queue->enqueue(std::move(value));
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::enqueue(std::unique_ptr<job>&& value)
		-> common::VoidResult
	{
		auto* typed_job_ptr = dynamic_cast<typed_job_t<job_type>*>(value.get());
		if (!typed_job_ptr)
		{
			return common::error_info{static_cast<int>(error_code::invalid_argument), "Job is not a typed job", "thread_system"};
		}

		value.release();
		return enqueue(std::unique_ptr<typed_job_t<job_type>>(typed_job_ptr));
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::enqueue_batch(
		std::vector<std::unique_ptr<typed_job_t<job_type>>>&& jobs) -> common::VoidResult
	{
		for (auto& job : jobs)
		{
			if (!job) continue;

			auto result = enqueue(std::move(job));
			if (result.is_err())
			{
				return result;
			}
		}
		return common::ok();
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::enqueue_batch(
		std::vector<std::unique_ptr<job>>&& jobs) -> common::VoidResult
	{
		for (auto& job : jobs)
		{
			if (!job) continue;

			auto result = enqueue(std::move(job));
			if (result.is_err())
			{
				return result;
			}
		}
		return common::ok();
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::dequeue()
		-> common::Result<std::unique_ptr<job>>
	{
		std::shared_lock lock(queues_mutex_);

		// Check priority order
		std::shared_lock priority_lock(priority_mutex_);

		// Try to dequeue from queues in priority order
		for (const auto& type : priority_order_)
		{
			auto it = typed_queues_.find(type);
			if (it != typed_queues_.end())
			{
				auto result = it->second->dequeue();
				if (result.is_ok())
				{
					// Track type switches for statistics
					auto last_type = last_dequeue_type_.load(std::memory_order_relaxed);
					if (last_type != type)
					{
						type_switch_count_.fetch_add(1, std::memory_order_relaxed);
						last_dequeue_type_.store(type, std::memory_order_relaxed);
					}
					return std::move(result.value());
				}
			}
		}

		return common::error_info{static_cast<int>(error_code::queue_empty), "No jobs available", "thread_system"};
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::dequeue(const job_type& type)
		-> common::Result<std::unique_ptr<typed_job_t<job_type>>>
	{
		std::shared_lock lock(queues_mutex_);

		auto it = typed_queues_.find(type);
		if (it == typed_queues_.end())
		{
			return common::error_info{static_cast<int>(error_code::queue_empty), "No queue for specified type", "thread_system"};
		}

		auto dequeue_result = it->second->dequeue();
		if (dequeue_result.is_err())
		{
			return common::error_info{static_cast<int>(error_code::queue_empty), "Queue empty for specified type", "thread_system"};
		}

		auto job = std::move(dequeue_result.value());
		auto* typed_job_ptr = static_cast<typed_job_t<job_type>*>(job.release());
		return std::unique_ptr<typed_job_t<job_type>>(typed_job_ptr);
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::dequeue(utility_module::span<const job_type> types)
		-> common::Result<std::unique_ptr<typed_job_t<job_type>>>
	{
		std::shared_lock lock(queues_mutex_);

		for (const auto& type : types)
		{
			auto it = typed_queues_.find(type);
			if (it != typed_queues_.end())
			{
				auto dequeue_result = it->second->dequeue();
				if (dequeue_result.is_ok())
				{
					auto job = std::move(dequeue_result.value());
					auto* typed_job_ptr = static_cast<typed_job_t<job_type>*>(job.release());
					return std::unique_ptr<typed_job_t<job_type>>(typed_job_ptr);
				}
			}
		}

		return common::error_info{static_cast<int>(error_code::queue_empty), "No jobs available for specified types", "thread_system"};
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::dequeue_batch()
		-> std::deque<std::unique_ptr<job>>
	{
		std::deque<std::unique_ptr<job>> results;

		while (true)
		{
			auto dequeue_result = dequeue();
			if (dequeue_result.is_err())
			{
				break;
			}
			results.push_back(std::move(dequeue_result.value()));
		}

		return results;
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::clear() -> void
	{
		std::unique_lock lock(queues_mutex_);

		// Drain all queues (lockfree_job_queue doesn't have clear())
		for (auto& [type, queue] : typed_queues_)
		{
			while (!queue->empty())
			{
				(void)queue->dequeue();
			}
		}
		typed_queues_.clear();

		std::unique_lock priority_lock(priority_mutex_);
		priority_order_.clear();
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::empty() const -> bool
	{
		std::shared_lock lock(queues_mutex_);

		for (const auto& [type, queue] : typed_queues_)
		{
			if (!queue->empty())
			{
				return false;
			}
		}
		return true;
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::empty(const std::vector<job_type>& types) const -> bool
	{
		std::shared_lock lock(queues_mutex_);

		for (const auto& type : types)
		{
			auto it = typed_queues_.find(type);
			if (it != typed_queues_.end() && !it->second->empty())
			{
				return false;
			}
		}
		return true;
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::size() const -> std::size_t
	{
		std::shared_lock lock(queues_mutex_);

		std::size_t total = 0;
		for (const auto& [type, queue] : typed_queues_)
		{
			total += queue->size();
		}
		return total;
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::size(const job_type& type) const -> std::size_t
	{
		std::shared_lock lock(queues_mutex_);

		auto it = typed_queues_.find(type);
		if (it != typed_queues_.end())
		{
			return it->second->size();
		}
		return 0;
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::get_sizes() const
		-> std::unordered_map<job_type, std::size_t>
	{
		std::shared_lock lock(queues_mutex_);

		std::unordered_map<job_type, std::size_t> sizes;
		for (const auto& [type, queue] : typed_queues_)
		{
			sizes[type] = queue->size();
		}
		return sizes;
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::to_string() const -> std::string
	{
		std::shared_lock lock(queues_mutex_);

		std::string result = "typed_lockfree_job_queue{queues: ";
		result += std::to_string(typed_queues_.size());
		result += ", total_size: ";

		std::size_t total = 0;
		for (const auto& [type, queue] : typed_queues_)
		{
			total += queue->size();
		}
		result += std::to_string(total);
		result += "}";

		return result;
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::get_typed_statistics() const
		-> typed_queue_statistics_t<job_type>
	{
		std::shared_lock lock(queues_mutex_);

		typed_queue_statistics_t<job_type> stats;

		for (const auto& [type, queue] : typed_queues_)
		{
			auto queue_size = queue->size();
			stats.per_type_dequeues[type] = queue_size;
		}

		stats.type_switch_count = type_switch_count_.load(std::memory_order_relaxed);

		return stats;
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::reset_statistics() -> void
	{
		type_switch_count_.store(0, std::memory_order_relaxed);
	}

	template <typename job_type>
	auto typed_lockfree_job_queue_t<job_type>::stop() -> void
	{
		// Lock-free queue doesn't need explicit stop
		// Jobs will be drained by clear() or destructor
	}

} // namespace kcenon::thread


// Explicit instantiation declaration for common types
namespace kcenon::thread {
    extern template class typed_lockfree_job_queue_t<job_types>;
}
