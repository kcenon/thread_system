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

#include <kcenon/thread/stealing/numa_work_stealer.h>

#include <kcenon/thread/lockfree/work_stealing_deque.h>

#include <algorithm>
#include <chrono>
#include <thread>

namespace kcenon::thread
{

numa_work_stealer::numa_work_stealer(std::size_t worker_count,
                                     deque_accessor_fn deque_accessor,
                                     cpu_accessor_fn cpu_accessor,
                                     enhanced_work_stealing_config config)
	: worker_count_(worker_count)
	, deque_accessor_(std::move(deque_accessor))
	, cpu_accessor_(std::move(cpu_accessor))
	, config_(config)
	, topology_(numa_topology::detect())
	, rng_(std::random_device{}())
{
	if (config_.track_locality && worker_count_ > 0)
	{
		affinity_tracker_ =
			std::make_unique<work_affinity_tracker>(worker_count_, config_.locality_history_size);
	}

	steal_backoff_config backoff_config;
	backoff_config.strategy = config_.backoff_strategy;
	backoff_config.initial_backoff = config_.initial_backoff;
	backoff_config.max_backoff = config_.max_backoff;
	backoff_config.multiplier = config_.backoff_multiplier;
	backoff_calculator_ = std::make_unique<backoff_calculator>(backoff_config);
}

auto numa_work_stealer::steal_for(std::size_t worker_id) -> job*
{
	if (!config_.enabled || worker_count_ <= 1)
	{
		return nullptr;
	}

	auto start_time = std::chrono::steady_clock::now();
	job* stolen_job = nullptr;

	auto victims = select_victims(worker_id, config_.max_steal_attempts);

	std::size_t attempt = 0;
	for (auto victim_id : victims)
	{
		if (config_.collect_statistics)
		{
			stats_.steal_attempts.fetch_add(1, std::memory_order_relaxed);
		}

		auto* victim_deque = deque_accessor_(victim_id);
		if (victim_deque == nullptr)
		{
			continue;
		}

		auto result = victim_deque->steal();
		if (result.has_value())
		{
			stolen_job = result.value();

			if (config_.collect_statistics)
			{
				stats_.successful_steals.fetch_add(1, std::memory_order_relaxed);
				stats_.jobs_stolen.fetch_add(1, std::memory_order_relaxed);

				if (workers_on_same_node(worker_id, victim_id))
				{
					stats_.same_node_steals.fetch_add(1, std::memory_order_relaxed);
				}
				else
				{
					stats_.cross_node_steals.fetch_add(1, std::memory_order_relaxed);
				}
			}

			record_steal(worker_id, victim_id);
			break;
		}
		else
		{
			if (config_.collect_statistics)
			{
				stats_.failed_steals.fetch_add(1, std::memory_order_relaxed);
			}

			// Apply backoff after failed steal
			if (++attempt < victims.size())
			{
				auto backoff_start = std::chrono::steady_clock::now();
				auto delay = backoff_calculator_->calculate(attempt);
				std::this_thread::sleep_for(delay);

				if (config_.collect_statistics)
				{
					auto backoff_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
										  std::chrono::steady_clock::now() - backoff_start)
					                      .count();
					stats_.total_backoff_time_ns.fetch_add(static_cast<std::uint64_t>(backoff_ns),
					                                       std::memory_order_relaxed);
				}
			}
		}
	}

	if (config_.collect_statistics)
	{
		auto elapsed_ns =
			std::chrono::duration_cast<std::chrono::nanoseconds>(
				std::chrono::steady_clock::now() - start_time)
				.count();
		stats_.total_steal_time_ns.fetch_add(static_cast<std::uint64_t>(elapsed_ns),
		                                     std::memory_order_relaxed);
	}

	return stolen_job;
}

