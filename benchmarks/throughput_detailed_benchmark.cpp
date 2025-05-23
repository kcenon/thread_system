/**
 * @file throughput_detailed_benchmark.cpp
 * @brief Detailed job throughput analysis for Thread System
 * 
 * This benchmark provides in-depth analysis of job throughput under various conditions:
 * - Different job sizes and complexities
 * - Various queue configurations
 * - Different worker counts
 * - Impact of job dependencies
 * - Effect of memory allocation patterns
 * - Throughput degradation over time
 */

#include <iostream>
#include <chrono>
#include <vector>
#include <atomic>
#include <random>
#include <iomanip>
#include <map>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <memory>
#include <fstream>

#include "thread_pool.h"
#include "priority_thread_pool.h"
#include "logger.h"

using namespace std::chrono;
using namespace thread_pool_module;
using namespace priority_thread_pool_module;

// Job complexity levels
enum class JobComplexity {
    Empty,          // No operation
    Trivial,        // Simple arithmetic
    Light,          // Light computation
    Medium,         // Medium computation
    Heavy,          // Heavy computation
    VeryHeavy,      // Very heavy computation
    Mixed           // Random mix
};

// Job memory patterns
enum class MemoryPattern {
    None,           // No memory allocation
    Small,          // Small allocations (<1KB)
    Medium,         // Medium allocations (1KB-100KB)
    Large,          // Large allocations (100KB-1MB)
    VeryLarge,      // Very large allocations (>1MB)
    Random          // Random sizes
};

class ThroughputDetailedBenchmark {
public:
    ThroughputDetailedBenchmark() {
        log_module::start();
        log_module::console_target(log_module::log_types::Information);
    }
    
    ~ThroughputDetailedBenchmark() {
        log_module::stop();
    }
    
    void run_all_benchmarks() {
        std::cout << "\n=== Detailed Job Throughput Analysis ===\n" << std::endl;
        
        benchmark_job_complexity_impact();
        benchmark_worker_count_scaling();
        benchmark_queue_depth_impact();
        benchmark_memory_allocation_impact();
        benchmark_job_size_distribution();
        benchmark_sustained_throughput();
        benchmark_burst_patterns();
        benchmark_job_dependencies();
        benchmark_priority_impact_on_throughput();
        benchmark_mixed_workload_throughput();
        
        generate_summary_report();
        
        std::cout << "\n=== Throughput Analysis Complete ===\n" << std::endl;
    }
    
private:
    struct ThroughputResult {
        double jobs_per_second;
        double avg_latency_us;
        double p50_latency_us;
        double p95_latency_us;
        double p99_latency_us;
        double cpu_efficiency;
        size_t total_jobs;
        double total_time_ms;
    };
    
    std::map<std::string, std::vector<ThroughputResult>> all_results_;
    
    // Helper function to execute jobs with a specific complexity
    void execute_job_with_complexity(JobComplexity complexity) {
        switch (complexity) {
            case JobComplexity::Empty:
                // No operation
                break;
                
            case JobComplexity::Trivial:
                {
                    volatile int x = 42;
                    x = x * 2 + 1;
                }
                break;
                
            case JobComplexity::Light:
                {
                    volatile double sum = 0;
                    for (int i = 0; i < 100; ++i) {
                        sum += std::sqrt(i);
                    }
                }
                break;
                
            case JobComplexity::Medium:
                {
                    volatile double sum = 0;
                    for (int i = 0; i < 1000; ++i) {
                        sum += std::sin(i) * std::cos(i);
                    }
                }
                break;
                
            case JobComplexity::Heavy:
                {
                    volatile double sum = 0;
                    for (int i = 0; i < 10000; ++i) {
                        sum += std::pow(std::sin(i), 2) + std::pow(std::cos(i), 2);
                    }
                }
                break;
                
            case JobComplexity::VeryHeavy:
                {
                    volatile double sum = 0;
                    for (int i = 0; i < 100000; ++i) {
                        sum += std::log(std::abs(std::sin(i)) + 1) * std::exp(-i/10000.0);
                    }
                }
                break;
                
            case JobComplexity::Mixed:
                {
                    static std::random_device rd;
                    static std::mt19937 gen(rd());
                    static std::uniform_int_distribution<> dis(0, 4);
                    
                    JobComplexity random_complexity = static_cast<JobComplexity>(dis(gen));
                    execute_job_with_complexity(random_complexity);
                }
                break;
        }
    }
    
    // Helper function to allocate memory based on pattern
    std::unique_ptr<char[]> allocate_with_pattern(MemoryPattern pattern) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        size_t size = 0;
        
        switch (pattern) {
            case MemoryPattern::None:
                return nullptr;
                
            case MemoryPattern::Small:
                size = std::uniform_int_distribution<>(100, 1024)(gen);
                break;
                
            case MemoryPattern::Medium:
                size = std::uniform_int_distribution<>(1024, 102400)(gen);
                break;
                
            case MemoryPattern::Large:
                size = std::uniform_int_distribution<>(102400, 1048576)(gen);
                break;
                
            case MemoryPattern::VeryLarge:
                size = std::uniform_int_distribution<>(1048576, 10485760)(gen);
                break;
                
            case MemoryPattern::Random:
                size = std::uniform_int_distribution<>(100, 10485760)(gen);
                break;
        }
        
