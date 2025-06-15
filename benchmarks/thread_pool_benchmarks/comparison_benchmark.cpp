/*
 * BSD 3-Clause License
 * 
 * Copyright (c) 2024, DongCheol Shin
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file comparison_benchmark.cpp
 * @brief Comparative benchmarks against standard library and common patterns
 * 
 * Compares Thread System performance with:
 * - std::async
 * - Raw std::thread
 * - OpenMP (if available)
 * - Custom thread pool implementations
 */

#include <chrono>
#include <vector>
#include <thread>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <numeric>
#include <algorithm>
#include <iomanip>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "thread_pool.h"
#include "typed_thread_pool.h"
#include "logger.h"
#include "formatter.h"

using namespace std::chrono;
using namespace thread_pool_module;

// Simple thread pool implementation for comparison
class SimpleThreadPool {
public:
    explicit SimpleThreadPool(size_t num_threads) : stop_(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                        
                        if (stop_ && tasks_.empty()) return;
                        
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }
    
    ~SimpleThreadPool() {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        
        for (auto& worker : workers_) {
            worker.join();
        }
    }
    
    void submit(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            tasks_.push(std::move(task));
        }
        cv_.notify_one();
    }
    
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_;
};

class ComparisonBenchmark {
public:
    ComparisonBenchmark() {
        log_module::start();
        log_module::console_target(log_module::log_types::Information);
    }
    
    ~ComparisonBenchmark() {
        log_module::stop();
    }
    
    void run_all_benchmarks() {
        log_module::information("\n=== Comparative Performance Benchmarks ===\n");
        
        compare_simple_task_execution();
        compare_parallel_computation();
        compare_io_bound_workload();
        compare_mixed_workload();
        compare_task_creation_overhead();
        compare_memory_usage();
        
        log_module::information("\n=== Comparison Complete ===\n");
    }
    
private:
    struct BenchmarkResult {
        std::string name;
        double time_ms;
        double speedup;
        size_t operations;
    };
    
    void print_comparison_table(const std::vector<BenchmarkResult>& results) {
        // Find baseline (first result)
        double baseline_time = results.empty() ? 1.0 : results[0].time_ms;
        
        log_module::information("\n");
        log_module::information(format_string("%*s%*s%*s%*s", 25, "Implementation", 12, "Time (ms)", 12, "Speedup", 15, "Ops/sec"));
        log_module::information(std::string(64, '-'));
        
        for (const auto& result : results) {
            double speedup = baseline_time / result.time_ms;
            double ops_per_sec = (result.operations * 1000.0) / result.time_ms;
            
            log_module::information(format_string("%*s%*.2f%*.2fx%*.0f", 25, result.name.c_str(), 12, result.time_ms, 12, speedup, 15, ops_per_sec));
        }
    }
    
