/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.
*****************************************************************************/

/**
 * @file scalability_benchmark.cpp
 * @brief Comprehensive scalability benchmark for thread pools
 * 
 * Tests how thread pools scale with different numbers of threads,
 * workload types, and system configurations.
 */

#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <random>
#include <algorithm>

#include "thread_pool.h"
#include "priority_thread_pool.h"
#include "logger.h"
#include "formatter.h"

using namespace thread_pool_module;
using namespace priority_thread_pool_module;
using namespace log_module;

class scalability_benchmark {
private:
    struct test_config {
        std::vector<size_t> thread_counts{1, 2, 4, 8, 16, std::thread::hardware_concurrency()};
        std::vector<size_t> job_counts{1000, 10000, 100000, 1000000};
        std::vector<std::chrono::microseconds> job_durations{
            std::chrono::microseconds(0),    // CPU-bound
            std::chrono::microseconds(1),    // Very light
            std::chrono::microseconds(10),   // Light
            std::chrono::microseconds(100),  // Medium
            std::chrono::microseconds(1000)  // Heavy
        };
    };

    test_config config_;
    std::atomic<uint64_t> completed_jobs_{0};
    std::atomic<uint64_t> total_work_time_{0};

    struct benchmark_result {
        size_t thread_count;
        size_t job_count;
        std::chrono::microseconds job_duration;
        std::chrono::milliseconds total_time;
        double throughput_jobs_per_sec;
        double efficiency_percent;
        double speedup;
    };

    std::vector<benchmark_result> results_;

public:
    void run_all_benchmarks() {
        information(format_string("=== Thread Pool Scalability Benchmark ==="));
        information(format_string("Hardware concurrency: {} threads\n", std::thread::hardware_concurrency()));

        // Test different workload patterns
        run_cpu_bound_scalability();
        run_io_bound_scalability();
        run_mixed_workload_scalability();
        run_burst_workload_scalability();
        
        print_summary();
    }

private:
    void run_cpu_bound_scalability() {
        information(format_string("--- CPU-Bound Workload Scalability ---"));
        
        for (auto job_count : {10000, 100000}) {
            information(format_string("Testing with {} CPU-intensive jobs:", job_count));
            
            double baseline_time = 0.0;
            
            for (auto thread_count : config_.thread_counts) {
                auto result = benchmark_cpu_workload(thread_count, job_count);
                
                if (thread_count == 1) {
                    baseline_time = result.total_time.count();
                }
                
                result.speedup = baseline_time / result.total_time.count();
                result.efficiency_percent = (result.speedup / thread_count) * 100.0;
                
                results_.push_back(result);
                print_result(result);
            }
            information(format_string(""));
        }
    }

    void run_io_bound_scalability() {
        information(format_string("--- I/O-Bound Workload Scalability ---"));
        
        for (auto delay : {std::chrono::microseconds(100), std::chrono::microseconds(1000)}) {
            information(format_string("Testing with {}μs I/O simulation:", delay.count()));
            
            double baseline_time = 0.0;
            
            for (auto thread_count : config_.thread_counts) {
                auto result = benchmark_io_workload(thread_count, 10000, delay);
                
                if (thread_count == 1) {
                    baseline_time = result.total_time.count();
                }
                
                result.speedup = baseline_time / result.total_time.count();
                result.efficiency_percent = (result.speedup / thread_count) * 100.0;
                
                results_.push_back(result);
                print_result(result);
            }
            information(format_string(""));
        }
    }

    void run_mixed_workload_scalability() {
        information(format_string("--- Mixed Workload Scalability ---"));
        
        for (auto thread_count : config_.thread_counts) {
            auto result = benchmark_mixed_workload(thread_count, 50000);
            results_.push_back(result);
            print_result(result);
        }
        information(format_string(""));
    }

    void run_burst_workload_scalability() {
        information(format_string("--- Burst Workload Scalability ---"));
        
        for (auto thread_count : config_.thread_counts) {
            auto result = benchmark_burst_workload(thread_count);
            results_.push_back(result);
            print_result(result);
        }
        information(format_string(""));
    }

