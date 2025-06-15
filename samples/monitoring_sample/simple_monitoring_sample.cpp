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

#include "monitoring/core/metrics_collector.h"
#include "logger/core/logger.h"
#include "utilities/core/formatter.h"

#include <chrono>
#include <thread>

using namespace monitoring_module;
using namespace log_module;

void print_header() {
    write_information("\n{}", std::string(60, '='));
    write_information("         MONITORING MODULE DEMO");
    write_information("{}\n", std::string(60, '='));
}

void print_metrics(const metrics_snapshot& snapshot, int iteration) {
    using namespace utility_module;
    
    write_information("Snapshot {} at {}s", 
                     iteration, 
                     std::chrono::duration_cast<std::chrono::seconds>(
                         snapshot.capture_time.time_since_epoch()).count() % 1000);
    
    write_information("   Memory: {} bytes | Threads: {}",
                     snapshot.system.memory_usage_bytes.load(),
                     snapshot.system.active_threads.load());
    
    write_information("   Pool Jobs: {} completed | {} pending",
                     snapshot.thread_pool.jobs_completed.load(),
                     snapshot.thread_pool.jobs_pending.load());
    
    write_information("{}", std::string(60, '-'));
}

int main() {
    print_header();
    
    // 1. 로거 시작
    if (auto result = start(); result) {
        std::cerr << "Failed to start logger: " << *result << "\n";
        return 1;
    }
    write_information("Starting logger...");
    
    console_target(log_types::Information);
    set_title("Monitoring Demo");
    
    // 2. 모니터링 시작
    write_information("Starting monitoring system...");
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
    write_information("Starting simulation...");
    write_information("   Monitoring for 10 seconds with 2-second intervals\n");
    
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
    write_information("\nStopping simulation...");
    running.store(false);
    
    if (simulator.joinable()) {
        simulator.join();
    }
    
    metrics::stop_global_monitoring();
    stop();
    
    // 7. 최종 통계
    write_information("\nFinal Statistics:");
    auto final_snapshot = metrics::get_current_metrics();
    write_information("   Total Jobs: {}", final_snapshot.thread_pool.jobs_completed.load());
    write_information("   Final Memory: {} bytes", final_snapshot.system.memory_usage_bytes.load());
    write_information("   Processing Time: {} ms", final_snapshot.worker.total_processing_time_ns.load() / 1000000);
    
    write_information("\nMonitoring demo completed!");
    write_information("\nFeatures Demonstrated:");
    write_information("  * Real-time metric collection");
    write_information("  * Thread-safe metric updates");
    write_information("  * Cross-platform compatibility");
    write_information("  * Memory-efficient storage");
    write_information("  * Easy integration API");
    
    return 0;
}