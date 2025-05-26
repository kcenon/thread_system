/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.
*****************************************************************************/

/**
 * @file contention_benchmark.cpp
 * @brief Benchmark for testing thread pool behavior under high contention scenarios
 * 
 * Tests queue contention, lock contention, and resource competition scenarios.
 */

#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <random>
#include <memory>
#include <unordered_map>

#include "thread_pool.h"
#include "logger.h"
#include "formatter.h"

using namespace thread_pool_module;
using namespace log_module;

class contention_benchmark {
private:
    struct contention_metrics {
        std::chrono::milliseconds total_time;
        std::atomic<uint64_t> jobs_completed{0};
        std::atomic<uint64_t> lock_contentions{0};
        std::atomic<uint64_t> queue_collisions{0};
        std::atomic<uint64_t> cache_misses{0};
        double throughput_jobs_per_sec;
        double contention_ratio;
    };

    std::shared_ptr<thread_pool> pool_;
    std::atomic<bool> running_{false};

public:
    void run_all_contention_tests() {
        information(format_string("=== Thread Pool Contention Benchmark ===\n"));

        test_queue_contention();
        test_shared_resource_contention();
        test_memory_contention();
        test_producer_consumer_contention();
        test_cascading_dependencies();
    }

private:
    void test_queue_contention() {
        information(format_string("--- Queue Contention Test ---"));
        information(format_string("Testing job submission/retrieval contention with multiple threads"));

        std::vector<size_t> producer_counts = {1, 2, 4, 8, 16};
        std::vector<size_t> consumer_counts = {1, 2, 4, 8, 16};

        for (auto producers : producer_counts) {
            for (auto consumers : consumer_counts) {
                auto metrics = run_queue_contention_test(producers, consumers, 10000);
                
                information(format_string("Producers: %2zu, Consumers: %2zu -> %8.1f jobs/sec, Contention: %6.2f%%",
                    producers, consumers, metrics.throughput_jobs_per_sec, metrics.contention_ratio));
            }
        }
        information(format_string(""));
    }

    void test_shared_resource_contention() {
        information(format_string("--- Shared Resource Contention Test ---"));
        information(format_string("Testing contention on shared data structures"));

        // Shared counters that all jobs will increment
        std::atomic<uint64_t> shared_counter{0};
        std::mutex shared_mutex;
        std::unordered_map<int, int> shared_map;

        auto metrics = run_shared_resource_test(shared_counter, shared_mutex, shared_map);
        
        information(format_string("Shared resource test completed:"));
        information(format_string("  Throughput: %.1f jobs/sec", metrics.throughput_jobs_per_sec));
        information(format_string("  Lock contentions: %llu", static_cast<unsigned long long>(metrics.lock_contentions.load())));
        information(format_string("  Final counter value: %llu", static_cast<unsigned long long>(shared_counter.load())));
        information(format_string("  Map entries: %zu\n", shared_map.size()));
    }

    void test_memory_contention() {
        information(format_string("--- Memory Contention Test ---"));
        information(format_string("Testing cache line bouncing and false sharing"));

        struct alignas(64) cache_line_data {
            std::atomic<uint64_t> counter{0};
            char padding[64 - sizeof(std::atomic<uint64_t>)];
        };

        const size_t num_cache_lines = std::thread::hardware_concurrency();
        std::vector<cache_line_data> cache_lines(num_cache_lines);

        auto metrics = run_memory_contention_test(cache_lines);
        
        information(format_string("Memory contention test completed:"));
        information(format_string("  Throughput: %.1f jobs/sec", metrics.throughput_jobs_per_sec));
        information(format_string("  Cache misses: %llu", static_cast<unsigned long long>(metrics.cache_misses.load())));
        
        uint64_t total_counts = 0;
        for (const auto& line : cache_lines) {
            total_counts += line.counter.load();
        }
        information(format_string("  Total operations: %llu\n", static_cast<unsigned long long>(total_counts)));
    }

    void test_producer_consumer_contention() {
        information(format_string("--- Producer-Consumer Contention Test ---"));
        information(format_string("Testing high-rate producer vs consumer scenarios"));

        std::vector<double> producer_rates = {0.1, 0.5, 1.0, 2.0, 5.0}; // Jobs per microsecond
        
        for (auto rate : producer_rates) {
            auto metrics = run_producer_consumer_test(rate);
            
            information(format_string("Rate: %4.1f jobs/μs -> Throughput: %8.1f jobs/sec, Queue collisions: %llu",
                rate, metrics.throughput_jobs_per_sec, static_cast<unsigned long long>(metrics.queue_collisions.load())));
        }
        information(format_string(""));
    }

    void test_cascading_dependencies() {
        information(format_string("--- Cascading Dependencies Test ---"));
        information(format_string("Testing jobs that spawn other jobs (dependency chains)"));

        std::vector<size_t> chain_lengths = {2, 4, 8, 16};
        std::vector<size_t> initial_jobs = {100, 500, 1000};

        for (auto chain_len : chain_lengths) {
            for (auto initial : initial_jobs) {
                auto metrics = run_cascading_dependencies_test(chain_len, initial);
                
                information(format_string("Chain: %2zu, Initial: %4zu -> %8.1f jobs/sec, Total jobs: %llu",
                    chain_len, initial, metrics.throughput_jobs_per_sec, static_cast<unsigned long long>(metrics.jobs_completed.load())));
            }
        }
        information(format_string(""));
    }

