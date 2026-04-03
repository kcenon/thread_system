// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/**
 * @file enhanced_work_stealing_config.h
 * @brief Configuration for enhanced work-stealing with NUMA awareness.
 *
 * @see numa_work_stealer
 */

#pragma once

#include "enhanced_steal_policy.h"
#include "steal_backoff_strategy.h"

#include <chrono>
#include <cstddef>

namespace kcenon::thread
{

/**
 * @struct enhanced_work_stealing_config
 * @brief Configuration for enhanced work-stealing with NUMA awareness
 *
 * This structure provides comprehensive configuration for the numa_work_stealer
 * class, including victim selection policies, NUMA optimization, batch stealing,
 * backoff strategies, and statistics collection.
 *
 * ### Usage Example
 * @code
 * enhanced_work_stealing_config config;
 * config.enabled = true;
 * config.policy = enhanced_steal_policy::numa_aware;
 * config.numa_aware = true;
 * config.prefer_same_node = true;
 * config.collect_statistics = true;
 *
 * numa_work_stealer stealer(workers, config);
 * @endcode
 *
 * ### Configuration Categories
 * - **Enable/Disable**: Master switch for work stealing
 * - **Policy**: Victim selection strategy
 * - **NUMA**: Cross-node steal penalty and preferences
 * - **Batch**: Multi-job stealing configuration
 * - **Attempts**: Retry and failure limits
 * - **Backoff**: Delay strategy between attempts
 * - **Locality**: Work affinity tracking
 * - **Statistics**: Performance metrics collection
 */
struct enhanced_work_stealing_config
{
	// ========================================================================
	// Enable/Disable
	// ========================================================================

	/// Master switch for work-stealing (default: disabled)
	bool enabled = false;

	// ========================================================================
	// Victim Selection Policy
	// ========================================================================

	/// Policy for selecting steal victims (default: adaptive)
	enhanced_steal_policy policy = enhanced_steal_policy::adaptive;

	// ========================================================================
	// NUMA Configuration
	// ========================================================================

	/// Enable NUMA-aware stealing (default: disabled)
	bool numa_aware = false;

	/// Cost multiplier for cross-NUMA node steals (default: 2.0)
	/// Higher values make cross-node steals less likely
	double numa_penalty_factor = 2.0;

	/// Prefer workers on the same NUMA node (default: true)
	bool prefer_same_node = true;

	// ========================================================================
	// Batch Stealing Configuration
	// ========================================================================

	/// Minimum number of jobs to steal in a batch (default: 1)
	std::size_t min_steal_batch = 1;

	/// Maximum number of jobs to steal in a batch (default: 4)
	std::size_t max_steal_batch = 4;

	/// Dynamically adjust batch size based on victim's queue depth (default: true)
	bool adaptive_batch_size = true;

	// ========================================================================
	// Steal Attempts Configuration
	// ========================================================================

	/// Maximum number of steal attempts per round (default: 3)
	std::size_t max_steal_attempts = 3;

	/// Maximum consecutive failures before yielding (default: 10)
	std::size_t max_consecutive_failures = 10;

	// ========================================================================
	// Backoff Configuration
	// ========================================================================

	/// Backoff strategy between failed steal attempts (default: exponential)
	steal_backoff_strategy backoff_strategy = steal_backoff_strategy::exponential;

	/// Initial backoff delay (default: 50 microseconds)
	std::chrono::microseconds initial_backoff{50};

	/// Maximum backoff delay cap (default: 1000 microseconds)
	std::chrono::microseconds max_backoff{1000};

	/// Backoff multiplier for exponential strategy (default: 2.0)
	double backoff_multiplier = 2.0;

	// ========================================================================
	// Locality Tracking
	// ========================================================================

	/// Enable work affinity tracking between workers (default: disabled)
	bool track_locality = false;

	/// Size of cooperation history for locality tracking (default: 16)
	std::size_t locality_history_size = 16;

	// ========================================================================
	// Statistics Collection
	// ========================================================================

	/// Enable statistics collection (default: disabled)
	bool collect_statistics = false;

	// ========================================================================
	// Factory Methods
	// ========================================================================

	/**
	 * @brief Create a default configuration (disabled)
	 * @return Default configuration with work stealing disabled
	 */
	static auto default_config() -> enhanced_work_stealing_config
	{
		return enhanced_work_stealing_config{};
	}

	/**
	 * @brief Create a configuration optimized for NUMA systems
	 * @return Configuration with NUMA awareness enabled
	 */
	static auto numa_optimized() -> enhanced_work_stealing_config
	{
		enhanced_work_stealing_config config;
		config.enabled = true;
		config.policy = enhanced_steal_policy::numa_aware;
		config.numa_aware = true;
		config.prefer_same_node = true;
		config.numa_penalty_factor = 2.0;
		config.collect_statistics = true;
		return config;
	}

	/**
	 * @brief Create a configuration optimized for cache locality
	 * @return Configuration with locality tracking enabled
	 */
	static auto locality_optimized() -> enhanced_work_stealing_config
	{
		enhanced_work_stealing_config config;
		config.enabled = true;
		config.policy = enhanced_steal_policy::locality_aware;
		config.track_locality = true;
		config.locality_history_size = 32;
		config.collect_statistics = true;
		return config;
	}

	/**
	 * @brief Create a configuration for aggressive batch stealing
	 * @return Configuration with large batch sizes
	 */
	static auto batch_optimized() -> enhanced_work_stealing_config
	{
		enhanced_work_stealing_config config;
		config.enabled = true;
		config.policy = enhanced_steal_policy::adaptive;
		config.min_steal_batch = 2;
		config.max_steal_batch = 8;
		config.adaptive_batch_size = true;
		return config;
	}

	/**
	 * @brief Create a configuration for hierarchical NUMA systems
	 * @return Configuration with hierarchical victim selection
	 */
	static auto hierarchical_numa() -> enhanced_work_stealing_config
	{
		enhanced_work_stealing_config config;
		config.enabled = true;
		config.policy = enhanced_steal_policy::hierarchical;
		config.numa_aware = true;
		config.prefer_same_node = true;
		config.numa_penalty_factor = 3.0;
		config.track_locality = true;
		config.collect_statistics = true;
		return config;
	}
};

} // namespace kcenon::thread
