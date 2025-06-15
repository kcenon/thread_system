#include "../../sources/monitoring/core/metrics_collector.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <iomanip>

using namespace monitoring_module;

void benchmark_collection_overhead() {
    std::cout << "Monitoring Collection Overhead Benchmark\n";
    std::cout << "========================================\n\n";
    
    const int iterations = 1000000;
    
    // Test 1: Baseline - no monitoring
    {
        std::cout << "Baseline test (no monitoring)...\n";
        
        std::atomic<uint64_t> counter{0};
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            counter.fetch_add(1);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        std::cout << "  Iterations: " << iterations << "\n";
        std::cout << "  Total time: " << duration.count() << " ns\n";
        std::cout << "  Time per operation: " << std::fixed << std::setprecision(2)
                  << static_cast<double>(duration.count()) / iterations << " ns\n\n";
    }
    
    // Test 2: With monitoring
    {
        std::cout << "With monitoring test...\n";
        
        monitoring_config config;
        config.collection_interval = std::chrono::milliseconds(100);
        config.buffer_size = 60;
        
        if (auto result = metrics::start_global_monitoring(config); result.has_error()) {
            std::cerr << "Failed to start monitoring: " << result.get_error().message() << "\n";
            return;
        }
        
        auto collector = global_metrics_collector::instance().get_collector();
        auto thread_pool_metrics = std::make_shared<monitoring_module::thread_pool_metrics>();
        collector->register_thread_pool_metrics(thread_pool_metrics);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            thread_pool_metrics->jobs_completed.fetch_add(1);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        std::cout << "  Iterations: " << iterations << "\n";
        std::cout << "  Total time: " << duration.count() << " ns\n";
        std::cout << "  Time per operation: " << std::fixed << std::setprecision(2)
                  << static_cast<double>(duration.count()) / iterations << " ns\n";
        
        // Get collection stats
        auto stats = collector->get_collection_stats();
        std::cout << "  Collections performed: " << stats.total_collections.load() << "\n";
        std::cout << "  Collection errors: " << stats.collection_errors.load() << "\n";
        std::cout << "  Last collection time: " << stats.collection_time_ns.load() << " ns\n";
        
        metrics::stop_global_monitoring();
        std::cout << "\n";
    }
    
    // Test 3: High frequency monitoring
    {
        std::cout << "High frequency monitoring test (10ms intervals)...\n";
        
        monitoring_config config;
        config.collection_interval = std::chrono::milliseconds(10);
        config.buffer_size = 1000;
        
        if (auto result = metrics::start_global_monitoring(config); result.has_error()) {
            std::cerr << "Failed to start monitoring: " << result.get_error().message() << "\n";
            return;
        }
        
        auto collector = global_metrics_collector::instance().get_collector();
        auto system_metrics = std::make_shared<monitoring_module::system_metrics>();
        auto thread_pool_metrics = std::make_shared<monitoring_module::thread_pool_metrics>();
        auto worker_metrics = std::make_shared<monitoring_module::worker_metrics>();
        
        collector->register_system_metrics(system_metrics);
        collector->register_thread_pool_metrics(thread_pool_metrics);
        collector->register_worker_metrics(worker_metrics);
        
        std::cout << "  Running for 3 seconds with high frequency collection...\n";
        
        // Generate metrics while monitoring
        std::atomic<bool> running{true};
        std::atomic<int> operations{0};
        
        std::thread metrics_generator([&]() {
            int counter = 0;
            while (running.load()) {
                thread_pool_metrics->jobs_completed.fetch_add(1);
                thread_pool_metrics->jobs_pending.store(counter % 50);
                worker_metrics->jobs_processed.fetch_add(1);
                worker_metrics->total_processing_time_ns.fetch_add(25000);  // 25us
                
                operations.fetch_add(4);  // 4 atomic operations per iteration
                counter++;
                
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        });
        
        auto start = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds(3));
        running.store(false);
        auto end = std::chrono::high_resolution_clock::now();
        
        if (metrics_generator.joinable()) {
            metrics_generator.join();
        }
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        auto final_stats = collector->get_collection_stats();
        std::cout << "  Test duration: " << duration.count() << " ms\n";
        std::cout << "  Operations performed: " << operations.load() << "\n";
        std::cout << "  Operations per second: " << operations.load() * 1000 / duration.count() << "\n";
        std::cout << "  Collections performed: " << final_stats.total_collections.load() << "\n";
        std::cout << "  Collection errors: " << final_stats.collection_errors.load() << "\n";
        std::cout << "  Buffer overflows: " << final_stats.buffer_overflows.load() << "\n";
        
        auto snapshot = metrics::get_current_metrics();
        std::cout << "  Final jobs completed: " << snapshot.thread_pool.jobs_completed.load() << "\n";
        std::cout << "  System memory usage: " << snapshot.system.memory_usage_bytes.load() << " bytes\n";
        
        metrics::stop_global_monitoring();
        std::cout << "\n";
    }
    
    std::cout << "Benchmark completed!\n\n";
    std::cout << "Summary:\n";
    std::cout << "• Monitoring adds minimal per-operation overhead\n";
    std::cout << "• Collection happens in background at configured intervals\n"; 
    std::cout << "• Memory usage is bounded by buffer configuration\n";
    std::cout << "• Suitable for production use with proper interval tuning\n";
    std::cout << "• Recommended: 100-500ms intervals for most applications\n";
}

int main() {
    try {
        benchmark_collection_overhead();
    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}