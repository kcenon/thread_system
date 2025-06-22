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

#include <benchmark/benchmark.h>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>

#include "thread_pool/core/thread_pool.h"
#include "thread_pool/core/lockfree_thread_pool.h"
#include "thread_base/jobs/callback_job.h"
#include "logger/core/logger.h"

using namespace thread_pool_module;
using namespace thread_module;

// Test configurations
constexpr int kSmallJobCount = 1000;
constexpr int kMediumJobCount = 10000;
constexpr int kLargeJobCount = 100000;

// Benchmark standard thread pool
static void BM_StandardThreadPool(benchmark::State& state) {
    const int worker_count = state.range(0);
    const int job_count = state.range(1);
    
    for (auto _ : state) {
        auto pool = std::make_shared<thread_pool>("StandardPool");
        
        // Add workers
        std::vector<std::unique_ptr<thread_worker>> workers;
        workers.reserve(worker_count);
        for (int i = 0; i < worker_count; ++i) {
            workers.push_back(std::make_unique<thread_worker>());
        }
        pool->enqueue_batch(std::move(workers));
        
        // Start pool
        auto start_result = pool->start();
        if (start_result.has_value()) {
            state.SkipWithError("Failed to start pool");
            return;
        }
        
        std::atomic<int> completed_jobs{0};
        
        // Submit jobs
        for (int i = 0; i < job_count; ++i) {
            auto job = std::make_unique<callback_job>([&completed_jobs]() -> result_void {
                completed_jobs.fetch_add(1);
                return {};
            });
            pool->enqueue(std::move(job));
        }
        
        // Wait for completion
        while (completed_jobs.load() < job_count) {
            std::this_thread::yield();
        }
        
        pool->stop();
    }
    
    state.SetItemsProcessed(state.iterations() * job_count);
}

// Benchmark lock-free thread pool
static void BM_LockfreeThreadPool(benchmark::State& state) {
    const int worker_count = state.range(0);
    const int job_count = state.range(1);
    
    for (auto _ : state) {
        auto pool = std::make_shared<lockfree_thread_pool>("LockfreePool");
        
        // Add workers
        std::vector<std::unique_ptr<lockfree_thread_worker>> workers;
        workers.reserve(worker_count);
        for (int i = 0; i < worker_count; ++i) {
            workers.push_back(std::make_unique<lockfree_thread_worker>());
        }
        pool->enqueue_batch(std::move(workers));
        
        // Start pool
        auto start_result = pool->start();
        if (start_result.has_value()) {
            state.SkipWithError("Failed to start pool");
            return;
        }
        
        std::atomic<int> completed_jobs{0};
        
        // Submit jobs
        for (int i = 0; i < job_count; ++i) {
            auto job = std::make_unique<callback_job>([&completed_jobs]() -> result_void {
                completed_jobs.fetch_add(1);
                return {};
            });
            pool->enqueue(std::move(job));
        }
        
        // Wait for completion
        while (completed_jobs.load() < job_count) {
            std::this_thread::yield();
        }
        
        pool->stop();
    }
    
    state.SetItemsProcessed(state.iterations() * job_count);
}

// Benchmark lock-free thread pool with batch processing
static void BM_LockfreeThreadPoolBatch(benchmark::State& state) {
    const int worker_count = state.range(0);
    const int job_count = state.range(1);
    const int batch_size = 1000;
    
    for (auto _ : state) {
        auto pool = std::make_shared<lockfree_thread_pool>("LockfreeBatchPool");
        
        // Add workers with batch processing enabled
        std::vector<std::unique_ptr<lockfree_thread_worker>> workers;
        workers.reserve(worker_count);
        for (int i = 0; i < worker_count; ++i) {
            auto worker = std::make_unique<lockfree_thread_worker>();
            worker->set_batch_processing(true, 32);
            workers.push_back(std::move(worker));
        }
        pool->enqueue_batch(std::move(workers));
        
        // Start pool
        auto start_result = pool->start();
        if (start_result.has_value()) {
            state.SkipWithError("Failed to start pool");
            return;
        }
        
        std::atomic<int> completed_jobs{0};
        
        // Submit jobs in batches
        std::vector<std::unique_ptr<job>> job_batch;
        job_batch.reserve(batch_size);
        
        for (int i = 0; i < job_count; ++i) {
            job_batch.push_back(std::make_unique<callback_job>([&completed_jobs]() -> result_void {
                completed_jobs.fetch_add(1);
                return {};
            }));
            
            if (job_batch.size() == batch_size || i == job_count - 1) {
                pool->enqueue_batch(std::move(job_batch));
                job_batch.clear();
                job_batch.reserve(batch_size);
            }
        }
        
        // Wait for completion
        while (completed_jobs.load() < job_count) {
            std::this_thread::yield();
        }
        
        pool->stop();
    }
    
    state.SetItemsProcessed(state.iterations() * job_count);
}

