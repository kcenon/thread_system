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
#include "typed_thread_pool/pool/typed_lockfree_thread_pool.h"
#include "typed_thread_pool/pool/typed_thread_pool.h"
#include "typed_thread_pool/jobs/callback_typed_job.h"
#include "logger/core/logger.h"

using namespace typed_thread_pool_module;
using namespace thread_module;

// Benchmarking configuration
constexpr size_t WORKER_COUNT = 4;
constexpr size_t MIN_JOBS = 100;
constexpr size_t MAX_JOBS = 10000;

// Job execution simulation
static std::atomic<size_t> job_counter{0};

void reset_counter() {
    job_counter.store(0);
}

void simple_job() {
    job_counter.fetch_add(1);
    // Simulate minimal work
    volatile int x = 0;
    for (int i = 0; i < 10; ++i) {
        x += i;
    }
}

void medium_job() {
    job_counter.fetch_add(1);
    // Simulate medium work
    volatile int x = 0;
    for (int i = 0; i < 100; ++i) {
        x += i * i;
    }
}

void heavy_job() {
    job_counter.fetch_add(1);
    // Simulate heavy work
    volatile int x = 0;
    for (int i = 0; i < 1000; ++i) {
        x += i * i * i;
    }
}

// Benchmark fixture for typed thread pool
class TypedThreadPoolFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        reset_counter();
        
        pool = std::make_shared<typed_thread_pool>("benchmark_pool");
        
        // Create workers
        std::vector<std::unique_ptr<typed_thread_worker>> workers;
        for (size_t i = 0; i < WORKER_COUNT; ++i) {
            if (i % 3 == 0) {
                workers.push_back(std::make_unique<typed_thread_worker>(
                    std::vector<job_types>{job_types::RealTime}));
            } else if (i % 3 == 1) {
                workers.push_back(std::make_unique<typed_thread_worker>(
                    std::vector<job_types>{job_types::Batch}));
            } else {
                workers.push_back(std::make_unique<typed_thread_worker>(
                    std::vector<job_types>{job_types::Background}));
            }
        }
        
        pool->enqueue_batch(std::move(workers));
        pool->start();
    }
    
    void TearDown(const benchmark::State& state) override {
        if (pool) {
            pool->stop();
            pool.reset();
        }
    }
    
protected:
    std::shared_ptr<typed_thread_pool> pool;
};

// Benchmark fixture for typed lockfree thread pool
class TypedLockfreeThreadPoolFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        reset_counter();
        
        pool = std::make_shared<typed_lockfree_thread_pool>("lockfree_benchmark_pool");
        
        // Create workers
        std::vector<std::unique_ptr<typed_lockfree_thread_worker>> workers;
        for (size_t i = 0; i < WORKER_COUNT; ++i) {
            if (i % 3 == 0) {
                workers.push_back(std::make_unique<typed_lockfree_thread_worker>(
                    std::vector<job_types>{job_types::RealTime}, "RealTime Worker"));
            } else if (i % 3 == 1) {
                workers.push_back(std::make_unique<typed_lockfree_thread_worker>(
                    std::vector<job_types>{job_types::Batch}, "Batch Worker"));
            } else {
                workers.push_back(std::make_unique<typed_lockfree_thread_worker>(
                    std::vector<job_types>{job_types::Background}, "Background Worker"));
            }
        }
        
        pool->enqueue_batch(std::move(workers));
        pool->start();
    }
    
    void TearDown(const benchmark::State& state) override {
        if (pool) {
            pool->stop();
            pool.reset();
        }
    }
    
protected:
    std::shared_ptr<typed_lockfree_thread_pool> pool;
};