    void compare_simple_task_execution() {
        log_module::information("\n1. Simple Task Execution Comparison\n");
        log_module::information("-----------------------------------\n");
        
        const size_t num_tasks = 100000;
        std::vector<BenchmarkResult> results;
        
        // Baseline: Sequential execution
        {
            std::atomic<size_t> counter{0};
            
            auto start = high_resolution_clock::now();
            
            for (size_t i = 0; i < num_tasks; ++i) {
                counter.fetch_add(1);
            }
            
            auto end = high_resolution_clock::now();
            double time_ms = duration_cast<milliseconds>(end - start).count();
            
            results.push_back({"Sequential", time_ms, 1.0, num_tasks});
        }
        
        // Thread System
        {
            auto [pool, error] = create_default(std::thread::hardware_concurrency());
            if (!error) {
                pool->start();
                
                std::atomic<size_t> counter{0};
                
                auto start = high_resolution_clock::now();
                
                for (size_t i = 0; i < num_tasks; ++i) {
                    pool->add_job([&counter] {
                        counter.fetch_add(1);
                    });
                }
                
                pool->stop();
                
                auto end = high_resolution_clock::now();
                double time_ms = duration_cast<milliseconds>(end - start).count();
                
                results.push_back({"Thread System", time_ms, 1.0, num_tasks});
            }
        }
        
        // std::async
        {
            std::atomic<size_t> counter{0};
            std::vector<std::future<void>> futures;
            
            auto start = high_resolution_clock::now();
            
            for (size_t i = 0; i < num_tasks; ++i) {
                futures.push_back(std::async(std::launch::async, [&counter] {
                    counter.fetch_add(1);
                }));
            }
            
            for (auto& f : futures) {
                f.get();
            }
            
            auto end = high_resolution_clock::now();
            double time_ms = duration_cast<milliseconds>(end - start).count();
            
            results.push_back({"std::async", time_ms, 1.0, num_tasks});
        }
        
        // Simple thread pool
        {
            SimpleThreadPool pool(std::thread::hardware_concurrency());
            std::atomic<size_t> counter{0};
            std::atomic<size_t> completed{0};
            
            auto start = high_resolution_clock::now();
            
            for (size_t i = 0; i < num_tasks; ++i) {
                pool.submit([&counter, &completed, num_tasks] {
                    counter.fetch_add(1);
                    completed.fetch_add(1);
                });
            }
            
            // Wait for completion
            while (completed.load() < num_tasks) {
                std::this_thread::sleep_for(milliseconds(1));
            }
            
            auto end = high_resolution_clock::now();
            double time_ms = duration_cast<milliseconds>(end - start).count();
            
            results.push_back({"Simple Thread Pool", time_ms, 1.0, num_tasks});
        }
        
        #ifdef _OPENMP
        // OpenMP
        {
            std::atomic<size_t> counter{0};
            
            auto start = high_resolution_clock::now();
            
            #pragma omp parallel for
            for (size_t i = 0; i < num_tasks; ++i) {
                counter.fetch_add(1);
            }
            
            auto end = high_resolution_clock::now();
            double time_ms = duration_cast<milliseconds>(end - start).count();
            
            results.push_back({"OpenMP", time_ms, 1.0, num_tasks});
        }
        #endif
        
        print_comparison_table(results);
    }
    
