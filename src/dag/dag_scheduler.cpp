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

#include <kcenon/thread/dag/dag_scheduler.h>
#include <kcenon/thread/core/callback_job.h>

#include <algorithm>
#include <format>
#include <queue>
#include <sstream>
#include <stack>
#include <thread>

namespace kcenon::thread
{

dag_scheduler::dag_scheduler(std::shared_ptr<thread_pool> pool, dag_config config)
	: pool_(std::move(pool))
	, config_(std::move(config))
{
}

dag_scheduler::~dag_scheduler()
{
	cancel_all();
}

dag_scheduler::dag_scheduler(dag_scheduler&& other) noexcept
	: pool_(std::move(other.pool_))
	, config_(std::move(other.config_))
	, jobs_(std::move(other.jobs_))
	, dependencies_(std::move(other.dependencies_))
	, dependents_(std::move(other.dependents_))
	, executing_(other.executing_.load())
	, cancelled_(other.cancelled_.load())
	, running_count_(other.running_count_.load())
	, execution_start_time_(other.execution_start_time_)
	, first_error_(std::move(other.first_error_))
	, retry_counts_(std::move(other.retry_counts_))
{
}

auto dag_scheduler::operator=(dag_scheduler&& other) noexcept -> dag_scheduler&
{
	if (this != &other)
	{
		pool_ = std::move(other.pool_);
		config_ = std::move(other.config_);
		jobs_ = std::move(other.jobs_);
		dependencies_ = std::move(other.dependencies_);
		dependents_ = std::move(other.dependents_);
		executing_.store(other.executing_.load());
		cancelled_.store(other.cancelled_.load());
		running_count_.store(other.running_count_.load());
		execution_start_time_ = other.execution_start_time_;
		first_error_ = std::move(other.first_error_);
		retry_counts_ = std::move(other.retry_counts_);
	}
	return *this;
}

// ============================================
// Job Management
// ============================================

auto dag_scheduler::add_job(std::unique_ptr<dag_job> j) -> job_id
{
	std::unique_lock lock(mutex_);

	auto id = j->get_dag_id();

	// Add dependencies from the job
	for (const auto& dep : j->get_dependencies())
	{
		dependencies_[id].push_back(dep);
		dependents_[dep].push_back(id);
	}

	// Check for cycles if configured
	if (config_.detect_cycles && detect_cycle())
	{
		// Rollback
		dependencies_.erase(id);
		for (const auto& dep : j->get_dependencies())
		{
			auto& deps = dependents_[dep];
			deps.erase(std::remove(deps.begin(), deps.end(), id), deps.end());
		}
		return INVALID_JOB_ID;
	}

	jobs_[id] = std::move(j);
	return id;
}

auto dag_scheduler::add_job(dag_job_builder&& builder) -> job_id
{
	return add_job(builder.build());
}

auto dag_scheduler::add_dependency(job_id dependent, job_id dependency) -> common::VoidResult
{
	std::unique_lock lock(mutex_);

	// Check if jobs exist
	if (jobs_.find(dependent) == jobs_.end())
	{
		return make_error_result(error_code::job_invalid,
		                         "Dependent job not found: " + std::to_string(dependent));
	}
	if (jobs_.find(dependency) == jobs_.end())
	{
		return make_error_result(error_code::job_invalid,
		                         "Dependency job not found: " + std::to_string(dependency));
	}

	// Add dependency
	dependencies_[dependent].push_back(dependency);
	dependents_[dependency].push_back(dependent);

	// Check for cycles
	if (config_.detect_cycles && detect_cycle())
	{
		// Rollback
		dependencies_[dependent].pop_back();
		dependents_[dependency].pop_back();
		return make_error_result(error_code::invalid_argument,
		                         "Adding dependency would create a cycle");
	}

	// Update job's dependencies
	jobs_[dependent]->add_dependency(dependency);

	return common::ok();
}

auto dag_scheduler::remove_job(job_id id) -> common::VoidResult
{
	std::unique_lock lock(mutex_);

	auto it = jobs_.find(id);
	if (it == jobs_.end())
	{
		return make_error_result(error_code::job_invalid,
		                         "Job not found: " + std::to_string(id));
	}

	auto state = it->second->get_state();
	if (state == dag_job_state::running)
	{
		return make_error_result(error_code::job_invalid,
		                         "Cannot remove running job: " + std::to_string(id));
	}

	// Remove from dependency graphs
	dependencies_.erase(id);
	dependents_.erase(id);

	// Remove from other jobs' dependencies
	for (auto& [job_id, deps] : dependencies_)
	{
		deps.erase(std::remove(deps.begin(), deps.end(), id), deps.end());
	}
	for (auto& [job_id, deps] : dependents_)
	{
		deps.erase(std::remove(deps.begin(), deps.end(), id), deps.end());
	}

	jobs_.erase(it);
	return common::ok();
}

// ============================================
// Execution Control
// ============================================

auto dag_scheduler::execute_all() -> std::future<common::VoidResult>
{
	auto promise = std::make_shared<std::promise<common::VoidResult>>();
	auto future = promise->get_future();

	// Start execution in a new thread
	std::thread([this, promise]() {
		{
			std::unique_lock lock(mutex_);
			if (executing_.exchange(true))
			{
				promise->set_value(make_error_result(error_code::thread_already_running,
				                                     "Execution already in progress"));
				return;
			}

			cancelled_.store(false);
			first_error_.reset();
			execution_start_time_ = std::chrono::steady_clock::now();

			// Mark all jobs with satisfied dependencies as ready
			for (auto& [id, job] : jobs_)
			{
				if (job->get_state() == dag_job_state::pending)
				{
					if (are_dependencies_satisfied(id))
					{
						job->set_state(dag_job_state::ready);
					}
				}
			}
		}

		// Schedule ready jobs
		schedule_ready_jobs();

		// Wait for completion
		auto result = wait();
		executing_.store(false);
		promise->set_value(result);
	}).detach();

	return future;
}

auto dag_scheduler::execute(job_id target) -> std::future<common::VoidResult>
{
	auto promise = std::make_shared<std::promise<common::VoidResult>>();
	auto future = promise->get_future();

	std::thread([this, target, promise]() {
		{
			std::unique_lock lock(mutex_);

			auto it = jobs_.find(target);
			if (it == jobs_.end())
			{
				promise->set_value(make_error_result(error_code::job_invalid,
				                                     "Target job not found: " + std::to_string(target)));
				return;
			}

			if (executing_.exchange(true))
			{
				promise->set_value(make_error_result(error_code::thread_already_running,
				                                     "Execution already in progress"));
				return;
			}

			cancelled_.store(false);
			first_error_.reset();
			execution_start_time_ = std::chrono::steady_clock::now();

			// Get all dependencies of target (transitively)
			std::unordered_set<job_id> needed_jobs;
			std::stack<job_id> to_visit;
			to_visit.push(target);

			while (!to_visit.empty())
			{
				auto current = to_visit.top();
				to_visit.pop();

				if (needed_jobs.count(current) > 0)
				{
					continue;
				}
				needed_jobs.insert(current);

				auto deps_it = dependencies_.find(current);
				if (deps_it != dependencies_.end())
				{
					for (const auto& dep : deps_it->second)
					{
						to_visit.push(dep);
					}
				}
			}

			// Mark needed jobs as ready if dependencies satisfied
			for (const auto& id : needed_jobs)
			{
				auto job_it = jobs_.find(id);
				if (job_it != jobs_.end() && job_it->second->get_state() == dag_job_state::pending)
				{
					bool deps_in_needed = true;
					auto deps_it = dependencies_.find(id);
					if (deps_it != dependencies_.end())
					{
						for (const auto& dep : deps_it->second)
						{
							if (needed_jobs.count(dep) == 0)
							{
								deps_in_needed = false;
								break;
							}
						}
					}

					if (deps_in_needed && are_dependencies_satisfied(id))
					{
						job_it->second->set_state(dag_job_state::ready);
					}
				}
			}
		}

		schedule_ready_jobs();

		auto result = wait();
		executing_.store(false);
		promise->set_value(result);
	}).detach();

	return future;
}

auto dag_scheduler::cancel_all() -> void
{
	std::unique_lock lock(mutex_);
	cancelled_.store(true);

	for (auto& [id, job] : jobs_)
	{
		auto state = job->get_state();
		if (state == dag_job_state::pending || state == dag_job_state::ready)
		{
			job->set_state(dag_job_state::cancelled);
			if (config_.state_callback)
			{
				config_.state_callback(id, state, dag_job_state::cancelled);
			}
		}
	}

	completion_cv_.notify_all();
}

auto dag_scheduler::wait() -> common::VoidResult
{
	std::shared_lock lock(mutex_);

	completion_cv_.wait(lock, [this]() {
		return is_execution_complete() || cancelled_.load();
	});

	if (first_error_)
	{
		return common::VoidResult(*first_error_);
	}

	return common::ok();
}

auto dag_scheduler::reset() -> common::VoidResult
{
	std::unique_lock lock(mutex_);

	if (executing_.load())
	{
		return make_error_result(error_code::thread_already_running,
		                         "Cannot reset while execution is in progress");
	}

	jobs_.clear();
	dependencies_.clear();
	dependents_.clear();
	retry_counts_.clear();
	first_error_.reset();
	cancelled_.store(false);

	return common::ok();
}

// ============================================
// Query
// ============================================

auto dag_scheduler::get_job_info(job_id id) const -> std::optional<dag_job_info>
{
	std::shared_lock lock(mutex_);

	auto it = jobs_.find(id);
	if (it == jobs_.end())
	{
		return std::nullopt;
	}

	auto info = it->second->get_info();

	// Add dependents from our tracking
	auto dep_it = dependents_.find(id);
	if (dep_it != dependents_.end())
	{
		info.dependents = dep_it->second;
	}

	return info;
}

auto dag_scheduler::get_all_jobs() const -> std::vector<dag_job_info>
{
	std::shared_lock lock(mutex_);

	std::vector<dag_job_info> result;
	result.reserve(jobs_.size());

	for (const auto& [id, job] : jobs_)
	{
		auto info = job->get_info();
		auto dep_it = dependents_.find(id);
		if (dep_it != dependents_.end())
		{
			info.dependents = dep_it->second;
		}
		result.push_back(std::move(info));
	}

	return result;
}

auto dag_scheduler::get_jobs_in_state(dag_job_state state) const -> std::vector<dag_job_info>
{
	std::shared_lock lock(mutex_);

	std::vector<dag_job_info> result;

	for (const auto& [id, job] : jobs_)
	{
		if (job->get_state() == state)
		{
			auto info = job->get_info();
			auto dep_it = dependents_.find(id);
			if (dep_it != dependents_.end())
			{
				info.dependents = dep_it->second;
			}
			result.push_back(std::move(info));
		}
	}

	return result;
}

auto dag_scheduler::get_ready_jobs() const -> std::vector<job_id>
{
	std::shared_lock lock(mutex_);

	std::vector<job_id> result;

	for (const auto& [id, job] : jobs_)
	{
		if (job->get_state() == dag_job_state::ready)
		{
			result.push_back(id);
		}
		else if (job->get_state() == dag_job_state::pending && are_dependencies_satisfied(id))
		{
			result.push_back(id);
		}
	}

	return result;
}

auto dag_scheduler::has_cycles() const -> bool
{
	std::shared_lock lock(mutex_);
	return detect_cycle();
}

auto dag_scheduler::get_execution_order() const -> std::vector<job_id>
{
	std::shared_lock lock(mutex_);
	return topological_sort();
}

// ============================================
// Visualization
// ============================================

auto dag_scheduler::to_dot() const -> std::string
{
	std::shared_lock lock(mutex_);

	std::ostringstream ss;
	ss << "digraph DAG {\n";
	ss << "    rankdir=TB;\n";
	ss << "    node [shape=box, style=filled];\n\n";

	// Nodes
	for (const auto& [id, job] : jobs_)
	{
		auto state = job->get_state();
		ss << "    " << id << " [label=\"" << job->get_name()
		   << "\\n(" << dag_job_state_to_string(state) << ")\""
		   << ", fillcolor=\"" << get_state_color(state) << "\"];\n";
	}

	ss << "\n";

	// Edges
	for (const auto& [dependent, deps] : dependencies_)
	{
		for (const auto& dependency : deps)
		{
			ss << "    " << dependency << " -> " << dependent << ";\n";
		}
	}

	ss << "}\n";

	return ss.str();
}

auto dag_scheduler::to_json() const -> std::string
{
	std::shared_lock lock(mutex_);

	std::ostringstream ss;
	ss << "{\n";
	ss << "  \"jobs\": [\n";

	bool first_job = true;
	for (const auto& [id, job] : jobs_)
	{
		if (!first_job) ss << ",\n";
		first_job = false;

		auto info = job->get_info();
		ss << "    {\n";
		ss << "      \"id\": " << id << ",\n";
		ss << "      \"name\": \"" << info.name << "\",\n";
		ss << "      \"state\": \"" << dag_job_state_to_string(info.state) << "\",\n";
		ss << "      \"dependencies\": [";

		bool first_dep = true;
		for (const auto& dep : info.dependencies)
		{
			if (!first_dep) ss << ", ";
			first_dep = false;
			ss << dep;
		}
		ss << "],\n";

		ss << "      \"dependents\": [";
		auto dep_it = dependents_.find(id);
		if (dep_it != dependents_.end())
		{
			first_dep = true;
			for (const auto& dep : dep_it->second)
			{
				if (!first_dep) ss << ", ";
				first_dep = false;
				ss << dep;
			}
		}
		ss << "],\n";

		auto wait_ms = info.get_wait_time().count();
		auto exec_ms = info.get_execution_time().count();
		ss << "      \"wait_time_ms\": " << wait_ms << ",\n";
		ss << "      \"execution_time_ms\": " << exec_ms;

		if (info.error_message)
		{
			ss << ",\n      \"error\": \"" << *info.error_message << "\"";
		}

		ss << "\n    }";
	}

	ss << "\n  ],\n";

	// Add statistics
	auto stats = get_stats();
	ss << "  \"stats\": {\n";
	ss << "    \"total_jobs\": " << stats.total_jobs << ",\n";
	ss << "    \"completed_jobs\": " << stats.completed_jobs << ",\n";
	ss << "    \"failed_jobs\": " << stats.failed_jobs << ",\n";
	ss << "    \"pending_jobs\": " << stats.pending_jobs << ",\n";
	ss << "    \"running_jobs\": " << stats.running_jobs << ",\n";
	ss << "    \"skipped_jobs\": " << stats.skipped_jobs << ",\n";
	ss << "    \"cancelled_jobs\": " << stats.cancelled_jobs << "\n";
	ss << "  }\n";

	ss << "}\n";

	return ss.str();
}

// ============================================
// Statistics
// ============================================

auto dag_scheduler::get_stats() const -> dag_stats
{
	// Note: This is called from to_json which already holds the lock
	// For standalone calls, we need to be careful about deadlock

	dag_stats stats;
	stats.total_jobs = jobs_.size();

	for (const auto& [id, job] : jobs_)
	{
		switch (job->get_state())
		{
			case dag_job_state::completed: ++stats.completed_jobs; break;
			case dag_job_state::failed:    ++stats.failed_jobs;    break;
			case dag_job_state::pending:
			case dag_job_state::ready:     ++stats.pending_jobs;   break;
			case dag_job_state::running:   ++stats.running_jobs;   break;
			case dag_job_state::skipped:   ++stats.skipped_jobs;   break;
			case dag_job_state::cancelled: ++stats.cancelled_jobs; break;
		}
	}

	if (executing_.load())
	{
		stats.total_execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - execution_start_time_);
	}