        if (size > 0) {
            auto buffer = std::make_unique<char[]>(size);
            // Touch memory to ensure allocation
            for (size_t i = 0; i < size; i += 4096) {
                buffer[i] = static_cast<char>(i & 0xFF);
            }
            return buffer;
        }
        
        return nullptr;
    }
    
    ThroughputResult measure_throughput(
        size_t worker_count,
        size_t num_jobs,
        std::function<void()> job_function,
        const std::string& test_name = ""
    ) {
        auto [pool, error] = create_default(worker_count);
        if (error) {
            return ThroughputResult{};
        }
        
        pool->start();
        
        std::vector<double> latencies;
        latencies.reserve(num_jobs);
        std::atomic<size_t> completed_jobs{0};
        
        auto total_start = high_resolution_clock::now();
        
        // Submit all jobs and measure individual latencies
        for (size_t i = 0; i < num_jobs; ++i) {
            auto job_start = high_resolution_clock::now();
            
            pool->add_job([job_function, &completed_jobs, job_start, &latencies] {
                job_function();
                
                auto job_end = high_resolution_clock::now();
                double latency = duration_cast<microseconds>(job_end - job_start).count();
                
                // Store latency (thread-safe for vector with pre-allocated size)
                size_t index = completed_jobs.fetch_add(1);
                if (index < latencies.capacity()) {
                    latencies[index] = latency;
                }
            });
        }
        
        pool->stop();
        
        auto total_end = high_resolution_clock::now();
        double total_time_ms = duration_cast<milliseconds>(total_end - total_start).count();
        
        // Calculate statistics
        ThroughputResult result;
        result.total_jobs = num_jobs;
        result.total_time_ms = total_time_ms;
        result.jobs_per_second = (num_jobs * 1000.0) / total_time_ms;
        
        if (!latencies.empty()) {
            // Resize to actual number of recorded latencies
            latencies.resize(completed_jobs.load());
            
            std::sort(latencies.begin(), latencies.end());
            
            result.avg_latency_us = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
            result.p50_latency_us = latencies[latencies.size() * 50 / 100];
            result.p95_latency_us = latencies[latencies.size() * 95 / 100];
            result.p99_latency_us = latencies[latencies.size() * 99 / 100];
            
            // Estimate CPU efficiency
            double ideal_time_ms = total_time_ms / worker_count;
            result.cpu_efficiency = (ideal_time_ms / total_time_ms) * 100.0;
        }
        
        return result;
    }
    
    void benchmark_job_complexity_impact() {
        std::cout << "\n1. Job Complexity Impact on Throughput\n";
        std::cout << "--------------------------------------\n";
        
        const size_t worker_count = std::thread::hardware_concurrency();
        const size_t base_job_count = 100000;
        
        struct ComplexityTest {
            JobComplexity complexity;
            std::string name;
            size_t job_count;
        };
        
        std::vector<ComplexityTest> tests = {
            {JobComplexity::Empty, "Empty", base_job_count * 10},
            {JobComplexity::Trivial, "Trivial", base_job_count * 5},
            {JobComplexity::Light, "Light", base_job_count},
            {JobComplexity::Medium, "Medium", base_job_count / 2},
            {JobComplexity::Heavy, "Heavy", base_job_count / 10},
            {JobComplexity::VeryHeavy, "VeryHeavy", base_job_count / 100},
            {JobComplexity::Mixed, "Mixed", base_job_count}
        };
        
        std::cout << std::setw(12) << "Complexity"
                 << std::setw(12) << "Jobs/sec"
                 << std::setw(12) << "Avg Latency"
                 << std::setw(12) << "P95 Latency"
                 << std::setw(12) << "P99 Latency"
                 << std::setw(12) << "CPU Eff %"
                 << std::endl;
        std::cout << std::string(84, '-') << std::endl;
        
        for (const auto& test : tests) {
            auto result = measure_throughput(
                worker_count,
                test.job_count,
                [complexity = test.complexity] {
                    execute_job_with_complexity(complexity);
                },
                test.name
            );
            
            all_results_["complexity_" + test.name].push_back(result);
            
            std::cout << std::setw(12) << test.name
                     << std::setw(12) << std::fixed << std::setprecision(0) << result.jobs_per_second
                     << std::setw(12) << std::setprecision(1) << result.avg_latency_us << "μs"
                     << std::setw(12) << result.p95_latency_us << "μs"
                     << std::setw(12) << result.p99_latency_us << "μs"
                     << std::setw(12) << std::setprecision(1) << result.cpu_efficiency
                     << std::endl;
        }
    }
    
    void benchmark_worker_count_scaling() {
        std::cout << "\n2. Worker Count Scaling Analysis\n";
        std::cout << "--------------------------------\n";
        
        std::vector<size_t> worker_counts = {1, 2, 4, 8, 16, 32, 64, 128};
        const size_t num_jobs = 100000;
        
        // Test with different job complexities
        std::vector<std::pair<JobComplexity, std::string>> complexities = {
            {JobComplexity::Light, "Light"},
            {JobComplexity::Medium, "Medium"},
            {JobComplexity::Heavy, "Heavy"}
        };
        
        for (const auto& [complexity, complexity_name] : complexities) {
            std::cout << "\n" << complexity_name << " workload:\n";
            std::cout << std::setw(8) << "Workers"
                     << std::setw(12) << "Jobs/sec"
                     << std::setw(12) << "Speedup"
                     << std::setw(12) << "Efficiency"
                     << std::setw(12) << "Avg Latency"
                     << std::endl;
            
            double baseline_throughput = 0;
            
            for (size_t workers : worker_counts) {
                if (workers > std::thread::hardware_concurrency() * 4) {
                    break;  // Skip unreasonable worker counts
                }
                
                auto result = measure_throughput(
                    workers,
                    num_jobs,
                    [complexity] {
                        execute_job_with_complexity(complexity);
                    }
                );
                
                if (baseline_throughput == 0) {
                    baseline_throughput = result.jobs_per_second;
                }
                
                double speedup = result.jobs_per_second / baseline_throughput;
                double efficiency = (speedup / workers) * 100.0;
                
                all_results_["scaling_" + complexity_name + "_" + std::to_string(workers)].push_back(result);
                
                std::cout << std::setw(8) << workers
                         << std::setw(12) << std::fixed << std::setprecision(0) << result.jobs_per_second
                         << std::setw(12) << std::setprecision(2) << speedup << "x"
                         << std::setw(12) << std::setprecision(1) << efficiency << "%"
                         << std::setw(12) << result.avg_latency_us << "μs"
                         << std::endl;
            }
        }
    }
    
    void benchmark_queue_depth_impact() {
        std::cout << "\n3. Queue Depth Impact on Throughput\n";
        std::cout << "-----------------------------------\n";
        
        const size_t worker_count = 8;
        
        // Test different queue depths by controlling submission rate
        struct QueueTest {
            size_t batch_size;      // Jobs submitted at once
            size_t total_jobs;
            int delay_between_batches_ms;
            std::string description;
        };
        
        std::vector<QueueTest> tests = {
            {1, 10000, 0, "Single job (no queue)"},
            {10, 10000, 0, "Small batches (10)"},
            {100, 10000, 0, "Medium batches (100)"},
            {1000, 10000, 0, "Large batches (1000)"},
            {10000, 10000, 0, "All at once"},
            {100, 10000, 1, "Controlled rate (100/ms)"},
            {1000, 10000, 10, "Controlled rate (100/ms)"}
        };
        
        std::cout << std::setw(30) << "Queue Pattern"
                 << std::setw(12) << "Jobs/sec"
                 << std::setw(12) << "Avg Queue"
                 << std::setw(12) << "Max Queue"
                 << std::setw(12) << "Avg Latency"
                 << std::endl;
        std::cout << std::string(78, '-') << std::endl;
        
        for (const auto& test : tests) {
            auto [pool, error] = create_default(worker_count);
            if (error) continue;
            
            pool->start();
            
            std::atomic<size_t> completed_jobs{0};
            std::atomic<size_t> max_queue_depth{0};
            std::atomic<size_t> total_queue_samples{0};
            std::atomic<size_t> total_queue_depth{0};
            
            // Monitor queue depth
            std::atomic<bool> monitoring{true};
            std::thread monitor([&pool, &max_queue_depth, &total_queue_samples, &total_queue_depth, &monitoring] {
                while (monitoring.load()) {
                    size_t current_depth = pool->queue_size();
                    
                    size_t current_max = max_queue_depth.load();
                    while (current_depth > current_max && 
                           !max_queue_depth.compare_exchange_weak(current_max, current_depth)) {
                        // Update max
                    }
                    
                    total_queue_depth.fetch_add(current_depth);
                    total_queue_samples.fetch_add(1);
                    
                    std::this_thread::sleep_for(microseconds(100));
                }
            });
            
            auto start = high_resolution_clock::now();
            
            // Submit jobs according to pattern
            for (size_t i = 0; i < test.total_jobs; i += test.batch_size) {
                size_t batch_end = std::min(i + test.batch_size, test.total_jobs);
                
                for (size_t j = i; j < batch_end; ++j) {
                    pool->add_job([&completed_jobs] {
                        execute_job_with_complexity(JobComplexity::Medium);
                        completed_jobs.fetch_add(1);
                    });
                }
                
                if (test.delay_between_batches_ms > 0 && batch_end < test.total_jobs) {
                    std::this_thread::sleep_for(milliseconds(test.delay_between_batches_ms));
                }
            }
            
            pool->stop();
            monitoring = false;
            monitor.join();
            
            auto end = high_resolution_clock::now();
            double total_time_ms = duration_cast<milliseconds>(end - start).count();
            double throughput = (test.total_jobs * 1000.0) / total_time_ms;
            double avg_queue = static_cast<double>(total_queue_depth.load()) / total_queue_samples.load();
            double avg_latency = total_time_ms / test.total_jobs * 1000.0;  // Convert to microseconds
            
            std::cout << std::setw(30) << test.description
                     << std::setw(12) << std::fixed << std::setprecision(0) << throughput
                     << std::setw(12) << std::setprecision(1) << avg_queue
                     << std::setw(12) << max_queue_depth.load()
                     << std::setw(12) << avg_latency << "μs"
                     << std::endl;
        }
    }
    
    void benchmark_memory_allocation_impact() {
        std::cout << "\n4. Memory Allocation Impact on Throughput\n";
        std::cout << "-----------------------------------------\n";
        
        const size_t worker_count = std::thread::hardware_concurrency();
        const size_t num_jobs = 50000;
        
        struct MemoryTest {
            MemoryPattern pattern;
            std::string name;
        };
        
        std::vector<MemoryTest> tests = {
            {MemoryPattern::None, "No allocation"},
            {MemoryPattern::Small, "Small (<1KB)"},
            {MemoryPattern::Medium, "Medium (1-100KB)"},
            {MemoryPattern::Large, "Large (100KB-1MB)"},
            {MemoryPattern::VeryLarge, "Very Large (>1MB)"},
            {MemoryPattern::Random, "Random size"}
        };
        
        std::cout << std::setw(20) << "Memory Pattern"
                 << std::setw(12) << "Jobs/sec"
                 << std::setw(12) << "vs No Alloc"
                 << std::setw(12) << "Avg Latency"
                 << std::setw(12) << "P99 Latency"
                 << std::endl;
        std::cout << std::string(68, '-') << std::endl;
        
        double baseline_throughput = 0;
        
        for (const auto& test : tests) {
            auto result = measure_throughput(
                worker_count,
                num_jobs,
                [pattern = test.pattern] {
                    auto buffer = allocate_with_pattern(pattern);
                    execute_job_with_complexity(JobComplexity::Light);
                }
            );
            
            if (baseline_throughput == 0) {
                baseline_throughput = result.jobs_per_second;
            }
            
            double relative_perf = (result.jobs_per_second / baseline_throughput) * 100.0;
            
            all_results_["memory_" + test.name].push_back(result);
            
            std::cout << std::setw(20) << test.name
                     << std::setw(12) << std::fixed << std::setprecision(0) << result.jobs_per_second
                     << std::setw(12) << std::setprecision(1) << relative_perf << "%"
                     << std::setw(12) << result.avg_latency_us << "μs"
                     << std::setw(12) << result.p99_latency_us << "μs"
                     << std::endl;
        }
    }
    
    void benchmark_job_size_distribution() {
        std::cout << "\n5. Job Size Distribution Impact\n";
        std::cout << "-------------------------------\n";
        
        const size_t worker_count = std::thread::hardware_concurrency();
        const size_t total_work_units = 1000000;
        
        struct DistributionTest {
            std::string name;
            std::function<std::vector<int>()> generate_sizes;
        };
        
        std::vector<DistributionTest> tests = {
            {
                "Uniform (all same)",
                [total_work_units] {
                    std::vector<int> sizes(10000, total_work_units / 10000);
                    return sizes;
                }
            },
            {
                "Normal distribution",
                [total_work_units] {
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::normal_distribution<> dis(100.0, 30.0);
                    
                    std::vector<int> sizes;
                    int remaining = total_work_units;
                    while (remaining > 0) {
                        int size = std::max(1, static_cast<int>(dis(gen)));
                        size = std::min(size, remaining);
                        sizes.push_back(size);
                        remaining -= size;
                    }
                    return sizes;
                }
            },
            {
                "Exponential (many small, few large)",
                [total_work_units] {
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::exponential_distribution<> dis(0.01);
                    
                    std::vector<int> sizes;
                    int remaining = total_work_units;
                    while (remaining > 0) {
                        int size = std::max(1, static_cast<int>(dis(gen)));
                        size = std::min(size, remaining);
                        sizes.push_back(size);
                        remaining -= size;
                    }
                    return sizes;
                }
            },
            {
                "Bimodal (small and large)",
                [total_work_units] {
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::bernoulli_distribution coin(0.8);
                    
                    std::vector<int> sizes;
                    int remaining = total_work_units;
                    while (remaining > 0) {
                        int size = coin(gen) ? 10 : 1000;
                        size = std::min(size, remaining);
                        sizes.push_back(size);
                        remaining -= size;
                    }
                    return sizes;
                }
            }
        };
        
        std::cout << std::setw(25) << "Distribution"
                 << std::setw(12) << "Jobs Count"
                 << std::setw(12) << "Jobs/sec"
                 << std::setw(12) << "Units/sec"
                 << std::setw(12) << "Avg Latency"
                 << std::endl;
        std::cout << std::string(73, '-') << std::endl;
        
        for (const auto& test : tests) {
            auto job_sizes = test.generate_sizes();
            
            auto [pool, error] = create_default(worker_count);
            if (error) continue;
            
            pool->start();
            
            std::atomic<size_t> completed_units{0};
            
            auto start = high_resolution_clock::now();
            
            for (int size : job_sizes) {
                pool->add_job([size, &completed_units] {
                    for (int i = 0; i < size; ++i) {
                        execute_job_with_complexity(JobComplexity::Trivial);
                    }
                    completed_units.fetch_add(size);
                });
            }
            
            pool->stop();
            
            auto end = high_resolution_clock::now();
            double total_time_ms = duration_cast<milliseconds>(end - start).count();
            double jobs_per_second = (job_sizes.size() * 1000.0) / total_time_ms;
            double units_per_second = (completed_units.load() * 1000.0) / total_time_ms;
            double avg_latency = total_time_ms / job_sizes.size() * 1000.0;
            
            std::cout << std::setw(25) << test.name
                     << std::setw(12) << job_sizes.size()
                     << std::setw(12) << std::fixed << std::setprecision(0) << jobs_per_second
                     << std::setw(12) << units_per_second
                     << std::setw(12) << std::setprecision(1) << avg_latency << "μs"
                     << std::endl;
        }
    }
    
    void benchmark_sustained_throughput() {
        std::cout << "\n6. Sustained Throughput Over Time\n";
        std::cout << "---------------------------------\n";
        
        const size_t worker_count = std::thread::hardware_concurrency();
        const int duration_seconds = 30;
        const int sample_interval_ms = 1000;
        
        auto [pool, error] = create_default(worker_count);
        if (error) return;
        
        pool->start();
        
        std::atomic<size_t> jobs_submitted{0};
        std::atomic<size_t> jobs_completed{0};
        std::atomic<bool> running{true};
        
        // Job submission thread
        std::thread submitter([&pool, &jobs_submitted, &running] {
            while (running.load()) {
                pool->add_job([&jobs_completed] {
                    execute_job_with_complexity(JobComplexity::Medium);
                    jobs_completed.fetch_add(1);
                });
                jobs_submitted.fetch_add(1);
                
                // Small delay to prevent overwhelming
                if (jobs_submitted.load() % 1000 == 0) {
                    std::this_thread::sleep_for(microseconds(10));
                }
            }
        });
        
        std::cout << "Time(s)  Submitted  Completed  Queue  Submit/s  Complete/s  Efficiency\n";
        std::cout << std::string(70, '-') << std::endl;
        
        auto start = high_resolution_clock::now();
        size_t last_submitted = 0;
        size_t last_completed = 0;
        
        for (int seconds = 1; seconds <= duration_seconds; ++seconds) {
            std::this_thread::sleep_for(milliseconds(sample_interval_ms));
            
            size_t current_submitted = jobs_submitted.load();
            size_t current_completed = jobs_completed.load();
            size_t queue_depth = current_submitted - current_completed;
            
            double submit_rate = (current_submitted - last_submitted) * (1000.0 / sample_interval_ms);
            double complete_rate = (current_completed - last_completed) * (1000.0 / sample_interval_ms);
            double efficiency = (current_completed * 100.0) / current_submitted;
            
            std::cout << std::setw(7) << seconds
                     << std::setw(10) << current_submitted
                     << std::setw(11) << current_completed
                     << std::setw(7) << queue_depth
                     << std::setw(10) << std::fixed << std::setprecision(0) << submit_rate
                     << std::setw(12) << complete_rate
                     << std::setw(12) << std::setprecision(1) << efficiency << "%"
                     << std::endl;
            
            last_submitted = current_submitted;
            last_completed = current_completed;
        }
        
        running = false;
        submitter.join();
        pool->stop();
        
        auto end = high_resolution_clock::now();
        double total_time_s = duration_cast<seconds>(end - start).count();
        
        std::cout << "\nSummary:\n";
        std::cout << "Total jobs: " << jobs_completed.load() << "\n";
        std::cout << "Average throughput: " << (jobs_completed.load() / total_time_s) << " jobs/s\n";
    }
    
    void benchmark_burst_patterns() {
        std::cout << "\n7. Burst Pattern Handling\n";
        std::cout << "-------------------------\n";
        
        const size_t worker_count = std::thread::hardware_concurrency();
        
        struct BurstPattern {
            std::string name;
            size_t burst_size;
            int burst_interval_ms;
            int quiet_period_ms;
            int num_bursts;
        };
        
        std::vector<BurstPattern> patterns = {
            {"Steady stream", 100, 10, 10, 100},
            {"Small bursts", 1000, 0, 100, 20},
            {"Large bursts", 10000, 0, 1000, 5},
            {"Flash crowd", 50000, 0, 5000, 2},
            {"Oscillating", 5000, 0, 500, 10}
        };
        
        std::cout << std::setw(20) << "Pattern"
                 << std::setw(12) << "Total Jobs"
                 << std::setw(12) << "Total Time"
                 << std::setw(12) << "Avg Tput"
                 << std::setw(12) << "Peak Tput"
                 << std::setw(12) << "Efficiency"
                 << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        
        for (const auto& pattern : patterns) {
            auto [pool, error] = create_default(worker_count);
            if (error) continue;
            
            pool->start();
            
            std::atomic<size_t> completed_jobs{0};
            std::vector<double> throughput_samples;
            
            auto start = high_resolution_clock::now();
            size_t total_jobs = 0;
            
            for (int burst = 0; burst < pattern.num_bursts; ++burst) {
                auto burst_start = high_resolution_clock::now();
                
                // Submit burst
                for (size_t i = 0; i < pattern.burst_size; ++i) {
                    pool->add_job([&completed_jobs] {
                        execute_job_with_complexity(JobComplexity::Light);
                        completed_jobs.fetch_add(1);
                    });
                    
                    if (pattern.burst_interval_ms > 0 && i % 100 == 0) {
                        std::this_thread::sleep_for(milliseconds(pattern.burst_interval_ms));
                    }
                }
                
                total_jobs += pattern.burst_size;
                
                // Measure burst completion
                size_t start_completed = completed_jobs.load();
                std::this_thread::sleep_for(milliseconds(100));
                size_t end_completed = completed_jobs.load();
                
                double burst_throughput = (end_completed - start_completed) * 10.0;  // per second
                throughput_samples.push_back(burst_throughput);
                
                // Quiet period
                if (burst < pattern.num_bursts - 1) {
                    std::this_thread::sleep_for(milliseconds(pattern.quiet_period_ms));
                }
            }
            
            pool->stop();
            
            auto end = high_resolution_clock::now();
            double total_time_ms = duration_cast<milliseconds>(end - start).count();
            double avg_throughput = (total_jobs * 1000.0) / total_time_ms;
            double peak_throughput = *std::max_element(throughput_samples.begin(), throughput_samples.end());
            double efficiency = (completed_jobs.load() * 100.0) / total_jobs;
            
            std::cout << std::setw(20) << pattern.name
                     << std::setw(12) << total_jobs
                     << std::setw(12) << std::fixed << std::setprecision(1) << (total_time_ms / 1000.0) << "s"
                     << std::setw(12) << std::setprecision(0) << avg_throughput
                     << std::setw(12) << peak_throughput
                     << std::setw(12) << std::setprecision(1) << efficiency << "%"
                     << std::endl;
        }
    }
    
    void benchmark_job_dependencies() {
        std::cout << "\n8. Job Dependencies Impact\n";
        std::cout << "--------------------------\n";
        
        const size_t worker_count = std::thread::hardware_concurrency();
        
        struct DependencyPattern {
            std::string name;
            size_t chain_length;
            size_t num_chains;
            bool parallel_chains;
        };
        
        std::vector<DependencyPattern> patterns = {
            {"Independent jobs", 1, 10000, true},
            {"Short chains (5)", 5, 2000, true},
            {"Medium chains (20)", 20, 500, true},
            {"Long chains (100)", 100, 100, true},
            {"Sequential chain", 10000, 1, false},
            {"Fan-out (1->10)", 10, 1000, true}
        };
        
        std::cout << std::setw(20) << "Pattern"
                 << std::setw(12) << "Total Jobs"
                 << std::setw(12) << "Time (ms)"
                 << std::setw(12) << "Jobs/sec"
                 << std::setw(15) << "vs Independent"
                 << std::endl;
        std::cout << std::string(71, '-') << std::endl;
        
        double baseline_throughput = 0;
        
        for (const auto& pattern : patterns) {
            auto [pool, error] = create_default(worker_count);
            if (error) continue;
            
            pool->start();
            
            std::atomic<size_t> completed_jobs{0};
            auto start = high_resolution_clock::now();
            
            if (pattern.name == "Fan-out (1->10)") {
                // Special case: fan-out pattern
                for (size_t i = 0; i < pattern.num_chains; ++i) {
                    // Root job
                    pool->add_job([&pool, &completed_jobs] {
                        execute_job_with_complexity(JobComplexity::Light);
                        completed_jobs.fetch_add(1);
                        
                        // Fan out to 10 child jobs
                        for (int j = 0; j < 10; ++j) {
                            pool->add_job([&completed_jobs] {
                                execute_job_with_complexity(JobComplexity::Light);
                                completed_jobs.fetch_add(1);
                            });
                        }
                    });
                }
            } else {
                // Chain patterns
                for (size_t chain = 0; chain < pattern.num_chains; ++chain) {
                    if (!pattern.parallel_chains && chain > 0) {
                        // Wait for previous chain to complete
                        while (completed_jobs.load() < chain * pattern.chain_length) {
                            std::this_thread::sleep_for(microseconds(100));
                        }
                    }
                    
                    // Create chain of dependent jobs
                    std::vector<std::promise<void>> promises(pattern.chain_length);
                    std::vector<std::future<void>> futures;
                    
                    for (auto& promise : promises) {
                        futures.push_back(promise.get_future());
                    }
                    
                    for (size_t i = 0; i < pattern.chain_length; ++i) {
                        pool->add_job([i, &futures, &promises, &completed_jobs, chain_length = pattern.chain_length] {
                            // Wait for previous job in chain
                            if (i > 0) {
                                futures[i-1].get();
                            }
                            
                            execute_job_with_complexity(JobComplexity::Light);
                            completed_jobs.fetch_add(1);
                            
                            // Signal completion
                            if (i < chain_length) {
                                promises[i].set_value();
                            }
                        });
                    }
                }
            }
            
            // Wait for all jobs to complete
            size_t total_jobs = (pattern.name == "Fan-out (1->10)") 
                              ? pattern.num_chains * 11 
                              : pattern.num_chains * pattern.chain_length;
            
            while (completed_jobs.load() < total_jobs) {
                std::this_thread::sleep_for(milliseconds(10));
            }
            
            pool->stop();
            
            auto end = high_resolution_clock::now();
            double elapsed_ms = duration_cast<milliseconds>(end - start).count();
            double throughput = (total_jobs * 1000.0) / elapsed_ms;
            
            if (baseline_throughput == 0) {
                baseline_throughput = throughput;
            }
            
            double relative_perf = (throughput / baseline_throughput) * 100.0;
            
            std::cout << std::setw(20) << pattern.name
                     << std::setw(12) << total_jobs
                     << std::setw(12) << std::fixed << std::setprecision(0) << elapsed_ms
                     << std::setw(12) << throughput
                     << std::setw(15) << std::setprecision(1) << relative_perf << "%"
                     << std::endl;
        }
    }
    
    void benchmark_priority_impact_on_throughput() {
        std::cout << "\n9. Priority Scheduling Impact on Throughput\n";
        std::cout << "------------------------------------------\n";
        
        enum class Priority { 
            Critical = 1,
            High = 10,
            Normal = 50,
            Low = 100,
            Background = 1000
        };
        
        const size_t worker_count = std::thread::hardware_concurrency();
        const size_t jobs_per_priority = 2000;
        
        // Test 1: Equal distribution
        {
            std::cout << "\nEqual distribution across priorities:\n";
            
            auto [pool, error] = create_priority_default<Priority>(worker_count);
            if (!error) {
                pool->start();
                
                std::map<Priority, std::atomic<size_t>> completed;
                for (auto p : {Priority::Critical, Priority::High, Priority::Normal, Priority::Low, Priority::Background}) {
                    completed[p] = 0;
                }
                
                auto start = high_resolution_clock::now();
                
                // Submit jobs with different priorities
                for (size_t i = 0; i < jobs_per_priority; ++i) {
                    for (auto priority : {Priority::Critical, Priority::High, Priority::Normal, Priority::Low, Priority::Background}) {
                        pool->add_job([&completed, priority] {
                            execute_job_with_complexity(JobComplexity::Light);
                            completed[priority].fetch_add(1);
                        }, priority);
                    }
                }
                
                // Sample completion rates
                std::cout << "Time(ms)  Critical  High  Normal  Low  Background\n";
                
                for (int sample = 1; sample <= 10; ++sample) {
                    std::this_thread::sleep_for(milliseconds(100));
                    
                    std::cout << std::setw(8) << sample * 100;
                    for (auto p : {Priority::Critical, Priority::High, Priority::Normal, Priority::Low, Priority::Background}) {
                        std::cout << std::setw(10) << completed[p].load();
                    }
                    std::cout << std::endl;
                }
                
                pool->stop();
                
                auto end = high_resolution_clock::now();
                double total_time_ms = duration_cast<milliseconds>(end - start).count();
                double total_throughput = (jobs_per_priority * 5 * 1000.0) / total_time_ms;
                
                std::cout << "\nTotal throughput: " << std::fixed << std::setprecision(0) 
                         << total_throughput << " jobs/s\n";
            }
        }
        
        // Test 2: Compare with non-priority pool
        {
            std::cout << "\nThroughput comparison:\n";
            
            // Non-priority pool
            auto [normal_pool, error1] = create_default(worker_count);
            if (!error1) {
                normal_pool->start();
                
                auto start = high_resolution_clock::now();
                
                for (size_t i = 0; i < jobs_per_priority * 5; ++i) {
                    normal_pool->add_job([] {
                        execute_job_with_complexity(JobComplexity::Light);
                    });
                }
                
                normal_pool->stop();
                
                auto end = high_resolution_clock::now();
                double normal_time_ms = duration_cast<milliseconds>(end - start).count();
                double normal_throughput = (jobs_per_priority * 5 * 1000.0) / normal_time_ms;
                
                std::cout << "Non-priority pool: " << std::fixed << std::setprecision(0) 
                         << normal_throughput << " jobs/s\n";
            }
            
            // Priority pool
            auto [priority_pool, error2] = create_priority_default<Priority>(worker_count);
            if (!error2) {
                priority_pool->start();
                
                auto start = high_resolution_clock::now();
                
                for (size_t i = 0; i < jobs_per_priority; ++i) {
                    for (auto p : {Priority::Critical, Priority::High, Priority::Normal, Priority::Low, Priority::Background}) {
                        priority_pool->add_job([] {
                            execute_job_with_complexity(JobComplexity::Light);
                        }, p);
                    }
                }
                
                priority_pool->stop();
                
                auto end = high_resolution_clock::now();
                double priority_time_ms = duration_cast<milliseconds>(end - start).count();
                double priority_throughput = (jobs_per_priority * 5 * 1000.0) / priority_time_ms;
                
                std::cout << "Priority pool: " << std::fixed << std::setprecision(0) 
                         << priority_throughput << " jobs/s\n";
            }
        }
    }
    
    void benchmark_mixed_workload_throughput() {
        std::cout << "\n10. Mixed Workload Throughput Analysis\n";
        std::cout << "--------------------------------------\n";
        
        const size_t worker_count = std::thread::hardware_concurrency();
        
        struct WorkloadMix {
            std::string name;
            double cpu_light_pct;
            double cpu_heavy_pct;
            double io_pct;
            double memory_pct;
        };
        
        std::vector<WorkloadMix> mixes = {
            {"CPU only (light)", 100.0, 0.0, 0.0, 0.0},
            {"CPU only (heavy)", 0.0, 100.0, 0.0, 0.0},
            {"I/O only", 0.0, 0.0, 100.0, 0.0},
            {"Memory only", 0.0, 0.0, 0.0, 100.0},
            {"Balanced", 25.0, 25.0, 25.0, 25.0},
            {"Web server", 60.0, 10.0, 25.0, 5.0},
            {"Data processing", 20.0, 50.0, 10.0, 20.0},
            {"Microservice", 40.0, 10.0, 40.0, 10.0}
        };
        
        std::cout << std::setw(20) << "Workload Mix"
                 << std::setw(12) << "Jobs/sec"
                 << std::setw(12) << "Avg Latency"
                 << std::setw(12) << "P95 Latency"
                 << std::setw(12) << "CPU Util %"
                 << std::endl;
        std::cout << std::string(68, '-') << std::endl;
        
        for (const auto& mix : mixes) {
            const size_t total_jobs = 10000;
            
            auto generate_job = [&mix]() -> std::function<void()> {
                static std::random_device rd;
                static std::mt19937 gen(rd());
                static std::uniform_real_distribution<> dis(0.0, 100.0);
                
                double roll = dis(gen);
                
                if (roll < mix.cpu_light_pct) {
                    return [] { execute_job_with_complexity(JobComplexity::Light); };
                } else if (roll < mix.cpu_light_pct + mix.cpu_heavy_pct) {
                    return [] { execute_job_with_complexity(JobComplexity::Heavy); };
                } else if (roll < mix.cpu_light_pct + mix.cpu_heavy_pct + mix.io_pct) {
                    return [] { std::this_thread::sleep_for(milliseconds(5)); };
                } else {
                    return [] { 
                        auto buffer = allocate_with_pattern(MemoryPattern::Medium);
                        execute_job_with_complexity(JobComplexity::Light);
                    };
                }
            };
            
            std::vector<double> latencies;
            latencies.reserve(total_jobs);
            
            auto [pool, error] = create_default(worker_count);
            if (error) continue;
            
            pool->start();
            
            auto start = high_resolution_clock::now();
            auto cpu_start = std::clock();
            
            for (size_t i = 0; i < total_jobs; ++i) {
                auto job_start = high_resolution_clock::now();
                auto job = generate_job();
                
                pool->add_job([job, job_start, &latencies, i] {
                    job();
                    
                    auto job_end = high_resolution_clock::now();
                    double latency = duration_cast<microseconds>(job_end - job_start).count();
                    if (i < latencies.capacity()) {
                        latencies[i] = latency;
                    }
                });
            }
            
            pool->stop();
            
            auto end = high_resolution_clock::now();
            auto cpu_end = std::clock();
            
            double elapsed_ms = duration_cast<milliseconds>(end - start).count();
            double throughput = (total_jobs * 1000.0) / elapsed_ms;
            
            // Calculate CPU utilization
            double cpu_time_ms = 1000.0 * (cpu_end - cpu_start) / CLOCKS_PER_SEC;
            double wall_time_ms = elapsed_ms;
            double cpu_utilization = (cpu_time_ms / (wall_time_ms * worker_count)) * 100.0;
            
            // Calculate latency percentiles
            latencies.resize(std::min(latencies.size(), total_jobs));
            std::sort(latencies.begin(), latencies.end());
            
            double avg_latency = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
            double p95_latency = latencies[latencies.size() * 95 / 100];
            
            std::cout << std::setw(20) << mix.name
                     << std::setw(12) << std::fixed << std::setprecision(0) << throughput
                     << std::setw(12) << std::setprecision(1) << avg_latency << "μs"
                     << std::setw(12) << p95_latency << "μs"
                     << std::setw(12) << cpu_utilization
                     << std::endl;
        }
    }
    
    void generate_summary_report() {
        std::cout << "\n=== Throughput Analysis Summary ===\n";
        std::cout << "\nKey Findings:\n";
        
        // Find best/worst configurations
        double best_throughput = 0;
        double worst_throughput = std::numeric_limits<double>::max();
        std::string best_config, worst_config;
        
        for (const auto& [name, results] : all_results_) {
            if (!results.empty()) {
                double avg_throughput = 0;
                for (const auto& result : results) {
                    avg_throughput += result.jobs_per_second;
                }
                avg_throughput /= results.size();
                
                if (avg_throughput > best_throughput) {
                    best_throughput = avg_throughput;
                    best_config = name;
                }
                
                if (avg_throughput < worst_throughput && avg_throughput > 0) {
                    worst_throughput = avg_throughput;
                    worst_config = name;
                }
            }
        }
        
        std::cout << "\n1. Best throughput configuration: " << best_config 
                 << " (" << std::fixed << std::setprecision(0) << best_throughput << " jobs/s)\n";
        std::cout << "2. Worst throughput configuration: " << worst_config 
                 << " (" << worst_throughput << " jobs/s)\n";
        std::cout << "3. Throughput ratio (best/worst): " 
                 << std::setprecision(1) << (best_throughput / worst_throughput) << "x\n";
        
        // Recommendations
        std::cout << "\nRecommendations:\n";
        std::cout << "- For CPU-bound work: Use " << std::thread::hardware_concurrency() << " workers\n";
        std::cout << "- For I/O-bound work: Use " << (std::thread::hardware_concurrency() * 2) << "-" 
                 << (std::thread::hardware_concurrency() * 4) << " workers\n";
        std::cout << "- For memory-intensive work: Consider memory allocation patterns\n";
        std::cout << "- For mixed workloads: Use priority scheduling to optimize latency\n";
        
        // Save detailed results to file
        std::ofstream report("throughput_analysis_report.csv");
        if (report.is_open()) {
            report << "Test,Jobs/sec,Avg Latency (us),P95 Latency (us),P99 Latency (us),CPU Efficiency (%)\n";
            
            for (const auto& [name, results] : all_results_) {
                for (const auto& result : results) {
                    report << name << ","
                          << result.jobs_per_second << ","
                          << result.avg_latency_us << ","
                          << result.p95_latency_us << ","
                          << result.p99_latency_us << ","
                          << result.cpu_efficiency << "\n";
                }
            }
            
            report.close();
            std::cout << "\nDetailed results saved to: throughput_analysis_report.csv\n";
        }
    }
};

int main() {
    ThroughputDetailedBenchmark benchmark;
    benchmark.run_all_benchmarks();
    
    return 0;
}