    benchmark_result benchmark_cpu_workload(size_t thread_count, size_t job_count) {
        auto pool = std::make_shared<thread_pool>();
        pool->start();

        // Add workers
        for (size_t i = 0; i < thread_count; ++i) {
            pool->enqueue(std::make_unique<thread_worker>(pool));
        }

        completed_jobs_ = 0;
        auto start_time = std::chrono::high_resolution_clock::now();

        // Submit CPU-intensive jobs
        for (size_t i = 0; i < job_count; ++i) {
            auto job = std::make_unique<callback_job>([this]() -> result_void {
                // CPU-intensive work: prime number calculation
                volatile uint64_t sum = 0;
                for (int j = 0; j < 1000; ++j) {
                    sum += j * j;
                }
                completed_jobs_.fetch_add(1, std::memory_order_relaxed);
                return {};
            });
            
            pool->enqueue(std::move(job));
        }

        // Wait for completion
        while (completed_jobs_.load() < job_count) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        pool->stop();

        benchmark_result result;
        result.thread_count = thread_count;
        result.job_count = job_count;
        result.job_duration = std::chrono::microseconds(0);
        result.total_time = duration;
        result.throughput_jobs_per_sec = (job_count * 1000.0) / duration.count();
        
        return result;
    }

    benchmark_result benchmark_io_workload(size_t thread_count, size_t job_count, 
                                         std::chrono::microseconds io_delay) {
        auto pool = std::make_shared<thread_pool>();
        pool->start();

        // Add workers
        for (size_t i = 0; i < thread_count; ++i) {
            pool->enqueue(std::make_unique<thread_worker>(pool));
        }

        completed_jobs_ = 0;
        auto start_time = std::chrono::high_resolution_clock::now();

        // Submit I/O-bound jobs
        for (size_t i = 0; i < job_count; ++i) {
            auto job = std::make_unique<callback_job>([this, io_delay]() -> result_void {
                // Simulate I/O wait
                std::this_thread::sleep_for(io_delay);
                completed_jobs_.fetch_add(1, std::memory_order_relaxed);
                return {};
            });
            
            pool->enqueue(std::move(job));
        }

        // Wait for completion
        while (completed_jobs_.load() < job_count) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        pool->stop();

        benchmark_result result;
        result.thread_count = thread_count;
        result.job_count = job_count;
        result.job_duration = io_delay;
        result.total_time = duration;
        result.throughput_jobs_per_sec = (job_count * 1000.0) / duration.count();
        
        return result;
    }

    benchmark_result benchmark_mixed_workload(size_t thread_count, size_t job_count) {
        auto pool = std::make_shared<thread_pool>();
        pool->start();

        // Add workers
        for (size_t i = 0; i < thread_count; ++i) {
            pool->enqueue(std::make_unique<thread_worker>(pool));
        }

        completed_jobs_ = 0;
        auto start_time = std::chrono::high_resolution_clock::now();

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> workload_dist(0, 2);

        // Submit mixed workload jobs
        for (size_t i = 0; i < job_count; ++i) {
            int workload_type = workload_dist(gen);
            
            auto job = std::make_unique<callback_job>([this, workload_type]() -> result_void {
                switch (workload_type) {
                    case 0: // CPU-intensive
                        {
                            volatile uint64_t sum = 0;
                            for (int j = 0; j < 500; ++j) {
                                sum += j * j;
                            }
                        }
                        break;
                    case 1: // I/O simulation
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                        break;
                    case 2: // Memory-intensive
                        {
                            std::vector<int> temp(1000);
                            std::iota(temp.begin(), temp.end(), 0);
                            std::sort(temp.begin(), temp.end(), std::greater<int>());
                        }
                        break;
                }
                completed_jobs_.fetch_add(1, std::memory_order_relaxed);
                return {};
            });
            
            pool->enqueue(std::move(job));
        }

        // Wait for completion
        while (completed_jobs_.load() < job_count) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        pool->stop();

        benchmark_result result;
        result.thread_count = thread_count;
        result.job_count = job_count;
        result.job_duration = std::chrono::microseconds(0);
        result.total_time = duration;
        result.throughput_jobs_per_sec = (job_count * 1000.0) / duration.count();
        
        return result;
    }

