/**
 * @file stress_test_benchmark.cpp
 * @brief Stress tests and edge case benchmarks for Thread System
 * 
 * Tests extreme conditions:
 * - Maximum load scenarios
 * - Resource exhaustion
 * - Error recovery
 * - Edge cases
 */

#include <iostream>
#include <chrono>
#include <vector>
#include <atomic>
#include <random>
#include <thread>
#include <future>
#include <limits>

#include "thread_pool.h"
#include "priority_thread_pool.h"
#include "logger.h"

using namespace std::chrono;
using namespace thread_pool_module;
using namespace priority_thread_pool_module;

class StressTestBenchmark {
public:
    StressTestBenchmark() {
        log_module::start();
        log_module::console_target(log_module::log_types::Error | log_module::log_types::Warning);
    }
    
    ~StressTestBenchmark() {
        log_module::stop();
    }
    
    void run_all_tests() {
        std::cout << "\n=== Stress Test Benchmarks ===\n" << std::endl;
        
        test_maximum_threads();
        test_queue_overflow();
        test_rapid_start_stop();
        test_exception_handling();
        test_memory_pressure();
        test_priority_starvation();
        test_thundering_herd();
        test_cascading_failures();
        
        std::cout << "\n=== Stress Tests Complete ===\n" << std::endl;
    }
    
private:
    void test_maximum_threads() {
        std::cout << "\n1. Maximum Thread Creation Test\n";
        std::cout << "-------------------------------\n";
        
        std::vector<size_t> thread_counts = {100, 500, 1000, 2000, 5000};
        
        for (size_t count : thread_counts) {
            auto start = high_resolution_clock::now();
            
            auto [pool, error] = create_default(count);
            
            if (error) {
                std::cout << std::setw(5) << count << " threads: FAILED - " 
                         << *error << std::endl;
                break;
            }
            
            auto result = pool->start();
            
            if (!result) {
                std::cout << std::setw(5) << count << " threads: FAILED - " 
                         << result.error() << std::endl;
                break;
            }
            
            auto end = high_resolution_clock::now();
            double creation_time_ms = duration_cast<milliseconds>(end - start).count();
            
            // Test basic functionality
            std::atomic<size_t> completed{0};
            const size_t test_jobs = 1000;
            
            for (size_t i = 0; i < test_jobs; ++i) {
                pool->add_job([&completed] {
                    completed.fetch_add(1);
                });
            }
            
            pool->stop();
            
            std::cout << std::setw(5) << count << " threads: "
                     << "Created in " << std::fixed << std::setprecision(0) 
                     << creation_time_ms << "ms, "
                     << "Completed " << completed.load() << "/" << test_jobs << " jobs"
                     << std::endl;
        }
    }
    
    void test_queue_overflow() {
        std::cout << "\n2. Queue Overflow Test\n";
        std::cout << "----------------------\n";
        
        auto [pool, error] = create_default(4);
        if (error) return;
        
        pool->start();
        
        // Submit jobs that take time to process
        const size_t slow_jobs = 100;
        for (size_t i = 0; i < slow_jobs; ++i) {
            pool->add_job([] {
                std::this_thread::sleep_for(seconds(10));
            });
        }
        
        // Now flood with many quick jobs
        std::vector<size_t> flood_sizes = {10000, 100000, 1000000};
        
        for (size_t flood_size : flood_sizes) {
            auto start = high_resolution_clock::now();
            
            try {
                for (size_t i = 0; i < flood_size; ++i) {
                    pool->add_job([] {
                        // Quick job
                    });
                }
                
                auto end = high_resolution_clock::now();
                double submission_time_ms = duration_cast<milliseconds>(end - start).count();
                double submission_rate = (flood_size * 1000.0) / submission_time_ms;
                
                std::cout << std::setw(8) << flood_size << " jobs: "
                         << "Submitted in " << std::fixed << std::setprecision(0)
                         << submission_time_ms << "ms ("
                         << std::setprecision(0) << submission_rate << " jobs/s)"
                         << std::endl;
                         
            } catch (const std::exception& e) {
                std::cout << std::setw(8) << flood_size << " jobs: "
                         << "FAILED - " << e.what() << std::endl;
                break;
            }
        }
        
        pool->stop();
    }
    