// High contention benchmark - many threads competing
static void BM_HighContention(benchmark::State& state) {
    const int worker_count = 2;  // Few workers
    const int producer_count = state.range(0);  // Many producers
    const int jobs_per_producer = 1000;
    const bool use_lockfree = state.range(1);
    
    for (auto _ : state) {
        std::shared_ptr<thread_pool> standard_pool;
        std::shared_ptr<lockfree_thread_pool> lockfree_pool;
        
        if (use_lockfree) {
            lockfree_pool = std::make_shared<lockfree_thread_pool>("HighContentionLockfree");
            std::vector<std::unique_ptr<lockfree_thread_worker>> workers;
            for (int i = 0; i < worker_count; ++i) {
                workers.push_back(std::make_unique<lockfree_thread_worker>());
            }
            lockfree_pool->enqueue_batch(std::move(workers));
            lockfree_pool->start();
        } else {
            standard_pool = std::make_shared<thread_pool>("HighContentionStandard");
            std::vector<std::unique_ptr<thread_worker>> workers;
            for (int i = 0; i < worker_count; ++i) {
                workers.push_back(std::make_unique<thread_worker>());
            }
            standard_pool->enqueue_batch(std::move(workers));
            standard_pool->start();
        }
        
        std::atomic<int> completed_jobs{0};
        std::vector<std::thread> producers;
        
        // Create producer threads
        for (int p = 0; p < producer_count; ++p) {
            producers.emplace_back([&, p]() {
                for (int i = 0; i < jobs_per_producer; ++i) {
                    auto job = std::make_unique<callback_job>([&completed_jobs]() -> result_void {
                        completed_jobs.fetch_add(1);
                        return {};
                    });
                    
                    if (use_lockfree) {
                        lockfree_pool->enqueue(std::move(job));
                    } else {
                        standard_pool->enqueue(std::move(job));
                    }
                }
            });
        }
        
        // Wait for all producers to finish
        for (auto& t : producers) {
            t.join();
        }
        
        // Wait for all jobs to complete
        const int total_jobs = producer_count * jobs_per_producer;
        while (completed_jobs.load() < total_jobs) {
            std::this_thread::yield();
        }
        
        if (use_lockfree) {
            lockfree_pool->stop();
        } else {
            standard_pool->stop();
        }
    }
    
    state.SetItemsProcessed(state.iterations() * producer_count * jobs_per_producer);
}

// Register benchmarks with various configurations
BENCHMARK(BM_StandardThreadPool)
    ->Args({4, kSmallJobCount})
    ->Args({4, kMediumJobCount})
    ->Args({8, kMediumJobCount})
    ->Args({16, kLargeJobCount})
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_LockfreeThreadPool)
    ->Args({4, kSmallJobCount})
    ->Args({4, kMediumJobCount})
    ->Args({8, kMediumJobCount})
    ->Args({16, kLargeJobCount})
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_LockfreeThreadPoolBatch)
    ->Args({4, kMediumJobCount})
    ->Args({8, kMediumJobCount})
    ->Args({16, kLargeJobCount})
    ->Unit(benchmark::kMillisecond);

// High contention tests: producers x use_lockfree(0/1)
BENCHMARK(BM_HighContention)
    ->Args({1, 0})   // 1 producer, standard
    ->Args({1, 1})   // 1 producer, lockfree
    ->Args({4, 0})   // 4 producers, standard
    ->Args({4, 1})   // 4 producers, lockfree
    ->Args({8, 0})   // 8 producers, standard
    ->Args({8, 1})   // 8 producers, lockfree
    ->Args({16, 0})  // 16 producers, standard
    ->Args({16, 1})  // 16 producers, lockfree
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();