# thread_system 변경 이력

이 프로젝트의 모든 주목할 만한 변경 사항은 이 파일에 문서화됩니다.

이 형식은 [Keep a Changelog](https://keepachangelog.com/ko/1.0.0/)를 기반으로 하며,
이 프로젝트는 [Semantic Versioning](https://semver.org/lang/ko/)을 준수합니다.

## [Unreleased]

### 추가
- **이슈 #390**: Phase 1.3.5 - 헬스 체크 구현
  - `<kcenon/thread/diagnostics/health_status.h>`에 새로운 `health_thresholds` 구조체:
    - `min_success_rate`: 정상 상태를 위한 최소 성공률 (기본값: 0.95)
    - `unhealthy_success_rate`: 비정상으로 판단되는 성공률 임계값 (기본값: 0.8)
    - `max_healthy_latency_ms`: 정상 상태를 위한 최대 평균 레이턴시 (기본값: 100.0ms)
    - `degraded_latency_ms`: 저하 상태로 판단되는 레이턴시 임계값 (기본값: 500.0ms)
    - `queue_saturation_warning`: 저하 상태를 위한 큐 포화도 임계값 (기본값: 0.8)
    - `queue_saturation_critical`: 비정상 상태를 위한 큐 포화도 임계값 (기본값: 0.95)
    - `worker_utilization_warning`: 저하 상태를 위한 워커 활용도 임계값 (기본값: 0.9)
    - `min_idle_workers`: 정상 상태를 위한 최소 유휴 워커 수 (기본값: 0)
  - `health_status` 구조체에 새로운 직렬화 메서드:
    - `to_json()`: HTTP 헬스 엔드포인트 호환 JSON 출력
    - `to_string()`: 로깅을 위한 사람이 읽기 쉬운 형식 출력
  - 향상된 `health_check()` 구현:
    - 메트릭에서 `avg_latency_ms` 계산
    - 작업 큐에서 `queue_capacity` 보고
    - `check_metrics_health()`를 통한 메트릭 컴포넌트 상태 확인 추가
  - 향상된 `check_queue_health()` 구현:
    - 큐 포화도 계산 및 보고
    - 임계값 기반 상태 결정
  - `diagnostics_config`에 `health_thresholds_config` 필드 추가

- **이슈 #381**: Phase 3.1 - 작업 의존성 그래프 (DAG 스케줄러)
  - `<kcenon/thread/dag/dag_job.h>`에 새로운 `dag_job` 클래스:
    - 의존성 지원으로 기본 `job` 클래스 확장
    - 원자적 카운터를 통한 고유 작업 ID 생성
    - 상태 머신: `pending`, `ready`, `running`, `completed`, `failed`, `cancelled`, `skipped`
    - `try_transition_state()`를 통한 원자적 상태 전환
    - `std::any`를 통한 작업 간 데이터 전달을 위한 결과 저장
    - 실패 복구를 위한 대체 함수 지원
    - 타이밍 메트릭 (제출, 시작, 종료 시간)
  - 유창한 작업 생성을 위한 새로운 `dag_job_builder` 클래스:
    - 메서드 체이닝: `work()`, `depends_on()`, `on_failure()`, `with_result()`
    - 단일 의존성 및 배치 의존성 지원
    - `std::unique_ptr<dag_job>` 빌드
  - `<kcenon/thread/dag/dag_scheduler.h>`에 새로운 `dag_scheduler` 클래스:
    - 병렬 실행을 위한 스레드 풀 통합
    - 실행 순서를 위한 위상 정렬
    - 3색 마킹을 사용한 DFS 기반 순환 감지
    - 순환 검증을 통한 동적 의존성 추가
    - `execute_all()` 및 `execute(target_id)` 비동기 실행
    - 완료 대기를 위한 `wait()`
    - 협력적 취소를 위한 `cancel_all()`
  - 구성 옵션이 포함된 새로운 `dag_config` 구조체:
    - `dag_failure_policy`: `fail_fast`, `continue_others`, `retry`, `fallback`
    - 재시도 정책을 위한 `max_retries` 및 `retry_delay`
    - 성능을 위한 `detect_cycles` 토글
    - 순차/병렬 실행을 위한 `execute_in_parallel` 토글
    - 상태 변경, 완료 및 오류 콜백
  - 시각화 지원:
    - `to_dot()`: 상태 색상이 포함된 Graphviz DOT 형식으로 DAG 내보내기
    - `to_json()`: 작업 및 통계가 포함된 JSON으로 DAG 내보내기
  - `dag_stats` 구조체를 통한 통계 추적:
    - 상태별 작업 수 (전체, 완료, 실패, 대기, 실행, 건너뜀, 취소)
    - 총 실행 시간
    - `all_succeeded()` 헬퍼 메서드
  - 모든 기능을 다루는 종합 테스트 스위트 (17개 테스트)

- **이슈 #388**: Phase 1.3.3 - Job Inspection 구현
  - 고유 ID를 사용한 작업 추적 구현:
    - `job` 클래스에 `job_id_` 멤버 및 `get_job_id()` 추가
    - 대기 시간 계산을 위한 `enqueue_time_` 멤버 및 `get_enqueue_time()` 추가
    - `next_job_id_` 정적 카운터를 통한 원자적 ID 생성
  - `thread_pool_diagnostics`에 `get_active_jobs()` 구현:
    - 현재 실행 중인 모든 작업에 대한 `job_info` 반환
    - 작업 ID, 이름, 시작 시간, 실행 시간, 워커 스레드 ID 포함
  - `thread_pool_diagnostics`에 `get_pending_jobs()` 구현:
    - 큐에서 대기 중인 작업에 대한 `job_info` 반환
    - 큐 등록 시간부터의 대기 시간 계산 포함
    - 구성 가능한 limit 매개변수 (기본값: 100)
  - `job_queue`에 `inspect_pending_jobs()` 추가:
    - 작업을 제거하지 않고 큐를 스레드 안전하게 검사
    - 타이밍 정보가 포함된 job_info 스냅샷 생성
  - `thread_worker`의 `get_current_job_info()` 업데이트:
    - 이제 job 클래스의 실제 job_id 사용
    - 정확한 enqueue_time 및 wait_time 계산

- **이슈 #377**: Phase 2.1 - 비동기 결과 반환을 위한 Future/Promise 통합
  - 새로운 `future_job<R>` 템플릿 클래스:
    - `std::promise<R>`로 callable을 래핑하여 비동기 결과 검색 가능
    - `if constexpr`를 통한 void 반환 타입 지원
    - Promise로의 예외 전파
    - 기존 `cancellation_token`과의 통합
    - `make_future_job()` 헬퍼 함수
  - `thread_pool`의 새로운 비동기 메서드:
    - `submit_async()`: callable을 제출하고 `std::future<R>` 반환
    - `submit_batch_async()`: 여러 callable을 제출하고 future 벡터 반환
    - `submit_all()`: 배치를 제출하고 모두 완료될 때까지 블록
    - `submit_any()`: 배치를 제출하고 먼저 완료된 결과 반환
  - 새로운 `cancellable_future<R>` 템플릿:
    - `std::future`를 `cancellation_token` 통합과 함께 래핑
    - `get_for(timeout)`: 타임아웃 지원 대기
    - `is_ready()`, `is_cancelled()` 상태 메서드
    - 협력적 취소를 위한 `cancel()` 메서드
  - `<kcenon/thread/utils/when_helpers.h>`의 새로운 when_all/when_any 헬퍼:
    - `when_all()`: 여러 이종 future를 튜플로 결합
    - `when_any()`: future 벡터에서 먼저 완료된 결과 반환
    - `when_any_with_index()`: 인덱스 정보와 함께 먼저 완료된 결과 반환
  - 모든 비동기 기능에 대한 포괄적인 테스트 스위트 (21개 테스트)

- **이슈 #387**: Phase 1.3.2 - Thread Dump 기능 향상
  - `dump_thread_states()`가 실제 워커 정보를 반환하도록 향상:
    - `thread_base::get_thread_id()`를 통한 실제 스레드 ID
    - `thread_worker::get_worker_id()`를 통한 고유 워커 ID
    - 활성 워커의 현재 작업 정보
  - `thread_worker`에 워커 통계 추적 추가:
    - `get_jobs_completed()`: 성공적으로 완료된 작업 수
    - `get_jobs_failed()`: 실패한 작업 수
    - `get_total_busy_time()`: 작업 실행에 소요된 누적 시간
    - `get_total_idle_time()`: 작업 대기에 소요된 누적 시간
    - `get_state_since()`: 마지막 상태 전환 타임스탬프
    - `get_current_job_info()`: 현재 실행 중인 작업 정보
  - `thread_pool`에 스레드 안전한 워커 정보 수집을 위한 `collect_worker_diagnostics()` 추가
  - 실제 busy/idle 시간 기반의 정확한 활용률 계산
  - 적절한 동기화를 통한 스레드 안전한 상태 수집
- **이슈 #374**: 히스토그램 및 백분위수 지원이 포함된 향상된 메트릭 시스템
  - `LatencyHistogram`: 지연 시간 분포를 위한 HDR 스타일 히스토그램
    - 정확한 백분위수 계산 (P50/P90/P99/P99.9)
    - < 100ns 오버헤드로 lock-free 원자적 작업
    - 메모리 효율: 히스토그램당 < 1KB
  - `SlidingWindowCounter`: 처리량 측정을 위한 시간 기반 카운터
    - 구성 가능한 윈도우 크기 (1초, 60초 등)
    - Lock-free 순환 버퍼 구현
  - `EnhancedThreadPoolMetrics`: 종합 메트릭 집계:
    - 큐 추가 지연 시간 히스토그램
    - 실행 지연 시간 히스토그램
    - 대기 시간 (큐 시간) 히스토그램
    - 처리량 카운터 (1초 및 1분 윈도우)
    - 워커별 활용률 추적
    - 큐 깊이 모니터링
  - 스레드 풀 통합:
    - `set_enhanced_metrics_enabled(bool)`: 향상된 메트릭 활성화/비활성화
    - `is_enhanced_metrics_enabled()`: 향상된 메트릭 활성화 여부 확인
    - `enhanced_metrics()`: 향상된 메트릭 접근 (비활성화시 예외 발생)
    - `enhanced_metrics_snapshot()`: 모든 메트릭의 스냅샷 가져오기
  - 내보내기 형식:
    - `to_json()`: JSON 직렬화
    - `to_prometheus(prefix)`: Prometheus/OpenMetrics 형식

### 수정
- **이슈 #387**: 유휴 상태 스레드 풀에서 `is_healthy()`가 false를 반환하는 문제 수정
  - 상태 검사 조건을 `get_active_worker_count() > 0`에서 `get_thread_count() > 0`으로 변경
  - 등록된 워커가 있는 실행 중인 풀(유휴 또는 활성 상태)이 이제 올바르게 정상으로 식별됨
  - 전체 워커 수를 사용하는 `check_worker_health()` 로직과 일관성 유지

### 변경
- **이슈 #359**: 오해의 소지가 있는 lockfree_queue 이름 수정 (Kent Beck "Reveals Intention" 원칙)
  - fine-grained locking 큐 구현을 위한 `concurrent/` 디렉토리 생성
  - `concurrent_queue<T>`를 `lockfree/`에서 `concurrent/concurrent_queue.h`로 이동
  - `lockfree/lockfree_queue.h`를 하위 호환성 헤더로 변환
  - "MISLEADING NAME" 경고를 포함하도록 deprecation 메시지 개선
  - 새 헤더 경로와 네임스페이스 참조로 문서 업데이트

- **이슈 #340**: `lockfree_queue<T>`를 `concurrent_queue<T>`로 이름 변경
  - 클래스 이름이 lock-free 알고리즘이 아닌 fine-grained locking을 사용하므로 오해의 소지가 있었음
  - 이전 이름 `lockfree_queue<T>`는 하위 호환성을 위해 deprecated 별칭으로 유지
  - deprecation 경고를 피하려면 기존 코드를 `concurrent_queue<T>`로 업데이트 필요

- **이슈 #338**: 중앙 레지스트리 준수를 위한 error_code enum 음수 범위 마이그레이션
  - 모든 error_code 값을 양수에서 음수 범위 (-100 ~ -199)로 변경
  - 에러 코드 범위 구성:
    - 일반 에러: -100 ~ -109
    - 스레드 에러: -110 ~ -119
    - 큐 에러: -120 ~ -129
    - 작업 에러: -130 ~ -139
    - 리소스 에러: -140 ~ -149
    - 동기화 에러: -150 ~ -159
    - IO 에러: -160 ~ -169
  - 일관성을 위해 sync/error_handling.h에 `queue_busy` 에러 코드 추가
  - static_assert를 통한 컴파일 타임 범위 검증 추가
  - **BREAKING CHANGE**: error_code 정수 값을 확인하는 모든 코드 업데이트 필요

### 수정
- **이슈 #358**: deprecated lockfree_job_queue에 대한 queue_factory_integration_test 수정
  - `RequirementsSatisfaction_LockFreeUnderLoad` 테스트가 `adaptive_job_queue`를 사용하도록 업데이트
  - `OptimalSelection_FunctionalUnderLoad`에서 `lockfree_job_queue`로 캐스트하는 불필요한 코드 제거
  - 하위 호환성 테스트를 위한 deprecation 경고 억제 추가

- **이슈 #333**: 예제 로거 구현에서 deprecated 5-파라미터 log() 메서드 제거
  - `composition_example.cpp`의 console_logger가 `log(const log_entry&)`를 직접 사용하도록 업데이트
  - `mock_logger.h`가 `log(const log_entry&)`를 직접 사용하도록 업데이트
  - common_system ILogger 인터페이스에서 deprecated 메서드 제거로 인한 빌드 실패 수정

### 변경
- **이슈 #333**: 통합된 KCENON_* 기능 플래그 채택
  - `THREAD_HAS_COMMON_EXECUTOR`, `THREAD_HAS_COMMON_RESULT`, `THREAD_HAS_COMMON_CONCEPTS`를 `KCENON_HAS_*` 동등 항목으로 교체
  - common_system의 `feature_flags.h` include 추가 (가드됨)
  - thread_pool 헤더에서 로컬 `__has_include` 기반 매크로 정의 제거
  - 레거시 `THREAD_HAS_*` 매크로는 하위 호환성을 위해 별칭으로 계속 정의됨 (deprecated, v1.0.0에서 제거 예정)
  - CMake에서 `KCENON_HAS_*` (주) 및 `THREAD_HAS_*` (레거시 별칭) 컴파일 정의를 모두 정의

### 제거됨
- **이슈 #331**: thread_logger.h에서 deprecated THREAD_LOG_* 매크로 제거
  - 사용되지 않던 THREAD_LOG_TRACE, THREAD_LOG_DEBUG, THREAD_LOG_INFO, THREAD_LOG_WARN, THREAD_LOG_ERROR 매크로 제거
  - 이 매크로들은 정의만 되어 있고 사용되지 않았으며, 표준 LOG_* 매크로와 혼동될 수 있음

### 변경 (계속)
- **이슈 #331**: deprecated common_system API로부터 마이그레이션
  - logger_system_adapter에서 레거시 ILogger::log() 메서드에 대한 deprecation 경고 억제 추가
  - 이 메서드는 ILogger 인터페이스의 순수 가상 함수를 override하므로 계속 구현됨
  - common_system v3.0.0에서 deprecated 기본 메서드가 제거되면 함께 제거될 예정

- **이슈 #329**: 컴파일러 플래그에서 deprecated 선언 경고 활성화
  - GCC/Clang에서 `-Wno-deprecated-declarations`를 `-Wdeprecated-declarations`로 변경
  - MSVC에서 deprecated 경고 활성화를 위해 `/wd4996` 플래그 제거
  - common_system v3.0.0 이전에 deprecated API 사용을 조기에 감지할 수 있음

## [3.0.0] - 2025-12-19

### 주요 변경 사항 (BREAKING CHANGES)

이 릴리스는 **common_system 전용** 공개 계약으로의 마이그레이션을 완료합니다. 다음 레거시 타입과 인터페이스가 공개 API에서 제거되었습니다:

**에러 처리**
- `kcenon::thread::result<T>` → `kcenon::common::Result<T>` 사용
- `kcenon::thread::result_void` → `kcenon::common::VoidResult` 사용
- `kcenon::thread::error` → `kcenon::common::error_info` 사용

**로깅**
- `kcenon::thread::logger_interface` → `kcenon::common::interfaces::ILogger` 사용
- `kcenon::thread::log_level` → `kcenon::common::log_level` 사용
- `kcenon::thread::logger_registry` → common_system의 로거 등록 사용

**모니터링**
- `kcenon::thread::monitoring_interface` → `kcenon::common::interfaces::IMonitor` 사용
- `kcenon::thread::monitorable_interface` → `kcenon::common::interfaces::IMonitorable` 사용

**Executor/공유 인터페이스**
- `kcenon::shared::*` 계약 → `kcenon::common::interfaces::IExecutor` 사용
- `shared_interfaces.h` 헤더 제거
- 레거시 어댑터들을 `thread_pool_executor_adapter`로 통합

### 마이그레이션 가이드

자세한 지침은 다음 마이그레이션 가이드를 참조하세요:
- [에러 시스템 마이그레이션 가이드](docs/advanced/ERROR_SYSTEM_MIGRATION_GUIDE.md)
- [로거 인터페이스 마이그레이션 가이드](docs/guides/LOGGER_INTERFACE_MIGRATION_GUIDE_KO.md)

**빠른 마이그레이션 요약:**

```cpp
// 이전 (v2.x)
#include <kcenon/thread/core/error_handling.h>
kcenon::thread::result<int> foo();

// 이후 (v3.0)
#include <kcenon/common/result.h>
kcenon::common::Result<int> foo();
```

```cpp
// 이전 (v2.x)
class MyLogger : public kcenon::thread::logger_interface { ... };

// 이후 (v3.0)
class MyLogger : public kcenon::common::interfaces::ILogger { ... };
```

### 제거됨
- **이슈 #313 - Phase 3**: shared_interfaces.h 제거 및 executor 어댑터 통합
  - 공개 헤더에서 `kcenon::shared::*` 컨트랙트 제거
  - `include/kcenon/thread/interfaces/shared_interfaces.h` 헤더 파일 제거
  - `include/kcenon/thread/adapters/thread_pool_executor.h` 레거시 어댑터 제거
  - `include/kcenon/thread/adapters/common_system_executor_adapter.h` 중복 어댑터 제거
  - `common_executor_adapter.h`의 `thread_pool_executor_adapter`로 단일 canonical 어댑터 통합
  - canonical `thread_pool_executor_adapter` 사용하도록 `service_registration.h` 업데이트
  - 모든 통합은 이제 `kcenon::common::interfaces::IExecutor`만 사용
- **이슈 #312 - Phase 3**: monitoring을 common::interfaces::IMonitor/IMonitorable로 마이그레이션
  - 공개 헤더에서 `kcenon::thread::monitoring_interface` 네임스페이스 제거
  - `include/kcenon/thread/interfaces/monitoring_interface.h` 헤더 파일 제거
  - `include/kcenon/thread/interfaces/monitorable_interface.h` 헤더 파일 제거
  - `include/kcenon/thread/adapters/common_system_monitoring_adapter.h` 어댑터 제거
  - `thread_context`는 이제 메트릭 기록에 `common::interfaces::IMonitor` 사용
  - 메트릭은 컴포넌트 식별을 위한 태그와 함께 `IMonitor::record_metric()`으로 기록
  - 모든 코드는 이제 common_system의 `kcenon::common::interfaces::IMonitor` 사용 필요
  - 새로운 IMonitor API 사용법을 보여주는 예제 업데이트

### 추가됨
- **이슈 #276**: C++20 Concepts 기능 감지를 위한 CMake 설정 추가
  - `ThreadSystemFeatures.cmake`에 새로운 `check_common_concepts_support()` 함수 추가
  - `common_system` C++20 concepts 헤더 사용 가능 여부 감지
  - 컴파일러 버전 요구사항 검증 (GCC 10+, Clang 10+, Apple Clang 12+, MSVC 19.23+)
  - concepts 사용 가능 시 `THREAD_HAS_COMMON_CONCEPTS` 매크로 정의
  - CMake 설정 중 사용 가능한 concept 카테고리 표시
  - 상위 이슈 #271의 일부 (Apply updated common_system with C++20 Concepts)

### 변경됨
- **이슈 #275**: atomic_wait.h를 C++20 concepts로 리팩토링
  - `std::enable_if<std::is_integral<U>::value>` SFINAE 패턴을 `requires std::integral<T>` 절로 교체
  - `USE_STD_CONCEPTS` 정의 시 `<concepts>` 헤더 include 추가
  - `#else` 블록 내 기존 SFINAE 패턴으로 C++17 폴백 유지
  - 더 깔끔한 템플릿 선언과 개선된 컴파일 타임 에러 메시지

### Deprecated
- **이슈 #263**: thread-local logger_interface를 deprecated로 표시
  - `logger_interface.h`의 `log_level` enum에 `[[deprecated]]` 속성 추가
  - `logger_interface`와 `logger_registry` 클래스는 이미 deprecation 속성 보유
  - deprecated 타입 사용 시 컴파일러 경고 발생
  - 포괄적인 마이그레이션 가이드 생성: `docs/guides/LOGGER_INTERFACE_MIGRATION_GUIDE_KO.md`
  - 마이그레이션 경로: common_system의 `common::interfaces::ILogger` 사용
  - **일정**: v1.x에서 deprecated, v2.0에서 제거 예정

### 수정됨
- **이슈 #316**: lockfree_job_queue에서 unsafe hazard_pointer를 safe_hazard_pointer로 교체
  - TICKET-002 후속 조치: 약한 메모리 모델 아키텍처(ARM64)의 메모리 순서 문제 수정
  - 프로덕션 코드에서 `HAZARD_POINTER_FORCE_ENABLE` 사용 제거
  - 명시적 메모리 순서 보장을 제공하는 `safe_hazard_pointer.h`로 마이그레이션
  - RAII 스타일 hazard pointer 관리를 위한 `safe_hazard_guard` 사용
  - 안전한 메모리 회수를 위한 `safe_retire_hazard<T>()` 사용
  - ARM64/Apple Silicon을 위한 약한 메모리 모델 검증 테스트 추가
  - 약한 메모리 모델 아키텍처의 CVSS 8.5 보안 이슈 해결
- **PR #319**: safe_hazard_pointer 통합 CI 실패 수정 (#316 후속 조치)
  - `retire()`에서 `collect()` 호출을 락 외부로 이동하여 데드락 수정
  - 메모리 재사용 시나리오에서 이중 해제 방지를 위한 중복 주소 처리 추가
  - 재사용 레코드의 오래된 포인터 방지를 위해 `acquire()`에서 hazard pointer 클리어
  - 레코드 재사용 중 경쟁 조건 처리를 위해 `collect_internal()`에서 모든 레코드 검사
  - 경합 중 행 방지를 위해 `enqueue()`와 `dequeue()`에 재시도 제한 추가
  - 재시도 제한 초과 시를 위한 `queue_busy` 에러 코드 추가
  - UAF 방지를 위해 `empty()`에 hazard pointer 보호 추가
  - 드레인 전 모드 업데이트로 `adaptive_job_queue::migrate_to_mode()`의 무한 드레인 루프 수정
- **이슈 #297**: SDOF 방지를 위한 atexit 핸들러 등록 타이밍 개선
  - 조기 atexit 핸들러 등록을 위한 `thread_logger_init.cpp` 추가
  - 플랫폼별 초기화 사용 (GCC/Clang `__attribute__((constructor(101)))`, MSVC CRT 섹션)
  - 정적 소멸 단계에서 `is_shutting_down()`이 true를 반환하도록 보장
  - 포괄적인 SDOF 방지 테스트 추가
  - 크로스 플랫폼 지원: Linux, macOS, Windows
  - 관련 이슈 #295, #296 (초기 SDOF 방지), network_system#301
- **이슈 #295**: thread_pool 소멸자와 stop() 메서드의 SDOF 방지
  - 정적 소멸 중 로깅 없이 종료하는 `stop_unsafe()` private 메서드 추가
  - 소멸자에서 `stop()` 호출 전 `thread_logger::is_shutting_down()` 체크 추가
  - 모든 `thread_context::log()` 메서드 오버로드에 종료 체크 추가
  - 정적 소멸 단계에서 thread_pool 파괴 시 발생하던 `free(): invalid pointer` 오류 방지
  - 관련 이슈 #293 (thread_logger Intentional Leak 패턴)
- **이슈 #293**: thread_logger의 정적 소멸 순서 문제 방지
  - `instance()`를 의도적 누수 패턴으로 변경 (`new`로 할당, 삭제 안 함)
  - 프로세스 종료 시 로깅을 건너뛰기 위한 `is_shutting_down_` 원자적 플래그 추가
  - 명시적 종료 신호를 위한 `prepare_shutdown()` 메서드 추가
  - Ubuntu의 network_system CI 테스트에서 발생하던 `free(): invalid pointer` 오류 수정
  - API 호환성 유지 - 순수하게 내부 구현 수정

### 추가됨
- **이슈 #246**: adaptive_queue_sample 예제 재활성화
  - adaptive_queue_sample.cpp에서 logger 의존성 제거
  - write_information/write_error를 std::cout/std::cerr로 교체
  - 새로운 kcenon/thread 헤더 구조 및 정책 기반 API로 업데이트
  - 5가지 포괄적 예제 시연: 정책 비교, 적응형 동작, 다양한 정책, 성능 모니터링, 웹 서버 시뮬레이션

### 변경됨
- **이슈 #227**: typed_pool 구현 헤더를 공개 include 경로로 이동
  - 14개 헤더 파일을 `src/impl/typed_pool/`에서 `include/kcenon/thread/impl/typed_pool/`로 이전
  - 포워딩 헤더가 안정적인 `<kcenon/thread/impl/typed_pool/...>` include 사용하도록 업데이트
  - 취약한 `../../../../src/impl/` 상대 경로 의존성 제거
  - 헤더가 다른 공개 헤더들과 함께 적절히 설치됨
  - IDE/툴링 호환성 및 정적 분석기 지원 개선

### 추가됨
- **이슈 #231**: Phase 2 - 뮤텍스 기반 큐 구현
  - `job_queue`가 `queue_capabilities_interface`를 상속하도록 확장
  - 뮤텍스 기반 역량을 반환하는 `get_capabilities()` 오버라이드 구현
  - 편의 메서드 추가: `has_exact_size()`, `is_lock_free()` 등
  - 100% 하위 호환성 유지 - 모든 기존 테스트 변경 없이 통과
  - job_queue 역량 쿼리에 대한 유닛 테스트
- **이슈 #230**: Phase 1 - 큐 역량 인프라스트럭처
  - 런타임 역량 기술을 위한 `queue_capabilities` 구조체
  - 역량 조회를 위한 `queue_capabilities_interface` 믹스인
  - 점진적 도입을 위한 비파괴적 추가 인터페이스
  - 모든 역량 쿼리에 대한 유닛 테스트
- 문서 표준화 준수
- README.md, ARCHITECTURE.md, CHANGELOG.md
- **ARM64 호환성 테스트**: macOS Apple Silicon을 위한 종합 테스트 (#223)
  - 수동 worker 배치 등록 검증
  - 다중 worker를 사용한 동시 작업 제출
  - 메모리 정렬 검증을 위한 정적 단언문
  - 개별 vs 배치 worker 등록 비교

### 수정됨
- **이슈 #291**: Windows MSVC 빌드에서 pthread.h 파일을 찾을 수 없는 오류
  - `find_or_fetch_gtest()` 함수에 `gtest_disable_pthreads ON` 옵션 추가
  - Windows MSVC에서 thread_system을 하위 디렉토리로 사용할 때 빌드 실패 수정
  - Windows MSVC는 기본적으로 pthread.h를 제공하지 않으므로 GTest의 pthread 지원을 비활성화해야 함
- **이슈 #225**: macOS ARM64에서 배치 worker 등록 시 EXC_BAD_ACCESS 발생 (#223 후속)
  - 근본 원인: `on_stop_requested()`와 `do_work()`의 job 파괴 간 데이터 레이스
  - `on_stop_requested()`가 job의 가상 메서드를 호출하는 동안 `do_work()`가
    동시에 job 객체를 파괴하면서 레이스 발생
  - 해결: job 파괴 중 접근을 보호하기 위한 뮤텍스 동기화 추가
  - `on_stop_requested()`가 현재 job 접근 전 `queue_mutex_` 획득
  - `do_work()`가 `queue_mutex_`를 보유한 상태에서 job 파괴
  - ThreadSanitizer 및 AddressSanitizer로 검증 (28개 테스트 모두 통과)

---

## [2.0.0] - 2025-11-15

### 추가됨
- **typed_thread_pool**: 타입 안전 스레드 풀 구현
  - 컴파일 타임 타입 안전성
  - 커스텀 처리 함수
  - 자동 타입 추론
- **adaptive_queue**: 동적 리사이징 큐
  - 자동 부하 기반 스케일링
  - 구성 가능한 임계값
  - 메모리 효율적
- **hazard_pointer**: Lock-free 구조를 위한 안전한 메모리 회수
  - ABA 문제 완화
  - 자동 가비지 컬렉션
- **서비스 인프라**: 서비스 라이프사이클 관리
  - 의존성 주입을 위한 service_registry
  - service_base 추상 클래스
  - 자동 정리

### 변경됨
- **thread_pool**: 주요 성능 개선
  - Work-stealing 알고리즘 구현
  - 4.5배 처리량 개선 (1.2M ops/sec)
  - 0.8 μs로 지연시간 감소
  - 16코어까지 선형에 가까운 스케일링
- **mpmc_queue**: Lock-free 최적화
  - 5.2배 성능 개선 (2.1M ops/sec)
  - 더 나은 캐시 지역성
  - False sharing 감소
- **thread_base**: 향상된 라이프사이클 관리
  - C++20 jthread 지원
  - 개선된 에러 처리
  - 더 나은 모니터링 기능

### 수정됨
- **이슈 #45**: thread_pool 종료 시 레이스 컨디션
  - 적절한 동기화 추가
  - 종료 전 모든 태스크 완료 보장
- **이슈 #38**: mpmc_queue의 메모리 누수
  - 해저드 포인터 구현
  - 노드 정리 로직 수정
- **이슈 #29**: service_registry의 데드락
  - 순환 의존성 제거
  - 데드락 감지 추가

### 성능
- **thread_pool**: 4.5배 개선
  - 이전: 267K ops/sec
  - 이후: 1.2M ops/sec
  - 지연시간: 3.6 μs → 0.8 μs
- **mpmc_queue**: 5.2배 개선
  - 이전: 404K ops/sec
  - 이후: 2.1M ops/sec
  - 지연시간: 2.5 μs → 0.5 μs
- **typed_thread_pool**: 기본 구현 대비 3.8배 개선
  - 980K ops/sec
  - 런타임 비용 없는 타입 안전성

---

## [1.5.0] - 2025-10-22

### 추가됨
- **spsc_queue**: Single-producer single-consumer 큐
  - Lock-free 순환 버퍼
  - 3.5M ops/sec 처리량
- **Read-Write Lock**: 읽기 중심 워크로드에 최적화
  - Writer 기아 방지
  - 구성 가능한 공정성

### 변경됨
- **thread_pool**: 우선순위 기반 태스크 실행
  - 3단계 우선순위 시스템
  - 공정한 스케줄링 알고리즘
- **thread_base**: 향상된 스레드 네이밍
  - 자동 ID 생성
  - 커스텀 이름 지원

### 수정됨
- **이슈 #22**: 조건 변수의 가짜 웨이크업
- **이슈 #18**: 태스크 실행의 예외 안전성

---

## [1.0.0] - 2025-09-15

### 추가됨
- thread_system 초기 릴리스
- **thread_base**: 기반 스레드 추상화
  - 시작/정지 라이프사이클
  - 조건 모니터링
  - 상태 관리
- **thread_pool**: 기본 스레드 풀 구현
  - 고정 크기 워커 풀
  - 태스크 큐
  - Future/Promise 패턴
- **mpmc_queue**: 기본 MPMC 큐
  - 뮤텍스 기반 구현
  - 스레드 안전 작업
- **동기화 프리미티브**:
  - spinlock
  - 기본 잠금 메커니즘

### 성능
- thread_pool: 267K ops/sec
- mpmc_queue: 404K ops/sec
- 기본 기능 검증 완료

---

## 버전 규칙

### Major Version (X.0.0)
- API 호환성이 깨지는 변경
- 아키텍처 대규모 변경
- 필수 의존성 주요 업데이트

### Minor Version (0.X.0)
- 새로운 기능 추가 (하위 호환성 유지)
- 성능 개선
- 내부 리팩토링

### Patch Version (0.0.X)
- 버그 수정
- 문서 업데이트
- 마이너한 개선

---

## 참조

- [프로젝트 이슈](https://github.com/kcenon/thread_system/issues)
- [마일스톤](https://github.com/kcenon/thread_system/milestones)

---

[미출시]: https://github.com/kcenon/thread_system/compare/v3.0.0...HEAD
[3.0.0]: https://github.com/kcenon/thread_system/compare/v2.0.0...v3.0.0
[2.0.0]: https://github.com/kcenon/thread_system/compare/v1.5.0...v2.0.0
[1.5.0]: https://github.com/kcenon/thread_system/compare/v1.0.0...v1.5.0
[1.0.0]: https://github.com/kcenon/thread_system/releases/tag/v1.0.0