    void test_rapid_start_stop() {
        std::cout << "\n3. Rapid Start/Stop Cycles\n";
        std::cout << "--------------------------\n";
        
        const size_t num_cycles = 1000;
        size_t successful_cycles = 0;
        std::vector<double> cycle_times;
        
        auto [pool, error] = create_default(8);
        if (error) return;
        
        for (size_t i = 0; i < num_cycles; ++i) {
            auto cycle_start = high_resolution_clock::now();
            
            auto start_result = pool->start();
            if (!start_result) {
                std::cout << "Start failed at cycle " << i << ": " 
                         << start_result.error() << std::endl;
                break;
            }
            
            // Submit a few jobs
            std::atomic<int> counter{0};
            for (int j = 0; j < 10; ++j) {
                pool->add_job([&counter] {
                    counter.fetch_add(1);
                });
            }
            
            auto stop_result = pool->stop();
            if (!stop_result) {
                std::cout << "Stop failed at cycle " << i << ": " 
                         << stop_result.error() << std::endl;
                break;
            }
            
            auto cycle_end = high_resolution_clock::now();
            double cycle_time_us = duration_cast<microseconds>(cycle_end - cycle_start).count();
            cycle_times.push_back(cycle_time_us);
            
            successful_cycles++;
        }
        
        if (!cycle_times.empty()) {
            double avg_cycle_time = std::accumulate(cycle_times.begin(), cycle_times.end(), 0.0) 
                                  / cycle_times.size();
            auto [min_it, max_it] = std::minmax_element(cycle_times.begin(), cycle_times.end());
            
            std::cout << "Completed " << successful_cycles << "/" << num_cycles << " cycles\n"
                     << "Average cycle time: " << std::fixed << std::setprecision(1) 
                     << avg_cycle_time << "μs\n"
                     << "Min: " << *min_it << "μs, Max: " << *max_it << "μs"
                     << std::endl;
        }
    }
    
    void test_exception_handling() {
        std::cout << "\n4. Exception Handling Under Load\n";
        std::cout << "--------------------------------\n";
        
        auto [pool, error] = create_default(8);
        if (error) return;
        
        pool->start();
        
        const size_t total_jobs = 10000;
        const double exception_rate = 0.1;  // 10% of jobs throw exceptions
        
        std::atomic<size_t> successful_jobs{0};
        std::atomic<size_t> failed_jobs{0};
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        
        auto start = high_resolution_clock::now();
        
        for (size_t i = 0; i < total_jobs; ++i) {
            pool->add_job([&dis, &gen, &successful_jobs, &failed_jobs, exception_rate]() 
                         -> std::optional<std::string> {
                if (dis(gen) < exception_rate) {
                    failed_jobs.fetch_add(1);
                    return "Simulated job failure";
                }
                
                // Simulate some work
                volatile int sum = 0;
                for (int j = 0; j < 1000; ++j) {
                    sum += j;
                }
                
                successful_jobs.fetch_add(1);
                return std::nullopt;
            });
        }
        
        pool->stop();
        
        auto end = high_resolution_clock::now();
        double elapsed_ms = duration_cast<milliseconds>(end - start).count();
        
        std::cout << "Total jobs: " << total_jobs << "\n"
                 << "Successful: " << successful_jobs.load() << "\n"
                 << "Failed: " << failed_jobs.load() << "\n"
                 << "Time: " << std::fixed << std::setprecision(0) << elapsed_ms << "ms\n"
                 << "Throughput: " << (total_jobs * 1000.0 / elapsed_ms) << " jobs/s"
                 << std::endl;
    }
    
