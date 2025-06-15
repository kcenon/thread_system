#pragma once

#include <chrono>
#include <atomic>
#include <string>
#include <memory>

namespace monitoring_module {

    // 시간 관련 타입 정의
    using time_point = std::chrono::steady_clock::time_point;
    using duration = std::chrono::nanoseconds;

    // 기본 메트릭 타입들
    enum class metric_type {
        counter,      // 누적 카운터 (작업 완료 수)
        gauge,        // 현재 값 (큐 길이, CPU 사용률)
        histogram,    // 분포 데이터 (지연시간)
        timer         // 시간 측정
    };

    // 시스템 메트릭 구조체
    struct system_metrics {
        std::atomic<std::uint64_t> cpu_usage_percent{0};
        std::atomic<std::uint64_t> memory_usage_bytes{0};
        std::atomic<std::uint64_t> active_threads{0};
        std::atomic<std::uint64_t> total_allocations{0};
        time_point timestamp{std::chrono::steady_clock::now()};

        system_metrics() = default;
        
        system_metrics(const system_metrics& other) 
            : cpu_usage_percent(other.cpu_usage_percent.load())
            , memory_usage_bytes(other.memory_usage_bytes.load())
            , active_threads(other.active_threads.load())
            , total_allocations(other.total_allocations.load())
            , timestamp(other.timestamp) {}

        system_metrics& operator=(const system_metrics& other) {
            if (this != &other) {
                cpu_usage_percent.store(other.cpu_usage_percent.load());
                memory_usage_bytes.store(other.memory_usage_bytes.load());
                active_threads.store(other.active_threads.load());
                total_allocations.store(other.total_allocations.load());
                timestamp = other.timestamp;
            }
            return *this;
        }
    };

    // 스레드 풀 메트릭 구조체
    struct thread_pool_metrics {
        std::atomic<std::uint64_t> jobs_completed{0};
        std::atomic<std::uint64_t> jobs_pending{0};
        std::atomic<std::uint64_t> total_execution_time_ns{0};
        std::atomic<std::uint64_t> average_latency_ns{0};
        std::atomic<std::uint64_t> worker_threads{0};
        std::atomic<std::uint64_t> idle_threads{0};
        time_point timestamp{std::chrono::steady_clock::now()};

        thread_pool_metrics() = default;
        
        thread_pool_metrics(const thread_pool_metrics& other)
            : jobs_completed(other.jobs_completed.load())
            , jobs_pending(other.jobs_pending.load())
            , total_execution_time_ns(other.total_execution_time_ns.load())
            , average_latency_ns(other.average_latency_ns.load())
            , worker_threads(other.worker_threads.load())
            , idle_threads(other.idle_threads.load())
            , timestamp(other.timestamp) {}

        thread_pool_metrics& operator=(const thread_pool_metrics& other) {
            if (this != &other) {
                jobs_completed.store(other.jobs_completed.load());
                jobs_pending.store(other.jobs_pending.load());
                total_execution_time_ns.store(other.total_execution_time_ns.load());
                average_latency_ns.store(other.average_latency_ns.load());
                worker_threads.store(other.worker_threads.load());
                idle_threads.store(other.idle_threads.load());
                timestamp = other.timestamp;
            }
            return *this;
        }
    };

    // 워커 스레드 메트릭 구조체
    struct worker_metrics {
        std::atomic<std::uint64_t> jobs_processed{0};
        std::atomic<std::uint64_t> total_processing_time_ns{0};
        std::atomic<std::uint64_t> idle_time_ns{0};
        std::atomic<std::uint64_t> context_switches{0};
        time_point timestamp{std::chrono::steady_clock::now()};

        worker_metrics() = default;
        
        worker_metrics(const worker_metrics& other)
            : jobs_processed(other.jobs_processed.load())
            , total_processing_time_ns(other.total_processing_time_ns.load())
            , idle_time_ns(other.idle_time_ns.load())
            , context_switches(other.context_switches.load())
            , timestamp(other.timestamp) {}

        worker_metrics& operator=(const worker_metrics& other) {
            if (this != &other) {
                jobs_processed.store(other.jobs_processed.load());
                total_processing_time_ns.store(other.total_processing_time_ns.load());
                idle_time_ns.store(other.idle_time_ns.load());
                context_switches.store(other.context_switches.load());
                timestamp = other.timestamp;
            }
            return *this;
        }
    };

    // 메트릭 스냅샷 (읽기 전용)
    struct metrics_snapshot {
        system_metrics system;
        thread_pool_metrics thread_pool;
        worker_metrics worker;
        time_point capture_time{std::chrono::steady_clock::now()};
    };

    // 모니터링 설정
    struct monitoring_config {
        std::chrono::milliseconds collection_interval{100};  // 100ms 간격
        std::size_t buffer_size{3600};                       // 1시간분 데이터 (100ms * 3600 = 6분)
        bool enable_system_metrics{true};
        bool enable_thread_pool_metrics{true};
        bool enable_worker_metrics{true};
        bool low_overhead_mode{false};                       // 성능 우선 모드
    };

    // RAII 타이머 클래스
    class scoped_timer {
    public:
        explicit scoped_timer(std::atomic<std::uint64_t>& target)
            : target_(target), start_time_(std::chrono::steady_clock::now()) {}

        ~scoped_timer() {
            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
                end_time - start_time_).count();
            target_.fetch_add(static_cast<std::uint64_t>(duration), std::memory_order_relaxed);
        }

        // 복사 및 이동 금지
        scoped_timer(const scoped_timer&) = delete;
        scoped_timer& operator=(const scoped_timer&) = delete;
        scoped_timer(scoped_timer&&) = delete;
        scoped_timer& operator=(scoped_timer&&) = delete;

    private:
        std::atomic<std::uint64_t>& target_;
        time_point start_time_;
    };

    // 메트릭 업데이터 헬퍼 클래스
    class metrics_updater {
    public:
        static auto increment_counter(std::atomic<std::uint64_t>& counter) -> void {
            counter.fetch_add(1, std::memory_order_relaxed);
        }

        static auto add_value(std::atomic<std::uint64_t>& target, std::uint64_t value) -> void {
            target.fetch_add(value, std::memory_order_relaxed);
        }

        static auto set_value(std::atomic<std::uint64_t>& target, std::uint64_t value) -> void {
            target.store(value, std::memory_order_relaxed);
        }

        static auto create_timer(std::atomic<std::uint64_t>& target) -> scoped_timer {
            return scoped_timer(target);
        }
    };

} // namespace monitoring_module