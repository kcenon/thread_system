# thread_system API 레퍼런스

> **버전**: 3.1.0
> **최종 업데이트**: 2025-01-09
> **언어**: [English](API_REFERENCE.md) | [한국어]

## 목차

1. [네임스페이스](#네임스페이스)
2. [thread_pool (권장)](#thread_pool-권장)
3. [typed_thread_pool](#typed_thread_pool)
4. [Lock-Free 큐](#lock-free-큐)
5. [동기화 프리미티브](#동기화-프리미티브)
6. [NUMA 토폴로지](#numa-토폴로지)

---

## 네임스페이스

### `kcenon::thread`

thread_system의 모든 공개 API는 이 네임스페이스에 포함됩니다.

**포함 항목**:
- `thread_pool` - 태스크 기반 스레드 풀
- `typed_thread_pool_t` - 타입 안전 스레드 풀
- `concurrent_queue<T>` - 스레드 안전 MPMC 큐
- `lockfree_job_queue` - Lock-free 작업 큐
- `adaptive_job_queue` - 적응형 작업 큐
- 동기화 프리미티브

---

## thread_pool (권장)

### 개요

**헤더**: `#include <kcenon/thread/thread_pool.h>`

**설명**: 고성능 태스크 기반 스레드 풀

**주요 기능**:
- Future/Promise 패턴
- 우선순위 기반 태스크 실행 (선택사항)
- 동적 워커 설정
- Common system IExecutor 통합 (사용 가능 시)

> **참고**: Work-stealing은 계획 중이지만 아직 구현되지 않았습니다. 설정 옵션은 `config.h`를 참조하세요.

### 생성 및 사용

```cpp
#include <kcenon/thread/thread_pool.h>

using namespace kcenon::thread;

// 4개의 워커 스레드로 생성
thread_pool pool(4);

// 태스크 추가 (결과 있음)
auto future = pool.enqueue([]() {
    return 42;
});

// 결과 대기
int result = future.get();  // 42

// Fire-and-forget 태스크
pool.enqueue([]() {
    std::cout << "백그라운드 태스크" << std::endl;
});
```

### 핵심 메서드

#### `enqueue()`

```cpp
template<typename F, typename... Args>
auto enqueue(F&& func, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>;
```

**설명**: 태스크를 큐에 추가하고 Future 반환

**매개변수**:
- `func`: 실행할 함수 또는 람다
- `args...`: 함수 인수

**반환값**: `std::future<Result>` - 태스크 결과

**예시**:
```cpp
// 매개변수 없는 함수
auto future1 = pool.enqueue([]() { return 42; });

// 매개변수 있는 함수
auto future2 = pool.enqueue([](int x, int y) { return x + y; }, 10, 20);

// 멤버 함수
struct Calculator {
    int add(int x, int y) { return x + y; }
};
Calculator calc;
auto future3 = pool.enqueue(&Calculator::add, &calc, 10, 20);
```

#### `thread_count()`

```cpp
size_t thread_count() const noexcept;
```

**설명**: 워커 스레드 수 반환

### 성능 특성

| 메트릭 | 값 | 비고 |
|--------|-------|-------|
| 처리량 | ~1M ops/sec | 워크로드에 따라 다름 |
| 지연시간 (p50) | ~1 μs | 태스크 스케줄링 |
| 확장성 | 16코어까지 선형에 가까움 | 태스크 크기에 따라 다름 |

> **참고**: 실제 성능은 워크로드 특성, 하드웨어 및 설정에 따라 다릅니다.

---

## typed_thread_pool

### 개요

**헤더**: `#include <kcenon/thread/typed_thread_pool.h>`

**설명**: 타입 안전 스레드 풀 (3.8배 성능 향상)

**주요 기능**:
- 컴파일 타임 타입 안전성
- 자동 타입 추론
- 커스텀 처리 함수
- 적응형 큐 관리

### 생성 및 사용

```cpp
#include <kcenon/thread/typed_thread_pool.h>

using namespace kcenon::thread;

// int 타입 태스크 처리용 스레드 풀
typed_thread_pool<int> pool(4);

// 처리 함수 설정
pool.set_process_function([](int value) {
    std::cout << "처리 중: " << value << std::endl;
    return value * 2;
});

// 타입 안전 태스크 추가
pool.enqueue(10);
pool.enqueue(20);
pool.enqueue(30);

// 시작
pool.start();
```

### 핵심 메서드

#### `enqueue()`

```cpp
template<typename T>
void enqueue(T&& item);
```

**설명**: 타입 안전 태스크 추가

#### `set_process_function()`

```cpp
void set_process_function(std::function<void(T)> func);
```

**설명**: 태스크 처리 함수 설정

#### `start()` / `stop()`

```cpp
void start();
void stop();
```

**설명**: 스레드 풀 시작/정지

### 성능 특성

| 메트릭 | 값 | 비고 |
|--------|-------|-------|
| 처리량 | 980K ops/sec | 타입 안전 |
| 지연시간 (p50) | 1.0 μs | 큐 오버헤드 |
| 메모리 오버헤드 | +10% vs 일반 | 타입 메타데이터 |

---

## Concurrent 큐

### concurrent_queue

**헤더**: `#include <kcenon/thread/concurrent/concurrent_queue.h>`

**네임스페이스**: `kcenon::thread::detail`

**설명**: 블로킹 대기를 지원하는 스레드 안전 MPMC 큐 (내부 구현)

**알고리즘**: 분리된 head/tail 뮤텍스를 사용하는 세밀한 잠금

> **참고**: 이것은 `detail::` 네임스페이스의 내부 구현입니다.
> 공개 API로는 `adaptive_job_queue` 또는 `job_queue`를 사용하세요.
> 기존 `lockfree_queue<T>` 별칭은 오해의 소지가 있는 이름으로 인해 deprecated되었습니다
> (lock-free 알고리즘이 아닌 세밀한 잠금 사용).

**주요 기능**:
- 다중 프로듀서/컨슈머의 스레드 안전한 동시 접근
- 타임아웃을 지원하는 블로킹 대기
- 이동 생성 가능한 모든 타입에서 동작
- enqueue와 dequeue 작업 간 낮은 경합

#### 사용 예시

```cpp
#include <kcenon/thread/concurrent/concurrent_queue.h>

using namespace kcenon::thread::detail;

concurrent_queue<std::string> queue;

// 프로듀서 스레드
std::thread producer([&queue]() {
    queue.enqueue("message");
});

// 컨슈머 스레드 (논블로킹)
std::thread consumer([&queue]() {
    if (auto value = queue.try_dequeue()) {
        process(*value);
    }
});

// 컨슈머 스레드 (타임아웃 블로킹)
if (auto value = queue.wait_dequeue(std::chrono::milliseconds{100})) {
    process(*value);
}
```

#### 핵심 메서드

```cpp
void enqueue(T value);                              // 큐에 추가
std::optional<T> try_dequeue();                     // 논블로킹 제거
std::optional<T> wait_dequeue(duration timeout);    // 타임아웃 블로킹 제거
size_t size() const;                                // 현재 크기 (근사값)
bool empty() const;                                 // 비어있는지 확인
void shutdown();                                    // 종료 신호
```

---

### adaptive_job_queue

**헤더**: `#include <kcenon/thread/queue/adaptive_job_queue.h>`

**설명**: 뮤텍스 모드와 Lock-free 모드를 자동 전환하는 적응형 큐

**알고리즘**: 정책 기반 동적 모드 전환

**주요 기능**:
- 뮤텍스 기반 및 Lock-free 큐 구현을 래핑
- 다중 선택 정책 (정확도, 성능, 균형, 수동)
- 임시 정확도 모드를 위한 RAII 가드
- 데이터 마이그레이션과 함께 스레드 안전한 모드 전환

#### 사용 예시

```cpp
#include <kcenon/thread/queue/adaptive_job_queue.h>

using namespace kcenon::thread;

// 적응형 큐 생성 (기본값: balanced 정책)
auto queue = std::make_unique<adaptive_job_queue>();

// 일반 큐처럼 사용
queue->enqueue(std::make_unique<my_job>());
auto job = queue->dequeue();

// 임시로 정확도 요구
{
    auto guard = queue->require_accuracy();
    size_t exact = queue->size();  // 이제 정확한 값 보장
}

// 특정 정책으로 생성
auto perf_queue = std::make_unique<adaptive_job_queue>(
    adaptive_job_queue::policy::performance_first
);
```

#### 정책

| 정책 | 설명 |
|------|------|
| `accuracy_first` | 항상 뮤텍스 모드 사용 |
| `performance_first` | 항상 Lock-free 모드 사용 |
| `balanced` | 사용량에 따라 자동 전환 |
| `manual` | 사용자가 모드 제어 |

#### 핵심 메서드

```cpp
auto enqueue(std::unique_ptr<job>&& j) -> common::VoidResult;
auto dequeue() -> std::unique_ptr<job>;
size_t size() const;
bool empty() const;
auto require_accuracy() -> accuracy_guard;  // 정확한 크기를 위한 RAII 가드
void set_mode(mode m);                      // 모드 전환 (manual 정책만)
mode current_mode() const;                  // 현재 모드 조회
```

---

## 동기화 프리미티브

### safe_hazard_pointer

**헤더**: `#include <kcenon/thread/core/safe_hazard_pointer.h>`

**설명**: Lock-free 데이터 구조를 위한 스레드 안전 해저드 포인터 구현

**주요 기능**:
- ARM 및 약한 메모리 모델 아키텍처를 위한 명시적 메모리 순서
- 자동 등록이 있는 스레드별 해저드 포인터 레코드
- 설정 가능한 임계값을 사용한 지연 회수
- 적절한 동기화를 통한 스레드 안전

> **참고**: 레거시 `hazard_pointer.h`는 메모리 순서 문제(TICKET-002)로 인해 더 이상 사용되지 않습니다.
> 대신 `safe_hazard_pointer.h` 또는 `atomic_shared_ptr.h`를 사용하세요.

#### 사용 예시

```cpp
#include <kcenon/thread/core/safe_hazard_pointer.h>

using namespace kcenon::thread;

// 해저드 포인터 도메인 생성
safe_hazard_pointer_domain<Node> hp_domain;

// 레코드 획득
auto* record = hp_domain.acquire();

// 포인터 보호 (기본값: 슬롯 0)
Node* node = shared_head.load();
record->protect(node);

// 노드를 안전하게 사용 - 다른 스레드가 회수하지 않음
process(node);

// 완료 시 보호 해제
record->clear();

// 지연 회수를 위해 노드 퇴역
hp_domain.retire(old_node);
```

---

### scoped_lock_guard

**헤더**: `#include <kcenon/thread/core/sync_primitives.h>`

**설명**: 선택적 타임아웃을 지원하는 RAII 기반 스코프 락 가드

**주요 기능**:
- 자동 락 획득 및 해제
- 타이밍 뮤텍스를 위한 선택적 타임아웃
- 예외 안전 락 관리

#### 사용 예시

```cpp
#include <kcenon/thread/core/sync_primitives.h>

using namespace kcenon::thread::sync;

std::mutex mtx;

// 기본 사용
{
    scoped_lock_guard<std::mutex> guard(mtx);
    // 크리티컬 섹션
}

// 타임아웃 사용 (timed_mutex용)
std::timed_mutex timed_mtx;
{
    scoped_lock_guard<std::timed_mutex> guard(
        timed_mtx, std::chrono::milliseconds{100}
    );
    if (guard.owns_lock()) {
        // 락 획득됨
    }
}
```

---

### atomic_shared_ptr

**헤더**: `#include <kcenon/thread/core/atomic_shared_ptr.h>`

**설명**: shared_ptr에 대한 스레드 안전 원자적 연산 (해저드 포인터 대안)

**주요 기능**:
- shared_ptr에 대한 Lock-free 원자적 load/store/exchange
- 해저드 포인터보다 간단한 API
- 표준 shared_ptr과 호환

#### 사용 예시

```cpp
#include <kcenon/thread/core/atomic_shared_ptr.h>

using namespace kcenon::thread;

atomic_shared_ptr<Node> shared_node;

// 원자적 저장
shared_node.store(std::make_shared<Node>());

// 원자적 로드
auto node = shared_node.load();

// 원자적 교환
auto old = shared_node.exchange(std::make_shared<Node>());
```

---

## 성능 비교

### 큐 성능 비교

| 큐 타입 | 설명 | 사용 사례 |
|---------|------|----------|
| **job_queue** | 뮤텍스 기반, 정확한 크기 | 정확한 크기가 중요할 때 |
| **lockfree_job_queue** | Lock-free MPMC | 고처리량 태스크 큐 |
| **concurrent_queue<T>** | 블로킹 대기 지원 스레드 안전 큐 | 일반 MPMC 사용 사례 |
| **adaptive_job_queue** | 모드 자동 전환 | 가변 부하 시스템 |

> **참고**: 실제 성능은 워크로드 특성과 하드웨어에 따라 다릅니다.

### 스레드 풀 비교

| 풀 타입 | 타입 안전 | 사용 사례 |
|---------|-----------|----------|
| **thread_pool** | 런타임 | 범용 태스크 실행 |
| **typed_thread_pool_t** | 컴파일 타임 | 우선순위 기반 작업 스케줄링 |

---

## 사용 예시

### 기본 사용

```cpp
#include <kcenon/thread/thread_pool.h>

using namespace kcenon::thread;

int main() {
    // 4개의 워커 스레드로 스레드 풀 생성
    thread_pool pool(4);

    // 태스크 추가
    auto future = pool.enqueue([]() {
        return 42;
    });

    // 결과 대기
    int result = future.get();
    std::cout << "결과: " << result << std::endl;

    return 0;
}
```

### 타입 안전 스레드 풀

```cpp
#include <kcenon/thread/typed_thread_pool.h>

using namespace kcenon::thread;

int main() {
    // int 타입 태스크 처리용 스레드 풀
    typed_thread_pool<int> pool(4);

    // 타입 안전 태스크 추가
    pool.enqueue(10);
    pool.enqueue(20);
    pool.enqueue(30);

    // 태스크 처리 함수 등록
    pool.set_process_function([](int value) {
        std::cout << "처리 중: " << value << std::endl;
    });

    pool.start();

    // 처리 대기
    std::this_thread::sleep_for(std::chrono::seconds(1));

    pool.stop();

    return 0;
}
```

---

## 마이그레이션 가이드

### v2.x에서 v3.0으로

**변경사항**:
- `common_system` Result/Error 타입으로 마이그레이션
- 레거시 `thread::result<T>` 및 `thread::error` 타입 제거
- `hazard_pointer.h` 폐기 (`safe_hazard_pointer.h` 사용)
- 안정적인 umbrella 헤더 추가 (`<kcenon/thread/thread_pool.h>`)
- `shared_interfaces.h` 제거 - `common_system` 인터페이스 사용

**마이그레이션 예시**:
```cpp
// v2.x - 레거시 include
#include <kcenon/thread/core/thread_pool.h>

// v3.0 - 안정적인 umbrella 헤더
#include <kcenon/thread/thread_pool.h>

// v2.x - thread-specific Result
thread::result<int> result = ...;

// v3.0 - common system Result
common::Result<int> result = ...;
```

---

## 작업 훔치기 덱 (Work-Stealing Deque)

### 개요

**헤더**: `#include <kcenon/thread/lockfree/work_stealing_deque.h>`

`work_stealing_deque` 클래스는 Chase-Lev 알고리즘을 기반으로 한 락프리 작업 훔치기 덱을 구현합니다. 소유자 스레드를 위한 효율적인 로컬 연산과 다른 스레드를 위한 동시 훔치기 기능을 제공합니다.

### 주요 기능

- **소유자 측 push/pop**: 캐시 지역성을 위한 LIFO 순서
- **도둑 측 steal**: 공정성을 위한 FIFO 순서
- **배치 훔치기**: 여러 항목을 한 번에 효율적으로 훔침
- **락프리 연산**: CAS 연산을 통한 적절한 메모리 순서
- **동적 크기 조정**: 가득 찰 때 자동 확장

### work_stealing_deque

```cpp
template<typename T>
class work_stealing_deque {
public:
    // 생성
    explicit work_stealing_deque(std::size_t log_initial_size = 5);

    // 소유자 연산 (단일 스레드)
    void push(T item);
    [[nodiscard]] std::optional<T> pop();

    // 도둑 연산 (멀티 스레드)
    [[nodiscard]] std::optional<T> steal();
    [[nodiscard]] std::vector<T> steal_batch(std::size_t max_count);

    // 조회
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::size_t capacity() const noexcept;

    // 유지보수
    void cleanup_old_arrays();
};
```

### 스레드 안전성

| 메서드 | 스레드 안전성 |
|--------|--------------|
| `push()` | 소유자 스레드만 |
| `pop()` | 소유자 스레드만 |
| `steal()` | 모든 도둑 스레드 (동시 안전) |
| `steal_batch()` | 모든 도둑 스레드 (동시 안전) |
| `empty()`, `size()` | 모든 스레드 (근사값) |

### 사용 예제

```cpp
#include <kcenon/thread/lockfree/work_stealing_deque.h>
#include <thread>
#include <vector>

using namespace kcenon::thread::lockfree;

int main() {
    work_stealing_deque<int*> deque;
    std::vector<int> values(100);

    // 소유자가 작업을 푸시
    for (int i = 0; i < 100; ++i) {
        values[i] = i;
        deque.push(&values[i]);
    }

    // 도둑들이 배치로 작업을 훔침
    std::vector<std::thread> thieves;
    for (int t = 0; t < 4; ++t) {
        thieves.emplace_back([&]() {
            while (!deque.empty()) {
                // 한 번에 최대 4개 항목을 배치로 훔침
                auto batch = deque.steal_batch(4);
                for (auto* item : batch) {
                    // 훔친 항목 처리
                }
            }
        });
    }

    // 소유자도 로컬로 팝 가능 (LIFO)
    while (auto item = deque.pop()) {
        // 로컬 항목 처리
    }

    for (auto& t : thieves) {
        t.join();
    }

    return 0;
}
```

### 배치 훔치기

`steal_batch()` 메서드는 여러 항목을 효율적으로 훔칠 수 있습니다:

```cpp
// 최대 4개 항목을 원자적으로 훔침
auto batch = deque.steal_batch(4);

// 반환값:
// - 덱이 비어있거나 경합 시 빈 벡터
// - FIFO 순서로 최대 max_count개 항목
// - 요청보다 적은 항목을 반환할 수 있음
```

**배치 훔치기의 장점**:
- 경합 오버헤드 감소 (여러 CAS 대신 한 번의 CAS)
- 작업 전송을 위한 더 나은 캐시 효율성
- 높은 경합 상황에서 향상된 처리량

---

## NUMA 토폴로지

### 개요

**헤더**: `#include <kcenon/thread/stealing/numa_topology.h>`

`numa_topology` 클래스는 시스템의 NUMA(Non-Uniform Memory Access) 토폴로지 정보를 제공하여 멀티 소켓 시스템에서 성능 향상을 위한 NUMA 인식 태스크 스케줄링을 가능하게 합니다.

### 플랫폼 지원

| 플랫폼 | 지원 수준 | 감지 방법 |
|--------|----------|----------|
| Linux | 전체 지원 | /sys/devices/system/node |
| macOS | 폴백 | 단일 노드 토폴로지 |
| Windows | 폴백 | 단일 노드 토폴로지 |

### numa_node

```cpp
struct numa_node {
    int node_id;                    // NUMA 노드 식별자
    std::vector<int> cpu_ids;       // 이 노드에 속한 CPU들
    std::size_t memory_size_bytes;  // 이 노드의 총 메모리
};
```

### numa_topology

```cpp
class numa_topology {
public:
    // 감지
    [[nodiscard]] static auto detect() -> numa_topology;

    // 노드 쿼리
    [[nodiscard]] auto get_node_for_cpu(int cpu_id) const -> int;
    [[nodiscard]] auto get_distance(int node1, int node2) const -> int;
    [[nodiscard]] auto is_same_node(int cpu1, int cpu2) const -> bool;
    [[nodiscard]] auto is_numa_available() const -> bool;

    // 통계
    [[nodiscard]] auto node_count() const -> std::size_t;
    [[nodiscard]] auto cpu_count() const -> std::size_t;
    [[nodiscard]] auto get_nodes() const -> const std::vector<numa_node>&;
    [[nodiscard]] auto get_cpus_for_node(int node_id) const -> std::vector<int>;
};
```

### 사용 예시

```cpp
#include <kcenon/thread/stealing/numa_topology.h>
#include <iostream>

using namespace kcenon::thread;

int main() {
    // 시스템 NUMA 토폴로지 감지
    auto topology = numa_topology::detect();

    std::cout << "NUMA 노드: " << topology.node_count() << "\n";
    std::cout << "총 CPU: " << topology.cpu_count() << "\n";

    if (topology.is_numa_available()) {
        // 시스템에 여러 NUMA 노드가 있음
        for (const auto& node : topology.get_nodes()) {
            std::cout << "노드 " << node.node_id << ": "
                      << node.cpu_ids.size() << " CPU\n";
        }

        // CPU 로컬리티 확인
        int cpu1 = 0, cpu2 = 4;
        if (topology.is_same_node(cpu1, cpu2)) {
            std::cout << "CPU " << cpu1 << "과 " << cpu2
                      << "는 같은 NUMA 노드에 있음\n";
        }

        // 노드 간 거리 조회
        int dist = topology.get_distance(0, 1);
        std::cout << "노드 0과 1 사이 거리: " << dist << "\n";
    } else {
        std::cout << "단일 NUMA 노드 (UMA 시스템)\n";
    }

    return 0;
}
```

### 거리 값

`get_distance()` 메서드는 통신 비용의 상대적 측정값을 반환합니다:

| 거리 | 의미 |
|------|------|
| 10 | 로컬 (같은 노드) |
| 20-40 | 인접 노드 |
| 50+ | 원격 노드 |
| -1 | 유효하지 않은 노드 ID |

---

## 참고사항

### 스레드 안전성

- **thread_pool**: 스레드 안전 (모든 메서드)
- **typed_thread_pool_t**: 스레드 안전 (모든 메서드)
- **concurrent_queue<T>**: 스레드 안전 (다중 프로듀서/컨슈머)
- **lockfree_job_queue**: 스레드 안전 (다중 프로듀서/컨슈머)
- **job_queue**: 스레드 안전 (뮤텍스 기반)
- **adaptive_job_queue**: 스레드 안전 (다중 프로듀서/컨슈머)
- **numa_topology**: 스레드 안전 (생성 후 불변)

### 권장사항

- **일반 태스크 처리**: `thread_pool` 사용 (권장)
- **우선순위 기반 스케줄링**: `typed_thread_pool_t` 사용
- **고처리량 큐**: `lockfree_job_queue` 사용
- **범용 타입 큐**: `concurrent_queue<T>` 사용
- **가변 부하**: `adaptive_job_queue` 사용
- **안전한 메모리 회수**: `safe_hazard_pointer` 또는 `atomic_shared_ptr` 사용

---

**작성일**: 2025-11-21
**업데이트**: 2025-01-09
**버전**: 3.1.0
**관리자**: kcenon@naver.com