	return stats;
}

// ============================================
// Internal Methods
// ============================================

auto dag_scheduler::on_job_completed(job_id id) -> void
{
	std::unique_lock lock(mutex_);

	auto it = jobs_.find(id);
	if (it == jobs_.end())
	{
		return;
	}

	auto old_state = it->second->get_state();
	it->second->set_state(dag_job_state::completed);
	it->second->record_end_time();
	--running_count_;

	if (config_.state_callback)
	{
		config_.state_callback(id, old_state, dag_job_state::completed);
	}

	if (config_.completion_callback)
	{
		config_.completion_callback(id);
	}

	// Check and update dependents
	auto dep_it = dependents_.find(id);
	if (dep_it != dependents_.end())
	{
		for (const auto& dependent : dep_it->second)
		{
			auto job_it = jobs_.find(dependent);
			if (job_it != jobs_.end() && job_it->second->get_state() == dag_job_state::pending)
			{
				if (are_dependencies_satisfied(dependent))
				{
					job_it->second->set_state(dag_job_state::ready);
					if (config_.state_callback)
					{
						config_.state_callback(dependent, dag_job_state::pending, dag_job_state::ready);
					}
				}
			}
		}
	}

	lock.unlock();

	// Schedule more jobs
	schedule_ready_jobs();

	// Notify waiters
	completion_cv_.notify_all();
}