// Typed thread pool benchmarks
BENCHMARK_DEFINE_F(TypedThreadPoolFixture, SimpleJobs)(benchmark::State& state) {
    const size_t job_count = state.range(0);
    
    for (auto _ : state) {
        reset_counter();
        
        // Create and enqueue jobs
        std::vector<std::unique_ptr<typed_job>> jobs;
        jobs.reserve(job_count);
        
        for (size_t i = 0; i < job_count; ++i) {
            job_types type = static_cast<job_types>(i % 3);
            jobs.push_back(std::make_unique<callback_typed_job>(
                []() -> result_void {
                    simple_job();
                    return {};
                },
                type
            ));
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        pool->enqueue_batch(std::move(jobs));
        
        // Wait for completion
        while (job_counter.load() < job_count) {
            std::this_thread::yield();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e9);
    }
    
    state.SetItemsProcessed(state.iterations() * job_count);
    state.SetLabel("mutex-based");
}

BENCHMARK_DEFINE_F(TypedLockfreeThreadPoolFixture, SimpleJobs)(benchmark::State& state) {
    const size_t job_count = state.range(0);
    
    for (auto _ : state) {
        reset_counter();
        
        // Create and enqueue jobs
        std::vector<std::unique_ptr<typed_job>> jobs;
        jobs.reserve(job_count);
        
        for (size_t i = 0; i < job_count; ++i) {
            job_types type = static_cast<job_types>(i % 3);
            jobs.push_back(std::make_unique<callback_typed_job>(
                []() -> result_void {
                    simple_job();
                    return {};
                },
                type
            ));
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        pool->enqueue_batch(std::move(jobs));
        
        // Wait for completion
        while (job_counter.load() < job_count) {
            std::this_thread::yield();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e9);
    }
    
    state.SetItemsProcessed(state.iterations() * job_count);
    state.SetLabel("lock-free");
}

// Medium workload benchmarks
BENCHMARK_DEFINE_F(TypedThreadPoolFixture, MediumJobs)(benchmark::State& state) {
    const size_t job_count = state.range(0);
    
    for (auto _ : state) {
        reset_counter();
        
        std::vector<std::unique_ptr<typed_job>> jobs;
        jobs.reserve(job_count);
        
        for (size_t i = 0; i < job_count; ++i) {
            job_types type = static_cast<job_types>(i % 3);
            jobs.push_back(std::make_unique<callback_typed_job>(
                []() -> result_void {
                    medium_job();
                    return {};
                },
                type
            ));
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        pool->enqueue_batch(std::move(jobs));
        
        while (job_counter.load() < job_count) {
            std::this_thread::yield();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e9);
    }
    
    state.SetItemsProcessed(state.iterations() * job_count);
    state.SetLabel("mutex-based");
}

BENCHMARK_DEFINE_F(TypedLockfreeThreadPoolFixture, MediumJobs)(benchmark::State& state) {
    const size_t job_count = state.range(0);
    
    for (auto _ : state) {
        reset_counter();
        
        std::vector<std::unique_ptr<typed_job>> jobs;
        jobs.reserve(job_count);
        
        for (size_t i = 0; i < job_count; ++i) {
            job_types type = static_cast<job_types>(i % 3);
            jobs.push_back(std::make_unique<callback_typed_job>(
                []() -> result_void {
                    medium_job();
                    return {};
                },
                type
            ));
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        pool->enqueue_batch(std::move(jobs));
        
        while (job_counter.load() < job_count) {
            std::this_thread::yield();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e9);
    }
    
    state.SetItemsProcessed(state.iterations() * job_count);
    state.SetLabel("lock-free");
}

// Priority scheduling benchmark
BENCHMARK_DEFINE_F(TypedLockfreeThreadPoolFixture, PriorityScheduling)(benchmark::State& state) {
    const size_t jobs_per_priority = state.range(0);
    
    for (auto _ : state) {
        reset_counter();
        
        std::vector<std::unique_ptr<typed_job>> jobs;
        jobs.reserve(jobs_per_priority * 3);
        
        // Add Background priority jobs first
        for (size_t i = 0; i < jobs_per_priority; ++i) {
            jobs.push_back(std::make_unique<callback_typed_job>(
                []() -> result_void {
                    simple_job();
                    return {};
                },
                job_types::Background
            ));
        }
        
        // Add Batch priority jobs
        for (size_t i = 0; i < jobs_per_priority; ++i) {
            jobs.push_back(std::make_unique<callback_typed_job>(
                []() -> result_void {
                    simple_job();
                    return {};
                },
                job_types::Batch
            ));
        }
        
        // Add RealTime priority jobs last (should be processed first)
        for (size_t i = 0; i < jobs_per_priority; ++i) {
            jobs.push_back(std::make_unique<callback_typed_job>(
                []() -> result_void {
                    simple_job();
                    return {};
                },
                job_types::RealTime
            ));
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        pool->enqueue_batch(std::move(jobs));
        
        while (job_counter.load() < jobs_per_priority * 3) {
            std::this_thread::yield();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e9);
    }
    
    state.SetItemsProcessed(state.iterations() * jobs_per_priority * 3);
    state.SetLabel("priority-scheduling");
}

// Contention benchmark
BENCHMARK_DEFINE_F(TypedLockfreeThreadPoolFixture, HighContention)(benchmark::State& state) {
    const size_t thread_count = state.range(0);
    const size_t jobs_per_thread = 1000;
    
    for (auto _ : state) {
        reset_counter();
        
        std::vector<std::thread> producer_threads;
        std::atomic<bool> start_flag{false};
        
        // Create producer threads
        for (size_t t = 0; t < thread_count; ++t) {
            producer_threads.emplace_back([&, t]() {
                while (!start_flag.load()) {
                    std::this_thread::yield();
                }
                
                for (size_t i = 0; i < jobs_per_thread; ++i) {
                    job_types type = static_cast<job_types>((t + i) % 3);
                    auto job = std::make_unique<callback_typed_job>(
                        []() -> result_void {
                            simple_job();
                            return {};
                        },
                        type
                    );
                    pool->enqueue(std::move(job));
                }
            });
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        start_flag.store(true);
        
        // Wait for all producers to finish
        for (auto& t : producer_threads) {
            t.join();
        }
        
        // Wait for all jobs to complete
        while (job_counter.load() < thread_count * jobs_per_thread) {
            std::this_thread::yield();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e9);
    }
    
    state.SetItemsProcessed(state.iterations() * thread_count * jobs_per_thread);
    state.SetLabel("high-contention");
}

// Register benchmarks
BENCHMARK_REGISTER_F(TypedThreadPoolFixture, SimpleJobs)
    ->Range(MIN_JOBS, MAX_JOBS)
    ->UseManualTime()
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(TypedLockfreeThreadPoolFixture, SimpleJobs)
    ->Range(MIN_JOBS, MAX_JOBS)
    ->UseManualTime()
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(TypedThreadPoolFixture, MediumJobs)
    ->Range(MIN_JOBS, MAX_JOBS / 10)
    ->UseManualTime()
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(TypedLockfreeThreadPoolFixture, MediumJobs)
    ->Range(MIN_JOBS, MAX_JOBS / 10)
    ->UseManualTime()
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(TypedLockfreeThreadPoolFixture, PriorityScheduling)
    ->Range(100, 1000)
    ->UseManualTime()
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(TypedLockfreeThreadPoolFixture, HighContention)
    ->Range(1, 16)
    ->UseManualTime()
    ->Unit(benchmark::kMicrosecond);

