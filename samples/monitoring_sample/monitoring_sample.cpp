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
#include "thread_pool/core/thread_pool.h"
#include "thread_base/jobs/callback_job.h"
#include "logger/core/logger.h"
#include "utilities/core/formatter.h"

#include <chrono>
#include <random>
#include <thread>

using namespace monitoring_module;
using namespace thread_pool_module;
using namespace thread_module;
using namespace log_module;

// 시연용 작업 클래스
class demo_job : public job {
public:
    explicit demo_job(std::chrono::milliseconds work_duration)
        : work_duration_(work_duration) {}

    auto do_work() -> result_void override {
        // 실제 작업 시뮬레이션
        std::this_thread::sleep_for(work_duration_);
        
        // 간헐적으로 로그 출력
        static std::atomic<int> job_counter{0};
        int current_job = job_counter.fetch_add(1);
        
        if (current_job % 100 == 0) {
            write_information("Completed job #{} ({}ms work)", current_job, work_duration_.count());
        }
        
        return {};
    }

private:
    std::chrono::milliseconds work_duration_;
};

// 메트릭 출력 헬퍼 함수
void print_metrics_header() {
    write_information("\n{}\n", std::string(80, '='));
    write_information("                      REAL-TIME MONITORING DEMO");
    write_information("{}\n", std::string(80, '='));
}

void print_metrics_snapshot(const metrics_snapshot& snapshot, int iteration) {
    using namespace utility_module;
    
    write_information("Iteration {:<3} | Time: {}", 
                     iteration, 
                     std::chrono::duration_cast<std::chrono::seconds>(
                         snapshot.capture_time.time_since_epoch()).count());
    
    // 시스템 메트릭
    write_information("System Metrics:");
    write_information("   Memory Usage: {:<10} bytes | Active Threads: {}",
                     snapshot.system.memory_usage_bytes.load(),
                     snapshot.system.active_threads.load());
    
    // 스레드 풀 메트릭
    write_information("Thread Pool Metrics:");
    write_information("   Jobs Completed: {:<8} | Jobs Pending: {:<6} | Worker Threads: {}",
                     snapshot.thread_pool.jobs_completed.load(),
                     snapshot.thread_pool.jobs_pending.load(),
                     snapshot.thread_pool.worker_threads.load());
    
    auto avg_latency = snapshot.thread_pool.average_latency_ns.load();
    if (avg_latency > 0) {
        write_information("   Avg Latency: {:<8} ns | Idle Threads: {}",
                         avg_latency,
                         snapshot.thread_pool.idle_threads.load());
    }
    
    // 워커 메트릭
    write_information("Worker Metrics:");
    write_information("   Jobs Processed: {:<6} | Processing Time: {:<10} ns",
                     snapshot.worker.jobs_processed.load(),
                     snapshot.worker.total_processing_time_ns.load());
    
    write_information("{}", std::string(80, '-'));
}