auto dag_scheduler::on_job_failed(job_id id, const std::string& error) -> void
{
	std::unique_lock lock(mutex_);

	auto it = jobs_.find(id);
	if (it == jobs_.end())
	{
		return;
	}

	it->second->set_error_message(error);
	it->second->record_end_time();

	// Handle based on failure policy
	switch (config_.failure_policy)
	{
		case dag_failure_policy::retry:
		{
			auto& retry_count = retry_counts_[id];
			if (retry_count < config_.max_retries)
			{
				++retry_count;
				// Reset state to ready for retry
				it->second->set_state(dag_job_state::ready);
				--running_count_;

				if (config_.state_callback)
				{
					config_.state_callback(id, dag_job_state::running, dag_job_state::ready);
				}

				lock.unlock();

				// Wait before retry
				std::this_thread::sleep_for(config_.retry_delay);

				schedule_ready_jobs();
				return;
			}
			// Fall through to fail_fast if max retries exceeded
			[[fallthrough]];
		}

		case dag_failure_policy::fallback:
		{
			if (config_.failure_policy == dag_failure_policy::fallback && it->second->has_fallback())
			{
				// Try fallback
				auto fallback = it->second->get_fallback();
				auto result = fallback();
				if (result.is_ok())
				{
					// Fallback succeeded
					it->second->set_state(dag_job_state::completed);
					--running_count_;
					if (config_.state_callback)
					{
						config_.state_callback(id, dag_job_state::running, dag_job_state::completed);
					}
					lock.unlock();
					schedule_ready_jobs();
					completion_cv_.notify_all();
					return;
				}
			}
			[[fallthrough]];
		}

		case dag_failure_policy::fail_fast:
		{
			auto old_state = it->second->get_state();
			it->second->set_state(dag_job_state::failed);
			--running_count_;

			if (!first_error_)
			{
				first_error_ = to_error_info(error_code::job_execution_failed, error);
			}

			if (config_.state_callback)
			{
				config_.state_callback(id, old_state, dag_job_state::failed);
			}

			if (config_.error_callback)
			{
				config_.error_callback(id, error);
			}

			// Cancel all dependents
			cancel_dependents(id);
			break;
		}

		case dag_failure_policy::continue_others:
		{
			auto old_state = it->second->get_state();
			it->second->set_state(dag_job_state::failed);
			--running_count_;

			if (!first_error_)
			{
				first_error_ = to_error_info(error_code::job_execution_failed, error);
			}

			if (config_.state_callback)
			{
				config_.state_callback(id, old_state, dag_job_state::failed);
			}

			if (config_.error_callback)
			{
				config_.error_callback(id, error);
			}

			// Skip dependents but continue others
			skip_dependents(id);
			break;
		}
	}

	lock.unlock();
	schedule_ready_jobs();
	completion_cv_.notify_all();
}

