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

#include "typed_job_queue.h"

// Lock-free queue is disabled by default due to TLS bug (TICKET-001).
// Only include when explicitly enabled for testing/debugging.
#ifdef TYPED_LOCKFREE_QUEUE_FORCE_ENABLE
#include "typed_lockfree_job_queue.h"
#endif

#include <atomic>
#include <chrono>
#include <thread>

namespace kcenon::thread
{
	/**
	 * @class adaptive_typed_job_queue_t
	 * @brief Adaptive priority queue that switches between mutex-based and lock-free implementations
	 * 
	 * This queue monitors performance metrics and automatically switches between
	 * a traditional mutex-based typed queue and a lock-free typed queue based on
	 * contention levels and performance characteristics.
	 */
	template <typename job_type = job_types>
	class adaptive_typed_job_queue_t : public typed_job_queue_t<job_type>
	{
	public:
		/**
		 * @brief Queue implementation strategy
		 */
		enum class queue_strategy
		{
			AUTO_DETECT,      ///< Automatically detect best strategy
			FORCE_LEGACY,     ///< Always use mutex-based queue
			FORCE_LOCKFREE,   ///< Always use lock-free queue
			ADAPTIVE          ///< Switch based on runtime metrics
		};
		
		/**
		 * @brief Constructor
		 * @param initial_strategy Initial queue strategy
		 * @note Default changed to FORCE_LEGACY due to lock-free queue TLS bug.
		 *       See KNOWN_ISSUES.md for details. Only use FORCE_LOCKFREE or
		 *       AUTO_DETECT in testing environments until hazard pointers are implemented.
		 */
		explicit adaptive_typed_job_queue_t(queue_strategy initial_strategy = queue_strategy::FORCE_LEGACY);
		
		/**
		 * @brief Destructor
		 */
		virtual ~adaptive_typed_job_queue_t();
		
		// typed_job_queue_t interface implementation
		[[nodiscard]] auto enqueue(std::unique_ptr<job>&& value) -> common::VoidResult override;
		[[nodiscard]] auto enqueue(std::unique_ptr<typed_job_t<job_type>>&& value) -> common::VoidResult;
		[[nodiscard]] auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> common::VoidResult override;
		[[nodiscard]] auto dequeue() -> common::Result<std::unique_ptr<job>> override;
		[[nodiscard]] auto dequeue_batch() -> std::deque<std::unique_ptr<job>> override;
		[[nodiscard]] auto dequeue(const std::vector<job_type>& types) -> common::Result<std::unique_ptr<typed_job_t<job_type>>>;
		auto clear() -> void override;
		[[nodiscard]] auto empty(const std::vector<job_type>& types) const -> bool;
		[[nodiscard]] auto size(const std::vector<job_type>& types) const -> std::size_t;
		[[nodiscard]] auto to_string() const -> std::string override;
		
		/**
		 * @brief Force evaluation and potential switch of queue implementation
		 */
		auto evaluate_and_switch() -> void;
		
		/**
		 * @brief Get current queue type
		 * @return Current queue implementation type
		 */
		[[nodiscard]] auto get_current_type() const -> std::string;
		
		/**
		 * @brief Performance metrics structure
		 */
		struct performance_metrics
		{
			uint64_t operation_count{0};
			uint64_t total_latency_ns{0};
			uint64_t contention_count{0};
			uint64_t switch_count{0};
			std::chrono::steady_clock::time_point last_evaluation;
			
			[[nodiscard]] auto get_average_latency_ns() const -> double
			{
				if (operation_count == 0) return 0.0;
				return static_cast<double>(total_latency_ns) / static_cast<double>(operation_count);
			}
			
			[[nodiscard]] auto get_contention_ratio() const -> double
			{
				if (operation_count == 0) return 0.0;
				return static_cast<double>(contention_count) / static_cast<double>(operation_count);
			}
		};
		
		/**
		 * @brief Get performance metrics
		 * @return Current performance metrics
		 */
		[[nodiscard]] auto get_metrics() const -> performance_metrics;
		
	private:
		enum class queue_type
		{
			LEGACY_MUTEX,
			LOCKFREE,
			HYBRID
		};

		// Queue implementations
		std::unique_ptr<typed_job_queue_t<job_type>> legacy_queue_;
#ifdef TYPED_LOCKFREE_QUEUE_FORCE_ENABLE
		std::unique_ptr<typed_lockfree_job_queue_t<job_type>> lockfree_queue_;
#endif
		
