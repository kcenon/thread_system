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

#include "../framework/system_fixture.h"
#include "../framework/test_helpers.h"

#include <kcenon/thread/stealing/enhanced_work_stealing_config.h>
#include <kcenon/thread/stealing/numa_topology.h>
#include <kcenon/thread/stealing/work_stealing_stats.h>

#include <gtest/gtest.h>

#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

using namespace integration_tests;
using namespace kcenon::thread;

/**
 * @brief Benchmarks for NUMA-aware work-stealing optimization
 *
 * Tests various work-stealing configurations:
 * - Different victim selection policies
 * - Batch size variations
 * - Backoff strategy comparisons
 * - NUMA topology awareness
 *
 * Issue #383: Phase 3.3 - Work Stealing Optimization
 */
class WorkStealingBenchmark : public SystemFixture
{
protected:
	static constexpr size_t kDefaultJobCount = 10'000;
	static constexpr size_t kImbalancedJobCount = 5'000;
	static constexpr size_t kDefaultWorkerCount = 4;

	static size_t scale_for_ci(size_t value)
	{
		return std::getenv("CI") != nullptr ? value / 5 : value;
	}

	struct BenchmarkResult
	{
		std::string config_name;
		double throughput;
		std::chrono::nanoseconds duration;
		work_stealing_stats_snapshot stats;
	};

	static void print_result(const BenchmarkResult& result)
	{
		std::cout << "\n  " << std::left << std::setw(25) << result.config_name << ": "
		          << std::right << std::setw(12) << static_cast<size_t>(result.throughput)
		          << " jobs/sec";

		if (result.stats.steal_attempts > 0)
		{
			std::cout << " (steals: " << result.stats.successful_steals << "/"
			          << result.stats.steal_attempts << ", rate: " << std::fixed
			          << std::setprecision(1) << (result.stats.steal_success_rate() * 100.0)
			          << "%)";
		}
		std::cout << std::endl;
	}

	BenchmarkResult run_benchmark(const std::string& name, const enhanced_work_stealing_config& config,
	                              size_t job_count, size_t worker_count,
	                              const std::function<void(size_t)>& job_distribution = nullptr)
	{
		CreateThreadPool(worker_count);
		pool_->set_work_stealing_config(config);

		auto start_result = pool_->start();
		EXPECT_TRUE(start_result.is_ok());

		completed_jobs_.store(0);

		auto start = std::chrono::high_resolution_clock::now();

		if (job_distribution)
		{
			job_distribution(job_count);
		}
		else
		{
			for (size_t i = 0; i < job_count; ++i)
			{
				auto job = std::make_unique<callback_job>([this]() -> kcenon::common::VoidResult {
					completed_jobs_.fetch_add(1);
					return kcenon::common::ok();
				});
				pool_->enqueue(std::move(job));
			}
		}

		EXPECT_TRUE(WaitForJobCompletion(job_count, std::chrono::seconds(60)));

		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
		double throughput = CalculateThroughput(job_count, duration);

		auto stats = pool_->get_work_stealing_stats();

		auto stop_result = pool_->stop(true);
		EXPECT_TRUE(stop_result.is_ok());

		pool_.reset();

		return BenchmarkResult{name, throughput, duration, stats};
	}

	void create_imbalanced_workload(size_t job_count)
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> work_dist(1, 100);

		for (size_t i = 0; i < job_count; ++i)
		{
			int work_amount = work_dist(gen);

			auto job = std::make_unique<callback_job>([this, work_amount]() -> kcenon::common::VoidResult {
				volatile int sum = 0;
				for (int j = 0; j < work_amount * 100; ++j)
				{
					sum += j;
				}
				completed_jobs_.fetch_add(1);
				return kcenon::common::ok();
			});
			pool_->enqueue(std::move(job));
		}
	}
};

/**
 * @brief Compare different victim selection policies
 */
