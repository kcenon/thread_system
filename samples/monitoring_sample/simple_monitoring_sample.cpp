#include "monitoring/core/metrics_collector.h"
#include "logger/core/logger.h"
#include "utilities/core/formatter.h"

#include <iostream>
#include <chrono>
#include <thread>

using namespace monitoring_module;
using namespace log_module;

void print_header() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "         MONITORING MODULE DEMO\n";
    std::cout << std::string(60, '=') << "\n\n";
}

void print_metrics(const metrics_snapshot& snapshot, int iteration) {
    using namespace utility_module;
    
    std::cout << formatter::format("📊 Snapshot {} at {}s\n", 
                                  iteration, 
                                  std::chrono::duration_cast<std::chrono::seconds>(
                                      snapshot.capture_time.time_since_epoch()).count() % 1000);
    
    std::cout << formatter::format("   Memory: {} bytes | Threads: {}\n",
                                  snapshot.system.memory_usage_bytes.load(),
                                  snapshot.system.active_threads.load());
    
    std::cout << formatter::format("   Pool Jobs: {} completed | {} pending\n",
                                  snapshot.thread_pool.jobs_completed.load(),
                                  snapshot.thread_pool.jobs_pending.load());
    
    std::cout << std::string(60, '-') << "\n";
}

int main() {
    print_header();
    
    // 1. 로거 시작
    std::cout << "🚀 Starting logger...\n";
    if (auto result = start(); result) {
        std::cerr << "Failed to start logger: " << *result << "\n";
        return 1;
    }
    
    console_target(log_types::Information);
    set_title("Monitoring Demo");
    
    // 2. 모니터링 시작
    std::cout << "📈 Starting monitoring system...\n";
    monitoring_config config;
    config.collection_interval = std::chrono::milliseconds(1000);  // 1초 간격
    config.buffer_size = 60;  // 1분간 데이터
    
    if (auto result = metrics::start_global_monitoring(config); result.has_error()) {
        std::cerr << "Failed to start monitoring: " << result.get_error().message() << "\n";
        stop();
        return 1;
    }
    
    // 3. 메트릭 등록
    auto collector = global_metrics_collector::instance().get_collector();
    if (!collector) {
        std::cerr << "Failed to get metrics collector\n";
        metrics::stop_global_monitoring();
        stop();
        return 1;
    }
    
    // 샘플 메트릭 객체 생성
    auto system_metrics = std::make_shared<monitoring_module::system_metrics>();
    auto thread_pool_metrics = std::make_shared<monitoring_module::thread_pool_metrics>();
    auto worker_metrics = std::make_shared<monitoring_module::worker_metrics>();
    
    collector->register_system_metrics(system_metrics);
    collector->register_thread_pool_metrics(thread_pool_metrics);
    collector->register_worker_metrics(worker_metrics);
    
    // 4. 시뮬레이션 데이터 생성
    std::cout << "⚡ Starting simulation...\n";
    std::cout << "   Monitoring for 10 seconds with 2-second intervals\n\n";
    
    std::atomic<bool> running{true};
    std::thread simulator([&]() {
        int job_count = 0;
        while (running.load()) {
            // 메트릭 시뮬레이션
            job_count += 10;
            thread_pool_metrics->jobs_completed.store(job_count);
            thread_pool_metrics->jobs_pending.store(std::max(0, 50 - job_count));
            thread_pool_metrics->worker_threads.store(4);
            thread_pool_metrics->idle_threads.store(std::max(0, 4 - (job_count / 20)));
            
            worker_metrics->jobs_processed.store(job_count);
            worker_metrics->total_processing_time_ns.store(job_count * 10000000); // 10ms per job
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });
    
    // 5. 실시간 모니터링 표시
    for (int i = 1; i <= 5; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        write_information("Monitoring iteration {}", i);
        
        auto snapshot = metrics::get_current_metrics();
        print_metrics(snapshot, i);
    }
    
    // 6. 정리
    std::cout << "\n🛑 Stopping simulation...\n";
    running.store(false);
    
    if (simulator.joinable()) {
        simulator.join();
    }
    
    metrics::stop_global_monitoring();
    stop();
    
    // 7. 최종 통계
    std::cout << "\n📋 Final Statistics:\n";
    auto final_snapshot = metrics::get_current_metrics();
    std::cout << utility_module::formatter::format(
        "   Total Jobs: {}\n"
        "   Final Memory: {} bytes\n"
        "   Processing Time: {} ms\n",
        final_snapshot.thread_pool.jobs_completed.load(),
        final_snapshot.system.memory_usage_bytes.load(),
        final_snapshot.worker.total_processing_time_ns.load() / 1000000
    );
    
    std::cout << "\n✅ Monitoring demo completed!\n";
    std::cout << "\nFeatures Demonstrated:\n";
    std::cout << "  ✓ Real-time metric collection\n";
    std::cout << "  ✓ Thread-safe metric updates\n";
    std::cout << "  ✓ Cross-platform compatibility\n";
    std::cout << "  ✓ Memory-efficient storage\n";
    std::cout << "  ✓ Easy integration API\n";
    
    return 0;
}