// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/**
 * @file autoscaling_policy.h
 * @brief Configuration for autoscaling behavior and thresholds.
 *
 * @see autoscaler
 */

#pragma once

#include "scaling_metrics.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <thread>

namespace kcenon::thread
{
	/**
	 * @brief Configuration for autoscaling behavior.
	 *
	 * This structure defines the policy for automatic scaling of thread pool
	 * workers. It includes thresholds for scaling up and down, cooldown periods,
	 * and sampling configuration.
	 *
	 * ### Design Principles
	 * - Scale-up is triggered by ANY threshold being exceeded (OR logic)
	 * - Scale-down requires ALL thresholds to be met (AND logic)
	 * - Cooldown periods prevent scaling oscillation
	 * - Multiple samples are aggregated before making decisions
	 *
	 * ### Example Configuration
	 * @code
	 * autoscaling_policy policy{
	 *     .min_workers = 2,
	 *     .max_workers = 16,
	 *     .scale_up = {
	 *         .utilization_threshold = 0.8,
	 *         .queue_depth_threshold = 100.0
	 *     },
	 *     .scaling_mode = autoscaling_policy::mode::automatic
	 * };
	 * @endcode
	 *
	 * @see autoscaler
	 */
	struct autoscaling_policy
	{
		/**
		 * @brief Autoscaling mode.
		 */
		enum class mode
		{
			disabled,   ///< No automatic scaling
			manual,     ///< Only scale on explicit trigger
			automatic   ///< Fully automatic scaling
		};

		/**
		 * @brief Configuration for scale-up triggers.
		 *
		 * Scale-up is triggered when ANY of these thresholds are exceeded.
		 */
		struct scale_up_config
		{
			/// Jobs per worker threshold (scale up when exceeded)
			double queue_depth_threshold = 100.0;

			/// Worker utilization threshold (0.0 - 1.0, scale up when exceeded)
			double utilization_threshold = 0.8;

			/// P95 latency threshold in milliseconds (scale up when exceeded)
			double latency_threshold_ms = 50.0;

			/// Absolute pending jobs threshold (scale up when exceeded)
			std::size_t pending_jobs_threshold = 1000;
		};

		/**
		 * @brief Configuration for scale-down triggers.
		 *
		 * Scale-down is triggered when ALL of these conditions are met.
		 */
		struct scale_down_config
		{
			/// Worker utilization threshold (0.0 - 1.0, scale down when below)
			double utilization_threshold = 0.3;

			/// Jobs per worker threshold (scale down when below)
			double queue_depth_threshold = 10.0;

			/// Duration worker must be idle before removal
			std::chrono::seconds idle_duration{60};
		};

		// ============================================
		// Worker Bounds
		// ============================================

		/// Minimum number of workers (never scale below this)
		std::size_t min_workers = 1;

		/// Maximum number of workers (never scale above this)
		std::size_t max_workers = std::thread::hardware_concurrency();

		// ============================================
		// Trigger Configurations
		// ============================================

		/// Scale-up trigger configuration
		scale_up_config scale_up;

		/// Scale-down trigger configuration
		scale_down_config scale_down;

		// ============================================
		// Scaling Behavior
		// ============================================

		/// Number of workers to add per scale-up event
		std::size_t scale_up_increment = 1;

		/// Number of workers to remove per scale-down event
		std::size_t scale_down_increment = 1;

		/// Multiplicative factor for scaling (used if use_multiplicative_scaling is true)
		double scale_up_factor = 1.5;

		/// Whether to use multiplicative scaling instead of additive
		bool use_multiplicative_scaling = false;

		// ============================================
		// Cooldown Periods
		// ============================================

		/// Minimum time between scale-up events
		std::chrono::seconds scale_up_cooldown{30};

		/// Minimum time between scale-down events
		std::chrono::seconds scale_down_cooldown{60};

		// ============================================
		// Sampling Configuration
		// ============================================

		/// Interval between metric samples
		std::chrono::milliseconds sample_interval{1000};

		/// Number of samples to aggregate before making a decision
		std::size_t samples_for_decision = 5;

		// ============================================
		// Mode
		// ============================================

		/// Autoscaling mode
		mode scaling_mode = mode::disabled;

		// ============================================
		// Callbacks
		// ============================================

		/// Callback invoked on scaling events
		/// Parameters: direction, reason, from_count, to_count
		std::function<void(scaling_direction, scaling_reason, std::size_t, std::size_t)>
			scaling_callback;

		/**
		 * @brief Validates the policy configuration.
		 * @return true if valid, false otherwise.
		 */
		[[nodiscard]] auto is_valid() const -> bool
		{
			if (min_workers == 0)
			{
				return false;
			}
			if (max_workers < min_workers)
			{
				return false;
			}
			if (scale_up.utilization_threshold <= 0.0 || scale_up.utilization_threshold > 1.0)
			{
				return false;
			}
			if (scale_down.utilization_threshold < 0.0 || scale_down.utilization_threshold >= 1.0)
			{
				return false;
			}
			if (scale_down.utilization_threshold >= scale_up.utilization_threshold)
			{
				return false;
			}
			if (scale_up_increment == 0 || scale_down_increment == 0)
			{
				return false;
			}
			if (samples_for_decision == 0)
			{
				return false;
			}
			return true;
		}
	};

} // namespace kcenon::thread