auto dag_scheduler::schedule_ready_jobs() -> void
{
	if (cancelled_.load())
	{
		return;
	}

	std::vector<job_id> ready_jobs;

	{
		std::shared_lock lock(mutex_);
		for (const auto& [id, job] : jobs_)
		{
			if (job->get_state() == dag_job_state::ready)
			{
				ready_jobs.push_back(id);
			}
		}
	}

	for (const auto& id : ready_jobs)
	{
		if (cancelled_.load())
		{
			break;
		}

		if (config_.execute_in_parallel)
		{
			execute_job(id);
		}
		else
		{
			// Execute sequentially
			execute_job(id);
			// Wait for completion before next job
			std::shared_lock lock(mutex_);
			completion_cv_.wait(lock, [this, id]() {
				auto it = jobs_.find(id);
				if (it == jobs_.end()) return true;
				auto state = it->second->get_state();
				return state != dag_job_state::running;
			});
		}
	}
}

auto dag_scheduler::execute_job(job_id id) -> void
{
	std::unique_lock lock(mutex_);

	auto it = jobs_.find(id);
	if (it == jobs_.end())
	{
		return;
	}

	if (!it->second->try_transition_state(dag_job_state::ready, dag_job_state::running))
	{
		return;  // Already running or in terminal state
	}

	it->second->record_start_time();
	++running_count_;

	if (config_.state_callback)
	{
		config_.state_callback(id, dag_job_state::ready, dag_job_state::running);
	}

	lock.unlock();

	// Create a callback job to execute the dag_job
	auto wrapper_job = std::make_unique<callback_job>(
		[this, id]() -> common::VoidResult {
			std::shared_lock slock(mutex_);
			auto job_it = jobs_.find(id);
			if (job_it == jobs_.end())
			{
				return make_error_result(error_code::job_invalid, "Job not found");
			}

			auto& dag_job_ptr = job_it->second;
			slock.unlock();

			auto result = dag_job_ptr->do_work();

			if (result.is_ok())
			{
				on_job_completed(id);
			}
			else
			{
				on_job_failed(id, result.error().message);
			}

			return result;
		},
		"dag_job_" + std::to_string(id)
	);

	pool_->enqueue(std::move(wrapper_job));
}

