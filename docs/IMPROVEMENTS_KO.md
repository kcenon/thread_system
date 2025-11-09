# Thread System - 개선 계획

> **Language:** [English](IMPROVEMENTS.md) | **한국어**

## 현재 상태

**버전:** 2.0.0
**최종 검토:** 2025-01-20
**전체 점수:** 3.5/5

### 강점
- 견고한 thread pool 구현
- Modern 동기화 primitives의 효과적 사용
- 포괄적인 metrics 및 모니터링 지원
- 잘 문서화된 API

### 개선이 필요한 영역
- 무제한 큐로 인한 OOM 가능성
- `using namespace`로 인한 네임스페이스 오염
- Atomic 카운터에서 false sharing 가능성
- Lock-free 자료구조 미지원

---

## 치명적 이슈

### 1. 무제한 Job Queue - OOM 위험

**위치:** `include/kcenon/thread/core/job_queue.h:260`

**현재 문제:**
```cpp
class job_queue {
private:
    std::deque<std::unique_ptr<job>> queue_;  // ❌ 무제한!
    std::mutex mutex_;
    std::condition_variable condition_;
};
```

**문제점:**
- 높은 부하 시 큐가 무한정 증가
- 메모리 고갈 → 애플리케이션 크래시
- 배압 메커니즘 부재

**제안된 해결책:**
```cpp
// 옵션 1: 설정 가능한 동작을 가진 제한된 큐
class job_queue {
public:
    enum class overflow_policy {
        block,           // 공간이 생길 때까지 producer 차단
        drop_oldest,     // 공간 확보를 위해 가장 오래된 작업 삭제
        drop_newest,     // 들어오는 작업 삭제
        reject           // 호출자에게 오류 반환
    };

    job_queue(size_t max_size = 10000,
             overflow_policy policy = overflow_policy::block)
        : max_size_(max_size)
        , policy_(policy) {}

    result_void enqueue(std::unique_ptr<job>&& value) override {
        std::unique_lock<std::mutex> lock(mutex_);

        // 큐가 가득 찼는지 확인
        if (queue_.size() >= max_size_) {
            switch (policy_) {
                case overflow_policy::block:
                    // 공간이 생길 때까지 대기
                    not_full_cv_.wait(lock, [this] {
                        return queue_.size() < max_size_ || stop_;
                    });
                    break;

                case overflow_policy::drop_oldest:
                    queue_.pop_front();
                    dropped_count_.fetch_add(1, std::memory_order_relaxed);
                    break;

                case overflow_policy::drop_newest:
                    dropped_count_.fetch_add(1, std::memory_order_relaxed);
                    return error_info{-1, "Queue full, job dropped", "job_queue"};

                case overflow_policy::reject:
                    return error_info{-1, "Queue full", "job_queue"};
            }
        }

        queue_.push_back(std::move(value));
        if (notify_) {
            condition_.notify_one();
        }
        return result_void{};
    }

private:
    size_t max_size_;
    overflow_policy policy_;
    std::condition_variable not_full_cv_;
    std::atomic<size_t> dropped_count_{0};
};
```

**우선순위:** P0 (치명적)
**작업량:** 3-4일
**영향:** 높음 (OOM 크래시 방지)

---

## 고우선순위 개선사항

### 2. 네임스페이스 오염 제거

**위치:** 여러 파일, 예: `include/kcenon/thread/core/thread_pool.h:51-52`

**현재 문제:**
```cpp
using namespace utility_module;      // ❌ 헤더에서!
using namespace kcenon::thread;      // ❌ 헤더에서!
```

**문제점:**
- 모든 includer의 네임스페이스 오염
- 이름 충돌 가능성
- C++ 모범 사례 위반

**제안된 해결책:**
```cpp
// 헤더에서 모든 `using namespace` 제거

// 구현 파일(.cpp)에서는 함수 스코프 내에서 OK:
void some_function() {
    using kcenon::thread::job_queue;
    using kcenon::thread::thread_pool;
    // ... 한정자 없이 사용 ...
}

// 또는 네임스페이스 별칭 사용
namespace kt = kcenon::thread;

// 헤더에서는 항상 한정:
namespace kcenon::thread {
    class thread_pool {
        std::shared_ptr<kt::job_queue> queue_;  // 명시적
    };
}
```

**우선순위:** P1
**작업량:** 1일 (찾기 및 교체)
**영향:** 중간 (코드 위생, 호환성)

---

### 3. Metrics 카운터의 False Sharing 수정

**위치:** Atomic 카운터를 사용하는 thread pool 구현