    benchmark_result benchmark_burst_workload(size_t thread_count) {
        auto pool = std::make_shared<thread_pool>();
        pool->start();

        // Add workers
        for (size_t i = 0; i < thread_count; ++i) {
            pool->enqueue(std::make_unique<thread_worker>(pool));
        }

        completed_jobs_ = 0;
        auto start_time = std::chrono::high_resolution_clock::now();

        // Submit jobs in bursts
        const size_t burst_size = 1000;
        const size_t num_bursts = 10;
        const auto burst_interval = std::chrono::milliseconds(50);

        for (size_t burst = 0; burst < num_bursts; ++burst) {
            // Submit burst of jobs
            for (size_t i = 0; i < burst_size; ++i) {
                auto job = std::make_unique<callback_job>([this]() -> result_void {
                    volatile uint64_t sum = 0;
                    for (int j = 0; j < 100; ++j) {
                        sum += j;
                    }
                    completed_jobs_.fetch_add(1, std::memory_order_relaxed);
                    return {};
                });
                
                pool->enqueue(std::move(job));
            }

            // Wait between bursts
            if (burst < num_bursts - 1) {
                std::this_thread::sleep_for(burst_interval);
            }
        }

        // Wait for completion
        const size_t total_jobs = burst_size * num_bursts;
        while (completed_jobs_.load() < total_jobs) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        pool->stop();

        benchmark_result result;
        result.thread_count = thread_count;
        result.job_count = total_jobs;
        result.job_duration = std::chrono::microseconds(0);
        result.total_time = duration;
        result.throughput_jobs_per_sec = (total_jobs * 1000.0) / duration.count();
        
        return result;
    }

    void print_result(const benchmark_result& result) {
        information(format_string("  {:2} threads: {:6}ms, {:8.2f} jobs/sec, {:5.2f}x speedup, {:5.2f}% efficiency",
                  result.thread_count, result.total_time.count(), result.throughput_jobs_per_sec,
                  result.speedup, result.efficiency_percent));
    }

    void print_summary() {
        information(format_string("=== Scalability Summary ==="));
        
        // Find best and worst efficiency results
        auto best_efficiency = std::max_element(results_.begin(), results_.end(),
            [](const auto& a, const auto& b) { return a.efficiency_percent < b.efficiency_percent; });
        
        auto worst_efficiency = std::min_element(results_.begin(), results_.end(),
            [](const auto& a, const auto& b) { return a.efficiency_percent < b.efficiency_percent; });

        if (best_efficiency != results_.end()) {
            information(format_string("Best efficiency: {:.1f}% with {} threads",
                      best_efficiency->efficiency_percent, best_efficiency->thread_count));
        }

        if (worst_efficiency != results_.end()) {
            information(format_string("Worst efficiency: {:.1f}% with {} threads",
                      worst_efficiency->efficiency_percent, worst_efficiency->thread_count));
        }

        // Calculate average efficiency per thread count
        std::map<size_t, std::vector<double>> efficiency_by_threads;
        for (const auto& result : results_) {
            efficiency_by_threads[result.thread_count].push_back(result.efficiency_percent);
        }

        information(format_string("\nAverage efficiency by thread count:"));
        for (const auto& [thread_count, efficiencies] : efficiency_by_threads) {
            double avg = std::accumulate(efficiencies.begin(), efficiencies.end(), 0.0) / efficiencies.size();
            information(format_string("  {:2} threads: {:.1f}%", thread_count, avg));
        }
    }
};

int main() {
    set_title("scalability_benchmark");
    console_target(log_types::Information | log_types::Warning | log_types::Error);
    start();

    try {
        scalability_benchmark benchmark;
        benchmark.run_all_benchmarks();
    } catch (const std::exception& e) {
        error(format_string("Benchmark failed: {}", e.what()));
        return 1;
    }

    stop();
    return 0;
}