auto dag_scheduler::topological_sort() const -> std::vector<job_id>
{
	std::vector<job_id> result;
	std::unordered_map<job_id, int> in_degree;
	std::queue<job_id> ready_queue;

	// Initialize in-degrees
	for (const auto& [id, _] : jobs_)
	{
		in_degree[id] = 0;
	}

	for (const auto& [id, deps] : dependencies_)
	{
		in_degree[id] = static_cast<int>(deps.size());
	}

	// Find initial ready jobs (in-degree 0)
	for (const auto& [id, degree] : in_degree)
	{
		if (degree == 0)
		{
			ready_queue.push(id);
		}
	}

	// Process queue
	while (!ready_queue.empty())
	{
		auto current = ready_queue.front();
		ready_queue.pop();
		result.push_back(current);

		// Reduce in-degree of dependents
		auto it = dependents_.find(current);
		if (it != dependents_.end())
		{
			for (const auto& dependent : it->second)
			{
				--in_degree[dependent];
				if (in_degree[dependent] == 0)
				{
					ready_queue.push(dependent);
				}
			}
		}
	}

	// If result doesn't contain all jobs, there's a cycle
	if (result.size() != jobs_.size())
	{
		return {};  // Cycle detected
	}

	return result;
}

auto dag_scheduler::detect_cycle() const -> bool
{
	enum class Color { White, Gray, Black };

	std::unordered_map<job_id, Color> color;
	for (const auto& [id, _] : jobs_)
	{
		color[id] = Color::White;
	}

	std::function<bool(job_id)> dfs = [&](job_id id) -> bool {
		color[id] = Color::Gray;

		auto it = dependencies_.find(id);
		if (it != dependencies_.end())
		{
			for (const auto& dep : it->second)
			{
				if (color[dep] == Color::Gray)
				{
					return true;  // Back edge = cycle
				}
				if (color[dep] == Color::White && dfs(dep))
				{
					return true;
				}
			}
		}

		color[id] = Color::Black;
		return false;
	};

	for (const auto& [id, _] : jobs_)
	{
		if (color[id] == Color::White && dfs(id))
		{
			return true;
		}
	}

	return false;
}

