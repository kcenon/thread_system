#include "monitoring/core/metrics_collector.h"
#include "thread_pool/core/thread_pool.h"
#include "thread_base/jobs/callback_job.h"
#include "logger/core/logger.h"
#include "utilities/core/formatter.h"

#include <iostream>
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
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "                      REAL-TIME MONITORING DEMO\n";
    std::cout << std::string(80, '=') << "\n\n";
}

void print_metrics_snapshot(const metrics_snapshot& snapshot, int iteration) {
    using namespace utility_module;
    
    std::cout << formatter::format("📊 Iteration {:<3} | Time: {}\n", 
                                  iteration, 
                                  std::chrono::duration_cast<std::chrono::seconds>(
                                      snapshot.capture_time.time_since_epoch()).count());
    
    // 시스템 메트릭
    std::cout << "🖥️  System Metrics:\n";
    std::cout << formatter::format("   Memory Usage: {:<10} bytes | Active Threads: {}\n",
                                  snapshot.system.memory_usage_bytes.load(),
                                  snapshot.system.active_threads.load());
    
    // 스레드 풀 메트릭
    std::cout << "🏭 Thread Pool Metrics:\n";
    std::cout << formatter::format("   Jobs Completed: {:<8} | Jobs Pending: {:<6} | Worker Threads: {}\n",
                                  snapshot.thread_pool.jobs_completed.load(),
                                  snapshot.thread_pool.jobs_pending.load(),
                                  snapshot.thread_pool.worker_threads.load());
    
    auto avg_latency = snapshot.thread_pool.average_latency_ns.load();
    if (avg_latency > 0) {
        std::cout << formatter::format("   Avg Latency: {:<8} ns | Idle Threads: {}\n",
                                      avg_latency,
                                      snapshot.thread_pool.idle_threads.load());
    }
    
    // 워커 메트릭
    std::cout << "👷 Worker Metrics:\n";
    std::cout << formatter::format("   Jobs Processed: {:<6} | Processing Time: {:<10} ns\n",
                                  snapshot.worker.jobs_processed.load(),
                                  snapshot.worker.total_processing_time_ns.load());
    
    std::cout << std::string(80, '-') << "\n";
}

int main() {
    print_metrics_header();
    
    // 1. 로거 시작
    std::cout << "🚀 Starting logger...\n";
    if (auto result = start(); result) {
        std::cerr << "Failed to start logger: " << *result << "\n";
        return 1;
    }
    
    // 로그 설정
    console_target(static_cast<log_types>(static_cast<int>(log_types::Information) | static_cast<int>(log_types::Error)));
    set_title("Monitoring Demo");
    
    // 2. 모니터링 시작
    std::cout << "📈 Starting monitoring system...\n";
    monitoring_config config;
    config.collection_interval = std::chrono::milliseconds(500);  // 0.5초 간격
    config.buffer_size = 120;  // 1분간 데이터 보관
    
    if (auto result = metrics::start_global_monitoring(config); result.has_error()) {
        std::cerr << "Failed to start monitoring: " << result.get_error().message() << "\n";
        stop();
        return 1;
    }
    
    // 3. 스레드 풀 생성 및 설정
    std::cout << "🏭 Creating thread pool...\n";
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
    std::cout << "⚡ Starting workload simulation...\n";
    std::cout << "   - Submitting jobs with varying complexity\n";
    std::cout << "   - Monitoring metrics every 2 seconds\n";
    std::cout << "   - Demo will run for 30 seconds\n\n";
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> work_duration(1, 100);  // 1-100ms 작업
    std::uniform_int_distribution<> burst_size(5, 20);      // 5-20개씩 일괄 제출
    
    // 작업 제출 스레드
    std::atomic<bool> keep_submitting{true};
    std::thread job_submitter([&]() {
        int job_id = 0;
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
    std::cout << "\n🛑 Stopping simulation...\n";
    keep_submitting.store(false);
    
    if (job_submitter.joinable()) job_submitter.join();
    if (metrics_updater.joinable()) metrics_updater.join();
    
    pool->stop();
    metrics::stop_global_monitoring();
    stop();
    
    // 8. 최종 통계 출력
    std::cout << "\n📋 Final Statistics:\n";
    auto final_snapshot = metrics::get_current_metrics();
    std::cout << utility_module::formatter::format(
        "   Total Jobs Completed: {}\n"
        "   Final Memory Usage: {} bytes\n"
        "   Total Processing Time: {} ms\n",
        final_snapshot.thread_pool.jobs_completed.load(),
        final_snapshot.system.memory_usage_bytes.load(),
        final_snapshot.worker.total_processing_time_ns.load() / 1000000
    );
    
    std::cout << "\n✅ Monitoring demo completed successfully!\n";
    std::cout << "\nKey Features Demonstrated:\n";
    std::cout << "  ✓ Real-time metric collection every 500ms\n";
    std::cout << "  ✓ Thread-safe metric updates during high load\n";
    std::cout << "  ✓ Cross-platform system metric monitoring\n";
    std::cout << "  ✓ Integration with existing thread pool system\n";
    std::cout << "  ✓ Memory-efficient ring buffer storage\n";
    
    return 0;
}