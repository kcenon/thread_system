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

#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include <iomanip>
#include <atomic>
#include <cmath>

#include "thread_pool.h"
#include "priority_thread_pool.h"
#include "logger.h"

using namespace std::chrono;
using namespace thread_pool_module;
using namespace priority_thread_pool_module;

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
        std::cout << "\n=== Thread System Performance Benchmarks ===\n" << std::endl;
        
        benchmark_pool_creation();
        benchmark_job_submission_latency();
        benchmark_job_throughput();
        benchmark_scaling_efficiency();
        benchmark_priority_scheduling();
        
        std::cout << "\n=== Benchmark Complete ===\n" << std::endl;
    }
    
private:
    void benchmark_pool_creation() {
        std::cout << "\n1. Thread Pool Creation Overhead\n";
        std::cout << "--------------------------------\n";
        
        std::vector<size_t> worker_counts = {1, 4, 8, 16, 32};
        
        for (size_t count : worker_counts) {
            std::vector<double> times;
            const size_t iterations = 100;
            
            for (size_t i = 0; i < iterations; ++i) {
                BenchmarkTimer timer;
                
                auto [pool, error] = create_default(count);
                if (error) {
                    std::cerr << "Error creating pool: " << *error << std::endl;
                    continue;
                }
                
                double elapsed = timer.elapsed_us();
                times.push_back(elapsed);
                
                // Let the pool go out of scope to measure full lifecycle
            }
            
            auto result = calculate_stats(times);
            std::cout << std::setw(3) << count << " workers: "
                     << std::fixed << std::setprecision(1)
                     << "avg=" << result.avg_time << "μs, "
                     << "min=" << result.min_time << "μs, "
                     << "max=" << result.max_time << "μs"
                     << std::endl;
        }
    }
    
    void benchmark_job_submission_latency() {
        std::cout << "\n2. Job Submission Latency\n";
        std::cout << "-------------------------\n";
        
        auto [pool, error] = create_default(8);
        if (error) {
            std::cerr << "Error creating pool: " << *error << std::endl;
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
            std::cout << "Queue size " << std::setw(5) << queue_size << ": "
                     << std::fixed << std::setprecision(1)
                     << "avg=" << result.avg_time << "μs, "
                     << "99%=" << calculate_percentile(times, 99) << "μs"
                     << std::endl;
            
            // Clear queue
            pool->stop();
            pool->start();
        }
        
        pool->stop();
    }
    
    void benchmark_job_throughput() {
        std::cout << "\n3. Job Throughput\n";
        std::cout << "-----------------\n";
        
        std::vector<size_t> worker_counts = {4, 8, 16};
        std::vector<size_t> job_durations_us = {0, 1, 10, 100, 1000};
        
        for (size_t duration_us : job_durations_us) {
            std::cout << "\nJob duration: " << duration_us << "μs\n";
            
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
                
                std::cout << "  " << std::setw(2) << workers << " workers: "
                         << std::fixed << std::setprecision(0)
                         << throughput << " jobs/s"
                         << std::endl;
            }
        }
    }
    
    void benchmark_scaling_efficiency() {
        std::cout << "\n4. Scaling Efficiency\n";
        std::cout << "---------------------\n";
        
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
        
        std::cout << "Single thread baseline: " 
                 << std::fixed << std::setprecision(1) 
                 << baseline_time << "ms\n\n";
        
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
            
            std::cout << std::setw(2) << workers << " workers: "
                     << "time=" << std::fixed << std::setprecision(1) << elapsed << "ms, "
                     << "speedup=" << std::setprecision(2) << speedup << "x, "
                     << "efficiency=" << std::setprecision(1) << efficiency << "%"
                     << std::endl;
        }
    }
    
    void benchmark_priority_scheduling() {
        std::cout << "\n5. Priority Scheduling Performance\n";
        std::cout << "----------------------------------\n";
        
        enum class Priority { High = 1, Medium = 5, Low = 10 };
        
        auto [pool, error] = create_priority_default<Priority>(8);
        if (error) {
            std::cerr << "Error creating priority pool: " << *error << std::endl;
            return;
        }
        
        pool->start();
        
        const size_t jobs_per_priority = 1000;
        std::atomic<size_t> high_completed{0};
        std::atomic<size_t> medium_completed{0};
        std::atomic<size_t> low_completed{0};
        
        // Submit jobs with different priorities
        for (size_t i = 0; i < jobs_per_priority; ++i) {
            pool->add_job(
                [&high_completed] { 
                    std::this_thread::sleep_for(microseconds(10));
                    high_completed.fetch_add(1);
                }, 
                Priority::High
            );
            
            pool->add_job(
                [&medium_completed] { 
                    std::this_thread::sleep_for(microseconds(10));
                    medium_completed.fetch_add(1);
                }, 
                Priority::Medium
            );
            
            pool->add_job(
                [&low_completed] { 
                    std::this_thread::sleep_for(microseconds(10));
                    low_completed.fetch_add(1);
                }, 
                Priority::Low
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
        std::cout << "Completion order (sampled):\n";
        std::cout << "Time(ms)  High  Medium  Low\n";
        for (size_t i = 0; i < high_samples.size(); ++i) {
            std::cout << std::setw(7) << (i + 1) * 50 << "  "
                     << std::setw(4) << high_samples[i] << "  "
                     << std::setw(6) << medium_samples[i] << "  "
                     << std::setw(3) << low_samples[i]
                     << std::endl;
        }
        
        std::cout << "\nFinal: High=" << high_completed.load()
                 << ", Medium=" << medium_completed.load()
                 << ", Low=" << low_completed.load()
                 << std::endl;
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