auto numa_work_stealer::steal_batch_for(std::size_t worker_id, std::size_t max_count)
	-> std::vector<job*>
{
	if (!config_.enabled || worker_count_ <= 1 || max_count == 0)
	{
		return {};
	}

	auto start_time = std::chrono::steady_clock::now();
	std::vector<job*> stolen_jobs;

	auto victims = select_victims(worker_id, config_.max_steal_attempts);

	std::size_t attempt = 0;
	for (auto victim_id : victims)
	{
		if (config_.collect_statistics)
		{
			stats_.steal_attempts.fetch_add(1, std::memory_order_relaxed);
		}

		auto* victim_deque = deque_accessor_(victim_id);
		if (victim_deque == nullptr)
		{
			continue;
		}

		auto queue_size = victim_deque->size();
		if (queue_size == 0)
		{
			if (config_.collect_statistics)
			{
				stats_.failed_steals.fetch_add(1, std::memory_order_relaxed);
			}
			continue;
		}

		auto batch_size = calculate_batch_size(queue_size);
		batch_size = std::min(batch_size, max_count);

		auto batch = victim_deque->steal_batch(batch_size);
		if (!batch.empty())
		{
			stolen_jobs = std::move(batch);

			if (config_.collect_statistics)
			{
				stats_.successful_steals.fetch_add(1, std::memory_order_relaxed);
				stats_.jobs_stolen.fetch_add(stolen_jobs.size(), std::memory_order_relaxed);
				stats_.batch_steals.fetch_add(1, std::memory_order_relaxed);
				stats_.total_batch_size.fetch_add(stolen_jobs.size(), std::memory_order_relaxed);

				if (workers_on_same_node(worker_id, victim_id))
				{
					stats_.same_node_steals.fetch_add(1, std::memory_order_relaxed);
				}
				else
				{
					stats_.cross_node_steals.fetch_add(1, std::memory_order_relaxed);
				}
			}

			record_steal(worker_id, victim_id);
			break;
		}
		else
		{
			if (config_.collect_statistics)
			{
				stats_.failed_steals.fetch_add(1, std::memory_order_relaxed);
			}

			// Apply backoff after failed steal
			if (++attempt < victims.size())
			{
				auto backoff_start = std::chrono::steady_clock::now();
				auto delay = backoff_calculator_->calculate(attempt);
				std::this_thread::sleep_for(delay);

				if (config_.collect_statistics)
				{
					auto backoff_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
										  std::chrono::steady_clock::now() - backoff_start)
					                      .count();
					stats_.total_backoff_time_ns.fetch_add(static_cast<std::uint64_t>(backoff_ns),
					                                       std::memory_order_relaxed);
				}
			}
		}
	}

	if (config_.collect_statistics)
	{
		auto elapsed_ns =
			std::chrono::duration_cast<std::chrono::nanoseconds>(
				std::chrono::steady_clock::now() - start_time)
				.count();
		stats_.total_steal_time_ns.fetch_add(static_cast<std::uint64_t>(elapsed_ns),
		                                     std::memory_order_relaxed);
	}

	return stolen_jobs;
}

auto numa_work_stealer::get_stats() const -> const work_stealing_stats&
{
	return stats_;
}

auto numa_work_stealer::get_stats_snapshot() const -> work_stealing_stats_snapshot
{
	return stats_.snapshot();
}

void numa_work_stealer::reset_stats()
{
	stats_.reset();
}

auto numa_work_stealer::get_topology() const -> const numa_topology&
{
	return topology_;
}

auto numa_work_stealer::get_config() const -> const enhanced_work_stealing_config&
{
	return config_;
}

void numa_work_stealer::set_config(const enhanced_work_stealing_config& config)
{
	config_ = config;

	// Update backoff calculator
	steal_backoff_config backoff_config;
	backoff_config.strategy = config_.backoff_strategy;
	backoff_config.initial_backoff = config_.initial_backoff;
	backoff_config.max_backoff = config_.max_backoff;
	backoff_config.multiplier = config_.backoff_multiplier;
	backoff_calculator_->set_config(backoff_config);

	// Update affinity tracker if needed
	if (config_.track_locality && !affinity_tracker_ && worker_count_ > 0)
	{
		affinity_tracker_ =
			std::make_unique<work_affinity_tracker>(worker_count_, config_.locality_history_size);
	}
	else if (!config_.track_locality)
	{
		affinity_tracker_.reset();
	}
}

auto numa_work_stealer::select_victims(std::size_t requester_id, std::size_t count)
	-> std::vector<std::size_t>
{
	switch (config_.policy)
	{
	case enhanced_steal_policy::random:
		return select_victims_random(requester_id, count);

	case enhanced_steal_policy::round_robin:
		return select_victims_round_robin(requester_id, count);

	case enhanced_steal_policy::adaptive:
		return select_victims_adaptive(requester_id, count);

	case enhanced_steal_policy::numa_aware:
		return select_victims_numa_aware(requester_id, count);

	case enhanced_steal_policy::locality_aware:
		return select_victims_locality_aware(requester_id, count);

	case enhanced_steal_policy::hierarchical:
		return select_victims_hierarchical(requester_id, count);

	default:
		return select_victims_random(requester_id, count);
	}
}

