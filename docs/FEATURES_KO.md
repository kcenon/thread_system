# Thread System 기능 상세

**버전**: 0.2.0
**최종 업데이트**: 2025-11-15
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

### 2. 경계 Job 큐

백프레셔 지원과 용량 제한이 있는 프로덕션 레디 큐입니다.

```cpp
class bounded_job_queue {
public:
    bounded_job_queue(size_t max_size);

    auto enqueue(std::unique_ptr<job>&& job,
                 std::optional<std::chrono::milliseconds> timeout = std::nullopt)
        -> result_void;
    auto dequeue() -> result<std::unique_ptr<job>>;
    auto get_metrics() const -> queue_metrics;
};

struct queue_metrics {
    size_t total_enqueued;
    size_t total_dequeued;
    size_t total_rejected;
    size_t timeout_count;
    size_t peak_size;
    size_t current_size;
};
```

**기능**:
- 최대 큐 크기 강제
- 용량 근처에서 백프레셔 시그널링
- enqueue 작업에 대한 타임아웃 지원
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

## 요약

thread_system은 다음을 제공하는 포괄적인 프로덕션 레디 스레딩 프레임워크입니다:

- 다양한 시나리오를 위한 **여러 큐 구현**
- 자동으로 최적화하는 **적응형 컴포넌트**
- 우선순위 워크로드를 위한 **타입 기반 스케줄링**
- 복잡한 시나리오를 위한 **풍부한 동기화 프리미티브**
- 클린 아키텍처를 위한 **서비스 인프라**
- 로깅 및 모니터링을 위한 **선택적 통합**

사용 사례에 맞는 올바른 기능을 선택하세요:
- **단순 작업**: 적응형 큐가 있는 표준 스레드 풀
- **우선순위 워크로드**: 타입 스레드 풀
- **제한된 리소스**: 경계 Job 큐
- **최대 성능**: Lock-free 큐 또는 적응형 모드
- **가변 부하**: 적응형 큐 (자동 최적화)

---

**참고**:
- [성능 벤치마크](BENCHMARKS.md)
- [아키텍처 가이드](advanced/ARCHITECTURE.md)
- [API 레퍼런스](guides/API_REFERENCE.md)
- [사용자 가이드](guides/USER_GUIDE.md)

---

**최종 업데이트**: 2025-11-15
**관리자**: kcenon@naver.com
