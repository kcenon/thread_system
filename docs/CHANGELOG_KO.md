# thread_system 변경 이력

이 프로젝트의 모든 주목할 만한 변경 사항은 이 파일에 문서화됩니다.

이 형식은 [Keep a Changelog](https://keepachangelog.com/ko/1.0.0/)를 기반으로 하며,
이 프로젝트는 [Semantic Versioning](https://semver.org/lang/ko/)을 준수합니다.

## [미출시]

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

[미출시]: https://github.com/kcenon/thread_system/compare/v2.0.0...HEAD
[2.0.0]: https://github.com/kcenon/thread_system/compare/v1.5.0...v2.0.0
[1.5.0]: https://github.com/kcenon/thread_system/compare/v1.0.0...v1.5.0
[1.0.0]: https://github.com/kcenon/thread_system/releases/tag/v1.0.0
