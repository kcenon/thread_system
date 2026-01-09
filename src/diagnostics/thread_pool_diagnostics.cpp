/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, kcenon
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

#include <kcenon/thread/diagnostics/thread_pool_diagnostics.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_worker.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <iomanip>
#include <sstream>

namespace kcenon::thread::diagnostics
{
	// =========================================================================
	// Constructor / Destructor
	// =========================================================================

	thread_pool_diagnostics::thread_pool_diagnostics(thread_pool& pool,
	                                                const diagnostics_config& config)
	    : pool_(pool)
	    , config_(config)
	    , tracing_enabled_(config.enable_tracing)
	    , start_time_(std::chrono::steady_clock::now())
	{
	}

	thread_pool_diagnostics::~thread_pool_diagnostics() = default;

	// =========================================================================
	// Thread Dump
	// =========================================================================

	auto thread_pool_diagnostics::dump_thread_states() const -> std::vector<thread_info>
	{
		// Delegate to thread_pool's collect_worker_diagnostics for actual worker info
		return pool_.collect_worker_diagnostics();
	}

	auto thread_pool_diagnostics::format_thread_dump() const -> std::string
	{
		std::ostringstream oss;

		auto threads = dump_thread_states();
		auto now = std::chrono::system_clock::now();
		auto time_t = std::chrono::system_clock::to_time_t(now);

		auto worker_count = pool_.get_thread_count();
		auto active_count = pool_.get_active_worker_count();
		auto idle_count = pool_.get_idle_worker_count();

		// Header
		oss << "=== Thread Pool Dump: " << pool_.to_string() << " ===\n";
		oss << "Time: " << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ") << "\n";
		oss << "Workers: " << worker_count << ", Active: " << active_count
		    << ", Idle: " << idle_count << "\n\n";

		// Worker details
		for (const auto& t : threads)
		{
			auto state_duration = t.state_duration();
			auto duration_sec = std::chrono::duration<double>(state_duration).count();

			oss << t.thread_name << " [tid:" << t.thread_id << "] "
			    << worker_state_to_string(t.state)
			    << " (" << std::fixed << std::setprecision(1) << duration_sec << "s)\n";

			if (t.current_job.has_value())
			{
				const auto& job = t.current_job.value();
				auto exec_time_ms = std::chrono::duration<double, std::milli>(
				    job.execution_time).count();
				oss << "  Current Job: " << job.job_name << "#" << job.job_id
				    << " (running " << std::fixed << std::setprecision(0)
				    << exec_time_ms << "ms)\n";
			}

			oss << "  Jobs: " << t.jobs_completed << " completed, "
			    << t.jobs_failed << " failed\n";
			oss << "  Utilization: " << std::fixed << std::setprecision(1)
			    << (t.utilization * 100.0) << "%\n\n";
		}

		return oss.str();
	}

	// =========================================================================
	// Job Inspection
	// =========================================================================

	auto thread_pool_diagnostics::get_active_jobs() const -> std::vector<job_info>
	{
		std::vector<job_info> result;

		// Get thread states which include current job info
		auto threads = dump_thread_states();

		for (const auto& thread : threads)
		{
			if (thread.current_job.has_value())
			{
				result.push_back(thread.current_job.value());
			}
		}

		return result;
	}

	auto thread_pool_diagnostics::get_pending_jobs(std::size_t limit) const
	    -> std::vector<job_info>
	{
		// Delegate to job_queue's inspect_pending_jobs
		auto queue = pool_.get_job_queue();
		if (!queue)
		{
			return {};
		}

		return queue->inspect_pending_jobs(limit);
	}

	auto thread_pool_diagnostics::get_recent_jobs(std::size_t limit) const
	    -> std::vector<job_info>
	{
		std::lock_guard<std::mutex> lock(jobs_mutex_);

		std::vector<job_info> result;
		auto count = std::min(limit, recent_jobs_.size());
		result.reserve(count);

		auto it = recent_jobs_.rbegin();
		for (std::size_t i = 0; i < count && it != recent_jobs_.rend(); ++i, ++it)
		{
			result.push_back(*it);
		}

		return result;
	}

