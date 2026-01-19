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

#include <kcenon/thread/scaling/autoscaler.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/utils/formatter.h>

#include <algorithm>
#include <numeric>

namespace kcenon::thread
{

autoscaler::autoscaler(thread_pool& pool, autoscaling_policy policy)
	: pool_(pool)
	, policy_(std::move(policy))
	, last_sample_time_(std::chrono::steady_clock::now())
{
	// Initialize stats with current worker count
	std::scoped_lock<std::mutex> lock(stats_mutex_);
	stats_.min_workers = pool_.get_active_worker_count();
	stats_.peak_workers = stats_.min_workers;
}

autoscaler::~autoscaler()
{
	stop();
}

auto autoscaler::start() -> void
{
	bool expected = false;
	if (!running_.compare_exchange_strong(expected, true))
	{
		// Already running
		return;
	}

	// Start monitor thread
	monitor_thread_ = std::make_unique<std::thread>([this]() {
		monitor_loop();
	});
}

auto autoscaler::stop() -> void
{
	bool expected = true;
	if (!running_.compare_exchange_strong(expected, false))
	{
		// Already stopped
		return;
	}

	// Wake up monitor thread
	{
		std::lock_guard<std::mutex> lock(mutex_);
		cv_.notify_one();
	}

	// Wait for thread to complete
	if (monitor_thread_ && monitor_thread_->joinable())
	{
		monitor_thread_->join();
	}
	monitor_thread_.reset();
}

auto autoscaler::is_active() const -> bool
{
	return running_.load(std::memory_order_acquire);
}

auto autoscaler::evaluate_now() -> scaling_decision
{
	// Collect current metrics
	auto sample = collect_metrics();

	// Add to history
	{
		std::scoped_lock<std::mutex> lock(history_mutex_);
		metrics_history_.push_back(sample);
		if (metrics_history_.size() > 60)
		{
			metrics_history_.pop_front();
		}
	}

	// Get recent samples for decision
	std::vector<scaling_metrics_sample> samples;
	{
		std::scoped_lock<std::mutex> lock(history_mutex_);
		std::size_t count = std::min(metrics_history_.size(), policy_.samples_for_decision);
		samples.reserve(count);
		auto it = metrics_history_.end();
		std::advance(it, -static_cast<std::ptrdiff_t>(count));
		for (; it != metrics_history_.end(); ++it)
		{
			samples.push_back(*it);
		}
	}

	// Make decision
	return make_decision(samples);
}

auto autoscaler::scale_to(std::size_t target_workers) -> common::VoidResult
{
	// Clamp to policy bounds
	target_workers = std::clamp(target_workers, policy_.min_workers, policy_.max_workers);

	std::size_t current_workers = pool_.get_active_worker_count();

	if (target_workers > current_workers)
	{
		return add_workers(target_workers - current_workers);
	}
	else if (target_workers < current_workers)
	{
		return remove_workers(current_workers - target_workers);
	}

	return common::ok();
}

auto autoscaler::scale_up() -> common::VoidResult
{
	std::size_t current = pool_.get_active_worker_count();
	std::size_t increment = policy_.use_multiplicative_scaling
		? static_cast<std::size_t>(current * (policy_.scale_up_factor - 1.0))
		: policy_.scale_up_increment;

	if (increment == 0)
	{
		increment = 1;
	}

	std::size_t target = std::min(current + increment, policy_.max_workers);
	return scale_to(target);
}

auto autoscaler::scale_down() -> common::VoidResult
{
	std::size_t current = pool_.get_active_worker_count();
	std::size_t target = current > policy_.scale_down_increment
		? current - policy_.scale_down_increment
		: policy_.min_workers;

	target = std::max(target, policy_.min_workers);
	return scale_to(target);
}

auto autoscaler::set_policy(autoscaling_policy policy) -> void
{
	std::scoped_lock<std::mutex> lock(mutex_);
	policy_ = std::move(policy);
}

auto autoscaler::get_policy() const -> const autoscaling_policy&
{
	return policy_;
}

auto autoscaler::get_current_metrics() const -> scaling_metrics_sample
{
	return collect_metrics();
}

auto autoscaler::get_metrics_history(std::size_t count) const
	-> std::vector<scaling_metrics_sample>
{
	std::scoped_lock<std::mutex> lock(history_mutex_);

	std::vector<scaling_metrics_sample> result;
	std::size_t actual_count = std::min(count, metrics_history_.size());
	result.reserve(actual_count);

	auto it = metrics_history_.end();
	std::advance(it, -static_cast<std::ptrdiff_t>(actual_count));
	for (; it != metrics_history_.end(); ++it)
	{
		result.push_back(*it);
	}

	return result;
}

auto autoscaler::get_stats() const -> autoscaling_stats
{
	std::scoped_lock<std::mutex> lock(stats_mutex_);
	return stats_;
}

auto autoscaler::reset_stats() -> void
{
	std::scoped_lock<std::mutex> lock(stats_mutex_);
	stats_ = autoscaling_stats{};
	stats_.min_workers = pool_.get_active_worker_count();
	stats_.peak_workers = stats_.min_workers;
}

auto autoscaler::monitor_loop() -> void
{
	while (running_.load(std::memory_order_acquire))
	{
		// Wait for sample interval
		{
			std::unique_lock<std::mutex> lock(mutex_);
			cv_.wait_for(lock, policy_.sample_interval, [this]() {
				return !running_.load(std::memory_order_acquire);
			});
		}

		if (!running_.load(std::memory_order_acquire))
		{
			break;
		}

		// Skip if pool is not running
		if (!pool_.is_running())
		{
			continue;
		}

		// Collect metrics
		auto sample = collect_metrics();

		// Add to history
		{
			std::scoped_lock<std::mutex> lock(history_mutex_);
			metrics_history_.push_back(sample);

			// Keep max 60 samples (1 minute at 1s interval)
			while (metrics_history_.size() > 60)
			{
				metrics_history_.pop_front();
			}
		}

		// Only auto-scale in automatic mode
		if (policy_.scaling_mode != autoscaling_policy::mode::automatic)
		{
			continue;
		}

		// Collect samples for decision
		std::vector<scaling_metrics_sample> samples;
		{
			std::scoped_lock<std::mutex> lock(history_mutex_);
			std::size_t count = std::min(metrics_history_.size(), policy_.samples_for_decision);
			if (count < policy_.samples_for_decision)
			{
				// Not enough samples yet
				continue;
			}

			samples.reserve(count);
			auto it = metrics_history_.end();
			std::advance(it, -static_cast<std::ptrdiff_t>(count));
			for (; it != metrics_history_.end(); ++it)
			{
				samples.push_back(*it);
			}
		}

		// Make and execute decision
		auto decision = make_decision(samples);
		if (decision.should_scale())
		{
			execute_scaling(decision);
		}

		// Update stats
		{
			std::scoped_lock<std::mutex> lock(stats_mutex_);
			++stats_.decisions_evaluated;

			std::size_t current = pool_.get_active_worker_count();
			stats_.peak_workers = std::max(stats_.peak_workers, current);
			if (stats_.min_workers == 0 || current < stats_.min_workers)
			{
				stats_.min_workers = current;
			}
		}
	}
}

auto autoscaler::collect_metrics() const -> scaling_metrics_sample
{
	auto now = std::chrono::steady_clock::now();

	scaling_metrics_sample sample;
	sample.timestamp = now;
	sample.worker_count = pool_.get_active_worker_count();
	sample.active_workers = sample.worker_count - pool_.get_idle_worker_count();
	sample.queue_depth = pool_.get_pending_task_count();

	// Calculate utilization
	if (sample.worker_count > 0)
	{
		sample.utilization = static_cast<double>(sample.active_workers) /
			static_cast<double>(sample.worker_count);
		sample.queue_depth_per_worker = static_cast<double>(sample.queue_depth) /
			static_cast<double>(sample.worker_count);
	}

	// Get metrics from pool
	auto metrics_snapshot = pool_.metrics().snapshot();
	sample.jobs_completed = metrics_snapshot.tasks_executed;
	sample.jobs_submitted = metrics_snapshot.tasks_submitted;

	// Calculate throughput if we have a previous sample
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
		now - last_sample_time_).count();
	if (duration > 0 && sample.jobs_completed >= last_jobs_completed_)
	{
		std::uint64_t jobs_delta = sample.jobs_completed - last_jobs_completed_;
		sample.throughput_per_second = static_cast<double>(jobs_delta) * 1000.0 /
			static_cast<double>(duration);
	}

