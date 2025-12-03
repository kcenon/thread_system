# Dependency Version Compatibility Matrix

> **Language:** [English](DEPENDENCY_COMPATIBILITY_MATRIX.md) | **한국어**

## 핵심 의존성

| Package | Minimum Version | Tested Versions | Compatibility Notes |
|---------|----------------|-----------------|-------------------|
| libiconv | latest | system default | 플랫폼 의존성 (Windows에서는 제외) |

> **참고**: 이 릴리스부터 thread_system은 C++20 `std::format`을 독점적으로 사용합니다. fmt 라이브러리는 더 이상 필요하지 않습니다. 최소 컴파일러 요구사항: GCC 13+, Clang 14+, MSVC 19.29+.

## 테스팅 의존성 (Feature: testing)

| Package | Minimum Version | Tested Versions | Compatibility Notes |
|---------|----------------|-----------------|-------------------|
| gtest | 1.14.0 | 1.14.0, 1.15.0 | GMock이 포함된 Google Test 프레임워크. C++20을 위해 버전 1.14+ 필요 |
| benchmark | 1.8.0 | 1.8.0, 1.8.3 | Google Benchmark 라이브러리. 최신 CMake 지원을 위해 버전 1.8+ 필요 |

## 로깅 의존성 (Feature: logging)

| Package | Minimum Version | Tested Versions | Compatibility Notes |
|---------|----------------|-----------------|-------------------|
| spdlog | 1.12.0 | 1.12.0, 1.13.0, 1.14.0 | 빠른 C++ 로깅 라이브러리 (선택적 외부 로깅). 버전 1.12+ 권장 |

## 버전 호환성 규칙

### C++20 std::format (내장)
- **컴파일러 요구사항**: GCC 13+, Clang 14+, MSVC 19.29+
- **참고**: 프로젝트는 이제 C++20 `std::format`을 독점적으로 사용합니다. 외부 포매팅 라이브러리가 필요하지 않습니다.

### Testing Framework
- **gtest 1.14.0+**: C++20 호환성과 최신 CMake 통합을 위해 필요
- **benchmark 1.8.0+**: 최소 오버헤드의 성능 테스트를 위해 필요
- **호환성**: 두 패키지 모두 충돌 없이 함께 작동

### Logging Framework
- **spdlog 1.12.0+**: 외부 로깅에 권장됨 (선택적)
- **Header-only Mode**: 최소 의존성을 위해 선호됨
- **성능**: 비동기 로깅 지원 검증됨

## 플랫폼별 고려사항

### Windows (MSVC, MinGW)
- `libiconv` 제외됨 (필요하지 않음)
- 다른 모든 의존성 지원됨
- Visual Studio 2019+ 권장

### Linux (GCC, Clang)
- 모든 의존성 지원됨
- `libiconv` 문자 인코딩 지원을 위해 포함됨
- GCC 10+ 또는 Clang 12+ 권장

### macOS (Clang)
- 모든 의존성 지원됨
- `libiconv`는 시스템 버전을 사용할 수 있음
- Xcode 12+ 권장

## 충돌 해결

### 일반적인 문제
1. **C++20 컴파일러 지원**: 컴파일러가 `std::format`을 지원하는지 확인 (GCC 13+, Clang 14+, MSVC 19.29+)
2. **gtest/benchmark 충돌**: 두 패키지 모두 동일한 C++ 표준 사용 확인

### 해결 전략
1. 버전 호환성 매트릭스 확인
2. 중요한 의존성에 대해 vcpkg 오버라이드 사용
3. 통합 테스팅을 통한 검증
4. 커스텀 패치 또는 임시 해결책 문서화

## 보안 고려사항

### 의존성 스캔
- 정기적인 취약점 스캔 예약
- 중요 업데이트는 48시간 내 적용
- 비중요 업데이트는 월별 평가

### 공급망 보안
- 모든 의존성은 공식 vcpkg 레지스트리에서 제공
- 서명 검증 활성화
- 중요 의존성에 대한 소스 코드 감사

## 업데이트 정책

### 자동 업데이트
- 패치 버전: 자동 (동일한 마이너 버전 내)
- 마이너 버전: 수동 검토 필요
- 메이저 버전: 완전한 호환성 테스트 필요

### 테스팅 요구사항
- 유닛 테스트: 100% 통과율 필요
- 통합 테스트: 전체 스위트 실행
- 성능 테스트: 5%를 초과하는 성능 저하 금지
- 보안 테스트: 취약점 스캔 클리어

## 최종 업데이트
**Date**: 2025-09-13
**Reviewer**: Backend Developer
**Next Review**: 2025-10-13

---

*Last Updated: 2025-10-20*