    void compare_parallel_computation() {
        log_module::information("\n2. Parallel Computation Comparison\n");
        log_module::information("----------------------------------\n");
        
        const size_t data_size = 10000000;
        std::vector<double> data(data_size);
        
        // Initialize data
        for (size_t i = 0; i < data_size; ++i) {
            data[i] = static_cast<double>(i) * 0.1;
        }
        
        std::vector<BenchmarkResult> results;
        
        // Baseline: Sequential
        {
            auto start = high_resolution_clock::now();
            
            double sum = 0;
            for (const auto& val : data) {
                sum += std::sin(val) * std::cos(val);
            }
            
            auto end = high_resolution_clock::now();
            double time_ms = duration_cast<milliseconds>(end - start).count();
            
            results.push_back({"Sequential", time_ms, 1.0, data_size});
        }
        
        // Thread System with batching
        {
            auto [pool, error] = create_default(std::thread::hardware_concurrency());
            if (!error) {
                pool->start();
                
                const size_t num_workers = std::thread::hardware_concurrency();
                const size_t chunk_size = data_size / num_workers;
                
                std::vector<std::future<double>> futures;
                std::vector<std::promise<double>> promises(num_workers);
                
                for (size_t i = 0; i < num_workers; ++i) {
                    futures.push_back(promises[i].get_future());
                }
                
                auto start = high_resolution_clock::now();
                
                for (size_t i = 0; i < num_workers; ++i) {
                    size_t start_idx = i * chunk_size;
                    size_t end_idx = (i == num_workers - 1) ? data_size : start_idx + chunk_size;
                    
                    pool->add_job([&data, start_idx, end_idx, p = std::move(promises[i])]() mutable {
                        double local_sum = 0;
                        for (size_t j = start_idx; j < end_idx; ++j) {
                            local_sum += std::sin(data[j]) * std::cos(data[j]);
                        }
                        p.set_value(local_sum);
                    });
                }
                
                double total_sum = 0;
                for (auto& f : futures) {
                    total_sum += f.get();
                }
                
                auto end = high_resolution_clock::now();
                double time_ms = duration_cast<milliseconds>(end - start).count();
                
                results.push_back({"Thread System", time_ms, 1.0, data_size});
                
                pool->stop();
            }
        }
        
        // std::async with futures
        {
            const size_t num_workers = std::thread::hardware_concurrency();
            const size_t chunk_size = data_size / num_workers;
            
            auto start = high_resolution_clock::now();
            
            std::vector<std::future<double>> futures;
            
            for (size_t i = 0; i < num_workers; ++i) {
                size_t start_idx = i * chunk_size;
                size_t end_idx = (i == num_workers - 1) ? data_size : start_idx + chunk_size;
                
                futures.push_back(std::async(std::launch::async, 
                    [&data, start_idx, end_idx] {
                        double local_sum = 0;
                        for (size_t j = start_idx; j < end_idx; ++j) {
                            local_sum += std::sin(data[j]) * std::cos(data[j]);
                        }
                        return local_sum;
                    }
                ));
            }
            
            double total_sum = 0;
            for (auto& f : futures) {
                total_sum += f.get();
            }
            
            auto end = high_resolution_clock::now();
            double time_ms = duration_cast<milliseconds>(end - start).count();
            
            results.push_back({"std::async", time_ms, 1.0, data_size});
        }
        
        #ifdef _OPENMP
        // OpenMP reduction
        {
            auto start = high_resolution_clock::now();
            
            double sum = 0;
            #pragma omp parallel for reduction(+:sum)
            for (size_t i = 0; i < data_size; ++i) {
                sum += std::sin(data[i]) * std::cos(data[i]);
            }
            
            auto end = high_resolution_clock::now();
            double time_ms = duration_cast<milliseconds>(end - start).count();
            
            results.push_back({"OpenMP", time_ms, 1.0, data_size});
        }
        #endif
        
        print_comparison_table(results);
    }
    
    void compare_io_bound_workload() {
        log_module::information("\n3. I/O Bound Workload Comparison\n");
        log_module::information("--------------------------------\n");
        
        const size_t num_operations = 1000;
        const int io_delay_ms = 10;
        
        std::vector<BenchmarkResult> results;
        
        // Thread System with many workers (good for I/O)
        {
            auto [pool, error] = create_default(std::thread::hardware_concurrency() * 4);
            if (!error) {
                pool->start();
                
                std::atomic<size_t> completed{0};
                
                auto start = high_resolution_clock::now();
                
                for (size_t i = 0; i < num_operations; ++i) {
                    pool->add_job([io_delay_ms, &completed] {
                        // Simulate I/O
                        std::this_thread::sleep_for(milliseconds(io_delay_ms));
                        completed.fetch_add(1);
                    });
                }
                
                pool->stop();
                
                auto end = high_resolution_clock::now();
                double time_ms = duration_cast<milliseconds>(end - start).count();
                
                results.push_back({"Thread System (4x workers)", time_ms, 1.0, num_operations});
            }
        }
        
        // Thread System with normal workers
        {
            auto [pool, error] = create_default(std::thread::hardware_concurrency());
            if (!error) {
                pool->start();
                
                std::atomic<size_t> completed{0};
                
                auto start = high_resolution_clock::now();
                
                for (size_t i = 0; i < num_operations; ++i) {
                    pool->add_job([io_delay_ms, &completed] {
                        std::this_thread::sleep_for(milliseconds(io_delay_ms));
                        completed.fetch_add(1);
                    });
                }
                
                pool->stop();
                
                auto end = high_resolution_clock::now();
                double time_ms = duration_cast<milliseconds>(end - start).count();
                
                results.push_back({"Thread System (1x workers)", time_ms, 1.0, num_operations});
            }
        }
        
        // std::async (unlimited threads)
        {
            std::vector<std::future<void>> futures;
            
            auto start = high_resolution_clock::now();
            
            for (size_t i = 0; i < num_operations; ++i) {
                futures.push_back(std::async(std::launch::async, [io_delay_ms] {
                    std::this_thread::sleep_for(milliseconds(io_delay_ms));
                }));
            }
            
            for (auto& f : futures) {
                f.get();
            }
            
            auto end = high_resolution_clock::now();
            double time_ms = duration_cast<milliseconds>(end - start).count();
            
            results.push_back({"std::async", time_ms, 1.0, num_operations});
        }
        
        print_comparison_table(results);
    }
    