	// Get P95 latency from enhanced metrics if available
	// Note: Using P99 wait time as closest approximation to P95
	if (pool_.is_enhanced_metrics_enabled())
	{
		auto enhanced_snapshot = pool_.enhanced_metrics_snapshot();
		// Convert from microseconds to milliseconds
		sample.p95_latency_ms = enhanced_snapshot.wait_time_p99_us / 1000.0;
	}

	// Update cached values for next sample
	const_cast<autoscaler*>(this)->last_jobs_completed_ = sample.jobs_completed;
	const_cast<autoscaler*>(this)->last_jobs_submitted_ = sample.jobs_submitted;
	const_cast<autoscaler*>(this)->last_sample_time_ = now;

	return sample;
}

auto autoscaler::make_decision(const std::vector<scaling_metrics_sample>& samples) const
	-> scaling_decision
{
	if (samples.empty())
	{
		return scaling_decision{};
	}

	// Calculate average metrics from samples
	double avg_utilization = 0.0;
	double avg_queue_depth_per_worker = 0.0;
	double avg_latency = 0.0;
	std::size_t avg_queue_depth = 0;

	for (const auto& sample : samples)
	{
		avg_utilization += sample.utilization;
		avg_queue_depth_per_worker += sample.queue_depth_per_worker;
		avg_latency += sample.p95_latency_ms;
		avg_queue_depth += sample.queue_depth;
	}

	auto sample_count = static_cast<double>(samples.size());
	avg_utilization /= sample_count;
	avg_queue_depth_per_worker /= sample_count;
	avg_latency /= sample_count;
	avg_queue_depth /= samples.size();

	std::size_t current_workers = pool_.get_active_worker_count();

	// Check scale-up triggers (ANY trigger)
	if (can_scale_up())
	{
		if (avg_utilization > policy_.scale_up.utilization_threshold)
		{
			std::size_t target = std::min(
				current_workers + policy_.scale_up_increment,
				policy_.max_workers);

			return scaling_decision{
				.direction = scaling_direction::up,
				.reason = scaling_reason::worker_utilization,
				.target_workers = target,
				.explanation = utility_module::formatter::format(
					"Utilization {:.1f}% exceeds threshold {:.1f}%",
					avg_utilization * 100, policy_.scale_up.utilization_threshold * 100)
			};
		}

		if (avg_queue_depth_per_worker > policy_.scale_up.queue_depth_threshold)
		{
			std::size_t target = std::min(
				current_workers + policy_.scale_up_increment,
				policy_.max_workers);

			return scaling_decision{
				.direction = scaling_direction::up,
				.reason = scaling_reason::queue_depth,
				.target_workers = target,
				.explanation = utility_module::formatter::format(
					"Queue depth per worker {:.1f} exceeds threshold {:.1f}",
					avg_queue_depth_per_worker, policy_.scale_up.queue_depth_threshold)
			};
		}

		if (avg_latency > policy_.scale_up.latency_threshold_ms && avg_latency > 0)
		{
			std::size_t target = std::min(
				current_workers + policy_.scale_up_increment,
				policy_.max_workers);

			return scaling_decision{
				.direction = scaling_direction::up,
				.reason = scaling_reason::latency,
				.target_workers = target,
				.explanation = utility_module::formatter::format(
					"P95 latency {:.1f}ms exceeds threshold {:.1f}ms",
					avg_latency, policy_.scale_up.latency_threshold_ms)
			};
		}

		if (avg_queue_depth > policy_.scale_up.pending_jobs_threshold)
		{
			std::size_t target = std::min(
				current_workers + policy_.scale_up_increment,
				policy_.max_workers);

			return scaling_decision{
				.direction = scaling_direction::up,
				.reason = scaling_reason::queue_depth,
				.target_workers = target,
				.explanation = utility_module::formatter::format(
					"Queue depth {} exceeds threshold {}",
					avg_queue_depth, policy_.scale_up.pending_jobs_threshold)
			};
		}
	}

	// Check scale-down triggers (ALL triggers)
	if (can_scale_down() && current_workers > policy_.min_workers)
	{
		bool utilization_ok = avg_utilization < policy_.scale_down.utilization_threshold;
		bool queue_depth_ok = avg_queue_depth_per_worker < policy_.scale_down.queue_depth_threshold;

		if (utilization_ok && queue_depth_ok)
		{
			std::size_t target = std::max(
				current_workers - policy_.scale_down_increment,
				policy_.min_workers);

			return scaling_decision{
				.direction = scaling_direction::down,
				.reason = scaling_reason::worker_utilization,
				.target_workers = target,
				.explanation = utility_module::formatter::format(
					"Utilization {:.1f}% below threshold {:.1f}%, queue depth {:.1f} below {:.1f}",
					avg_utilization * 100, policy_.scale_down.utilization_threshold * 100,
					avg_queue_depth_per_worker, policy_.scale_down.queue_depth_threshold)
			};
		}
	}

	return scaling_decision{};
}

