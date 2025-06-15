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

/**
 * @file thread_pool_benchmark.cpp
 * @brief Performance benchmarks for Thread System
 * 
 * This file contains comprehensive benchmarks to measure:
 * - Thread pool creation overhead
 * - Job submission latency
 * - Job throughput
 * - Scaling efficiency
 * - Memory usage
 */

#include <chrono>
#include <vector>
#include <numeric>
#include <atomic>
#include <cmath>

#include "thread_pool.h"
#include "typed_thread_pool.h"
#include "logger.h"
#include "formatter.h"

using namespace std::chrono;
using namespace thread_pool_module;
using namespace typed_thread_pool_module;
using namespace utility_module;

class BenchmarkTimer {
public:
    BenchmarkTimer() : start_(high_resolution_clock::now()) {}
    
    double elapsed_us() const {
        auto end = high_resolution_clock::now();
        return duration_cast<microseconds>(end - start_).count();
    }
    
    double elapsed_ms() const {
        return elapsed_us() / 1000.0;
    }
    
    void reset() {
        start_ = high_resolution_clock::now();
    }
    
private:
    high_resolution_clock::time_point start_;
};

struct BenchmarkResult {
    std::string name;
    double avg_time;
    double min_time;
    double max_time;
    double std_dev;
    size_t iterations;
};

class ThreadPoolBenchmark {
public:
    ThreadPoolBenchmark() {
        log_module::start();
        log_module::console_target(log_module::log_types::Information);
    }
    
    ~ThreadPoolBenchmark() {
        log_module::stop();
    }
    
    void run_all_benchmarks() {
        log_module::write_information("\n=== Thread System Performance Benchmarks ===\n");
        
        benchmark_pool_creation();
        benchmark_job_submission_latency();
        benchmark_job_throughput();
        benchmark_scaling_efficiency();
        benchmark_priority_scheduling();
        
        log_module::write_information("\n=== Benchmark Complete ===\n");
    }
    
private:
    void benchmark_pool_creation() {
        log_module::write_information("\n1. Thread Pool Creation Overhead\n");
        log_module::write_information("--------------------------------\n");
        
        std::vector<size_t> worker_counts = {1, 4, 8, 16, 32};
        
        for (size_t count : worker_counts) {
            std::vector<double> times;
            const size_t iterations = 100;
            
            for (size_t i = 0; i < iterations; ++i) {
                BenchmarkTimer timer;
                
                auto [pool, error] = create_default(count);
                if (error) {
                    log_module::write_error("Error creating pool: {}", *error);
                    continue;
                }
                
                double elapsed = timer.elapsed_us();
                times.push_back(elapsed);
                
                // Let the pool go out of scope to measure full lifecycle
            }
            
            auto result = calculate_stats(times);
            log_module::write_information("{:3d} workers: avg={:.1f}μs, min={:.1f}μs, max={:.1f}μs",
                                        count, result.avg_time, result.min_time, result.max_time);
        }
    }
    
    void benchmark_job_submission_latency() {
        log_module::write_information("\n2. Job Submission Latency\n");
        log_module::write_information("-------------------------\n");
        
        auto [pool, error] = create_default(8);
        if (error) {
            log_module::write_error("Error creating pool: {}", *error);
            return;
        }
        
        pool->start();
        
        // Test with different queue states
        std::vector<size_t> queue_sizes = {0, 100, 1000, 10000};
        
        for (size_t queue_size : queue_sizes) {
            // Pre-fill queue
            for (size_t i = 0; i < queue_size; ++i) {
                pool->add_job([] { 
                    std::this_thread::sleep_for(milliseconds(100));
                });
            }
            
            // Measure submission latency
            std::vector<double> times;
            const size_t iterations = 10000;
            
            for (size_t i = 0; i < iterations; ++i) {
                BenchmarkTimer timer;
                
                pool->add_job([] {});
                
                double elapsed = timer.elapsed_us();
                times.push_back(elapsed);
            }
            
            auto result = calculate_stats(times);
            log_module::write_information("Queue size {:5d}: avg={:.1f}μs, 99%={:.1f}μs",
                                        queue_size, result.avg_time, calculate_percentile(times, 99));
            
            // Clear queue
            pool->stop();
            pool->start();
        }
        
        pool->stop();
    }
    
    void benchmark_job_throughput() {
        log_module::write_information("\n3. Job Throughput\n");
        log_module::write_information("-----------------\n");
        
        std::vector<size_t> worker_counts = {4, 8, 16};
        std::vector<size_t> job_durations_us = {0, 1, 10, 100, 1000};
        
        for (size_t duration_us : job_durations_us) {
            log_module::write_information("\nJob duration: {}μs", duration_us);
            
            for (size_t workers : worker_counts) {
                auto [pool, error] = create_default(workers);
                if (error) continue;
                
                pool->start();
                
                const size_t num_jobs = (duration_us == 0) ? 1000000 : 
                                       (duration_us <= 10) ? 100000 : 10000;
                
                std::atomic<size_t> completed_jobs{0};
                
                BenchmarkTimer timer;
                
                for (size_t i = 0; i < num_jobs; ++i) {
                    pool->add_job([duration_us, &completed_jobs] {
                        if (duration_us > 0) {
                            auto end = high_resolution_clock::now() + 
                                      microseconds(duration_us);
                            while (high_resolution_clock::now() < end) {
                                // Busy wait
                            }
                        }
                        completed_jobs.fetch_add(1);
                    });
                }
                
                pool->stop();
                
                double elapsed_ms = timer.elapsed_ms();
                double throughput = (num_jobs * 1000.0) / elapsed_ms;
                
                log_module::write_information("  {:2d} workers: {:.0f} jobs/s", workers, throughput);
            }
        }
    }
    