    void test_memory_pressure() {
        std::cout << "\n5. Memory Pressure Test\n";
        std::cout << "-----------------------\n";
        
        auto [pool, error] = create_default(8);
        if (error) return;
        
        pool->start();
        
        // Test with increasingly large captured data
        std::vector<size_t> data_sizes_mb = {1, 10, 50, 100};
        
        for (size_t size_mb : data_sizes_mb) {
            std::atomic<size_t> completed{0};
            std::atomic<bool> out_of_memory{false};
            
            const size_t num_jobs = 100;
            
            auto start = high_resolution_clock::now();
            
            try {
                for (size_t i = 0; i < num_jobs; ++i) {
                    // Create large data to capture
                    std::vector<char> large_data(size_mb * 1024 * 1024);
                    std::fill(large_data.begin(), large_data.end(), 'X');
                    
                    pool->add_job([data = std::move(large_data), &completed] {
                        // Access data to ensure it's not optimized away
                        volatile char c = data[data.size() / 2];
                        (void)c;
                        
                        completed.fetch_add(1);
                    });
                }
                
                pool->stop();
                pool->start();  // Reset for next test
                
                auto end = high_resolution_clock::now();
                double elapsed_ms = duration_cast<milliseconds>(end - start).count();
                
                std::cout << std::setw(3) << size_mb << "MB per job: "
                         << "Completed " << completed.load() << "/" << num_jobs
                         << " in " << std::fixed << std::setprecision(0) << elapsed_ms << "ms"
                         << std::endl;
                         
            } catch (const std::bad_alloc&) {
                out_of_memory = true;
                std::cout << std::setw(3) << size_mb << "MB per job: "
                         << "OUT OF MEMORY after " << completed.load() << " jobs"
                         << std::endl;
                break;
            }
        }
        
        pool->stop();
    }
    
    void test_priority_starvation() {
        std::cout << "\n6. Priority Starvation Test\n";
        std::cout << "---------------------------\n";
        
        enum class Priority { 
            Highest = 1,
            High = 10,
            Medium = 50,
            Low = 100,
            Lowest = 1000
        };
        
        auto [pool, error] = create_priority_default<Priority>(4);
        if (error) return;
        
        pool->start();
        
        std::atomic<size_t> highest_completed{0};
        std::atomic<size_t> high_completed{0};
        std::atomic<size_t> medium_completed{0};
        std::atomic<size_t> low_completed{0};
        std::atomic<size_t> lowest_completed{0};
        
        const size_t jobs_per_priority = 1000;
        
        // Submit all jobs
        for (size_t i = 0; i < jobs_per_priority; ++i) {
            pool->add_job([&highest_completed] {
                std::this_thread::sleep_for(microseconds(100));
                highest_completed.fetch_add(1);
            }, Priority::Highest);
            
            pool->add_job([&high_completed] {
                std::this_thread::sleep_for(microseconds(100));
                high_completed.fetch_add(1);
            }, Priority::High);
            
            pool->add_job([&medium_completed] {
                std::this_thread::sleep_for(microseconds(100));
                medium_completed.fetch_add(1);
            }, Priority::Medium);
            
            pool->add_job([&low_completed] {
                std::this_thread::sleep_for(microseconds(100));
                low_completed.fetch_add(1);
            }, Priority::Low);
            
            pool->add_job([&lowest_completed] {
                std::this_thread::sleep_for(microseconds(100));
                lowest_completed.fetch_add(1);
            }, Priority::Lowest);
        }
        
        // Check progress at intervals
        std::cout << "Time(s)  Highest  High  Medium  Low  Lowest\n";
        
        for (int seconds = 1; seconds <= 10; ++seconds) {
            std::this_thread::sleep_for(seconds(1));
            
            std::cout << std::setw(7) << seconds << "  "
                     << std::setw(7) << highest_completed.load() << "  "
                     << std::setw(4) << high_completed.load() << "  "
                     << std::setw(6) << medium_completed.load() << "  "
                     << std::setw(3) << low_completed.load() << "  "
                     << std::setw(6) << lowest_completed.load()
                     << std::endl;
                     
            // Check if low priority jobs are starving
            if (highest_completed.load() == jobs_per_priority &&
                high_completed.load() == jobs_per_priority &&
                lowest_completed.load() == 0) {
                std::cout << "WARNING: Lowest priority jobs are starving!" << std::endl;
            }
        }
        
        pool->stop();
    }
    