    contention_metrics run_queue_contention_test(size_t producers, size_t consumers, size_t jobs_per_producer) {
        contention_metrics metrics;
        
        // Create thread pool with specified number of consumers
        pool_ = std::make_shared<thread_pool>();
        pool_->start();

        for (size_t i = 0; i < consumers; ++i) {
            pool_->enqueue(std::make_unique<thread_worker>(pool_));
        }

        running_ = true;
        auto start_time = std::chrono::high_resolution_clock::now();

        // Start producer threads
        std::vector<std::thread> producer_threads;
        for (size_t i = 0; i < producers; ++i) {
            producer_threads.emplace_back([this, &metrics, jobs_per_producer, i]() {
                std::random_device rd;
                std::mt19937 gen(rd() + i);
                std::uniform_int_distribution<> work_dist(100, 1000);

                for (size_t j = 0; j < jobs_per_producer; ++j) {
                    int work_amount = work_dist(gen);
                    
                    auto job = std::make_unique<callback_job>([&metrics, work_amount]() -> result_void {
                        // Simulate work
                        volatile uint64_t sum = 0;
                        for (int k = 0; k < work_amount; ++k) {
                            sum += k;
                        }
                        metrics.jobs_completed.fetch_add(1, std::memory_order_relaxed);
                        return {};
                    });

                    // Measure queue contention
                    auto queue_start = std::chrono::high_resolution_clock::now();
                    pool_->enqueue(std::move(job));
                    auto queue_end = std::chrono::high_resolution_clock::now();
                    
                    if (std::chrono::duration_cast<std::chrono::microseconds>(queue_end - queue_start).count() > 10) {
                        metrics.queue_collisions.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            });
        }

        // Wait for all producers to finish
        for (auto& t : producer_threads) {
            t.join();
        }

        // Wait for all jobs to complete
        const size_t total_jobs = producers * jobs_per_producer;
        while (metrics.jobs_completed.load() < total_jobs) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        metrics.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        metrics.throughput_jobs_per_sec = (total_jobs * 1000.0) / metrics.total_time.count();
        metrics.contention_ratio = (metrics.queue_collisions.load() * 100.0) / total_jobs;

        running_ = false;
        pool_->stop();

        return metrics;
    }

    contention_metrics run_shared_resource_test(std::atomic<uint64_t>& shared_counter, 
                                              std::mutex& shared_mutex,
                                              std::unordered_map<int, int>& shared_map) {
        contention_metrics metrics;
        const size_t num_jobs = 10000;
        
        pool_ = std::make_shared<thread_pool>();
        pool_->start();

        // Add workers
        for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
            pool_->enqueue(std::make_unique<thread_worker>(pool_));
        }

        auto start_time = std::chrono::high_resolution_clock::now();

        // Submit jobs that contend for shared resources
        for (size_t i = 0; i < num_jobs; ++i) {
            auto job = std::make_unique<callback_job>([&, i, &metrics]() -> result_void {
                // Atomic operation (low contention)
                shared_counter.fetch_add(1, std::memory_order_relaxed);

                // Mutex-protected operation (high contention)
                auto lock_start = std::chrono::high_resolution_clock::now();
                {
                    std::lock_guard<std::mutex> lock(shared_mutex);
                    shared_map[i % 100] = i;
                }
                auto lock_end = std::chrono::high_resolution_clock::now();
                
                if (std::chrono::duration_cast<std::chrono::microseconds>(lock_end - lock_start).count() > 50) {
                    metrics.lock_contentions.fetch_add(1, std::memory_order_relaxed);
                }

                metrics.jobs_completed.fetch_add(1, std::memory_order_relaxed);
                return {};
            });

            pool_->enqueue(std::move(job));
        }

        // Wait for completion
        while (metrics.jobs_completed.load() < num_jobs) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        metrics.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        metrics.throughput_jobs_per_sec = (num_jobs * 1000.0) / metrics.total_time.count();

        pool_->stop();
        return metrics;
    }

    contention_metrics run_memory_contention_test(std::vector<cache_line_data>& cache_lines) {
        contention_metrics metrics;
        const size_t num_jobs = 50000;
        
        pool_ = std::make_shared<thread_pool>();
        pool_->start();

        // Add workers
        for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
            pool_->enqueue(std::make_unique<thread_worker>(pool_));
        }

        auto start_time = std::chrono::high_resolution_clock::now();

        // Submit jobs that cause false sharing
        std::random_device rd;
        for (size_t i = 0; i < num_jobs; ++i) {
            auto job = std::make_unique<callback_job>([&cache_lines, &metrics, i]() -> result_void {
                // Access different cache lines to cause bouncing
                size_t line_index = i % cache_lines.size();
                
                // Simulate cache miss with random access pattern
                for (int j = 0; j < 100; ++j) {
                    cache_lines[line_index].counter.fetch_add(1, std::memory_order_relaxed);
                    
                    // Access other cache lines to force cache misses
                    if (j % 10 == 0) {
                        size_t other_line = (line_index + 1) % cache_lines.size();
                        volatile auto value = cache_lines[other_line].counter.load(std::memory_order_relaxed);
                        if (value % 1000 == 0) {
                            metrics.cache_misses.fetch_add(1, std::memory_order_relaxed);
                        }
                    }
                }

                metrics.jobs_completed.fetch_add(1, std::memory_order_relaxed);
                return {};
            });

            pool_->enqueue(std::move(job));
        }

        // Wait for completion
        while (metrics.jobs_completed.load() < num_jobs) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        metrics.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        metrics.throughput_jobs_per_sec = (num_jobs * 1000.0) / metrics.total_time.count();

        pool_->stop();
        return metrics;
    }