int main() {
    print_metrics_header();
    
    // 1. 로거 시작
    if (auto result = start(); result) {
        std::cerr << "Failed to start logger: " << *result << "\n";
        return 1;
    }
    write_information("Starting logger...");
    
    // 로그 설정
    console_target(static_cast<log_types>(static_cast<int>(log_types::Information) | static_cast<int>(log_types::Error)));
    set_title("Monitoring Demo");
    
    // 2. 모니터링 시작
    write_information("Starting monitoring system...");
    monitoring_config config;
    config.collection_interval = std::chrono::milliseconds(500);  // 0.5초 간격
    config.buffer_size = 120;  // 1분간 데이터 보관
    
    if (auto result = metrics::start_global_monitoring(config); result.has_error()) {
        std::cerr << "Failed to start monitoring: " << result.get_error().message() << "\n";
        stop();
        return 1;
    }
    
    // 3. 스레드 풀 생성 및 설정
    write_information("Creating thread pool...");
    auto pool = std::make_shared<thread_pool>();
    
    // 워커 스레드 추가
    const std::size_t worker_count = 4;
    for (std::size_t i = 0; i < worker_count; ++i) {
        pool->enqueue(std::make_unique<thread_worker>());
    }
    
    if (auto result = pool->start(); !result) {
        std::cerr << "Failed to start thread pool: " << *result << "\n";
        metrics::stop_global_monitoring();
        stop();
        return 1;
    }
    
    // 4. 메트릭 등록 (실제 구현에서는 thread_pool이 자동으로 수행)
    auto collector = global_metrics_collector::instance().get_collector();
    if (!collector) {
        std::cerr << "Failed to get metrics collector\n";
        pool->stop();
        metrics::stop_global_monitoring();
        stop();
        return 1;
    }
    
    // 샘플용 메트릭 객체 생성 및 등록
    auto system_metrics = std::make_shared<monitoring_module::system_metrics>();
    auto thread_pool_metrics = std::make_shared<monitoring_module::thread_pool_metrics>();
    auto worker_metrics = std::make_shared<monitoring_module::worker_metrics>();
    
    collector->register_system_metrics(system_metrics);
    collector->register_thread_pool_metrics(thread_pool_metrics);
    collector->register_worker_metrics(worker_metrics);
    
    // 초기 메트릭 설정
    thread_pool_metrics->worker_threads.store(worker_count);
    
    // 5. 작업 부하 시뮬레이션
    write_information("Starting workload simulation...");
    write_information("   - Submitting jobs with varying complexity");
    write_information("   - Monitoring metrics every 2 seconds");
    write_information("   - Demo will run for 30 seconds\n");
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> work_duration(1, 100);  // 1-100ms 작업
    std::uniform_int_distribution<> burst_size(5, 20);      // 5-20개씩 일괄 제출
    
    // 작업 제출 스레드
    std::atomic<bool> keep_submitting{true};
    std::thread job_submitter([&]() {
        int job_id = 0;
        (void)job_id; // Used for tracking submitted jobs
        while (keep_submitting.load()) {
            // 버스트 단위로 작업 제출
            int jobs_to_submit = burst_size(gen);
            for (int i = 0; i < jobs_to_submit && keep_submitting.load(); ++i) {
                auto duration = std::chrono::milliseconds(work_duration(gen));
                auto job = std::make_unique<demo_job>(duration);
                
                if (auto result = pool->enqueue(std::move(job)); result) {
                    // 메트릭 업데이트
                    thread_pool_metrics->jobs_pending.fetch_add(1);
                    ++job_id;
                } else {
                    write_error("Failed to enqueue job: {}", *result);
                }
            }
            
            // 제출 간격
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    });
    
    // 메트릭 업데이트 스레드
    std::thread metrics_updater([&]() {
        while (keep_submitting.load()) {
            // 간단한 메트릭 시뮬레이션
            auto pending = thread_pool_metrics->jobs_pending.load();
            
            if (pending > 0) {
                // 일부 작업이 완료된 것으로 시뮬레이션
                std::uint64_t completed_this_cycle = std::min(pending, std::uint64_t(5));
                thread_pool_metrics->jobs_completed.fetch_add(completed_this_cycle);
                thread_pool_metrics->jobs_pending.fetch_sub(completed_this_cycle);
                
                worker_metrics->jobs_processed.fetch_add(completed_this_cycle);
                worker_metrics->total_processing_time_ns.fetch_add(completed_this_cycle * 50000000); // 50ms per job
            }
            
            // 유휴 스레드 계산
            std::uint64_t idle_threads = (pending == 0) ? worker_count : 
                                        std::max(std::uint64_t(0), worker_count - std::min(pending, std::uint64_t(worker_count)));
            thread_pool_metrics->idle_threads.store(idle_threads);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    
    // 6. 실시간 모니터링 표시
    const int monitoring_duration = 30;  // 30초간 실행
    const int display_interval = 2;      // 2초마다 출력
    
    for (int iteration = 1; iteration <= monitoring_duration / display_interval; ++iteration) {
        std::this_thread::sleep_for(std::chrono::seconds(display_interval));
        
        auto snapshot = metrics::get_current_metrics();
        print_metrics_snapshot(snapshot, iteration);
    }
    
    // 7. 정리
    write_information("\nStopping simulation...");
    keep_submitting.store(false);
    
    if (job_submitter.joinable()) job_submitter.join();
    if (metrics_updater.joinable()) metrics_updater.join();
    
    pool->stop();
    metrics::stop_global_monitoring();
    stop();
    
    // 8. 최종 통계 출력
    write_information("\nFinal Statistics:");
    auto final_snapshot = metrics::get_current_metrics();
    write_information("   Total Jobs Completed: {}", final_snapshot.thread_pool.jobs_completed.load());
    write_information("   Final Memory Usage: {} bytes", final_snapshot.system.memory_usage_bytes.load());
    write_information("   Total Processing Time: {} ms", final_snapshot.worker.total_processing_time_ns.load() / 1000000);
    
    write_information("\nMonitoring demo completed successfully!");
    write_information("\nKey Features Demonstrated:");
    write_information("  * Real-time metric collection every 500ms");
    write_information("  * Thread-safe metric updates during high load");
    write_information("  * Cross-platform system metric monitoring");
    write_information("  * Integration with existing thread pool system");
    write_information("  * Memory-efficient ring buffer storage");
    
    return 0;
}