auto dag_scheduler::are_dependencies_satisfied(job_id id) const -> bool
{
	auto it = dependencies_.find(id);
	if (it == dependencies_.end() || it->second.empty())
	{
		return true;  // No dependencies
	}

	for (const auto& dep : it->second)
	{
		auto job_it = jobs_.find(dep);
		if (job_it == jobs_.end())
		{
			return false;  // Dependency not found
		}
		if (job_it->second->get_state() != dag_job_state::completed)
		{
			return false;  // Dependency not completed
		}
	}

	return true;
}

auto dag_scheduler::skip_dependents(job_id failed_id) -> void
{
	auto it = dependents_.find(failed_id);
	if (it == dependents_.end())
	{
		return;
	}

	std::queue<job_id> to_skip;
	for (const auto& dep : it->second)
	{
		to_skip.push(dep);
	}

	while (!to_skip.empty())
	{
		auto current = to_skip.front();
		to_skip.pop();

		auto job_it = jobs_.find(current);
		if (job_it == jobs_.end())
		{
			continue;
		}

		auto old_state = job_it->second->get_state();
		if (old_state == dag_job_state::pending || old_state == dag_job_state::ready)
		{
			job_it->second->set_state(dag_job_state::skipped);
			if (config_.state_callback)
			{
				config_.state_callback(current, old_state, dag_job_state::skipped);
			}

			// Also skip dependents of skipped job
			auto dep_it = dependents_.find(current);
			if (dep_it != dependents_.end())
			{
				for (const auto& dep : dep_it->second)
				{
					to_skip.push(dep);
				}
			}
		}
	}
}