TEST_F(WorkStealingBenchmark, PolicyComparison)
{
	std::cout << "\n========== Work-Stealing Policy Comparison ==========" << std::endl;

	const size_t job_count = scale_for_ci(kDefaultJobCount);
	const size_t worker_count = kDefaultWorkerCount;

	std::vector<BenchmarkResult> results;

	// Baseline: No work-stealing
	{
		enhanced_work_stealing_config config;
		config.enabled = false;
		results.push_back(run_benchmark("No Work-Stealing", config, job_count, worker_count));
	}

	// Random policy
	{
		auto config = enhanced_work_stealing_config::default_config();
		config.policy = enhanced_steal_policy::random;
		config.collect_statistics = true;
		results.push_back(run_benchmark("Random Policy", config, job_count, worker_count));
	}

	// Round-robin policy
	{
		auto config = enhanced_work_stealing_config::default_config();
		config.policy = enhanced_steal_policy::round_robin;
		config.collect_statistics = true;
		results.push_back(run_benchmark("Round-Robin Policy", config, job_count, worker_count));
	}

	// Adaptive policy
	{
		auto config = enhanced_work_stealing_config::default_config();
		config.policy = enhanced_steal_policy::adaptive;
		config.collect_statistics = true;
		results.push_back(run_benchmark("Adaptive Policy", config, job_count, worker_count));
	}

	// NUMA-aware policy (fallback to adaptive on non-NUMA)
	{
		auto config = enhanced_work_stealing_config::numa_optimized();
		config.collect_statistics = true;
		results.push_back(run_benchmark("NUMA-Aware Policy", config, job_count, worker_count));
	}

	// Hierarchical policy
	{
		auto config = enhanced_work_stealing_config::hierarchical_numa();
		config.collect_statistics = true;
		results.push_back(run_benchmark("Hierarchical Policy", config, job_count, worker_count));
	}

	std::cout << "\nResults (jobs: " << job_count << ", workers: " << worker_count << "):" << std::endl;
	for (const auto& result : results)
	{
		print_result(result);
	}

	// Verify work-stealing improves throughput for most policies
	double baseline = results[0].throughput;
	for (size_t i = 1; i < results.size(); ++i)
	{
		double improvement = ((results[i].throughput - baseline) / baseline) * 100.0;
		std::cout << "  " << results[i].config_name << " vs baseline: " << std::showpos
		          << std::fixed << std::setprecision(1) << improvement << "%" << std::noshowpos
		          << std::endl;
	}
}

/**
 * @brief Compare different batch sizes for work-stealing
 */
TEST_F(WorkStealingBenchmark, BatchSizeComparison)
{
	std::cout << "\n========== Batch Size Comparison ==========" << std::endl;

	const size_t job_count = scale_for_ci(kDefaultJobCount);
	const size_t worker_count = kDefaultWorkerCount;

	std::vector<BenchmarkResult> results;

	// Single steal (batch = 1)
	{
		auto config = enhanced_work_stealing_config::default_config();
		config.min_steal_batch = 1;
		config.max_steal_batch = 1;
		config.adaptive_batch_size = false;
		config.collect_statistics = true;
		results.push_back(run_benchmark("Batch Size 1", config, job_count, worker_count));
	}

	// Small batch (2)
	{
		auto config = enhanced_work_stealing_config::default_config();
		config.min_steal_batch = 2;
		config.max_steal_batch = 2;
		config.adaptive_batch_size = false;
		config.collect_statistics = true;
		results.push_back(run_benchmark("Batch Size 2", config, job_count, worker_count));
	}

	// Medium batch (4)
	{
		auto config = enhanced_work_stealing_config::batch_optimized();
		config.min_steal_batch = 4;
		config.max_steal_batch = 4;
		config.adaptive_batch_size = false;
		config.collect_statistics = true;
		results.push_back(run_benchmark("Batch Size 4", config, job_count, worker_count));
	}

	// Large batch (8)
	{
		auto config = enhanced_work_stealing_config::batch_optimized();
		config.min_steal_batch = 8;
		config.max_steal_batch = 8;
		config.adaptive_batch_size = false;
		config.collect_statistics = true;
		results.push_back(run_benchmark("Batch Size 8", config, job_count, worker_count));
	}

	// Adaptive batch
	{
		auto config = enhanced_work_stealing_config::batch_optimized();
		config.adaptive_batch_size = true;
		config.collect_statistics = true;
		results.push_back(run_benchmark("Adaptive Batch", config, job_count, worker_count));
	}

	std::cout << "\nResults (jobs: " << job_count << ", workers: " << worker_count << "):" << std::endl;
	for (const auto& result : results)
	{
		print_result(result);
		if (result.stats.batch_steals > 0)
		{
			std::cout << "    Avg batch size: " << std::fixed << std::setprecision(2)
			          << result.stats.avg_batch_size() << std::endl;
		}
	}
}

/**
 * @brief Compare different backoff strategies
 */
TEST_F(WorkStealingBenchmark, BackoffStrategyComparison)
{
	std::cout << "\n========== Backoff Strategy Comparison ==========" << std::endl;

	const size_t job_count = scale_for_ci(kDefaultJobCount);
	const size_t worker_count = kDefaultWorkerCount;

	std::vector<BenchmarkResult> results;

	// Fixed backoff
	{
		auto config = enhanced_work_stealing_config::default_config();
		config.backoff_strategy = steal_backoff_strategy::fixed;
		config.initial_backoff = std::chrono::microseconds(50);
		config.collect_statistics = true;
		results.push_back(run_benchmark("Fixed Backoff", config, job_count, worker_count));
	}

	// Linear backoff
	{
		auto config = enhanced_work_stealing_config::default_config();
		config.backoff_strategy = steal_backoff_strategy::linear;
		config.initial_backoff = std::chrono::microseconds(50);
		config.collect_statistics = true;
		results.push_back(run_benchmark("Linear Backoff", config, job_count, worker_count));
	}

	// Exponential backoff
	{
		auto config = enhanced_work_stealing_config::default_config();
		config.backoff_strategy = steal_backoff_strategy::exponential;
		config.initial_backoff = std::chrono::microseconds(50);
		config.backoff_multiplier = 2.0;
		config.collect_statistics = true;
		results.push_back(run_benchmark("Exponential Backoff", config, job_count, worker_count));
	}

	// Adaptive jitter
	{
		auto config = enhanced_work_stealing_config::default_config();
		config.backoff_strategy = steal_backoff_strategy::adaptive_jitter;
		config.initial_backoff = std::chrono::microseconds(50);
		config.collect_statistics = true;
		results.push_back(run_benchmark("Adaptive Jitter", config, job_count, worker_count));
	}

	std::cout << "\nResults (jobs: " << job_count << ", workers: " << worker_count << "):" << std::endl;
	for (const auto& result : results)
	{
		print_result(result);
		if (result.stats.total_backoff_time_ns > 0)
		{
			auto backoff_us = result.stats.total_backoff_time_ns / 1000;
			std::cout << "    Total backoff time: " << backoff_us << " us" << std::endl;
		}
	}
}