		// Current state
		std::atomic<queue_type> current_type_;
		queue_strategy strategy_;
		
		// Performance monitoring
		mutable struct {
			std::atomic<uint64_t> operation_count{0};
			std::atomic<uint64_t> total_latency_ns{0};
			std::atomic<uint64_t> contention_count{0};
			std::atomic<uint64_t> switch_count{0};
			std::chrono::steady_clock::time_point last_evaluation;
		} metrics_;
		
		std::unique_ptr<std::thread> monitor_thread_;
		std::atomic<bool> stop_monitor_{false};
		
		// Configuration
		static constexpr auto EVALUATION_INTERVAL = std::chrono::seconds(5);
		static constexpr double CONTENTION_THRESHOLD_HIGH = 0.1;
		static constexpr double CONTENTION_THRESHOLD_LOW = 0.05;
		static constexpr double LATENCY_THRESHOLD_HIGH_NS = 1000.0;
		static constexpr double LATENCY_THRESHOLD_LOW_NS = 500.0;
		static constexpr size_t MIN_OPERATIONS_FOR_SWITCH = 1000;
		
		// Internal methods
		auto initialize_strategy() -> void;
		auto start_performance_monitor() -> void;
		auto stop_performance_monitor() -> void;
		auto monitor_performance() -> void;
		auto should_switch_to_lockfree() const -> bool;
		auto should_switch_to_legacy() const -> bool;
		auto migrate_to_lockfree() -> void;
		auto migrate_to_legacy() -> void;
		auto update_metrics(std::chrono::nanoseconds duration, bool had_contention = false) -> void;
		
		// Helper to get current implementation
		[[nodiscard]] auto get_current_impl() -> typed_job_queue_t<job_type>*;
		[[nodiscard]] auto get_current_impl() const -> const typed_job_queue_t<job_type>*;
	};
	
	/**
	 * @brief Factory function to create appropriate typed job queue
	 * @param strategy Queue selection strategy
	 * @param max_threads Maximum number of threads (for lockfree queue)
	 * @return Shared pointer to created queue
	 */
	template <typename job_type = job_types>
	[[nodiscard]] auto create_typed_job_queue(
		typename adaptive_typed_job_queue_t<job_type>::queue_strategy strategy = 
			adaptive_typed_job_queue_t<job_type>::queue_strategy::AUTO_DETECT,
		size_t max_threads = 128
	) -> std::shared_ptr<typed_job_queue_t<job_type>>;

	// =============================================================================
	// Template Implementation
	// =============================================================================

	template <typename job_type>
	adaptive_typed_job_queue_t<job_type>::adaptive_typed_job_queue_t(queue_strategy initial_strategy)
		: typed_job_queue_t<job_type>()
		, legacy_queue_(std::make_unique<typed_job_queue_t<job_type>>())
		, current_type_(queue_type::LEGACY_MUTEX)
		, strategy_(initial_strategy)
	{
		metrics_.last_evaluation = std::chrono::steady_clock::now();
		initialize_strategy();
	}