auto dag_scheduler::cancel_dependents(job_id failed_id) -> void
{
	auto it = dependents_.find(failed_id);
	if (it == dependents_.end())
	{
		return;
	}

	std::queue<job_id> to_cancel;
	for (const auto& dep : it->second)
	{
		to_cancel.push(dep);
	}

	while (!to_cancel.empty())
	{
		auto current = to_cancel.front();
		to_cancel.pop();

		auto job_it = jobs_.find(current);
		if (job_it == jobs_.end())
		{
			continue;
		}

		auto old_state = job_it->second->get_state();
		if (old_state == dag_job_state::pending || old_state == dag_job_state::ready)
		{
			job_it->second->set_state(dag_job_state::cancelled);
			if (config_.state_callback)
			{
				config_.state_callback(current, old_state, dag_job_state::cancelled);
			}

			// Also cancel dependents of cancelled job
			auto dep_it = dependents_.find(current);
			if (dep_it != dependents_.end())
			{
				for (const auto& dep : dep_it->second)
				{
					to_cancel.push(dep);
				}
			}
		}
	}
}

auto dag_scheduler::get_state_color(dag_job_state state) -> std::string
{
	switch (state)
	{
		case dag_job_state::pending:   return "#FFFFFF";  // White
		case dag_job_state::ready:     return "#87CEEB";  // Light blue
		case dag_job_state::running:   return "#FFFF00";  // Yellow
		case dag_job_state::completed: return "#90EE90";  // Light green
		case dag_job_state::failed:    return "#FF6B6B";  // Light red
		case dag_job_state::cancelled: return "#D3D3D3";  // Light gray
		case dag_job_state::skipped:   return "#FFA500";  // Orange
		default:                       return "#FFFFFF";
	}
}

auto dag_scheduler::is_execution_complete() const -> bool
{
	for (const auto& [id, job] : jobs_)
	{
		auto state = job->get_state();
		if (state == dag_job_state::pending ||
		    state == dag_job_state::ready ||
		    state == dag_job_state::running)
		{
			return false;
		}
	}
	return true;
}

} // namespace kcenon::thread
