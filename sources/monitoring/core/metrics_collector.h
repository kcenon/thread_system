#pragma once

#include "monitoring_types.h"
#include "../storage/ring_buffer.h"
#include "../../thread_base/sync/error_handling.h"

#include <memory>
#include <thread>
#include <atomic>
#include <chrono>

namespace monitoring_module {

    // 수집 통계 구조체
    struct collection_statistics {
        std::atomic<std::uint64_t> total_collections{0};
        std::atomic<std::uint64_t> collection_errors{0};
        std::atomic<std::uint64_t> buffer_overflows{0};
        std::atomic<std::uint64_t> collection_time_ns{0};

        collection_statistics() = default;

        collection_statistics(const collection_statistics& other)
            : total_collections(other.total_collections.load())
            , collection_errors(other.collection_errors.load())
            , buffer_overflows(other.buffer_overflows.load())
            , collection_time_ns(other.collection_time_ns.load()) {}

        collection_statistics& operator=(const collection_statistics& other) {
            if (this != &other) {
                total_collections.store(other.total_collections.load());
                collection_errors.store(other.collection_errors.load());
                buffer_overflows.store(other.buffer_overflows.load());
                collection_time_ns.store(other.collection_time_ns.load());
            }
            return *this;
        }
    };

    // 메트릭 수집기 클래스
    class metrics_collector {
    public:
        explicit metrics_collector(monitoring_config config = {});
        ~metrics_collector();

        // 수집 시작/중지
        auto start() -> thread_module::result_void;
        auto stop() -> void;

        // 메트릭 등록
        auto register_system_metrics(std::shared_ptr<system_metrics> metrics) -> void;
        auto register_thread_pool_metrics(std::shared_ptr<thread_pool_metrics> metrics) -> void;
        auto register_worker_metrics(std::shared_ptr<worker_metrics> metrics) -> void;

        // 메트릭 조회
        auto get_current_snapshot() const -> metrics_snapshot;
        auto get_recent_snapshots(std::size_t count) const -> std::vector<metrics_snapshot>;

        // 통계 조회
        auto get_collection_stats() const -> collection_statistics;

        // 설정 업데이트
        auto update_config(const monitoring_config& config) -> void;

        // 수집 상태 확인
        auto is_running() const -> bool { return running_.load(std::memory_order_acquire); }

    private:

        // 내부 메서드
        auto collection_loop() -> void;
        auto collect_metrics() -> void;
        auto collect_system_metrics() -> void;
        auto collect_thread_pool_metrics() -> void;
        auto collect_worker_metrics() -> void;

        // 멤버 변수
        monitoring_config config_;
        std::atomic<bool> running_{false};
        std::atomic<bool> stop_requested_{false};

        // 메트릭 저장소
        std::unique_ptr<thread_safe_ring_buffer<metrics_snapshot>> snapshot_buffer_;
        
        // 등록된 메트릭들
        std::shared_ptr<system_metrics> system_metrics_;
        std::shared_ptr<thread_pool_metrics> thread_pool_metrics_;
        std::shared_ptr<worker_metrics> worker_metrics_;

        // 수집 스레드
        std::unique_ptr<std::thread> collection_thread_;

        // 통계
        mutable collection_statistics stats_;

        // 동기화
        mutable std::mutex metrics_mutex_;
    };

    // 전역 메트릭 수집기 (싱글톤)
    class global_metrics_collector {
    public:
        static auto instance() -> global_metrics_collector&;
        
        auto initialize(monitoring_config config = {}) -> thread_module::result_void;
        auto shutdown() -> void;
        
        auto get_collector() -> std::shared_ptr<metrics_collector>;
        auto is_initialized() const -> bool { return collector_ != nullptr; }

    private:
        global_metrics_collector() = default;
        ~global_metrics_collector() = default;

        std::shared_ptr<metrics_collector> collector_;
        std::mutex init_mutex_;
    };

    // 편의 함수들
    namespace metrics {
        auto start_global_monitoring(monitoring_config config = {}) -> thread_module::result_void;
        auto stop_global_monitoring() -> void;
        auto get_current_metrics() -> metrics_snapshot;
        auto get_recent_metrics(std::size_t count) -> std::vector<metrics_snapshot>;
        auto is_monitoring_active() -> bool;
    }

} // namespace monitoring_module