    void benchmark_scaling_efficiency() {
        log_module::write_information("\n4. Scaling Efficiency\n");
        log_module::write_information("---------------------\n");
        
        // CPU-bound workload
        const size_t work_items = 1000000;
        const size_t work_per_item = 1000;
        
        // Baseline: single thread
        double baseline_time = 0;
        {
            BenchmarkTimer timer;
            
            for (size_t i = 0; i < work_items; ++i) {
                volatile double result = 0;
                for (size_t j = 0; j < work_per_item; ++j) {
                    result += std::sin(i * j);
                }
            }
            
            baseline_time = timer.elapsed_ms();
        }
        
        log_module::write_information("Single thread baseline: {:.1f}ms\n", baseline_time);
        
        // Test with multiple workers
        std::vector<size_t> worker_counts = {1, 2, 4, 8, 16};
        
        for (size_t workers : worker_counts) {
            auto [pool, error] = create_default(workers);
            if (error) continue;
            
            pool->start();
            
            std::atomic<size_t> items_processed{0};
            
            BenchmarkTimer timer;
            
            for (size_t i = 0; i < work_items; ++i) {
                pool->add_job([i, work_per_item, &items_processed] {
                    volatile double result = 0;
                    for (size_t j = 0; j < work_per_item; ++j) {
                        result += std::sin(i * j);
                    }
                    items_processed.fetch_add(1);
                });
            }
            
            pool->stop();
            
            double elapsed = timer.elapsed_ms();
            double speedup = baseline_time / elapsed;
            double efficiency = (speedup / workers) * 100;
            
            log_module::write_information("{:2d} workers: time={:.1f}ms, speedup={:.2f}x, efficiency={:.1f}%",
                                         workers, elapsed, speedup, efficiency);
        }
    }
    
    void benchmark_priority_scheduling() {
        log_module::write_information("\n5. Type Scheduling Performance\n");
        log_module::write_information("----------------------------------\n");
        
        enum class Type { RealTime = 1, Medium = 5, Background = 10 };
        
        auto [pool, error] = create_priority_default<Type>(8);
        if (error) {
            log_module::write_error("Error creating priority pool: {}", *error);
            return;
        }
        
        pool->start();
        
        const size_t jobs_per_priority = 1000;
        std::atomic<size_t> high_completed{0};
        std::atomic<size_t> medium_completed{0};
        std::atomic<size_t> low_completed{0};
        
        // Submit jobs with different types
        for (size_t i = 0; i < jobs_per_priority; ++i) {
            pool->add_job(
                [&high_completed] { 
                    std::this_thread::sleep_for(microseconds(10));
                    high_completed.fetch_add(1);
                }, 
                Type::RealTime
            );
            
            pool->add_job(
                [&medium_completed] { 
                    std::this_thread::sleep_for(microseconds(10));
                    medium_completed.fetch_add(1);
                }, 
                Type::Medium
            );
            
            pool->add_job(
                [&low_completed] { 
                    std::this_thread::sleep_for(microseconds(10));
                    low_completed.fetch_add(1);
                }, 
                Type::Background
            );
        }
        
        // Sample completion order at intervals
        std::vector<size_t> high_samples, medium_samples, low_samples;
        
        for (int i = 0; i < 10; ++i) {
            std::this_thread::sleep_for(milliseconds(50));
            high_samples.push_back(high_completed.load());
            medium_samples.push_back(medium_completed.load());
            low_samples.push_back(low_completed.load());
        }
        
        pool->stop();
        
        // Analyze priority ordering
        log_module::write_information("Completion order (sampled):");
        log_module::write_information("Time(ms)  RealTime  Medium  Background");
        for (size_t i = 0; i < high_samples.size(); ++i) {
            log_module::write_information("{:7d}  {:4d}  {:6d}  {:3d}",
                                        (i + 1) * 50, high_samples[i], medium_samples[i], low_samples[i]);
        }
        
        log_module::write_information("\nFinal: RealTime={}, Medium={}, Background={}",
                                    high_completed.load(), medium_completed.load(), low_completed.load());
    }
    
    BenchmarkResult calculate_stats(const std::vector<double>& times) {
        BenchmarkResult result;
        result.iterations = times.size();
        
        if (times.empty()) {
            return result;
        }
        
        // Calculate average
        result.avg_time = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
        
        // Calculate min/max
        auto [min_it, max_it] = std::minmax_element(times.begin(), times.end());
        result.min_time = *min_it;
        result.max_time = *max_it;
        
        // Calculate standard deviation
        double variance = 0;
        for (double time : times) {
            variance += std::pow(time - result.avg_time, 2);
        }
        result.std_dev = std::sqrt(variance / times.size());
        
        return result;
    }
    
    double calculate_percentile(std::vector<double> times, double percentile) {
        if (times.empty()) return 0;
        
        std::sort(times.begin(), times.end());
        size_t index = static_cast<size_t>(times.size() * percentile / 100.0);
        return times[std::min(index, times.size() - 1)];
    }
};

int main(int argc, char* argv[]) {
    ThreadPoolBenchmark benchmark;
    benchmark.run_all_benchmarks();
    
    return 0;
}