**현재 문제:**
```cpp
class thread_pool {
private:
    std::atomic<size_t> active_count_{0};      // 메모리상 인접
    std::atomic<size_t> total_created_{0};     // 메모리상 인접
    std::atomic<size_t> completed_jobs_{0};    // 메모리상 인접
};
```

**문제점:**
- 이 atomic들이 캐시 라인(일반적으로 64바이트)을 공유할 가능성
- False sharing → 캐시 스래싱 → 성능 저하
- 높은 경합 시 특히 심각

**제안된 해결책:**
```cpp
// 옵션 1: 패딩
class thread_pool {
private:
    alignas(64) std::atomic<size_t> active_count_{0};
    char padding1[64 - sizeof(std::atomic<size_t>)];

    alignas(64) std::atomic<size_t> total_created_{0};
    char padding2[64 - sizeof(std::atomic<size_t>)];

    alignas(64) std::atomic<size_t> completed_jobs_{0};
};

// 옵션 2: 헬퍼 템플릿
template<typename T>
struct alignas(64) cache_line_aligned {
    T value;
    operator T&() noexcept { return value; }
};

class thread_pool {
private:
    cache_line_aligned<std::atomic<size_t>> active_count_{0};
    cache_line_aligned<std::atomic<size_t>> total_created_{0};
};
```

**성능 영향:**
- **이전:** 경합 시 atomic 증가당 ~500ns
- **이후:** atomic 증가당 ~10ns
- **개선:** 높은 경합 하에서 50배 빠름

**우선순위:** P1
**작업량:** 1-2일
**영향:** 높음 (성능)

---

### 4. Lock-Free Job Queue 도입

**현재 문제:**
- `job_queue`가 mutex + condition variable 사용
- 높은 부하 시 lock 경합
- 컨텍스트 스위칭 오버헤드

**제안된 해결책:**
Dmitry Vyukov의 bounded MPMC 큐 알고리즘을 기반으로 한 lock-free 구현

**우선순위:** P1
**작업량:** 5-7일 (철저한 테스팅 포함)
**영향:** 높음 (성능, 확장성)

---

## 중우선순위 개선사항

### 5. Adaptive Thread Pool 크기 조정

자동으로 워커 스레드 수를 조정하여 리소스 효율성 향상

**우선순위:** P2
**작업량:** 5-7일

---

### 6. 더 나은 부하 분산을 위한 Work Stealing 구현

각 워커가 자신의 큐를 가지고, 유휴 워커는 다른 워커로부터 작업을 훔쳐옴

**우선순위:** P2
**작업량:** 7-10일

---

### 7. Priority Queue 지원 추가

작업 우선순위에 따라 실행 순서 결정

**우선순위:** P3
**작업량:** 3-4일

---

## 구현 로드맵

| 단계 | 작업 | 기간 | 위험도 |
|------|------|------|--------|
| Phase 1 (치명적) | 제한된 큐, 오버플로우 정책 | 3-4일 | 낮음 |
| Phase 2 (높음) | 네임스페이스 오염 제거 | 1일 | 낮음 |
| Phase 3 (높음) | False sharing 수정 | 1-2일 | 낮음 |
| Phase 4 (높음) | Lock-free 큐 | 5-7일 | 중간 |
| Phase 5 (중간) | Adaptive 크기 조정 | 5-7일 | 중간 |
| Phase 6 (중간) | Work stealing | 7-10일 | 높음 |
| Phase 7 (낮음) | Priority 큐 | 3-4일 | 낮음 |
| Phase 8 (낮음) | Metrics 대시보드 | 3-4일 | 낮음 |

**총 예상 작업량:** 28-43일 (테스트 포함)

---

## 성능 목표

| 지표 | 현재 | 목표 | 개선 |
|------|------|------|------|
| Job 처리량 (lock-based) | 100K jobs/sec | 300K jobs/sec | 3배 |
| Job 처리량 (lock-free) | N/A | 1M jobs/sec | 10배 |
| Tail 지연 시간 (P99) | 10ms | 1ms | 10배 |
| Lock 경합 | 높음 | 최소화 | 90% 감소 |
| CPU 효율성 | 60% | 85% | 40% 개선 |

---

## 참고 자료

- [Lock-Free Programming](https://preshing.com/20120612/an-introduction-to-lock-free-programming/)
- [Dmitry Vyukov's MPMC Queue](http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue)
- [False Sharing](https://mechanical-sympathy.blogspot.com/2011/07/false-sharing.html)
- [Work Stealing](https://en.wikipedia.org/wiki/Work_stealing)
- [C++ Concurrency in Action](https://www.manning.com/books/c-plus-plus-concurrency-in-action-second-edition)