auto autoscaler::execute_scaling(const scaling_decision& decision) -> void
{
	std::size_t current_workers = pool_.get_active_worker_count();
	auto now = std::chrono::steady_clock::now();

	if (decision.direction == scaling_direction::up)
	{
		auto result = add_workers(decision.target_workers - current_workers);
		if (result.is_ok())
		{
			last_scale_up_time_ = now;

			std::scoped_lock<std::mutex> lock(stats_mutex_);
			++stats_.scale_up_count;
			stats_.last_scale_up = now;

			if (policy_.scaling_callback)
			{
				policy_.scaling_callback(
					scaling_direction::up,
					decision.reason,
					current_workers,
					decision.target_workers);
			}
		}
	}
	else if (decision.direction == scaling_direction::down)
	{
		auto result = remove_workers(current_workers - decision.target_workers);
		if (result.is_ok())
		{
			last_scale_down_time_ = now;

			std::scoped_lock<std::mutex> lock(stats_mutex_);
			++stats_.scale_down_count;
			stats_.last_scale_down = now;

			if (policy_.scaling_callback)
			{
				policy_.scaling_callback(
					scaling_direction::down,
					decision.reason,
					current_workers,
					decision.target_workers);
			}
		}
	}
}

auto autoscaler::can_scale_up() const -> bool
{
	if (pool_.get_active_worker_count() >= policy_.max_workers)
	{
		return false;
	}

	auto now = std::chrono::steady_clock::now();
	auto since_last = std::chrono::duration_cast<std::chrono::seconds>(
		now - last_scale_up_time_);

	return since_last >= policy_.scale_up_cooldown;
}

auto autoscaler::can_scale_down() const -> bool
{
	if (pool_.get_active_worker_count() <= policy_.min_workers)
	{
		return false;
	}

	auto now = std::chrono::steady_clock::now();
	auto since_last = std::chrono::duration_cast<std::chrono::seconds>(
		now - last_scale_down_time_);

	return since_last >= policy_.scale_down_cooldown;
}

auto autoscaler::add_workers(std::size_t count) -> common::VoidResult
{
	if (count == 0)
	{
		return common::ok();
	}

	// Get current context from pool
	const auto& context = pool_.get_context();

	for (std::size_t i = 0; i < count; ++i)
	{
		auto worker = std::make_unique<thread_worker>(true, context);
		auto result = pool_.enqueue(std::move(worker));
		if (result.is_err())
		{
			return result;
		}
	}

	return common::ok();
}

auto autoscaler::remove_workers(std::size_t count) -> common::VoidResult
{
	if (count == 0)
	{
		return common::ok();
	}

	// Request pool to remove workers using internal method
	// This will gracefully stop idle workers
	auto result = pool_.remove_workers_internal(count, policy_.min_workers);
	return result;
}

} // namespace kcenon::thread