	void thread_pool_diagnostics::record_job_completion(const job_info& info)
	{
		std::lock_guard<std::mutex> lock(jobs_mutex_);

		recent_jobs_.push_back(info);
		while (recent_jobs_.size() > config_.recent_jobs_capacity)
		{
			recent_jobs_.pop_front();
		}
	}

	// =========================================================================
	// Bottleneck Detection
	// =========================================================================

	auto thread_pool_diagnostics::detect_bottlenecks() const -> bottleneck_report
	{
		bottleneck_report report;

		// Gather metrics
		auto metrics_snap = pool_.metrics().snapshot();
		auto worker_count = pool_.get_thread_count();
		auto active_count = pool_.get_active_worker_count();
		auto idle_count = pool_.get_idle_worker_count();
		auto queue_depth = pool_.get_pending_task_count();

		report.queue_depth = queue_depth;
		report.idle_workers = idle_count;
		report.total_workers = worker_count;

		// Calculate queue saturation
		auto queue = pool_.get_job_queue();
		if (queue)
		{
			auto max_size = queue->get_max_size();
			if (max_size.has_value() && max_size.value() > 0)
			{
				report.queue_saturation = static_cast<double>(queue_depth) /
				                          static_cast<double>(max_size.value());
			}
			else if (queue_depth > 0)
			{
				// For unbounded queues, use heuristic: saturation based on queue depth vs workers
				// High queue depth relative to workers indicates potential saturation
				report.queue_saturation = std::min(1.0,
					static_cast<double>(queue_depth) / static_cast<double>(worker_count * 10));
			}
		}

		// Calculate worker utilization (instantaneous)
		if (worker_count > 0)
		{
			report.worker_utilization = static_cast<double>(active_count) /
			                            static_cast<double>(worker_count);
		}

		// Get per-worker utilization for variance calculation
		auto thread_states = pool_.collect_worker_diagnostics();
		if (!thread_states.empty())
		{
			// Calculate mean utilization from worker stats
			double sum_utilization = 0.0;
			for (const auto& t : thread_states)
			{
				sum_utilization += t.utilization;
			}
			double mean_utilization = sum_utilization / static_cast<double>(thread_states.size());

			// Calculate variance
			double variance_sum = 0.0;
			for (const auto& t : thread_states)
			{
				double diff = t.utilization - mean_utilization;
				variance_sum += diff * diff;
			}
			report.utilization_variance = variance_sum / static_cast<double>(thread_states.size());

			// Use mean utilization from actual worker stats if available
			if (mean_utilization > 0.0)
			{
				report.worker_utilization = mean_utilization;
			}
		}

		// Calculate average wait time from metrics
		auto total_jobs = metrics_snap.tasks_executed + metrics_snap.tasks_failed;
		if (total_jobs > 0)
		{
			// Estimate wait time from idle time (approximation)
			auto avg_idle_ns = metrics_snap.total_idle_time_ns / total_jobs;
			report.avg_wait_time_ms = static_cast<double>(avg_idle_ns) / 1e6;

			// Calculate estimated backlog time
			// Average execution time per job
			double avg_exec_time_ms = 0.0;
			if (metrics_snap.total_busy_time_ns > 0 && total_jobs > 0)
			{
				avg_exec_time_ms = static_cast<double>(metrics_snap.total_busy_time_ns) /
				                   static_cast<double>(total_jobs) / 1e6;
			}

			// Estimated time to clear backlog = (queue_depth * avg_exec_time) / active_workers
			if (active_count > 0 && avg_exec_time_ms > 0)
			{
				report.estimated_backlog_time_ms = static_cast<std::size_t>(
					(static_cast<double>(queue_depth) * avg_exec_time_ms) /
					static_cast<double>(active_count));
			}
			else if (worker_count > 0 && avg_exec_time_ms > 0)
			{
				report.estimated_backlog_time_ms = static_cast<std::size_t>(
					(static_cast<double>(queue_depth) * avg_exec_time_ms) /
					static_cast<double>(worker_count));
			}
		}

		// Jobs rejected tracking not available in basic metrics
		report.jobs_rejected = 0;

		// Detect bottleneck type (ordered by severity)
		// 1. Queue full - most critical
		if (report.queue_saturation > 0.95 || report.jobs_rejected > 0)
		{
			report.has_bottleneck = true;
			report.type = bottleneck_type::queue_full;
			report.description = "Queue is at or near capacity, jobs are being rejected";
		}
		// 2. Worker starvation - high utilization with growing backlog
		else if (report.worker_utilization > 0.95 && queue_depth > worker_count * 2)
		{
			report.has_bottleneck = true;
			report.type = bottleneck_type::worker_starvation;
			report.description = "Not enough workers to handle the workload";
		}
		// 3. Slow consumer - high wait time with high utilization
		else if (report.avg_wait_time_ms > config_.wait_time_threshold_ms &&
		         report.worker_utilization > config_.utilization_high_threshold)
		{
			report.has_bottleneck = true;
			report.type = bottleneck_type::slow_consumer;
			report.description = "Workers cannot keep up with job submission rate";
		}
		// 4. Uneven distribution - high variance in worker utilization
		else if (report.utilization_variance > 0.1 && worker_count > 1)
		{
			// Variance > 0.1 means standard deviation > ~0.32 which is significant
			report.has_bottleneck = true;
			report.type = bottleneck_type::uneven_distribution;
			report.description = "Work is not evenly distributed across workers";
		}
		// 5. Lock contention - high wait time but low utilization (workers waiting on locks)
		else if (report.avg_wait_time_ms > config_.wait_time_threshold_ms * 2 &&
		         report.worker_utilization < 0.5 && active_count > 0)
		{
			report.has_bottleneck = true;
			report.type = bottleneck_type::lock_contention;
			report.description = "High wait times with low utilization suggests lock contention";
		}
		// 6. Memory pressure - check queue memory usage
		else if (queue)
		{
			auto mem_stats = queue->get_memory_stats();
			// Consider memory pressure if queue uses more than 100MB
			constexpr std::size_t memory_threshold = 100 * 1024 * 1024;
			if (mem_stats.queue_size_bytes > memory_threshold)
			{
				report.has_bottleneck = true;
				report.type = bottleneck_type::memory_pressure;
				report.description = "Excessive memory usage in job queue";
			}
		}

		// Generate recommendations if bottleneck detected
		if (report.has_bottleneck)
		{
			generate_recommendations(report);
		}

		return report;
	}

