# 변경 이력

[English](CHANGELOG.md) | **한국어**

Thread System 프로젝트의 주요 변경사항이 이 파일에 기록됩니다.

형식은 [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)를 기반으로 하며,
이 프로젝트는 [Semantic Versioning](https://semver.org/spec/v2.0.0.html)을 따릅니다.

## [Unreleased]

### Added
- **내보내기 및 직렬화** (#392)
  - `job_info`, `thread_info`, `bottleneck_report`의 `to_json()` 및 `to_string()` 메서드
  - Prometheus 호환 메트릭 내보내기를 위한 `to_prometheus()` 메서드
  - 시간 변환을 위한 헬퍼 메서드 (wait_time_ms, execution_time_ms, busy_time_ms, idle_time_ms)

- **향상된 취소 토큰** (#382)
  - 타임아웃 및 데드라인 지원이 포함된 `enhanced_cancellation_token`
  - 취소 이유를 추적하기 위한 `cancellation_reason` 구조체
  - 구조화된 예외 처리를 위한 `operation_cancelled_exception`
  - 헬퍼 클래스: `cancellation_callback_guard`, `cancellation_scope`, `cancellation_context`
  - 29개의 포괄적인 단위 테스트

- **이벤트 트레이싱** (#391)
  - `job_execution_event`의 `to_json()` 및 `to_string()` 직렬화 메서드
  - 워커 스레드에서 이벤트 생성 (dequeued, started, completed, failed)
  - 이벤트 기록을 위한 `thread_worker`의 `set_diagnostics()` 메서드
  - `thread_pool`에서 자동 진단 전파
  - 이벤트 트레이싱을 위한 포괄적인 테스트 (12개 테스트)

- **병목 현상 탐지** (#389)
  - 큐 포화도 계산을 포함한 완전한 병목 현상 분석
  - 불균등 작업 분배 탐지를 위한 활용도 분산 계산
  - 처리 속도와 큐 깊이 기반 예상 백로그 시간
  - 잠금 경합 탐지 (높은 대기 시간 + 낮은 활용도 패턴)
  - 큐 메모리 통계를 사용한 메모리 압박 탐지
  - 각 병목 유형에 대한 실행 가능한 권장사항
  - 병목 현상 탐지를 위한 포괄적인 단위 테스트

---

## [3.0.0] - 2025-12-19

이 릴리스는 common_system 전용 공개 계약으로의 마이그레이션을 완료합니다. 전체 릴리스 노트는 [docs/CHANGELOG.kr.md](../CHANGELOG.kr.md)를 참조하세요.

---

## [Unreleased: 이전 마일스톤] - 2025-09-13

참고: 아래의 모든 항목은 이전 사전 릴리스 마일스톤입니다.

### Changed
- **주요 모듈화** 🏗️
  - logger 모듈을 별도 프로젝트로 이동 (https://github.com/kcenon/logger_system)
  - monitoring 모듈을 별도 프로젝트로 이동 (https://github.com/kcenon/monitoring_system)
  - 핵심 thread system에서 ~8,700+ 줄의 코드 제거
  - 핵심 thread system은 이제 순수 스레딩에 집중하는 ~2,700줄
  - 외부 모듈 통합을 위한 인터페이스 기반 아키텍처 생성

### Removed
- **Logger 모듈** 📝
  - 모든 logger 구현 파일 (27개 파일)
  - Logger 벤치마크 및 샘플
  - Logger 단위 테스트
  - Logger는 이제 선택적 외부 의존성

- **Monitoring 모듈** 📊
  - metrics_collector 구현
  - monitoring_types 및 ring_buffer
  - Monitoring 벤치마크 및 샘플
  - Monitoring 단위 테스트
  - Monitoring은 이제 선택적 외부 의존성

- **사용하지 않는 유틸리티** 🔧
  - file_handler (I/O 유틸리티)
  - argument_parser (파싱 유틸리티)
  - datetime_tool (시간 유틸리티)
  - utilities의 이전 logger 인터페이스

### Added
- **인터페이스 기반 설계** 🔌
  - 선택적 logger 통합을 위한 logger_interface.h
  - 선택적 monitoring 통합을 위한 monitoring_interface.h
  - 핵심 기능과 보조 기능 간의 명확한 분리

### Documentation
- **모든 문서 업데이트** 📚
  - README.md에 모듈화 아키텍처 반영
  - api-reference.md가 핵심 API에 집중
  - architecture.md에 새로운 모듈화 설계 설명
  - USER_GUIDE.md에 상세한 빌드 지침 및 완전한 샘플 목록 추가
  - **신규: PLATFORM_BUILD_GUIDE.md** - 플랫폼별 포괄적 빌드 지침
  - dependency_compatibility_matrix.md에 최신 의존성 버전으로 업데이트
  - MIGRATION.md에 현재 상태 업데이트 (2025-09-13)
  - INTERFACES.md 현재 구현과의 일관성 검증
  - 성능 벤치마크를 2025-09-13로 업데이트
  - samples/에서 핵심만 사용하는 경우와 모듈과 함께 사용하는 경우 시연
  - 모든 문서가 현재 코드베이스 구조와 동기화됨

## [Unreleased: Milestone 2.1.0] - 2025-09-06

### Added
- 인터페이스: `executor_interface`, `scheduler_interface`, `monitorable_interface`
- 경량 DI를 위한 `service_registry` (header-only)
- 인터페이스 테스트 및 service registry 샘플
- 문서: `docs/INTERFACES.md`, `docs/USER_GUIDE.md`, `docs/QUALITY.md`, `docs/COVERAGE.md`, 모듈 README
- CMake: docs 타겟 (Doxygen), sanitizer 및 clang-tidy 옵션

### Changed
- `thread_pool`, `typed_thread_pool`이 `executor_interface` 구현
- `job_queue`가 `scheduler_interface` 구현
- CMake 모듈 구조 및 설치 규칙이 새로운 레이아웃에 맞게 업데이트됨

### Testing
- job_queue, thread_pool, typed_thread_pool에 대한 오류 경로 테스트 추가
- 단위 테스트 타겟에 sanitizer 연결

## [Unreleased: Milestone 2.0.0] - 2025-07-22

### Added
- **메모리 최적화 개선** 💾
  - adaptive_job_queue에 대한 lazy initialization 구현
    - Legacy 및 MPMC queue는 이제 첫 사용 시에만 초기화됨
    - queue가 사용되지 않을 때 초기 메모리 사용량 ~50% 감소
  - node_pool 초기 할당 최적화
    - 초기 청크를 4에서 1로 감소
    - 청크 크기를 1024 노드에서 256 노드로 감소
    - node pool의 초기 메모리 ~93.75% 절약
  - 성능을 유지하면서 전체 메모리 사용량 크게 감소

### Changed
- **메모리 관리** 🔧
  - adaptive_job_queue 생성자가 더 이상 queue를 사전 할당하지 않음
  - node_pool이 이제 더 보수적인 초기 할당 전략 사용
  - 메모리가 사전에 할당되지 않고 필요에 따라 할당됨

## [Unreleased: 2025-07-09]

### Changed
- **주요 코드 정리** 🧹
  - 코드베이스 전반에서 중복 코드 ~2,800줄 제거
  - 내부 디렉토리의 중복 job_queue 파일 제거
  - 중복 typed_job_queue 구현 제거
  - 사용하지 않는 builder 패턴 제거 (thread_pool_builder, pool_factory)
  - 사용하지 않는 C++20 coroutine 구현 제거 (task.h - 867줄)
  - formatter 중복을 제거하기 위한 formatter_macros.h 생성
  - 모든 성능 기능을 유지하면서 아키텍처 단순화

### Added
- **코드 품질 개선** ✨
  - 상용구 코드를 줄이기 위한 formatter_macros.h 추가
  - 더 깔끔한 아키텍처로 코드 유지보수성 향상
  - 단순화된 구조를 반영하도록 모든 문서 업데이트

### Documentation
- **전체 문서 업데이트** 📚
  - 제거된 컴포넌트를 반영하도록 README.md 업데이트
  - 현재 코드베이스와 일치하도록 api-reference.md 재작성
  - 더 깔끔한 구조로 architecture.md 업데이트
  - USER_GUIDE.md 예제 업데이트
  - adaptive queue에 집중하도록 performance.md 업데이트
  - 제거된 컴포넌트에 대한 모든 참조 정리

## [Unreleased: 2025-06-30]

### Fixed
- **테스트 안정성 개선** 🔧
  - `MultipleProducerConsumer` 테스트의 segmentation fault 수정
  - MPMC queue 테스트의 race condition 처리 개선
  - 정리 관련 race condition에 대한 허용 오차 추가
  - hazard pointer 정리 문제를 방지하기 위해 테스트 복잡도 감소

### Added
- **코드 품질 분석** 📊
  - 15-20% dead code를 식별하는 포괄적인 미사용 클래스 분석
  - 실험적 기능에 대한 상세 보고서 (867줄 coroutine 시스템)
  - internal/external 경로의 중복 구현 식별
  - 잠재적으로 사용되지 않는 것으로 표시된 Builder 패턴 클래스

### Changed
- **빌드 시스템 신뢰성** ⚡
  - 병렬 컴파일 중 리소스 관리 개선
  - 리소스 제약 환경을 위한 단일 스레드 대체
  - make jobserver 제한에 대한 오류 처리 강화
  - 모든 106개 테스트가 이제 일관되게 통과

### Removed
- **Lock-Free 시스템 정리** 🧹
  - `lockfree_thread_pool` 및 관련 컴포넌트 제거
  - `typed_lockfree_thread_pool` 및 관련 컴포넌트 제거
  - `lockfree_thread_worker` 및 `typed_lockfree_thread_worker` 제거
  - lock-free job queue 구현 제거
  - lock-free logger 구현 제거
  - adaptive queue를 legacy 전용 모드로 단순화
  - 모든 lock-free 샘플 및 벤치마크 제거
  - 단순화된 아키텍처를 반영하도록 문서 업데이트

## [Unreleased: 2025-06-29]

### Added
- **완전한 Lock-Free Thread Pool 시스템** 🆕
  - 평균 2.14배 성능 향상을 보이는 `lockfree_thread_pool` 클래스
  - 고급 통계 및 배치 처리를 지원하는 `lockfree_thread_worker`
  - MPMC queue 및 hazard pointer를 사용하는 `lockfree_job_queue`
  - 높은 경합 상태에서 우수한 확장성 (16+ producer에서 최대 3.46배 향상)

- **Lock-Free Typed Thread Pool 시스템** 🆕
  - 타입별 lock-free queue를 사용하는 `typed_lockfree_thread_pool_t<T>` 템플릿
  - 우선순위 인식 처리를 지원하는 `typed_lockfree_thread_worker_t<T>`
  - 각 job 타입에 대해 별도 queue를 사용하는 `typed_lockfree_job_queue_t<T>`
  - mutex 기반 typed pool 대비 7-71% 성능 향상

- **고성능 Lock-Free Logger** 🆕
  - wait-free enqueue 연산을 지원하는 `lockfree_logger` 싱글톤
  - lock-free job queue를 사용하는 `lockfree_log_collector`
  - 16 스레드에서 표준 logger 대비 최대 238% 향상된 처리량
  - 표준 logger와 동일한 API를 제공하는 드롭인 대체

- **고급 메모리 관리 시스템** 🆕
  - 안전한 lock-free 메모리 회수를 위한 `hazard_pointer` 구현
  - 고성능 메모리 할당을 위한 `node_pool<T>` 템플릿
  - lock-free 데이터 구조에서 ABA 문제 및 메모리 누수 방지
  - 최적 성능을 위한 cache-line 정렬 구조

- **포괄적인 성능 모니터링** 🆕
  - 모든 lock-free 컴포넌트에 대한 상세 통계 API
  - worker별 성능 메트릭 (처리된 job, 지연시간, 배치 연산)
  - Queue 연산 통계 (enqueue/dequeue 지연시간, 재시도 횟수)
  - metrics_collector를 통한 실시간 모니터링

- **향상된 문서**
  - 모든 lock-free 구현에 대한 완전한 API 참조 문서
  - 성능 비교 표 및 벤치마킹 데이터
  - 각 컴포넌트에 대한 사용 예제 및 모범 사례
  - lock-free 알고리즘에 대한 상세 아키텍처 문서
  - 포괄적인 프로젝트 개요 및 장점을 포함하는 향상된 README.md

- **배치 처리 지원**
  - 향상된 처리량을 위한 배치 job 제출
  - worker를 위한 구성 가능한 배치 크기
  - lock-free queue를 위한 배치 dequeue 연산

- **고급 구성 옵션**
  - lock-free 연산에서 경합 처리를 위한 백오프 전략
  - 메모리 관리를 위한 hazard pointer 구성
  - 런타임 통계 수집 및 재설정 기능

### Changed
- **주요 변경**: `priority_thread_pool`을 `typed_thread_pool`로 이름 변경하여 job 타입 기반 스케줄링 패러다임을 더 잘 반영
- **주요 변경**: `job_priorities` enum을 `job_types`로 변경하고 값을: RealTime, Batch, Background로 변경
- 일관성을 위해 모든 `priority_*` 클래스를 `typed_*`로 이름 변경
- **향상된 성능 특성**:
  - Lock-free thread pool: **평균 2.14배 성능 향상**
  - Lock-free typed thread pool: 부하 상태에서 **7-71% 성능 향상**
  - Lock-free logger: 높은 동시성에서 **최대 238% 향상된 처리량**
  - Lock-free queue 연산: 경합 상태에서 **enqueue 7.7배 빠름**, **dequeue 5.4배 빠름**
- **개선된 API 설계**:
  - 모든 lock-free 구현은 표준 버전의 드롭인 대체
  - 상세한 오류 메시지를 포함하는 향상된 오류 처리
  - 성능 모니터링을 위한 포괄적인 통계 API
  - 최대 유연성을 위한 템플릿 기반 설계
- **메모리 관리 향상**:
  - Hazard pointer 구현으로 lock-free 구조에서 메모리 누수 방지
  - Node pooling으로 할당 오버헤드 감소
  - Cache-line 정렬로 false sharing 방지
- **업데이트된 문서**:
  - 모든 lock-free 구현에 대한 완전한 API 참조
  - 성능 벤치마킹 데이터 및 비교 표
  - lock-free 예제가 포함된 업데이트된 샘플
  - 향상된 아키텍처 문서
- **MPMC Queue 개선**:
  - 안정성 향상을 위해 thread-local storage 완전 제거
  - 무한 루프를 방지하기 위한 재시도 제한 추가 (MAX_TOTAL_RETRIES = 1000)
  - 모든 스트레스 테스트가 이제 활성화되어 안정적으로 통과
  - 안전한 메모리 회수를 위한 Hazard pointer 통합

### Fixed
- typed job queue의 스레드 안전성 개선
- worker 스레드 소멸 시 메모리 누수 수정
- 플랫폼별 컴파일 문제
- 높은 경합 상태에서 MPMC queue 무한 루프 문제
- Thread-local storage 정리 segmentation fault
- 모든 비활성화된 테스트가 이제 활성화되어 통과

## [Unreleased: Initial] - 2024-01-15

### Added
- Thread System 프레임워크의 초기 릴리스
- C++20 std::jthread 지원을 포함하는 핵심 thread_base 클래스
- 우선순위 기반 thread pool 시스템
- 다중 출력 대상을 지원하는 비동기 로깅 프레임워크
- 포괄적인 샘플 애플리케이션
- vcpkg 통합을 포함하는 크로스 플랫폼 빌드 시스템
- Google Test를 사용한 광범위한 단위 테스트 스위트

### 핵심 컴포넌트
- **Thread Base 모듈**: 모든 스레딩 연산의 기반
- **로깅 시스템**: 고성능 비동기 로깅
- **Thread Pool 시스템**: 효율적인 worker 스레드 관리
- **Priority Thread Pool 시스템**: 고급 우선순위 기반 스케줄링 (이후 버전에서 Typed Thread Pool로 이름 변경)

### 지원 플랫폼
- Windows (MSVC 2019+, MSYS2)
- Linux (GCC 9+, Clang 10+)
- macOS (Clang 10+)

### 성능 특성

#### 최신 성능 메트릭 (Apple M1, 8-core @ 3.2GHz, 16GB, macOS Sonoma)

**핵심 성능 개선:**
- **최대 처리량**: 최대 15.2M jobs/second (lock-free, 1 worker, 빈 job)
- **실제 처리량**:
  - 표준 thread pool: 1.16M jobs/s (10 worker)
  - **Lock-free thread pool**: 2.48M jobs/s (8 worker) - **2.14배 향상**
  - Typed thread pool (mutex): 1.24M jobs/s (6 worker, 3 타입)
  - **Typed lock-free thread pool**: 2.38M jobs/s (100 jobs), mutex 대비 **+7.2%**

**Lock-Free Queue 성능:**
- **Enqueue 연산**: 높은 경합 상태에서 **7.7배 빠름**
- **Dequeue 연산**: 높은 경합 상태에서 **5.4배 빠름**
- **확장성**: 경합 상태에서 **2-4배 더 나은 확장**
- **메모리 효율성**: ~1.5MB (hazard pointer 사용) vs <1MB (표준)

**Logger 성능:**
- **단일 스레드**: 표준 logger 4.34M/s, Lock-free logger 3.90M/s
- **다중 스레드 (16 스레드)**: Lock-free logger **+238% 향상** (0.54M/s vs 0.16M/s)
- **지연시간**: 표준 logger 148ns, 업계 대안 대비 **15.7배 낮음**

**메모리 관리:**
- 스레드 생성 오버헤드: ~24.5 마이크로초 (Apple M1에서 측정)
- Job 스케줄링 지연시간: ~77 나노초 (표준), ~320ns (lock-free enqueue)
- Lock-free 연산은 경합과 관계없이 **일관된 성능** 유지
- Hazard pointer 오버헤드: 메모리 안전 보장을 위한 <5%

---

## 기여하기

코드 행동 강령 및 pull request 제출 프로세스에 대한 자세한 내용은 [CONTRIBUTING.md](CONTRIBUTING.md)를 읽어주세요.

## 라이선스

이 프로젝트는 BSD 3-Clause License에 따라 라이선스가 부여됩니다 - 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.
