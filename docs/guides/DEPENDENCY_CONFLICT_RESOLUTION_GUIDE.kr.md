# Dependency Conflict Resolution Guide

> **Language:** [English](DEPENDENCY_CONFLICT_RESOLUTION_GUIDE.md) | **한국어**

## 개요

본 가이드는 개발자가 thread_system 프로젝트의 의존성 충돌을 식별하고, 이해하며, 해결하는 데 도움을 줍니다. 일반적인 문제에 대한 단계별 솔루션과 예방 조치를 제공합니다.

## 목차

1. [일반적인 충돌 유형](#일반적인-충돌-유형)
2. [탐지 방법](#탐지-방법)
3. [해결 전략](#해결-전략)
4. [예방 가이드라인](#예방-가이드라인)
5. [긴급 절차](#긴급-절차)
6. [문제 해결 도구](#문제-해결-도구)

---

## 일반적인 충돌 유형

### 1. 버전 충돌

**증상:**
- "version not found" 에러로 빌드 실패
- 호환되지 않는 API 사용 경고
- ABI 비호환성으로 인한 런타임 크래시

**일반적인 시나리오:**
```
❌ fmt 9.1.0 + spdlog 1.12.0 → spdlog는 fmt 10+ 필요
❌ gtest 1.11.0 + benchmark 1.8.0 → 호환되지 않는 C++20 기능
❌ vcpkg 구버전 → 레지스트리에서 패키지를 찾을 수 없음
```

### 2. 플랫폼별 충돌

**증상:**
- "Package not available for platform" 에러
- Windows vs Linux/macOS에서 다른 동작
- 시스템 라이브러리 누락

**일반적인 시나리오:**
```
❌ Windows에서 libiconv → 필요하지 않음, 충돌 발생
❌ Windows에서 pthread → std::thread 사용
❌ 다른 컴파일러 버전 → ABI 비호환성
```

### 3. 기능 충돌

**증상:**
- 선택적 기능 누락
- 충돌하는 기능 플래그
- CMake 구성 경고

**일반적인 시나리오:**
```
❌ gmock 기능 없는 gtest → 모킹 기능 누락
❌ spdlog header-only vs compiled → 링킹 충돌
❌ 다른 표준 버전의 fmt → API 불일치
```

### 4. 전이 의존성 충돌

**증상:**
- 간접 의존성 버전 불일치
- 다이아몬드 의존성 문제
- 복잡한 에러 메시지

**일반적인 시나리오:**
```
thread-system → fmt 10.2.1
thread-system → spdlog 1.12.0 → fmt 10.1.0
결과: 직접 의존성과 전이 의존성 간 fmt 버전 충돌
```

---

## 탐지 방법

### 1. 자동 탐지

#### CMake Dependency Checker
```bash
# CMake에서 의존성 검사 활성화
cmake -B build -DCHECK_DEPENDENCIES=ON

# 빌드가 상세한 충돌 정보와 함께 실패함
```

#### Dependency Analyzer Tool
```bash
# 의존성 트리 분석
./scripts/dependency_analyzer.py --visualize

# HTML 리포트 생성
./scripts/dependency_analyzer.py --html

# 보안 스캔
./scripts/dependency_analyzer.py --security-scan
```

### 2. 수동 탐지

#### vcpkg.json 일관성 확인
```bash
# JSON 구문 검증
python3 -m json.tool vcpkg.json

# 버전 제약 누락 확인
grep -E '"name".*:' vcpkg.json | grep -v version
```

#### 빌드 로그 분석
```bash
# 일반적인 충돌 지표 찾기
cmake --build build 2>&1 | grep -E "(version|conflict|incompatible)"

# 누락된 패키지 확인
cmake --build build 2>&1 | grep -E "(not found|could not find)"
```

---

## 해결 전략

### 1. 버전 제약 해결

#### 1단계: 충돌하는 버전 식별
```bash
# dependency analyzer 사용
./scripts/dependency_analyzer.py --project-root .

# 호환성 매트릭스 확인
cat docs/dependency_compatibility_matrix.md
```

#### 2단계: vcpkg.json 업데이트
```json
{
  "dependencies": [
    {
      "name": "fmt",
      "version>=": "10.2.0"
    },
    {
      "name": "spdlog",
      "version>=": "1.12.0"
    }
  ],
  "overrides": [
    {
      "name": "fmt",
      "version": "10.2.1"
    }
  ]
}
```

#### 3단계: 해결 검증
```bash
# 정리 및 재빌드
rm -rf build vcpkg_installed
cmake -B build -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg.cmake
cmake --build build
```

### 2. 플랫폼별 해결

#### 크로스 플랫폼 의존성
```json
{
  "dependencies": [
    {
      "name": "libiconv",
      "platform": "!windows"
    },
    {
      "name": "pthread",
      "platform": "linux"
    }
  ]
}
```

#### 조건부 CMake 로직
```cmake
if(WIN32)
    # Windows 전용 의존성
    find_package(WindowsSDK REQUIRED)
elseif(UNIX AND NOT APPLE)
    # Linux 전용 의존성
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ICONV REQUIRED iconv)
endif()
```

### 3. 기능 플래그 해결

#### 필요한 기능 활성화
```json
{
  "dependencies": [
    {
      "name": "gtest",
      "version>=": "1.14.0",
      "features": ["gmock"]
    }
  ]
}
```

#### CMake 기능 탐지
```cmake
# 기능 사용 가능 여부 확인
if(TARGET GTest::gmock)
    message(STATUS "✅ GMock available")
    set(HAVE_GMOCK ON)
else()
    message(WARNING "⚠️ GMock not available - some tests will be disabled")
    set(HAVE_GMOCK OFF)
endif()
```

### 4. 전이 의존성 해결

#### 버전 오버라이드 사용
```json
{
  "overrides": [
    {
      "name": "fmt",
      "version": "10.2.1"
    }
  ]
}
```

#### 명시적 의존성 선언
```json
{
  "dependencies": [
    {
      "name": "fmt",
      "version>=": "10.2.0",
      "comment": "Explicit version to prevent transitive conflicts"
    }
  ]
}
```

---

## 예방 가이드라인

### 1. 사전 예방적 의존성 관리

#### 정기 업데이트
```bash
# 월별 의존성 확인
./scripts/upgrade_dependencies.sh --dry-run

# 보안 업데이트 (알림 시 즉시 실행)
./scripts/upgrade_dependencies.sh --security-only
```

#### 버전 고정 전략
- 안정성을 위해 `overrides`에서 메이저 버전 고정
- 보안 패치를 위해 `version>=` 사용
- 메이저 버전 업데이트 전 철저한 테스트

#### 문서 유지관리
- 각 변경사항과 함께 호환성 매트릭스 업데이트
- 알려진 문제 및 임시 해결책 문서화
- 의존성 업데이트의 변경 로그 유지

### 2. 개발 모범 사례

#### Pre-commit 검사
```bash
# .pre-commit-hooks.yaml에 추가
- repo: local
  hooks:
  - id: dependency-check
    name: Check dependencies
    entry: cmake -B build_check -DCHECK_DEPENDENCIES=ON
    language: system
    pass_filenames: false
```

#### 브랜치 보호
- CI/CD에서 의존성 검증 요구
- 해결되지 않은 충돌 시 병합 차단
- 자동화된 보안 스캔

#### 팀 가이드라인
- 항상 깨끗한 vcpkg_installed 디렉토리로 테스트
- 수동 vcpkg 수정사항 문서화
- 의존성 업데이트에 기능 브랜치 사용

---

## 긴급 절차

### 1. 치명적 빌드 실패

#### 즉각 조치 (< 5분)
```bash
# 1. 마지막 정상 상태로 롤백
git checkout HEAD~1 -- vcpkg.json

# 2. vcpkg 캐시 정리
rm -rf vcpkg_installed build

# 3. 빠른 재빌드 테스트
cmake -B build && cmake --build build --target thread_base
```

#### 전체 복구 (< 30분)
```bash
# 1. 의존성 롤백 스크립트 사용
./scripts/upgrade_dependencies.sh --rollback $(date -d yesterday +%Y%m%d_120000)

# 2. 핵심 기능 검증
cmake --build build --target thread_pool interfaces
./build/bin/thread_base_unit --gtest_brief
```

### 2. 배포일 문제

#### 배포 전 체크리스트
- [ ] 모든 의존성 검사 통과
- [ ] 보안 스캔 클리어
- [ ] 호환성 매트릭스 업데이트
- [ ] 백업 생성

#### 긴급 롤백 계획
```bash
# 1. 즉각 롤백 명령
./scripts/upgrade_dependencies.sh --rollback YYYYMMDD_HHMMSS

# 2. 팀에 알림
echo "Dependency rollback completed due to: [REASON]" | \
mail -s "Emergency Rollback - thread_system" team@company.com

# 3. 사고 보고서 작성
./scripts/dependency_analyzer.py --html --output-dir incident_$(date +%s)
```

---

## 문제 해결 도구

### 1. 내장 도구

#### CMake Dependency Checker
```bash
# 상세 의존성 검사 활성화
cmake -B build -DCHECK_DEPENDENCIES=ON -DCMAKE_VERBOSE_MAKEFILE=ON
```

#### Dependency Analyzer
```bash
# 모든 옵션을 사용한 전체 분석
./scripts/dependency_analyzer.py \
  --visualize \
  --html \
  --security-scan \
  --output-dir debug_$(date +%s)
```

### 2. 외부 도구

#### vcpkg Tools
```bash
# 설치된 패키지 목록
vcpkg list

# 패키지 상세 정보 표시
vcpkg show fmt

# 의존성 그래프
vcpkg depend-info fmt --recurse
```

#### 시스템 패키지 매니저
```bash
# macOS - 시스템 충돌 확인
brew doctor
brew deps fmt

# Ubuntu - 시스템 패키지 확인
apt list --installed | grep -E "(fmt|spdlog|gtest)"
dpkg -l | grep -E "(fmt|spdlog|gtest)"
```

### 3. 디버그 명령

#### CMake Debug Output
```bash
# 디버그 출력 활성화
cmake -B build --debug-output

# 모든 변수 표시
cmake -B build -LAH | grep -E "(FMT|SPDLOG|GTEST)"

# 의존성 해결 추적
cmake -B build --trace
```

#### 컴파일 디버그
```bash
# 상세 컴파일
make VERBOSE=1 -C build

# 전처리기 출력 (포함된 헤더 확인)
g++ -E src/main.cpp | grep -E "(fmt|spdlog)"
```

---

## 빠른 참조

### 일반적인 해결 명령

```bash
# 1. 깨끗한 상태에서 재빌드
rm -rf build vcpkg_installed && cmake -B build

# 2. 의존성 업그레이드 (안전)
./scripts/upgrade_dependencies.sh --security-only

# 3. 최신 버전 강제 (위험)
./scripts/upgrade_dependencies.sh --latest --force

# 4. 현재 상태 분석
./scripts/dependency_analyzer.py --html

# 5. 긴급 롤백
./scripts/upgrade_dependencies.sh --rollback 20251201_143022
```

### 긴급 연락처

- **빌드 시스템 문제**: DevOps Team
- **보안 취약점**: Security Team
- **의존성 질문**: Senior Developer
- **치명적 프로덕션 문제**: Incident Response Team

---

## 부록

### A. 알려진 문제 데이터베이스

| Issue | Symptoms | Solution | Status |
|-------|----------|----------|--------|
| fmt 9.x + spdlog 1.12+ | 링크 에러 | fmt를 10+로 업그레이드 | 해결됨 |
| gtest missing gmock | 테스트 실패 | "gmock" 기능 추가 | 해결됨 |
| Windows libiconv | 빌드 실패 | 플랫폼 제외 사용 | 해결됨 |

### B. 호환성 매트릭스 빠른 참조

| fmt | spdlog | gtest | benchmark | Status |
|-----|--------|-------|-----------|---------|
| 10.2.1 | 1.12.0 | 1.14.0 | 1.8.0 | ✅ 권장 |
| 10.1.1 | 1.12.0 | 1.14.0 | 1.8.0 | ✅ 지원됨 |
| 9.1.0 | 1.12.0 | - | - | ❌ 비호환 |

### C. 업데이트 체크리스트 템플릿

```markdown
## Dependency Update Checklist

- [ ] 백업 생성: `backup_YYYYMMDD_HHMMSS`
- [ ] 호환성 매트릭스 확인
- [ ] 보안 스캔 통과
- [ ] 로컬 빌드 성공
- [ ] 유닛 테스트 통과
- [ ] 통합 테스트 통과
- [ ] 문서 업데이트
- [ ] 팀 알림 전송
- [ ] 롤백 계획 확인
```

---

**Last Updated**: 2025-09-13
**Version**: 0.1.0.0
**Maintainer**: DevOps Team