	void thread_pool_diagnostics::generate_recommendations(bottleneck_report& report) const
	{
		switch (report.type)
		{
			case bottleneck_type::queue_full:
				report.recommendations.push_back("Consider increasing queue capacity");
				report.recommendations.push_back("Enable backpressure with adaptive policy");
				report.recommendations.push_back("Add more worker threads if CPU permits");
				break;

			case bottleneck_type::slow_consumer:
				report.recommendations.push_back("Add more worker threads");
				report.recommendations.push_back("Optimize job execution time");
				report.recommendations.push_back("Consider job batching for small tasks");
				break;

			case bottleneck_type::worker_starvation:
				report.recommendations.push_back("Increase worker thread count");
				report.recommendations.push_back("Consider scaling based on hardware cores");
				report.recommendations.push_back("Enable autoscaling for dynamic adjustment");
				break;

			case bottleneck_type::uneven_distribution:
				report.recommendations.push_back("Enable work stealing if not already");
				report.recommendations.push_back("Review job distribution patterns");
				report.recommendations.push_back("Consider using priority-based scheduling");
				break;

			case bottleneck_type::lock_contention:
				report.recommendations.push_back("Review shared resource access patterns");
				report.recommendations.push_back("Consider using lock-free data structures");
				report.recommendations.push_back("Reduce critical section scope");
				report.recommendations.push_back("Use finer-grained locking strategies");
				break;

			case bottleneck_type::memory_pressure:
				report.recommendations.push_back("Reduce queue capacity or enable backpressure");
				report.recommendations.push_back("Optimize job object size");
				report.recommendations.push_back("Add more workers to process jobs faster");
				report.recommendations.push_back("Consider job prioritization to clear backlog");
				break;

			case bottleneck_type::none:
			default:
				break;
		}
	}