	template <typename job_type>
	adaptive_typed_job_queue_t<job_type>::~adaptive_typed_job_queue_t()
	{
		stop_performance_monitor();
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::initialize_strategy() -> void
	{
		switch (strategy_)
		{
		case queue_strategy::FORCE_LOCKFREE:
#ifdef TYPED_LOCKFREE_QUEUE_FORCE_ENABLE
			lockfree_queue_ = std::make_unique<typed_lockfree_job_queue_t<job_type>>();
			current_type_.store(queue_type::LOCKFREE, std::memory_order_release);
#else
			// Fall back to legacy if lock-free is not enabled
			current_type_.store(queue_type::LEGACY_MUTEX, std::memory_order_release);
#endif
			break;

		case queue_strategy::ADAPTIVE:
			// Start with legacy, may switch based on metrics
			current_type_.store(queue_type::LEGACY_MUTEX, std::memory_order_release);
			start_performance_monitor();
			break;

		case queue_strategy::AUTO_DETECT:
		case queue_strategy::FORCE_LEGACY:
		default:
			current_type_.store(queue_type::LEGACY_MUTEX, std::memory_order_release);
			break;
		}
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::get_current_impl() -> typed_job_queue_t<job_type>*
	{
#ifdef TYPED_LOCKFREE_QUEUE_FORCE_ENABLE
		if (current_type_.load(std::memory_order_acquire) == queue_type::LOCKFREE)
		{
			return lockfree_queue_.get();
		}
#endif
		return legacy_queue_.get();
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::get_current_impl() const -> const typed_job_queue_t<job_type>*
	{
#ifdef TYPED_LOCKFREE_QUEUE_FORCE_ENABLE
		if (current_type_.load(std::memory_order_acquire) == queue_type::LOCKFREE)
		{
			return lockfree_queue_.get();
		}
#endif
		return legacy_queue_.get();
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::enqueue(std::unique_ptr<job>&& value) -> common::VoidResult
	{
		auto start = std::chrono::steady_clock::now();
		auto result = get_current_impl()->enqueue(std::move(value));
		auto duration = std::chrono::steady_clock::now() - start;
		update_metrics(std::chrono::duration_cast<std::chrono::nanoseconds>(duration), false);
		return result;
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::enqueue(std::unique_ptr<typed_job_t<job_type>>&& value) -> common::VoidResult
	{
		auto start = std::chrono::steady_clock::now();
		auto result = get_current_impl()->enqueue(std::move(value));
		auto duration = std::chrono::steady_clock::now() - start;
		update_metrics(std::chrono::duration_cast<std::chrono::nanoseconds>(duration), false);
		return result;
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> common::VoidResult
	{
		auto start = std::chrono::steady_clock::now();
		auto result = get_current_impl()->enqueue_batch(std::move(jobs));
		auto duration = std::chrono::steady_clock::now() - start;
		update_metrics(std::chrono::duration_cast<std::chrono::nanoseconds>(duration), false);
		return result;
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::dequeue() -> common::Result<std::unique_ptr<job>>
	{
		auto start = std::chrono::steady_clock::now();
		auto result = get_current_impl()->dequeue();
		auto duration = std::chrono::steady_clock::now() - start;
		update_metrics(std::chrono::duration_cast<std::chrono::nanoseconds>(duration), false);
		return result;
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::dequeue_batch() -> std::deque<std::unique_ptr<job>>
	{
		auto start = std::chrono::steady_clock::now();
		auto result = get_current_impl()->dequeue_batch();
		auto duration = std::chrono::steady_clock::now() - start;
		update_metrics(std::chrono::duration_cast<std::chrono::nanoseconds>(duration), false);
		return result;
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::dequeue(const std::vector<job_type>& types)
		-> common::Result<std::unique_ptr<typed_job_t<job_type>>>
	{
		auto start = std::chrono::steady_clock::now();
		auto result = get_current_impl()->dequeue(types);
		auto duration = std::chrono::steady_clock::now() - start;
		update_metrics(std::chrono::duration_cast<std::chrono::nanoseconds>(duration), false);
		return result;
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::clear() -> void
	{
		get_current_impl()->clear();
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::empty(const std::vector<job_type>& types) const -> bool
	{
		return get_current_impl()->empty(types);
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::size(const std::vector<job_type>& types) const -> std::size_t
	{
		// This is an approximate size - exact counting would require draining the queue
		// For now, we return 0 if empty, or estimate based on metrics
		bool all_empty = true;
		for (const auto& type : types)
		{
			std::vector<job_type> single_type = { type };
			if (!get_current_impl()->empty(single_type))
			{
				all_empty = false;
				break;
			}
		}

		if (all_empty)
		{
			return 0;
		}

		// Return a non-zero value if not empty
		// The exact count is not easily available without draining the queue
		return metrics_.operation_count.load(std::memory_order_acquire) > 0 ? 1 : 0;
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::to_string() const -> std::string
	{
		return "adaptive_typed_job_queue[" + get_current_type() + "]";
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::get_current_type() const -> std::string
	{
		switch (current_type_.load(std::memory_order_acquire))
		{
		case queue_type::LOCKFREE:
			return "lockfree";
		case queue_type::HYBRID:
			return "hybrid";
		case queue_type::LEGACY_MUTEX:
		default:
			return "legacy_mutex";
		}
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::get_metrics() const -> performance_metrics
	{
		performance_metrics result;
		result.operation_count = metrics_.operation_count.load(std::memory_order_acquire);
		result.total_latency_ns = metrics_.total_latency_ns.load(std::memory_order_acquire);
		result.contention_count = metrics_.contention_count.load(std::memory_order_acquire);
		result.switch_count = metrics_.switch_count.load(std::memory_order_acquire);
		result.last_evaluation = metrics_.last_evaluation;
		return result;
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::evaluate_and_switch() -> void
	{
		// Only evaluate if we have enough data
		if (metrics_.operation_count.load(std::memory_order_acquire) < MIN_OPERATIONS_FOR_SWITCH)
		{
			return;
		}

		// Check if we should switch
		if (should_switch_to_lockfree())
		{
			migrate_to_lockfree();
		}
		else if (should_switch_to_legacy())
		{
			migrate_to_legacy();
		}

		metrics_.last_evaluation = std::chrono::steady_clock::now();
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::update_metrics(std::chrono::nanoseconds duration, bool had_contention) -> void
	{
		metrics_.operation_count.fetch_add(1, std::memory_order_relaxed);
		metrics_.total_latency_ns.fetch_add(duration.count(), std::memory_order_relaxed);
		if (had_contention)
		{
			metrics_.contention_count.fetch_add(1, std::memory_order_relaxed);
		}
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::start_performance_monitor() -> void
	{
		// Performance monitoring is optional for now
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::stop_performance_monitor() -> void
	{
		stop_monitor_.store(true, std::memory_order_release);
		if (monitor_thread_ && monitor_thread_->joinable())
		{
			monitor_thread_->join();
		}
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::monitor_performance() -> void
	{
		// Placeholder for performance monitoring logic
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::should_switch_to_lockfree() const -> bool
	{
#ifdef TYPED_LOCKFREE_QUEUE_FORCE_ENABLE
		if (current_type_.load(std::memory_order_acquire) == queue_type::LOCKFREE)
		{
			return false;
		}

		auto ratio = static_cast<double>(metrics_.contention_count.load(std::memory_order_acquire)) /
					 static_cast<double>(metrics_.operation_count.load(std::memory_order_acquire));
		return ratio > CONTENTION_THRESHOLD_HIGH;
#else
		return false;
#endif
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::should_switch_to_legacy() const -> bool
	{
		if (current_type_.load(std::memory_order_acquire) == queue_type::LEGACY_MUTEX)
		{
			return false;
		}

		auto ratio = static_cast<double>(metrics_.contention_count.load(std::memory_order_acquire)) /
					 static_cast<double>(metrics_.operation_count.load(std::memory_order_acquire));
		return ratio < CONTENTION_THRESHOLD_LOW;
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::migrate_to_lockfree() -> void
	{
#ifdef TYPED_LOCKFREE_QUEUE_FORCE_ENABLE
		if (!lockfree_queue_)
		{
			lockfree_queue_ = std::make_unique<typed_lockfree_job_queue_t<job_type>>();
		}

		// Migrate all jobs from legacy to lock-free
		while (true)
		{
			auto result = legacy_queue_->dequeue();
			if (result.is_err())
			{
				break;
			}
			lockfree_queue_->enqueue(std::move(result.value()));
		}

		current_type_.store(queue_type::LOCKFREE, std::memory_order_release);
		metrics_.switch_count.fetch_add(1, std::memory_order_relaxed);
#endif
	}

	template <typename job_type>
	auto adaptive_typed_job_queue_t<job_type>::migrate_to_legacy() -> void
	{
#ifdef TYPED_LOCKFREE_QUEUE_FORCE_ENABLE
		if (!legacy_queue_)
		{
			legacy_queue_ = std::make_unique<typed_job_queue_t<job_type>>();
		}

		// Migrate all jobs from lock-free to legacy
		while (true)
		{
			auto result = lockfree_queue_->dequeue();
			if (result.is_err())
			{
				break;
			}
			legacy_queue_->enqueue(std::move(result.value()));
		}
#endif

		current_type_.store(queue_type::LEGACY_MUTEX, std::memory_order_release);
		metrics_.switch_count.fetch_add(1, std::memory_order_relaxed);
	}

	template <typename job_type>
	auto create_typed_job_queue(
		typename adaptive_typed_job_queue_t<job_type>::queue_strategy strategy,
		[[maybe_unused]] size_t max_threads
	) -> std::shared_ptr<typed_job_queue_t<job_type>>
	{
		return std::make_shared<adaptive_typed_job_queue_t<job_type>>(strategy);
	}

} // namespace kcenon::thread