    void test_thundering_herd() {
        std::cout << "\n7. Thundering Herd Test\n";
        std::cout << "-----------------------\n";
        
        auto [pool, error] = create_default(8);
        if (error) return;
        
        pool->start();
        
        const size_t num_waiters = 1000;
        std::promise<void> start_signal;
        auto start_future = start_signal.get_future();
        
        std::atomic<size_t> started{0};
        std::atomic<size_t> completed{0};
        
        // Create many jobs that all wait for the same signal
        for (size_t i = 0; i < num_waiters; ++i) {
            pool->add_job([&start_future, &started, &completed] {
                // Wait for signal
                start_future.wait();
                started.fetch_add(1);
                
                // Simulate work
                volatile int sum = 0;
                for (int j = 0; j < 10000; ++j) {
                    sum += j;
                }
                
                completed.fetch_add(1);
            });
        }
        
        // Give jobs time to queue up
        std::this_thread::sleep_for(milliseconds(100));
        
        // Release the herd
        auto release_time = high_resolution_clock::now();
        start_signal.set_value();
        
        // Monitor progress
        std::vector<std::pair<size_t, size_t>> progress;
        
        for (int i = 0; i < 50; ++i) {  // Monitor for 500ms
            std::this_thread::sleep_for(milliseconds(10));
            progress.push_back({started.load(), completed.load()});
        }
        
        pool->stop();
        
        // Analyze the thundering herd behavior
        std::cout << "Jobs started within:\n";
        size_t thresholds[] = {100, 500, 900, 950, 990, 1000};
        
        for (size_t threshold : thresholds) {
            auto it = std::find_if(progress.begin(), progress.end(),
                                  [threshold](const auto& p) { 
                                      return p.first >= threshold; 
                                  });
            
            if (it != progress.end()) {
                size_t time_ms = (it - progress.begin()) * 10;
                std::cout << "  " << std::setw(4) << threshold << " jobs: " 
                         << time_ms << "ms\n";
            }
        }
    }
    
    void test_cascading_failures() {
        std::cout << "\n8. Cascading Failure Test\n";
        std::cout << "-------------------------\n";
        
        auto [pool, error] = create_default(8);
        if (error) return;
        
        pool->start();
        
        // Simulate a chain of dependent tasks where failure propagates
        const size_t chain_length = 100;
        const size_t num_chains = 10;
        
        std::atomic<size_t> successful_chains{0};
        std::atomic<size_t> failed_chains{0};
        
        for (size_t chain = 0; chain < num_chains; ++chain) {
            // Decide if this chain will fail
            bool will_fail = (chain % 3 == 0);  // Every 3rd chain fails
            
            std::vector<std::promise<bool>> promises(chain_length);
            std::vector<std::future<bool>> futures;
            
            for (auto& promise : promises) {
                futures.push_back(promise.get_future());
            }
            
            // Submit chain of dependent tasks
            for (size_t i = 0; i < chain_length; ++i) {
                pool->add_job([i, &futures, &promises, will_fail, chain_length, 
                              &successful_chains, &failed_chains] {
                    // Wait for previous task (except first)
                    if (i > 0) {
                        try {
                            bool prev_success = futures[i-1].get();
                            if (!prev_success) {
                                // Previous task failed, propagate failure
                                promises[i].set_value(false);
                                
                                // If this is the last task, count the failed chain
                                if (i == chain_length - 1) {
                                    failed_chains.fetch_add(1);
                                }
                                return;
                            }
                        } catch (...) {
                            promises[i].set_value(false);
                            return;
                        }
                    }
                    
                    // Simulate work
                    std::this_thread::sleep_for(microseconds(100));
                    
                    // Inject failure at middle of chain
                    if (will_fail && i == chain_length / 2) {
                        promises[i].set_value(false);
                    } else {
                        promises[i].set_value(true);
                        
                        // If this is the last task and we succeeded, count it
                        if (i == chain_length - 1) {
                            successful_chains.fetch_add(1);
                        }
                    }
                });
            }
        }
        
        pool->stop();
        
        std::cout << "Total chains: " << num_chains << "\n"
                 << "Successful: " << successful_chains.load() << "\n"
                 << "Failed: " << failed_chains.load() << "\n"
                 << "Failure propagation rate: " 
                 << std::fixed << std::setprecision(1)
                 << (failed_chains.load() * 100.0 / num_chains) << "%"
                 << std::endl;
    }
};

int main() {
    StressTestBenchmark benchmark;
    benchmark.run_all_tests();
    
    return 0;
}