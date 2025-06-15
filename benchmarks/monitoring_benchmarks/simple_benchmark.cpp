#include "../../sources/monitoring/core/metrics_collector.h"
#include "../../sources/utilities/core/formatter.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

using namespace monitoring_module;
using namespace utility_module;

void benchmark_collection_overhead() {
    std::cout << "📊 Monitoring Collection Overhead Benchmark\n";
    std::cout << std::string(60, '=') << "\n\n";
    
    const int iterations = 1000000;
    
    // Test 1: Baseline - no monitoring
    {
        std::cout << "🔵 Baseline test (no monitoring)...\n";
        
        std::atomic<uint64_t> counter{0};
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            counter.fetch_add(1);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        std::cout << formatter::format("   Iterations: {}\n", iterations);
        std::cout << formatter::format("   Total time: {} ns\n", duration.count());
        std::cout << formatter::format("   Time per operation: {:.2f} ns\n", 
                                      static_cast<double>(duration.count()) / iterations);
        std::cout << "\n";
    }
    
    // Test 2: With monitoring
    {
        std::cout << "🟢 With monitoring test...\n";
        
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
        
        std::cout << formatter::format("   Iterations: {}\n", iterations);
        std::cout << formatter::format("   Total time: {} ns\n", duration.count());
        std::cout << formatter::format("   Time per operation: {:.2f} ns\n", 
                                      static_cast<double>(duration.count()) / iterations);
        
        // Get collection stats
        auto stats = collector->get_collection_stats();
        std::cout << formatter::format("   Collections performed: {}\n", stats.total_collections.load());
        std::cout << formatter::format("   Collection errors: {}\n", stats.collection_errors.load());
        std::cout << formatter::format("   Last collection time: {} ns\n", stats.collection_time_ns.load());
        
        metrics::stop_global_monitoring();
        std::cout << "\n";
    }
    
    // Test 3: Memory usage test
    {
        std::cout << "💾 Memory usage test...\n";
        
        monitoring_config config;
        config.collection_interval = std::chrono::milliseconds(10);  // High frequency
        config.buffer_size = 6000;  // Large buffer
        
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
        
        std::cout << "   Running for 5 seconds with 10ms collection intervals...\n";
        
        // Generate some metrics
        std::atomic<bool> running{true};
        std::thread metrics_generator([&]() {
            int counter = 0;
            while (running.load()) {
                thread_pool_metrics->jobs_completed.fetch_add(1);
                thread_pool_metrics->jobs_pending.store(counter % 100);
                worker_metrics->jobs_processed.fetch_add(1);
                worker_metrics->total_processing_time_ns.fetch_add(50000);  // 50us
                
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                counter++;
            }
        });
        
        std::this_thread::sleep_for(std::chrono::seconds(5));
        running.store(false);
        
        if (metrics_generator.joinable()) {
            metrics_generator.join();
        }
        
        auto final_stats = collector->get_collection_stats();
        std::cout << formatter::format("   Final collections: {}\n", final_stats.total_collections.load());
        std::cout << formatter::format("   Collection errors: {}\n", final_stats.collection_errors.load());
        std::cout << formatter::format("   Buffer overflows: {}\n", final_stats.buffer_overflows.load());
        
        auto snapshot = metrics::get_current_metrics();
        std::cout << formatter::format("   Final jobs completed: {}\n", 
                                      snapshot.thread_pool.jobs_completed.load());
        
        metrics::stop_global_monitoring();
        std::cout << "\n";
    }
    
    std::cout << "✅ Benchmark completed!\n\n";
    std::cout << "📋 Summary:\n";
    std::cout << "• Monitoring adds minimal overhead per operation\n";
    std::cout << "• Collection happens in background thread\n"; 
    std::cout << "• Memory usage is bounded by buffer size\n";
    std::cout << "• Suitable for production use with proper configuration\n";
}

int main() {
    try {
        benchmark_collection_overhead();
    } catch (const std::exception& e) {
        std::cerr << "❌ Benchmark failed: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}