	// =========================================================================
	// Health Checks
	// =========================================================================

	auto thread_pool_diagnostics::health_check() const -> health_status
	{
		health_status status;
		status.check_time = std::chrono::steady_clock::now();

		// Calculate uptime
		auto uptime = status.check_time - start_time_;
		status.uptime_seconds = std::chrono::duration<double>(uptime).count();

		// Get metrics
		auto metrics_snap = pool_.metrics().snapshot();
		status.total_jobs_processed = metrics_snap.tasks_executed +
		                              metrics_snap.tasks_failed;

		if (status.total_jobs_processed > 0)
		{
			status.success_rate = static_cast<double>(metrics_snap.tasks_executed) /
			                      static_cast<double>(status.total_jobs_processed);
		}

		// Worker stats
		status.total_workers = pool_.get_thread_count();
		status.active_workers = pool_.get_active_worker_count();
		status.queue_depth = pool_.get_pending_task_count();

		// Check components
		status.components.push_back(check_worker_health());
		status.components.push_back(check_queue_health());

		// Calculate overall status
		status.calculate_overall_status();

		return status;
	}

	auto thread_pool_diagnostics::is_healthy() const -> bool
	{
		return pool_.is_running() && pool_.get_thread_count() > 0;
	}

	auto thread_pool_diagnostics::check_worker_health() const -> component_health
	{
		component_health health;
		health.name = "workers";

		auto total = pool_.get_thread_count();
		auto active = pool_.get_active_worker_count();
		auto idle = pool_.get_idle_worker_count();

		health.details["total"] = std::to_string(total);
		health.details["active"] = std::to_string(active);
		health.details["idle"] = std::to_string(idle);

		if (!pool_.is_running())
		{
			health.state = health_state::unhealthy;
			health.message = "Thread pool is not running";
		}
		else if (total == 0)
		{
			health.state = health_state::unhealthy;
			health.message = "No workers available";
		}
		else if (active == total)
		{
			health.state = health_state::degraded;
			health.message = "All workers are busy";
		}
		else
		{
			health.state = health_state::healthy;
			health.message = std::to_string(idle) + " workers available";
		}

		return health;
	}

	auto thread_pool_diagnostics::check_queue_health() const -> component_health
	{
		component_health health;
		health.name = "queue";

		auto depth = pool_.get_pending_task_count();
		health.details["depth"] = std::to_string(depth);

		// Note: Job rejection tracking requires backpressure queue
		// For basic queue, assume no rejections
		std::uint64_t rejected = 0;
		health.details["rejected"] = std::to_string(rejected);

		if (rejected > 0)
		{
			health.state = health_state::degraded;
			health.message = std::to_string(rejected) + " jobs rejected due to backpressure";
		}
		else
		{
			health.state = health_state::healthy;
			health.message = "Queue operational";
		}

		return health;
	}

	// =========================================================================
	// Event Tracing
	// =========================================================================

	void thread_pool_diagnostics::enable_tracing(bool enable, std::size_t history_size)
	{
		tracing_enabled_.store(enable, std::memory_order_relaxed);

		if (enable)
		{
			std::lock_guard<std::mutex> lock(events_mutex_);
			// Clear and resize if needed
			while (event_history_.size() > history_size)
			{
				event_history_.pop_front();
			}
		}

		// Update config
		config_.event_history_size = history_size;
		config_.enable_tracing = enable;
	}

	auto thread_pool_diagnostics::is_tracing_enabled() const -> bool
	{
		return tracing_enabled_.load(std::memory_order_relaxed);
	}

	void thread_pool_diagnostics::add_event_listener(
	    std::shared_ptr<execution_event_listener> listener)
	{
		if (!listener) return;

		std::lock_guard<std::mutex> lock(listeners_mutex_);
		listeners_.push_back(std::move(listener));
	}

	void thread_pool_diagnostics::remove_event_listener(
	    std::shared_ptr<execution_event_listener> listener)
	{
		if (!listener) return;

		std::lock_guard<std::mutex> lock(listeners_mutex_);
		auto it = std::find(listeners_.begin(), listeners_.end(), listener);
		if (it != listeners_.end())
		{
			listeners_.erase(it);
		}
	}