    void compare_mixed_workload() {
        log_module::information("\n4. Mixed CPU/IO Workload Comparison\n");
        log_module::information("-----------------------------------\n");
        
        const size_t num_tasks = 1000;
        const int cpu_work_units = 1000;
        const int io_delay_ms = 5;
        
        std::vector<BenchmarkResult> results;
        
        auto mixed_work = [cpu_work_units, io_delay_ms] {
            // CPU work
            volatile double result = 0;
            for (int i = 0; i < cpu_work_units; ++i) {
                result += std::sin(i) * std::cos(i);
            }
            
            // I/O work
            std::this_thread::sleep_for(milliseconds(io_delay_ms));
        };
        
        // Thread System
        {
            auto [pool, error] = create_default(std::thread::hardware_concurrency());
            if (!error) {
                pool->start();
                
                auto start = high_resolution_clock::now();
                
                for (size_t i = 0; i < num_tasks; ++i) {
                    pool->add_job(mixed_work);
                }
                
                pool->stop();
                
                auto end = high_resolution_clock::now();
                double time_ms = duration_cast<milliseconds>(end - start).count();
                
                results.push_back({"Thread System", time_ms, 1.0, num_tasks});
            }
        }
        
        // Type Thread System (with different types for CPU vs I/O)
        {
            enum class TaskType { CPU = 1, IO = 10 };
            
            auto [pool, error] = create_priority_default<TaskType>(std::thread::hardware_concurrency());
            if (!error) {
                pool->start();
                
                auto start = high_resolution_clock::now();
                
                for (size_t i = 0; i < num_tasks / 2; ++i) {
                    // CPU-heavy tasks get higher priority
                    pool->add_job([cpu_work_units] {
                        volatile double result = 0;
                        for (int j = 0; j < cpu_work_units * 2; ++j) {
                            result += std::sin(j) * std::cos(j);
                        }
                    }, TaskType::CPU);
                    
                    // I/O-heavy tasks get lower priority
                    pool->add_job([io_delay_ms] {
                        std::this_thread::sleep_for(milliseconds(io_delay_ms * 2));
                    }, TaskType::IO);
                }
                
                pool->stop();
                
                auto end = high_resolution_clock::now();
                double time_ms = duration_cast<milliseconds>(end - start).count();
                
                results.push_back({"Type Thread System", time_ms, 1.0, num_tasks});
            }
        }
        
        // std::async
        {
            std::vector<std::future<void>> futures;
            
            auto start = high_resolution_clock::now();
            
            for (size_t i = 0; i < num_tasks; ++i) {
                futures.push_back(std::async(std::launch::async, mixed_work));
            }
            
            for (auto& f : futures) {
                f.get();
            }
            
            auto end = high_resolution_clock::now();
            double time_ms = duration_cast<milliseconds>(end - start).count();
            
            results.push_back({"std::async", time_ms, 1.0, num_tasks});
        }
        
        print_comparison_table(results);
    }
    
