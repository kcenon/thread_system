# Thread System에 기여하기

> **Language:** [English](CONTRIBUTING.md) | **한국어**

## 목차

- [시작하기](#시작하기)
  - [필수 요구사항](#필수-요구사항)
  - [개발 환경 설정](#개발-환경-설정)
- [기여 워크플로우](#기여-워크플로우)
  - [1. 기능 브랜치 생성](#1-기능-브랜치-생성)
  - [2. 변경 사항 작성](#2-변경-사항-작성)
  - [3. 변경 사항 커밋](#3-변경-사항-커밋)
  - [4. Pull Request 제출](#4-pull-request-제출)
- [코딩 표준](#코딩-표준)
  - [C++ 스타일 가이드라인](#c-스타일-가이드라인)
  - [오류 처리](#오류-처리)
- [테스트 가이드라인](#테스트-가이드라인)
  - [단위 테스트](#단위-테스트)
  - [성능 테스트](#성능-테스트)
- [문서화 가이드라인](#문서화-가이드라인)
  - [코드 문서화](#코드-문서화)
- [리뷰 프로세스](#리뷰-프로세스)
  - [Pull Request 요구사항](#pull-request-요구사항)
  - [리뷰 기준](#리뷰-기준)
- [도움 받기](#도움-받기)
  - [지원 채널](#지원-채널)
  - [응답 시간](#응답-시간)

Thread System 프로젝트에 기여해 주셔서 감사합니다! 이 문서는 기여자를 위한 가이드라인과 정보를 제공합니다.

## 시작하기

### 필수 요구사항

기여하기 전에 다음 사항을 확인하세요:
- **C++20 호환 컴파일러**: GCC 9+, Clang 10+, 또는 MSVC 2019+
- **CMake 3.16+**: 빌드 시스템 관리용
- **vcpkg**: 의존성을 위한 패키지 매니저
- **Git**: 버전 관리 시스템

### 개발 환경 설정

```bash
# 저장소 포크 및 클론
git clone https://github.com/YOUR_USERNAME/thread_system.git
cd thread_system

# 의존성 설치
./dependency.sh  # Linux/macOS
./dependency.bat # Windows

# 프로젝트 빌드
./build.sh       # Linux/macOS
./build.bat      # Windows

# 테스트 실행 (Linux만 해당)
cd build && ctest --verbose
```

## 기여 워크플로우

### 1. 기능 브랜치 생성

```bash
git checkout -b feature/your-feature-name
```

### 2. 변경 사항 작성

- 코딩 표준을 따르세요
- 새로운 기능에 대한 테스트를 추가하세요
- 필요에 따라 문서를 업데이트하세요
- 모든 기존 테스트가 통과하는지 확인하세요

### 3. 변경 사항 커밋

```bash
git commit -m "Add feature: Brief description of changes

- Detailed explanation of what was added/changed
- Why the change was necessary
- Any breaking changes or migration notes"
```

### 4. Pull Request 제출

- 자신의 포크로 푸시하세요
- GitHub를 통해 pull request를 생성하세요
- 변경 사항에 대한 명확한 설명을 제공하세요
- 관련 이슈를 연결하세요

## 코딩 표준

### C++ 스타일 가이드라인

```cpp
// 클래스: snake_case, 템플릿은 _t 접미사
class thread_pool;
template<typename T> class typed_job_queue_t;

// 함수 및 변수: snake_case
auto process_job() -> result_void;
std::atomic<bool> is_running_;

// 상수: SCREAMING_SNAKE_CASE
constexpr size_t DEFAULT_THREAD_COUNT = 4;

// 타입 추론을 위해 auto 선호
auto result = create_thread_pool();

// 복잡한 시그니처는 trailing return type 사용
auto process_batch(std::vector<job>&& jobs) -> result_void;

// 메모리 관리에 스마트 포인터 사용
std::unique_ptr<thread_base> worker;
std::shared_ptr<job_queue> queue;
```

### 오류 처리

```cpp
// 오류 처리를 위해 result<T> 패턴 사용
auto create_worker() -> result<std::unique_ptr<thread_worker>> {
    if (invalid_configuration) {
        return error{error_code::configuration_invalid, "Invalid worker configuration"};
    }
    return std::make_unique<thread_worker>(config);
}

// result를 적절히 확인
auto worker_result = create_worker();
if (worker_result.has_error()) {
    return worker_result.get_error();
}
```

## 테스트 가이드라인

### 단위 테스트

```cpp
#include <gtest/gtest.h>
#include "your_component.h"

class YourComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        component_ = std::make_unique<your_component>(test_config_);
    }

    std::unique_ptr<your_component> component_;
    configuration test_config_;
};

TEST_F(YourComponentTest, StartStopCycle) {
    ASSERT_TRUE(component_->start().has_value() == false);
    EXPECT_TRUE(component_->is_running());

    component_->stop();
    EXPECT_FALSE(component_->is_running());
}
```

### 성능 테스트

- 중요한 성능 경로를 벤치마크하세요
- 성능 퇴보가 없는지 확인하세요
- 현실적인 부하 조건에서 테스트하세요
- 메모리 사용량과 효율성을 측정하세요

## 문서화 가이드라인

### 코드 문서화

```cpp
/**
 * @brief 지정된 설정으로 새 thread pool을 생성합니다.
 *
 * @param config Thread pool을 위한 설정 매개변수
 * @return result<std::unique_ptr<thread_pool>> 생성된 thread pool 또는 오류
 *
 * @note Thread pool은 파괴되기 전에 중지되어야 합니다
 *
 * @example
 * @code
 * auto config = thread_pool_configuration{}.with_worker_count(4);
 * auto pool_result = create_thread_pool(config);
 * if (!pool_result.has_error()) {
 *     auto pool = std::move(pool_result.get_value());
 *     pool->start();
 * }
 * @endcode
 */
auto create_thread_pool(const thread_pool_configuration& config)
    -> result<std::unique_ptr<thread_pool>>;
```

## 리뷰 프로세스

### Pull Request 요구사항

제출하기 전에:
- [ ] 모든 테스트가 로컬에서 통과함
- [ ] 코드가 확립된 스타일 가이드라인을 따름
- [ ] 새로운 기능에 적절한 테스트가 포함됨
- [ ] 공개 API 변경에 대해 문서가 업데이트됨
- [ ] 성능 영향이 측정되고 허용 가능함

### 리뷰 기준

리뷰어는 다음 사항을 평가합니다:
1. **정확성**: 코드가 의도한 문제를 해결하는가?
2. **성능**: 성능 퇴보가 있는가?
3. **유지보수성**: 코드가 읽기 쉽고 잘 구조화되어 있는가?
4. **Thread 안전성**: 동시 액세스 패턴이 안전한가?
5. **API 설계**: 공개 인터페이스가 직관적이고 일관성이 있는가?

## 도움 받기

### 지원 채널

- **GitHub Issues**: 버그 리포트 및 기능 요청용
- **GitHub Discussions**: 질문 및 일반 토론용
- **Code Review**: 개발 중 구현 가이드용

### 응답 시간

- **버그 리포트**: 24-48시간
- **기능 요청**: 3-5 영업일
- **보안 이슈**: 12-24시간

---

Thread System에 기여해 주셔서 감사합니다!

---

*Last Updated: 2025-10-20*