	void thread_pool_diagnostics::record_event(const job_execution_event& event)
	{
		if (!tracing_enabled_.load(std::memory_order_relaxed))
		{
			return;
		}

		// Store in history
		{
			std::lock_guard<std::mutex> lock(events_mutex_);
			event_history_.push_back(event);
			while (event_history_.size() > config_.event_history_size)
			{
				event_history_.pop_front();
			}
		}

		// Notify listeners
		notify_listeners(event);
	}

	void thread_pool_diagnostics::notify_listeners(const job_execution_event& event)
	{
		std::vector<std::shared_ptr<execution_event_listener>> listeners_copy;
		{
			std::lock_guard<std::mutex> lock(listeners_mutex_);
			listeners_copy = listeners_;
		}

		for (const auto& listener : listeners_copy)
		{
			if (listener)
			{
				listener->on_event(event);
			}
		}
	}

	auto thread_pool_diagnostics::get_recent_events(std::size_t limit) const
	    -> std::vector<job_execution_event>
	{
		std::lock_guard<std::mutex> lock(events_mutex_);

		std::vector<job_execution_event> result;
		auto count = std::min(limit, event_history_.size());
		result.reserve(count);

		auto it = event_history_.rbegin();
		for (std::size_t i = 0; i < count && it != event_history_.rend(); ++i, ++it)
		{
			result.push_back(*it);
		}

		return result;
	}

	// =========================================================================
	// Export
	// =========================================================================

	auto thread_pool_diagnostics::to_json() const -> std::string
	{
		std::ostringstream oss;
		oss << "{\n";

		// Health status
		auto health = health_check();
		oss << "  \"health\": {\n";
		oss << "    \"status\": \"" << health_state_to_string(health.overall_status) << "\",\n";
		oss << "    \"message\": \"" << health.status_message << "\",\n";
		oss << "    \"uptime_seconds\": " << std::fixed << std::setprecision(2)
		    << health.uptime_seconds << ",\n";
		oss << "    \"total_jobs_processed\": " << health.total_jobs_processed << ",\n";
		oss << "    \"success_rate\": " << std::fixed << std::setprecision(4)
		    << health.success_rate << "\n";
		oss << "  },\n";

		// Workers
		oss << "  \"workers\": {\n";
		oss << "    \"total\": " << health.total_workers << ",\n";
		oss << "    \"active\": " << health.active_workers << ",\n";
		oss << "    \"idle\": " << (health.total_workers - health.active_workers) << "\n";
		oss << "  },\n";

		// Queue
		oss << "  \"queue\": {\n";
		oss << "    \"depth\": " << health.queue_depth << "\n";
		oss << "  },\n";

		// Bottleneck
		auto bottleneck = detect_bottlenecks();
		oss << "  \"bottleneck\": {\n";
		oss << "    \"detected\": " << (bottleneck.has_bottleneck ? "true" : "false") << ",\n";
		oss << "    \"type\": \"" << bottleneck_type_to_string(bottleneck.type) << "\",\n";
		oss << "    \"severity\": \"" << bottleneck.severity_string() << "\"\n";
		oss << "  }\n";

		oss << "}";
		return oss.str();
	}

	auto thread_pool_diagnostics::to_string() const -> std::string
	{
		return format_thread_dump();
	}

	// =========================================================================
	// Configuration
	// =========================================================================

	auto thread_pool_diagnostics::get_config() const -> diagnostics_config
	{
		return config_;
	}

	void thread_pool_diagnostics::set_config(const diagnostics_config& config)
	{
		config_ = config;
		tracing_enabled_.store(config.enable_tracing, std::memory_order_relaxed);
	}

	auto thread_pool_diagnostics::get_worker_info(const thread_worker& worker,
	                                              std::size_t index) const -> thread_info
	{
		thread_info info;
		info.worker_id = worker.get_worker_id();
		info.thread_name = "Worker-" + std::to_string(index);
		info.state = worker.is_idle() ? worker_state::idle : worker_state::active;
		info.state_since = std::chrono::steady_clock::now();
		return info;
	}

} // namespace kcenon::thread::diagnostics