auto numa_work_stealer::select_victims_random(std::size_t requester_id, std::size_t count)
	-> std::vector<std::size_t>
{
	std::vector<std::size_t> victims;
	victims.reserve(count);

	std::vector<std::size_t> candidates;
	candidates.reserve(worker_count_ - 1);

	for (std::size_t i = 0; i < worker_count_; ++i)
	{
		if (i != requester_id)
		{
			candidates.push_back(i);
		}
	}

	// Shuffle and take first 'count' elements
	std::shuffle(candidates.begin(), candidates.end(), rng_);

	auto num_victims = std::min(count, candidates.size());
	victims.insert(victims.end(), candidates.begin(), candidates.begin() + num_victims);

	return victims;
}

auto numa_work_stealer::select_victims_round_robin(std::size_t requester_id, std::size_t count)
	-> std::vector<std::size_t>
{
	std::vector<std::size_t> victims;
	victims.reserve(count);

	auto start_index = round_robin_index_.fetch_add(1, std::memory_order_relaxed) % worker_count_;

	for (std::size_t i = 0; i < worker_count_ && victims.size() < count; ++i)
	{
		auto victim_id = (start_index + i) % worker_count_;
		if (victim_id != requester_id)
		{
			victims.push_back(victim_id);
		}
	}

	return victims;
}

auto numa_work_stealer::select_victims_adaptive(std::size_t requester_id, std::size_t count)
	-> std::vector<std::size_t>
{
	// Score workers by queue size (larger queues are better targets)
	std::vector<std::pair<std::size_t, std::size_t>> scored_workers;
	scored_workers.reserve(worker_count_ - 1);

	for (std::size_t i = 0; i < worker_count_; ++i)
	{
		if (i == requester_id)
		{
			continue;
		}

		auto* deque = deque_accessor_(i);
		auto queue_size = deque ? deque->size() : 0;
		scored_workers.emplace_back(i, queue_size);
	}

	// Sort by queue size (descending)
	std::sort(scored_workers.begin(),
	          scored_workers.end(),
	          [](const auto& a, const auto& b) { return a.second > b.second; });

	// Take top 'count' workers with non-empty queues
	std::vector<std::size_t> victims;
	victims.reserve(count);

	for (const auto& [worker_id, queue_size] : scored_workers)
	{
		if (victims.size() >= count)
		{
			break;
		}
		if (queue_size > 0)
		{
			victims.push_back(worker_id);
		}
	}

	// If not enough with non-empty queues, add some randomly
	if (victims.size() < count)
	{
		for (const auto& [worker_id, queue_size] : scored_workers)
		{
			if (victims.size() >= count)
			{
				break;
			}
			if (std::find(victims.begin(), victims.end(), worker_id) == victims.end())
			{
				victims.push_back(worker_id);
			}
		}
	}

	return victims;
}

auto numa_work_stealer::select_victims_numa_aware(std::size_t requester_id, std::size_t count)
	-> std::vector<std::size_t>
{
	if (!config_.numa_aware || !topology_.is_numa_available())
	{
		return select_victims_adaptive(requester_id, count);
	}

	// Get requester's NUMA node
	int requester_cpu = get_worker_cpu(requester_id);
	int requester_node = topology_.get_node_for_cpu(requester_cpu);

	// Score workers: same node workers get higher priority
	std::vector<std::pair<std::size_t, double>> scored_workers;
	scored_workers.reserve(worker_count_ - 1);

	for (std::size_t i = 0; i < worker_count_; ++i)
	{
		if (i == requester_id)
		{
			continue;
		}

		int victim_cpu = get_worker_cpu(i);
		int victim_node = topology_.get_node_for_cpu(victim_cpu);

		auto* deque = deque_accessor_(i);
		auto queue_size = deque ? static_cast<double>(deque->size()) : 0.0;

		// Apply NUMA penalty for cross-node workers
		double score = queue_size;
		if (requester_node != victim_node && requester_node >= 0 && victim_node >= 0)
		{
			score /= config_.numa_penalty_factor;
		}

		scored_workers.emplace_back(i, score);
	}

	// Sort by score (descending)
	std::sort(scored_workers.begin(),
	          scored_workers.end(),
	          [](const auto& a, const auto& b) { return a.second > b.second; });

	std::vector<std::size_t> victims;
	victims.reserve(count);

	for (const auto& [worker_id, score] : scored_workers)
	{
		if (victims.size() >= count)
		{
			break;
		}
		victims.push_back(worker_id);
	}

	return victims;
}