    contention_metrics run_producer_consumer_test(double jobs_per_microsecond) {
        contention_metrics metrics;
        const auto test_duration = std::chrono::seconds(5);
        
        pool_ = std::make_shared<thread_pool>();
        pool_->start();

        // Add workers
        for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
            pool_->enqueue(std::make_unique<thread_worker>(pool_));
        }

        running_ = true;
        auto start_time = std::chrono::high_resolution_clock::now();

        // Producer thread
        std::thread producer([this, &metrics, jobs_per_microsecond, test_duration, start_time]() {
            auto next_submit = start_time;
            const auto submit_interval = std::chrono::microseconds(static_cast<int>(1.0 / jobs_per_microsecond));
            
            while (std::chrono::high_resolution_clock::now() - start_time < test_duration) {
                auto now = std::chrono::high_resolution_clock::now();
                if (now >= next_submit) {
                    auto job = std::make_unique<callback_job>([&metrics]() -> result_void {
                        // Simulate light work
                        volatile int sum = 0;
                        for (int i = 0; i < 100; ++i) {
                            sum += i;
                        }
                        metrics.jobs_completed.fetch_add(1, std::memory_order_relaxed);
                        return {};
                    });

                    auto queue_start = std::chrono::high_resolution_clock::now();
                    pool_->enqueue(std::move(job));
                    auto queue_end = std::chrono::high_resolution_clock::now();
                    
                    if (std::chrono::duration_cast<std::chrono::microseconds>(queue_end - queue_start).count() > 5) {
                        metrics.queue_collisions.fetch_add(1, std::memory_order_relaxed);
                    }

                    next_submit += submit_interval;
                } else {
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
            }
        });

        producer.join();

        // Wait for remaining jobs to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto end_time = std::chrono::high_resolution_clock::now();
        metrics.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        metrics.throughput_jobs_per_sec = (metrics.jobs_completed.load() * 1000.0) / metrics.total_time.count();

        running_ = false;
        pool_->stop();
        return metrics;
    }

    contention_metrics run_cascading_dependencies_test(size_t chain_length, size_t initial_jobs) {
        contention_metrics metrics;
        
        pool_ = std::make_shared<thread_pool>();
        pool_->start();

        // Add workers
        for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
            pool_->enqueue(std::make_unique<thread_worker>(pool_));
        }

        auto start_time = std::chrono::high_resolution_clock::now();

        // Submit initial jobs that will spawn chains
        for (size_t i = 0; i < initial_jobs; ++i) {
            submit_chain_job(chain_length, metrics);
        }

        // Wait for all jobs in all chains to complete
        // Expected total jobs = initial_jobs * chain_length
        const size_t expected_jobs = initial_jobs * chain_length;
        while (metrics.jobs_completed.load() < expected_jobs) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        metrics.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        metrics.throughput_jobs_per_sec = (metrics.jobs_completed.load() * 1000.0) / metrics.total_time.count();

        pool_->stop();
        return metrics;
    }

    void submit_chain_job(size_t remaining_depth, contention_metrics& metrics) {
        auto job = std::make_unique<callback_job>([this, remaining_depth, &metrics]() -> result_void {
            // Do some work
            volatile int sum = 0;
            for (int i = 0; i < 200; ++i) {
                sum += i * i;
            }

            metrics.jobs_completed.fetch_add(1, std::memory_order_relaxed);

            // Spawn next job in chain if not at end
            if (remaining_depth > 1) {
                submit_chain_job(remaining_depth - 1, metrics);
            }

            return {};
        });

        pool_->enqueue(std::move(job));
    }

    struct cache_line_data {
        alignas(64) std::atomic<uint64_t> counter{0};
    };
};

int main() {
    set_title("contention_benchmark");
    console_target(log_types::Information | log_types::Warning | log_types::Error);
    start();

    try {
        contention_benchmark benchmark;
        benchmark.run_all_contention_tests();
    } catch (const std::exception& e) {
        error(format_string("Benchmark failed: %s", e.what()));
        return 1;
    }

    stop();
    return 0;
}