    void compare_task_creation_overhead() {
        log_module::information("\n5. Task Creation Overhead Comparison\n");
        log_module::information("------------------------------------\n");
        
        const size_t num_iterations = 100;
        const size_t tasks_per_iteration = 1000;
        
        std::vector<BenchmarkResult> results;
        
        // Thread System
        {
            auto [pool, error] = create_default(4);
            if (!error) {
                pool->start();
                
                std::vector<double> times;
                
                for (size_t iter = 0; iter < num_iterations; ++iter) {
                    auto start = high_resolution_clock::now();
                    
                    for (size_t i = 0; i < tasks_per_iteration; ++i) {
                        pool->add_job([] {});
                    }
                    
                    auto end = high_resolution_clock::now();
                    times.push_back(duration_cast<microseconds>(end - start).count());
                }
                
                pool->stop();
                
                double avg_time_us = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
                double per_task_ns = (avg_time_us * 1000.0) / tasks_per_iteration;
                
                results.push_back({"Thread System", per_task_ns / 1000.0, 1.0, tasks_per_iteration});
                
                log_module::information(format_string("Thread System: %.1f ns per task submission\n", per_task_ns));
            }
        }
        
        // std::async
        {
            std::vector<double> times;
            
            for (size_t iter = 0; iter < num_iterations; ++iter) {
                std::vector<std::future<void>> futures;
                
                auto start = high_resolution_clock::now();
                
                for (size_t i = 0; i < tasks_per_iteration; ++i) {
                    futures.push_back(std::async(std::launch::deferred, [] {}));
                }
                
                auto end = high_resolution_clock::now();
                times.push_back(duration_cast<microseconds>(end - start).count());
                
                // Clean up futures
                futures.clear();
            }
            
            double avg_time_us = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
            double per_task_ns = (avg_time_us * 1000.0) / tasks_per_iteration;
            
            results.push_back({"std::async (deferred)", per_task_ns / 1000.0, 1.0, tasks_per_iteration});
            
            log_module::information(format_string("std::async (deferred): %.1f ns per task creation\n", per_task_ns));
        }
        
        // Raw function object creation
        {
            std::vector<double> times;
            
            for (size_t iter = 0; iter < num_iterations; ++iter) {
                std::vector<std::function<void()>> tasks;
                
                auto start = high_resolution_clock::now();
                
                for (size_t i = 0; i < tasks_per_iteration; ++i) {
                    tasks.push_back([] {});
                }
                
                auto end = high_resolution_clock::now();
                times.push_back(duration_cast<microseconds>(end - start).count());
            }
            
            double avg_time_us = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
            double per_task_ns = (avg_time_us * 1000.0) / tasks_per_iteration;
            
            log_module::information(format_string("Raw lambda creation: %.1f ns per lambda\n", per_task_ns));
        }
    }
    
    void compare_memory_usage() {
        log_module::information("\n6. Memory Usage Comparison\n");
        log_module::information("--------------------------\n");
        log_module::information("(Memory measurements are approximations)\n\n");
        
        const size_t num_queued_tasks = 100000;
        
        // Estimate memory per task
        size_t thread_system_memory = sizeof(job) * num_queued_tasks;
        size_t async_memory = sizeof(std::future<void>) * num_queued_tasks + 
                             sizeof(std::promise<void>) * num_queued_tasks;
        size_t simple_pool_memory = sizeof(std::function<void()>) * num_queued_tasks;
        
        log_module::information(format_string("Memory per %zu queued tasks:\n", num_queued_tasks));
        log_module::information(format_string("  Thread System: %.2f MB (%zu bytes/task)\n", 
                                             (thread_system_memory / 1024.0 / 1024.0), 
                                             (thread_system_memory / num_queued_tasks)));
        log_module::information(format_string("  std::async: %.2f MB (%zu bytes/task)\n", 
                                             (async_memory / 1024.0 / 1024.0), 
                                             (async_memory / num_queued_tasks)));
        log_module::information(format_string("  Simple Pool: %.2f MB (%zu bytes/task)\n", 
                                             (simple_pool_memory / 1024.0 / 1024.0), 
                                             (simple_pool_memory / num_queued_tasks)));
    }
};

int main() {
    ComparisonBenchmark benchmark;
    benchmark.run_all_benchmarks();
    
    return 0;
}