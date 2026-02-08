# Thread System 기능 상세

**버전**: 0.3.0
**최종 업데이트**: 2026-02-08
**언어**: [English](FEATURES.md) | [한국어]

---

## 개요

이 문서는 thread_system의 모든 기능에 대한 상세한 설명, 사용 사례, 구현 세부사항을 제공합니다.

---

## 목차

1. [핵심 스레딩 기능](#핵심-스레딩-기능)
2. [큐 구현](#큐-구현)
3. [스레드 풀 기능](#스레드-풀-기능)
4. [타입 스레드 풀](#타입-스레드-풀)
5. [적응형 컴포넌트](#적응형-컴포넌트)
6. [동기화 프리미티브](#동기화-프리미티브)
7. [서비스 인프라](#서비스-인프라)
8. [고급 기능](#고급-기능)
9. [DAG 스케줄러](#dag-스케줄러)
10. [NUMA 인식 Work Stealing](#numa-인식-work-stealing)
11. [오토스케일링](#오토스케일링)
12. [진단 시스템](#진단-시스템)

---

## 핵심 스레딩 기능

### thread_base 클래스

시스템의 모든 스레드 작업을 위한 기본 추상 클래스입니다.

#### 주요 기능

- **듀얼 스레드 지원**: 조건부 컴파일을 통해 `std::jthread` (C++20)와 `std::thread` 모두 지원
- **라이프사이클 관리**: 커스터마이징 가능한 훅이 있는 내장 시작/정지 라이프사이클
- **스레드 모니터링**: 조건 모니터링 및 상태 관리
- **커스텀 네이밍**: 의미있는 스레드 식별을 통한 향상된 디버깅

#### API 개요

```cpp
class thread_base {
public:
    virtual auto start() -> result_void;
    virtual auto stop(bool immediately = false) -> result_void;
    virtual auto is_running() const -> bool;

protected:
    virtual auto on_initialize() -> result_void;
    virtual auto on_execute() -> result_void;
    virtual auto on_destroy() -> result_void;
};
```

#### 사용 사례

- 워커 스레드의 기본 클래스
- 커스텀 스레드 구현
- 스레드 풀 워커
- 백그라운드 서비스 스레드

---

## 큐 구현

### 1. 표준 Job 큐

기본 작업 관리를 위한 스레드 안전 FIFO 큐입니다.

```cpp
class job_queue {
public:
    auto enqueue(std::unique_ptr<job>&& job) -> result_void;
    auto dequeue() -> result<std::unique_ptr<job>>;
    auto size() const -> std::size_t;
    auto empty() const -> bool;
};
```

**기능**:
- 뮤텍스 기반 스레드 안전성
- FIFO 순서 보장
- 블로킹 dequeue 작업
- 무제한 용량

**적합한 사용 사례**:
- 범용 작업 큐잉
- 저~중간 경합 시나리오
- 신뢰성이 필요한 단순 사용 사례

---

### 2. 경계 크기 제한 Job 큐

`max_size` 파라미터를 통한 선택적 용량 제한이 있는 스레드 안전 큐입니다.

```cpp
class job_queue {
public:
    // 무제한 큐 생성 (기본값)
    job_queue();

    // 최대 용량이 있는 경계 큐 생성
    explicit job_queue(std::optional<std::size_t> max_size);

    auto enqueue(std::unique_ptr<job>&& job) -> common::VoidResult;
    auto dequeue() -> common::Result<std::unique_ptr<job>>;
    auto is_bounded() const -> bool;
    auto get_max_size() const -> std::optional<std::size_t>;
    auto is_full() const -> bool;
};
```

**기능**:
- 선택적 최대 큐 크기 강제
- `backpressure_job_queue`를 통한 백프레셔 지원
- 뮤텍스 보호를 통한 스레드 안전 작업
- 포괄적인 메트릭 추적
- 메모리 고갈 방지

**적합한 사용 사례**:
- 제한된 리소스를 가진 프로덕션 시스템
- 백프레셔 처리가 필요한 시스템
- 고신뢰성 애플리케이션
- 리소스 제약 환경

---

### 3. Lock-Free Job 큐

해저드 포인터 메모리 회수를 사용하는 고성능 lock-free MPMC 큐입니다.

```cpp
class lockfree_job_queue {
public:
    auto enqueue(std::unique_ptr<job>&& job) -> result_void;
    auto dequeue() -> result<std::unique_ptr<job>>;
    auto size() const -> std::size_t;
    auto empty() const -> bool;
};
```

**기능**:
- Lock-free 멀티 프로듀서, 멀티 컨슈머 (MPMC) 설계
- 해저드 포인터 기반 메모리 회수
- CAS (Compare-And-Swap) 작업
- 효율성을 위한 노드 풀링
- 뮤텍스 기반 큐보다 **4배 빠름** (71 μs vs 291 μs)

**적합한 사용 사례**:
- 고경합 시나리오 (8+ 스레드)
- 최대 처리량 요구사항
- 저지연 크리티컬 경로
- 락 오버헤드가 중요한 시스템

**성능**:
| 스레드 | 처리량 | 지연시간 |
|---------|-----------|---------|
| 1-2     | ~96 ns    | 저경합 |
| 4       | ~142 ns   | 중간 부하 |
| 8+      | ~320 ns   | 고경합 (여전히 37% 빠름) |

---

### 4. 적응형 Job 큐

뮤텍스와 lock-free 전략 사이를 자동으로 전환하는 지능형 큐입니다.

```cpp
class adaptive_job_queue {
public:
    auto enqueue(std::unique_ptr<job>&& job) -> result_void;
    auto dequeue() -> result<std::unique_ptr<job>>;
    auto get_statistics() -> queue_statistics;
};

struct queue_statistics {
    size_t total_enqueued;
    size_t total_dequeued;
    size_t current_size;
    queue_mode current_mode;  // mutex 또는 lockfree
};
```

**기능**:
- 경합에 따른 **자동 전략 선택**
- 뮤텍스와 lock-free 모드 간 원활한 전환
- 성능 모니터링 및 적응
- 설정 불필요

**적응형 전략**:
- **저경합** (1-2 스레드): 뮤텍스 기반 큐 사용
- **중간 경합** (4 스레드): 평가 및 적응
- **고경합** (8+ 스레드): lock-free 모드로 전환

**적합한 사용 사례**:
- 가변 워크로드 패턴
- 시간에 따라 경합이 변하는 시스템
- 최적 성능이 중요한 배포
- "설정 후 잊기" 최적화가 필요한 애플리케이션

---

## 스레드 풀 기능

### 스레드 풀 선택 가이드

thread_system은 서로 다른 사용 사례에 최적화된 두 가지 스레드 풀 구현을 제공합니다:

| 기능 | `thread_pool` | `typed_thread_pool_t<T>` |
|------|--------------|--------------------------|
| 스케줄링 | FIFO (선입선출) | 우선순위 기반 |
| 작업 우선순위 | 지원 안함 | 템플릿 파라미터 (컴파일 타임) |
| 워커 특수화 | 모든 워커가 모든 작업 처리 | 워커별 특정 우선순위 할당 가능 |
| Work Stealing | 지원 | 미지원 |
| 메트릭 수집 | 내장 `ThreadPoolMetrics` | 기본 통계 |
| 헬스 체크 | 지원 | 미지원 |
| 적합한 용도 | 범용 작업 실행 | 우선순위 중심 워크로드 |

**`thread_pool` 사용 시기:**
- 범용 동시 작업 실행
- 모든 작업이 동일한 우선순위를 가질 때
- Work-stealing 로드 밸런싱이 필요할 때
- 상세한 메트릭과 헬스 모니터링이 필요할 때

**`typed_thread_pool_t<T>` 사용 시기:**
- 엄격한 우선순위 요구사항이 있는 실시간 시스템
- 작업이 우선순위 순서대로 처리되어야 할 때
- 워커가 특정 우선순위 레벨만 처리하도록 특수화해야 할 때
- 우선순위에 대한 컴파일 타임 타입 안전성이 필요할 때

---

### 표준 스레드 풀

적응형 큐 지원이 있는 멀티 워커 스레드 풀입니다.

```cpp
class thread_pool {
public:
    thread_pool(const std::string& name = "ThreadPool");

    auto start() -> result_void;
    auto stop(bool immediately = false) -> result_void;

    // 작업 제출
    auto enqueue(std::unique_ptr<job>&& job) -> result_void;
    auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> result_void;
    bool submit_task(std::function<void()> task);  // 편의 API

    // 워커 관리
    auto add_worker(std::unique_ptr<thread_worker>&& worker) -> result_void;
    auto add_workers(size_t count) -> result_void;

    // 모니터링
    auto get_thread_count() const -> size_t;
    auto get_pending_task_count() const -> size_t;
    auto get_idle_worker_count() const -> size_t;

    // 종료
    bool shutdown_pool(bool immediately = false);
};
```

**주요 기능**:

1. **동적 워커 관리**
   - 런타임에 워커 추가/제거
   - 자동 워커 라이프사이클 관리
   - 워커 통계 추적

2. **적응형 큐 아키텍처**
   - 부하에 따른 자동 최적화
   - 듀얼 모드 지원 (뮤텍스/lock-free)
   - 배치 처리 기능

3. **듀얼 API 설계**
   - 상세한 에러 처리를 위한 Result 기반 API
   - 단순성을 위한 편의 API (`submit_task`, `shutdown_pool`)

4. **포괄적인 모니터링**
   - 워커 수 추적
   - 대기 작업 모니터링
   - 유휴 워커 감지
   - 성능 메트릭

**성능**:
- **처리량**: 1.16M jobs/초 (10 워커, 프로덕션 워크로드)
- **지연시간**: 작업당 ~77 ns 스케줄링
- **메모리**: <1 MB 기본 오버헤드
- **확장성**: 8 워커까지 96% 효율

**사용 사례**:
- 범용 동시 작업 실행
- 웹 서버 요청 처리
- 백그라운드 작업 처리
- 병렬 알고리즘 실행

---

## 타입 스레드 풀

타입 인식 작업 스케줄링이 있는 우선순위 기반 스레드 풀입니다.

### 개요

```cpp
template<typename T>
class typed_thread_pool_t {
public:
    auto start() -> result_void;
    auto stop(bool clear_queue = false) -> result_void;

    auto enqueue(std::unique_ptr<typed_job_t<T>>&& job) -> result_void;
    auto enqueue_batch(std::vector<std::unique_ptr<typed_job_t<T>>>&& jobs)
        -> result_void;

    auto get_statistics() const -> typed_pool_statistics_t<T>;
};
```

### 작업 타입

기본 우선순위 레벨:

```cpp
enum class job_types {
    RealTime,    // 최고 우선순위
    Batch,       // 중간 우선순위
    Background   // 최저 우선순위
};
```

**커스텀 타입**: 도메인별 우선순위화를 위해 자체 열거형이나 타입을 사용할 수 있습니다.

### 기능

1. **타입별 적응형 큐**
   - 각 작업 타입은 자체 적응형 큐를 가짐
   - 우선순위 레벨별 독립적 최적화
   - 자동 큐 라이프사이클 관리

2. **우선순위 기반 라우팅**
   - RealTime > Batch > Background 순서
   - 워커는 최고 우선순위부터 가져옴
   - 같은 타입 내에서 FIFO 보장

3. **타입 인식 워커**
   - 구성 가능한 타입 책임 목록
   - 워커가 여러 타입 처리 가능
   - 동적 타입 적응

4. **고급 통계**
   - 타입별 성능 메트릭
   - 큐 깊이 모니터링
   - 처리 시간 추적

### 성능

**타입 풀 비교**:
| 구성 | 처리량 | 기본 풀 대비 | 타입 정확도 |
|--------------|------------|---------------|---------------|
| 단일 타입  | 525K/s     | -3%           | 100%          |
| 3 타입      | 495K/s     | -9%           | 99.6%         |
| 실제 워크로드| **1.24M/s**| **+6.9%**     | 100%          |

### 사용 사례

- 우선순위 요구사항이 있는 실시간 시스템
- 게임 엔진 (렌더링 vs 물리 vs AI)
- 미디어 처리 (인코딩 vs 디코딩 vs I/O)
- 금융 시스템 (트레이딩 vs 리포팅 vs 분석)

---

## 적응형 컴포넌트

### 해저드 포인터

Lock-free 데이터 구조를 위한 안전한 메모리 회수입니다.

```cpp
class hazard_pointer {
public:
    static auto protect(const void* ptr) -> hazard_pointer_guard;
    static auto retire(void* ptr, std::function<void(void*)> deleter) -> void;
    static auto scan() -> void;
};
```

**기능**:
- 동시 환경에서 안전한 메모리 회수
- use-after-free 방지
- 자동 가비지 컬렉션
- 제로 오탐지

**작동 방식**:
1. 스레드가 사용 중인 포인터를 "위험"으로 표시
2. 삭제된 노드는 retire 목록에 추가
3. 주기적 스캔으로 안전한 삭제 확인
4. 참조되지 않는 노드만 해제

---

## 동기화 프리미티브

향상된 동기화 래퍼와 현대 C++ 기능입니다.

### 범위 지정 락 가드

타임아웃 지원이 있는 RAII 락입니다.

```cpp
class scoped_lock_guard {
public:
    scoped_lock_guard(std::mutex& mtx,
                     std::optional<std::chrono::milliseconds> timeout = std::nullopt);

    auto is_locked() const -> bool;
};
```

**사용법**:
```cpp
std::mutex mtx;
{
    scoped_lock_guard lock(mtx, std::chrono::milliseconds(100));
    if (lock.is_locked()) {
        // 크리티컬 섹션
    } else {
        // 타임아웃 처리
    }
}  // 자동 해제
```

---

### 취소 토큰

향상된 협력적 취소 메커니즘입니다.

```cpp
class cancellation_token {
public:
    auto cancel() -> void;
    auto is_cancelled() const -> bool;

    auto register_callback(std::function<void()> callback) -> void;
    auto create_linked_token() -> std::shared_ptr<cancellation_token>;
};
```

**기능**:
- 계층적 취소를 위한 연결된 토큰 생성
- 스레드 안전 콜백 등록
- 취소 신호의 자동 전파
- 논블로킹 취소 확인

**사용법**:
```cpp
auto token = std::make_shared<cancellation_token>();

// 워커 스레드에서
pool->submit_task([token]() {
    for (int i = 0; i < 1000000; ++i) {
        if (token->is_cancelled()) {
            return;  // 조기 종료
        }
        // 작업 수행
    }
});

// 제어 스레드에서
token->cancel();  // 취소 요청
```

---

## 서비스 인프라

### 서비스 레지스트리

경량 의존성 주입 컨테이너입니다.

```cpp
class service_registry {
public:
    template<typename T>
    auto register_service(std::shared_ptr<T> service) -> void;

    template<typename T>
    auto get_service() -> std::shared_ptr<T>;

    template<typename T>
    auto has_service() const -> bool;
};
```

**기능**:
- 타입 안전 서비스 등록
- shared_mutex를 통한 스레드 안전 액세스
- shared_ptr를 통한 자동 라이프타임 관리
- 글로벌 싱글톤 패턴

**사용법**:
```cpp
// 서비스 등록
auto logger = std::make_shared<ConsoleLogger>();
service_registry::instance().register_service(logger);

// 서비스 조회
auto logger = service_registry::instance().get_service<ILogger>();
```

---

## 고급 기능

### 워커 정책 시스템

워커 동작에 대한 세밀한 제어입니다.

```cpp
struct worker_policy {
    scheduling_policy scheduling;  // FIFO, LIFO, Priority, Work-stealing
    idle_strategy idle_behavior;   // Timeout, Yield, Sleep
    size_t max_batch_size;
    std::optional<int> cpu_affinity;

    static auto default_policy() -> worker_policy;
    static auto high_performance() -> worker_policy;
    static auto low_latency() -> worker_policy;
    static auto power_efficient() -> worker_policy;
};
```

**사전 정의된 정책**:

1. **Default**: 균형 잡힌 성능과 효율성
2. **High Performance**: 최소 지연시간, 최대 처리량
3. **Low Latency**: 가장 빠른 응답 시간
4. **Power Efficient**: 낮은 CPU 사용량

---

### 예외 안전성

프레임워크 전체에서 강력한 예외 안전성 보장입니다.

**보장**:
- 예외 경로에서 리소스 누수 없음
- 예외 후 일관된 상태
- RAII 기반 자동 정리
- 예외 안전 큐 작업

**에러 처리**:
```cpp
// 명시적 에러 처리를 위한 Result<T> 패턴
auto result = pool->start();
if (result.has_error()) {
    const auto& error = result.get_error();
    std::cerr << "에러: " << error.message()
              << " (코드: " << static_cast<int>(error.code()) << ")\n";
}
```

---

## DAG 스케줄러

의존성 관리 기능을 갖춘 방향 비순환 그래프(DAG) 기반 작업 스케줄링입니다.

### 개요

DAG 스케줄러는 작업 간 상호 의존성이 있는 복잡한 작업 오케스트레이션을 지원합니다. 의존성이 충족되면 자동으로 실행되며, 독립적인 작업은 병렬로 실행됩니다.

```cpp
#include <kcenon/thread/dag/dag_job.h>
#include <kcenon/thread/dag/dag_job_builder.h>
#include <kcenon/thread/dag/dag_scheduler.h>
```

### dag_job_builder (Fluent 빌더)

```cpp
auto job = dag_job_builder("process_data")
    .depends_on(fetch_id)
    .work([]() -> common::VoidResult {
        // 작업 실행
        return common::ok();
    })
    .on_failure([]() -> common::VoidResult {
        // 대체 로직
        return common::ok();
    })
    .build();
```

### dag_scheduler

```cpp
class dag_scheduler {
public:
    dag_scheduler(std::shared_ptr<thread_pool> pool, dag_config config = {});

    // 작업 관리
    auto add_job(std::unique_ptr<dag_job> job) -> job_id;
    auto add_job(dag_job_builder&& builder) -> job_id;
    auto add_dependency(job_id dependent, job_id dependency) -> common::VoidResult;

    // 실행
    auto execute_all() -> std::future<common::VoidResult>;
    auto execute(job_id target) -> std::future<common::VoidResult>;
    auto cancel_all() -> void;
    auto wait() -> common::VoidResult;

    // 조회
    auto get_execution_order() -> std::vector<job_id>;
    auto has_cycles() -> bool;
    template<typename T> auto get_result(job_id id) -> const T&;

    // 시각화
    auto to_dot() -> std::string;   // Graphviz DOT 형식
    auto to_json() -> std::string;  // JSON 내보내기
    auto get_stats() -> dag_stats;
};
```

### 기능

- **의존성 해결**: 자동 위상 정렬 및 의존성 추적
- **병렬 실행**: 독립적인 작업은 스레드 풀에서 동시 실행
- **순환 감지**: 실행 전 DAG 구조 유효성 검증
- **타입 결과 전달**: `std::any`를 통한 작업 간 타입 안전 결과 전달
- **실패 정책**: 작업 실패 시 구성 가능한 동작
  - `fail_fast`: 즉시 모든 의존 작업 취소
  - `continue_others`: 관련 없는 작업은 계속, 의존 작업만 건너뛰기
  - `retry`: 구성 가능한 지연 및 최대 재시도 횟수로 재시도
  - `fallback`: 실패 시 대체 함수 실행
- **시각화**: 디버깅용 DOT(Graphviz) 또는 JSON 내보내기
- **통계**: 크리티컬 경로 시간 및 병렬 효율성 포함 실행 메트릭

### 설정

```cpp
dag_config config;
config.failure_policy = dag_failure_policy::continue_others;
config.max_retries = 3;
config.retry_delay = std::chrono::milliseconds(1000);
config.detect_cycles = true;
config.execute_in_parallel = true;
config.state_callback = [](job_id id, dag_job_state old_s, dag_job_state new_s) {
    // 상태 전환 모니터링
};
```

### 작업 상태

| 상태 | 설명 |
|------|------|
| `pending` | 의존성 대기 중 |
| `ready` | 의존성 충족됨 |
| `running` | 실행 중 |
| `completed` | 성공적으로 완료 |
| `failed` | 실행 실패 |
| `cancelled` | 사용자 또는 실패 정책에 의해 취소 |
| `skipped` | 의존성 실패로 건너뜀 |

### 사용 사례

- 의존 단계가 있는 ETL 파이프라인
- 컴파일 의존성이 있는 빌드 시스템
- 워크플로 오케스트레이션
- Fan-in/Fan-out 패턴의 데이터 처리 그래프

---

## NUMA 인식 Work Stealing

최적의 메모리 지역성을 위한 NUMA 토폴로지 인식 향상된 work-stealing 스케줄러입니다.

### 개요

NUMA(Non-Uniform Memory Access) 시스템에서 크로스 노드 메모리 접근은 상당한 지연 패널티를 초래합니다. NUMA 인식 work stealer는 같은 NUMA 노드의 워커에서 우선적으로 작업을 가져와 이러한 패널티를 최소화합니다.

```cpp
#include <kcenon/thread/stealing/numa_topology.h>
#include <kcenon/thread/stealing/numa_work_stealer.h>
#include <kcenon/thread/stealing/enhanced_work_stealing_config.h>
```

### NUMA 토폴로지 감지

```cpp
auto topology = numa_topology::detect();

// 토폴로지 조회
auto node_count = topology.node_count();
auto cpu_count = topology.cpu_count();
auto is_numa = topology.is_numa_available();

// 지역성 확인
auto node = topology.get_node_for_cpu(cpu_id);
auto distance = topology.get_distance(node1, node2);
auto same = topology.is_same_node(cpu1, cpu2);
```

**플랫폼 지원**:
- **Linux**: `/sys/devices/system/node`을 통한 전체 지원
- **macOS/Windows**: 단일 노드 토폴로지 폴백 (성능 저하 없음)

### Steal 정책

| 정책 | 설명 | 적합한 용도 |
|------|------|-------------|
| `random` | 무작위 대상 선택 | 기준선, 균일 부하 |
| `round_robin` | 순차 순환 | 결정적 동작 |
| `adaptive` | 큐 크기 기반 선택 | 불균등 워크로드 |
| `numa_aware` | 같은 NUMA 노드 우선 | NUMA 시스템 |
| `locality_aware` | 과거 협력 이력 추적 | 반복 패턴 |
| `hierarchical` | NUMA 노드 우선, 이후 무작위 | 대규모 NUMA 시스템 |

### 설정

```cpp
// 사전 구축 설정
auto config = enhanced_work_stealing_config::numa_optimized();
auto config = enhanced_work_stealing_config::locality_optimized();
auto config = enhanced_work_stealing_config::batch_optimized();
auto config = enhanced_work_stealing_config::hierarchical_numa();

// 커스텀 설정
enhanced_work_stealing_config config;
config.enabled = true;
config.policy = enhanced_steal_policy::numa_aware;
config.numa_aware = true;
config.numa_penalty_factor = 2.0;      // 크로스 노드 비용 배율
config.prefer_same_node = true;
config.max_steal_batch = 4;
config.adaptive_batch_size = true;
config.collect_statistics = true;
```

### 통계

```cpp
auto stats = stealer.get_stats_snapshot();
auto success_rate = stats.steal_success_rate();    // 0.0 - 1.0
auto cross_ratio = stats.cross_node_ratio();       // 지역성 지표
auto avg_batch = stats.avg_batch_size();
auto avg_time = stats.avg_steal_time_ns();
```

### 기능

- **자동 토폴로지 감지**: 초기화 시 NUMA 레이아웃 자동 발견
- **지역성 최적화 Stealing**: 크로스 노드 메모리 접근 최소화
- **적응형 배치 크기**: 큐 깊이에 따른 동적 배치 크기
- **지수 백오프**: Steal 실패 시 구성 가능한 백오프 전략
- **상세 통계**: Steal 효율성 모니터링을 위한 원자적 카운터
- **우아한 폴백**: 비NUMA 시스템에서 오버헤드 없이 표준 work stealing 사용

---

## 오토스케일링

워크로드 메트릭 기반 동적 스레드 풀 크기 조절입니다.

### 개요

오토스케일러는 스레드 풀 메트릭을 모니터링하고 최적 처리량을 유지하면서 리소스 사용을 최소화하는 스케일링 결정을 내립니다.

### API 개요

```cpp
#include <kcenon/thread/scaling/autoscaler.h>
#include <kcenon/thread/scaling/autoscaling_policy.h>

autoscaling_policy policy;
policy.min_workers = 2;
policy.max_workers = 16;
policy.scaling_mode = autoscaling_policy::mode::automatic;

autoscaler scaler(*pool, policy);
scaler.start();

// 수동 오버라이드
auto decision = scaler.evaluate_now();
scaler.scale_to(8);
scaler.scale_up();
scaler.scale_down();

// 메트릭
auto metrics = scaler.get_current_metrics();
auto history = scaler.get_metrics_history(60);
auto stats = scaler.get_stats();

scaler.stop();
```

### 스케일링 정책

**스케일 업 트리거** (ANY 조건이 트리거):
- 워커당 큐 깊이가 임계값 초과 (기본값: 100)
- 워커 활용률이 임계값 초과 (기본값: 80%)
- P95 지연시간이 임계값 초과 (기본값: 50ms)
- 대기 작업이 임계값 초과 (기본값: 1000)

**스케일 다운 트리거** (ALL 조건 필요):
- 워커 활용률이 임계값 미만 (기본값: 30%)
- 워커당 큐 깊이가 임계값 미만 (기본값: 10)
- 유휴 지속시간 초과 (기본값: 60초)

### 설정

```cpp
autoscaling_policy policy;
policy.min_workers = 2;
policy.max_workers = std::thread::hardware_concurrency();
policy.scale_up.utilization_threshold = 0.8;
policy.scale_down.utilization_threshold = 0.3;
policy.scale_up_cooldown = std::chrono::seconds{30};
policy.scale_down_cooldown = std::chrono::seconds{60};
policy.sample_interval = std::chrono::milliseconds{1000};
policy.samples_for_decision = 5;
policy.scaling_callback = [](scaling_direction dir, scaling_reason reason,
                             std::size_t old_count, std::size_t new_count) {
    // 스케일링 이벤트 모니터링
};
```

### 스케일링 모드

| 모드 | 설명 |
|------|------|
| `disabled` | 자동 스케일링 비활성화 |
| `manual` | 명시적 API 호출로만 스케일링 |
| `automatic` | 메트릭 기반 완전 자동 스케일링 |

### 기능

- **비대칭 스케일링**: 빠른 스케일 업(반응적), 느린 스케일 다운(보수적)
- **쿨다운 기간**: 스케일링 진동 방지
- **다중 샘플 결정**: 구성 가능한 윈도우에서 메트릭 집계
- **수동 오버라이드**: 직접 scale-to, scale-up, scale-down 명령
- **메트릭 이력**: 분석용 과거 메트릭 접근
- **승수 스케일링**: 급속 스케일링을 위한 선택적 승수 계수

### 사용 사례

- 가변 트래픽의 클라우드 네이티브 서비스
- 작업량이 변동하는 배치 처리
- 오토스케일링이 필요한 마이크로서비스
- 동적 리소스 할당을 통한 비용 최적화

---

## 진단 시스템

포괄적인 스레드 풀 건강 모니터링, 병목 감지, 이벤트 추적입니다.

### 개요

진단 시스템은 스레드 풀 동작에 대한 비침투적 관찰성을 제공하며, 건강 검사, 병목 분석, 실행 이벤트 추적을 포함합니다.

```cpp
#include <kcenon/thread/diagnostics/thread_pool_diagnostics.h>

diagnostics_config config;
config.enable_tracing = true;
config.recent_jobs_capacity = 1000;

thread_pool_diagnostics diag(*pool, config);
```

### 건강 검사

```cpp
auto health = diag.health_check();

// HTTP 호환 건강 엔드포인트
int status_code = health.http_status_code();  // 200 또는 503
std::string json = health.to_json();

// 컴포넌트별 건강 상태
for (const auto& component : health.components) {
    // name, state, message, details
}

// 빠른 확인
bool ok = diag.is_healthy();
```

**건강 상태**:

| 상태 | HTTP 코드 | 설명 |
|------|-----------|------|
| `healthy` | 200 | 완전 작동 |
| `degraded` | 200 | 제한된 용량으로 작동 |
| `unhealthy` | 503 | 작동 불가 |
| `unknown` | 503 | 상태 확인 불가 |

### 병목 감지

```cpp
auto report = diag.detect_bottlenecks();

if (report.has_bottleneck) {
    // type, description, severity (0-3)
    for (const auto& rec : report.recommendations) {
        // 실행 가능한 권장 사항
    }
}
```

**병목 유형**:

| 유형 | 설명 |
|------|------|
| `queue_full` | 큐 용량 초과 |
| `slow_consumer` | 워커가 생산 속도를 따라가지 못함 |
| `worker_starvation` | 부하 대비 워커 부족 |
| `lock_contention` | 높은 뮤텍스 대기 시간 |
| `uneven_distribution` | 작업이 균등하게 분배되지 않음 |
| `memory_pressure` | 과도한 메모리 할당 |

### 스레드 덤프

```cpp
auto threads = diag.dump_thread_states();
std::string formatted = diag.format_thread_dump();
// 워커별 상태, 현재 작업, 활용률 출력
```

### 이벤트 추적

```cpp
// 추적 활성화
diag.enable_tracing(true, 1000);

// 커스텀 이벤트 리스너
class MyListener : public execution_event_listener {
public:
    void on_event(const job_execution_event& event) override {
        // event.type: enqueued, started, completed, failed 등
        // event.to_json()으로 구조화된 출력
    }
};

diag.add_event_listener(std::make_shared<MyListener>());

// 최근 이벤트 조회
auto events = diag.get_recent_events(100);
```

### 내보내기 형식

```cpp
std::string json = diag.to_json();           // 대시보드용 JSON
std::string text = diag.to_string();          // 사람이 읽을 수 있는 형식
std::string prom = diag.to_prometheus();      // Prometheus 메트릭
```

**Prometheus 메트릭**:
- `thread_pool_health_status` (gauge)
- `thread_pool_jobs_total` (counter)
- `thread_pool_success_rate` (gauge)
- `thread_pool_latency_avg_ms` (gauge)
- `thread_pool_workers_active` / `_idle` (gauge)
- `thread_pool_queue_depth` / `_saturation` (gauge)

### 기능

- **비침투적**: 적극적으로 조회하지 않을 때 최소 오버헤드
- **스레드 안전**: 모든 메서드를 모든 스레드에서 호출 가능
- **Kubernetes 통합**: liveness/readiness 프로브용 HTTP 호환 건강 검사
- **Prometheus 호환**: Prometheus 형식으로 메트릭 내보내기
- **병목 분석**: 심각도 수준 및 권장 사항이 있는 자동 감지
- **이벤트 추적**: 리스너 패턴으로 세밀한 실행 이벤트 추적
- **스레드 덤프**: 디버깅용 워커별 상태 스냅샷

### 사용 사례

- Kubernetes liveness 및 readiness 프로브
- 성능 대시보드 및 알림
- 프로덕션 디버깅 및 근본 원인 분석
- SLA 모니터링 및 용량 계획

---

## 요약

thread_system은 다음을 제공하는 포괄적인 고성능 스레딩 프레임워크입니다:

- 다양한 시나리오를 위한 **여러 큐 구현**
- 자동으로 최적화하는 **적응형 컴포넌트**
- 우선순위 워크로드를 위한 **타입 기반 스케줄링**
- 복잡한 의존성 그래프를 위한 **DAG 기반 오케스트레이션**
- 최적 메모리 지역성을 위한 **NUMA 인식 스케줄링**
- 워크로드 반응형 풀 크기 조절을 위한 **동적 오토스케일링**
- 건강 모니터링 및 병목 감지를 위한 **포괄적 진단**
- 복잡한 시나리오를 위한 **풍부한 동기화 프리미티브**
- 클린 아키텍처를 위한 **서비스 인프라**
- 로깅 및 모니터링을 위한 **선택적 통합**

사용 사례에 맞는 올바른 기능을 선택하세요:
- **단순 작업**: 적응형 큐가 있는 표준 스레드 풀
- **우선순위 워크로드**: 타입 스레드 풀
- **제한된 리소스**: 경계 Job 큐
- **최대 성능**: Lock-free 큐 또는 적응형 모드
- **가변 부하**: 적응형 큐 (자동 최적화)
- **복잡한 워크플로**: 의존성 관리를 위한 DAG 스케줄러
- **다중 소켓 서버**: NUMA 인식 work stealing
- **클라우드 서비스**: 진단이 포함된 오토스케일링

---

**참고**:
- [성능 벤치마크](BENCHMARKS.md)
- [아키텍처 가이드](advanced/ARCHITECTURE.md)
- [API 레퍼런스](guides/API_REFERENCE.md)
- [사용자 가이드](guides/USER_GUIDE.md)

---

**최종 업데이트**: 2026-02-08
**관리자**: kcenon@naver.com