auto numa_work_stealer::select_victims_locality_aware(std::size_t requester_id, std::size_t count)
	-> std::vector<std::size_t>
{
	if (!config_.track_locality || !affinity_tracker_)
	{
		return select_victims_adaptive(requester_id, count);
	}

	// Get preferred victims from affinity tracker
	auto preferred = affinity_tracker_->get_preferred_victims(requester_id, count);

	// If not enough preferred victims, fill with adaptive selection
	if (preferred.size() < count)
	{
		auto additional = select_victims_adaptive(requester_id, count - preferred.size());
		for (auto victim_id : additional)
		{
			if (std::find(preferred.begin(), preferred.end(), victim_id) == preferred.end())
			{
				preferred.push_back(victim_id);
			}
			if (preferred.size() >= count)
			{
				break;
			}
		}
	}

	return preferred;
}

auto numa_work_stealer::select_victims_hierarchical(std::size_t requester_id, std::size_t count)
	-> std::vector<std::size_t>
{
	if (!config_.numa_aware || !topology_.is_numa_available())
	{
		return select_victims_adaptive(requester_id, count);
	}

	int requester_cpu = get_worker_cpu(requester_id);
	int requester_node = topology_.get_node_for_cpu(requester_cpu);

	// First: workers on same NUMA node
	std::vector<std::size_t> same_node_victims;
	// Second: workers on other NUMA nodes
	std::vector<std::size_t> other_node_victims;

	for (std::size_t i = 0; i < worker_count_; ++i)
	{
		if (i == requester_id)
		{
			continue;
		}

		int victim_cpu = get_worker_cpu(i);
		int victim_node = topology_.get_node_for_cpu(victim_cpu);

		if (victim_node == requester_node)
		{
			same_node_victims.push_back(i);
		}
		else
		{
			other_node_victims.push_back(i);
		}
	}

	// Shuffle within each group
	std::shuffle(same_node_victims.begin(), same_node_victims.end(), rng_);
	std::shuffle(other_node_victims.begin(), other_node_victims.end(), rng_);

	// Combine: same node first, then other nodes
	std::vector<std::size_t> victims;
	victims.reserve(count);

	for (auto victim_id : same_node_victims)
	{
		if (victims.size() >= count)
		{
			break;
		}
		victims.push_back(victim_id);
	}

	for (auto victim_id : other_node_victims)
	{
		if (victims.size() >= count)
		{
			break;
		}
		victims.push_back(victim_id);
	}

	return victims;
}

auto numa_work_stealer::calculate_batch_size(std::size_t victim_queue_size) const -> std::size_t
{
	if (!config_.adaptive_batch_size)
	{
		return config_.max_steal_batch;
	}

	// Adaptive batch size: steal at most half of victim's queue
	auto half_queue = victim_queue_size / 2;
	if (half_queue < config_.min_steal_batch)
	{
		return config_.min_steal_batch;
	}

	return std::min(half_queue, config_.max_steal_batch);
}

auto numa_work_stealer::get_worker_cpu(std::size_t worker_id) const -> int
{
	if (!cpu_accessor_)
	{
		return -1;
	}
	return cpu_accessor_(worker_id);
}

auto numa_work_stealer::workers_on_same_node(std::size_t worker_a, std::size_t worker_b) const
	-> bool
{
	if (!topology_.is_numa_available())
	{
		return true; // On single-node systems, all workers are on the same node
	}

	int cpu_a = get_worker_cpu(worker_a);
	int cpu_b = get_worker_cpu(worker_b);

	return topology_.is_same_node(cpu_a, cpu_b);
}

void numa_work_stealer::record_steal(std::size_t thief_id, std::size_t victim_id)
{
	if (config_.track_locality && affinity_tracker_)
	{
		affinity_tracker_->record_cooperation(thief_id, victim_id);
	}
}

} // namespace kcenon::thread