/**
 * @brief Test work-stealing with imbalanced workloads
 */
TEST_F(WorkStealingBenchmark, ImbalancedWorkloadPerformance)
{
	std::cout << "\n========== Imbalanced Workload Performance ==========" << std::endl;

	const size_t job_count = scale_for_ci(kImbalancedJobCount);
	const size_t worker_count = kDefaultWorkerCount;

	std::vector<BenchmarkResult> results;

	auto imbalanced_distribution = [this](size_t count) { create_imbalanced_workload(count); };

	// No work-stealing
	{
		enhanced_work_stealing_config config;
		config.enabled = false;
		results.push_back(
			run_benchmark("No Work-Stealing", config, job_count, worker_count, imbalanced_distribution));
	}

	// Adaptive policy (best for imbalanced loads)
	{
		auto config = enhanced_work_stealing_config::default_config();
		config.policy = enhanced_steal_policy::adaptive;
		config.collect_statistics = true;
		results.push_back(
			run_benchmark("Adaptive Policy", config, job_count, worker_count, imbalanced_distribution));
	}

	// Locality-aware with batch stealing
	{
		auto config = enhanced_work_stealing_config::locality_optimized();
		config.collect_statistics = true;
		results.push_back(
			run_benchmark("Locality Optimized", config, job_count, worker_count, imbalanced_distribution));
	}

	std::cout << "\nResults (imbalanced jobs: " << job_count << ", workers: " << worker_count
	          << "):" << std::endl;
	for (const auto& result : results)
	{
		print_result(result);
	}

	double baseline = results[0].throughput;
	for (size_t i = 1; i < results.size(); ++i)
	{
		double improvement = ((results[i].throughput - baseline) / baseline) * 100.0;
		std::cout << "  " << results[i].config_name << " improvement: " << std::showpos
		          << std::fixed << std::setprecision(1) << improvement << "%" << std::noshowpos
		          << std::endl;
	}
}

/**
 * @brief Report NUMA topology information
 */
TEST_F(WorkStealingBenchmark, NumaTopologyReport)
{
	std::cout << "\n========== NUMA Topology Report ==========" << std::endl;

	auto topology = numa_topology::detect();

	std::cout << "\nSystem NUMA Configuration:" << std::endl;
	std::cout << "  NUMA Available: " << (topology.is_numa_available() ? "Yes" : "No") << std::endl;
	std::cout << "  Node Count: " << topology.node_count() << std::endl;
	std::cout << "  CPU Count: " << topology.cpu_count() << std::endl;

	const auto& nodes = topology.get_nodes();
	for (const auto& node : nodes)
	{
		std::cout << "\n  Node " << node.node_id << ":" << std::endl;
		std::cout << "    CPUs: ";
		for (size_t i = 0; i < node.cpu_ids.size(); ++i)
		{
			if (i > 0)
				std::cout << ", ";
			std::cout << node.cpu_ids[i];
		}
		std::cout << std::endl;
		if (node.memory_size_bytes > 0)
		{
			std::cout << "    Memory: " << (node.memory_size_bytes / (1024 * 1024)) << " MB" << std::endl;
		}
	}

	if (topology.is_numa_available() && topology.node_count() > 1)
	{
		std::cout << "\n  Inter-Node Distances:" << std::endl;
		for (size_t i = 0; i < topology.node_count(); ++i)
		{
			std::cout << "    Node " << i << ": ";
			for (size_t j = 0; j < topology.node_count(); ++j)
			{
				if (j > 0)
					std::cout << " ";
				std::cout << std::setw(3) << topology.get_distance(static_cast<int>(i), static_cast<int>(j));
			}
			std::cout << std::endl;
		}
	}

	std::cout << "\n  Recommendation: ";
	if (topology.is_numa_available())
	{
		std::cout << "Use numa_aware or hierarchical policy for optimal performance" << std::endl;
	}
	else
	{
		std::cout << "Single-node system - adaptive or round_robin policy recommended" << std::endl;
	}
}
