# License Compatibility Analysis

> **Language:** [English](LICENSE_COMPATIBILITY.md) | **한국어**

## 프로젝트 라이선스
**thread-system**은 **MIT License** 하에 라이선스되며, 이는 허용적이고 대부분의 오픈 소스 라이선스와 호환됩니다.

## 의존성 라이선스 분석

### 핵심 의존성

| Package | License | Compatibility | Notes |
|---------|---------|---------------|-------|
| libiconv | LGPL-2.1+ | ✅ 호환 | LGPL은 카피레프트 의무 없이 동적 링킹 허용 |

> **참고**: 이 릴리스부터 thread_system은 C++20 `std::format`을 독점적으로 사용합니다. 외부 포매팅 라이브러리 의존성이 없습니다.

### 테스팅 의존성

| Package | License | Compatibility | Notes |
|---------|---------|---------------|-------|
| gtest | BSD-3-Clause | ✅ 호환 | BSD는 MIT와 호환됨 |
| benchmark | Apache-2.0 | ✅ 호환 | Apache 2.0은 MIT와 호환됨 |

### 로깅 의존성

| Package | License | Compatibility | Notes |
|---------|---------|---------------|-------|
| spdlog | MIT | ✅ 호환 | 동일한 라이선스, 제한 없음 |

## 호환성 매트릭스

### MIT License (우리 프로젝트)
- **호환**: MIT, BSD, Apache 2.0, LGPL (동적 링킹)
- **비호환**: GPL (정적 링킹), AGPL, Proprietary
- **참고**: MIT는 가장 허용적인 라이선스 중 하나

### 라이선스 의무 요약

| License | Attribution Required | Share Alike | Patent Grant | Commercial Use |
|---------|---------------------|-------------|--------------|----------------|
| MIT (our project) | ✅ | ❌ | ❌ | ✅ |
| MIT (spdlog - 선택적) | ✅ | ❌ | ❌ | ✅ |
| BSD-3-Clause (gtest) | ✅ | ❌ | ❌ | ✅ |
| Apache-2.0 (benchmark) | ✅ | ❌ | ✅ | ✅ |
| LGPL-2.1+ (libiconv) | ✅ | ⚠️ (동적만) | ❌ | ✅ |

## 준수 요구사항

### 귀속 요구사항
모든 의존성은 배포되는 소프트웨어에 귀속을 요구합니다:

```
This software uses the following open source packages:

- gtest (BSD-3-Clause) - Copyright 2008, Google Inc.
- benchmark (Apache 2.0) - Copyright 2015 Google Inc.
- spdlog (MIT License, optional) - Copyright (c) 2016 Gabi Melman
- libiconv (LGPL-2.1+) - Copyright (C) 1999-2003 Free Software Foundation, Inc.
```

### 배포 요구사항

#### 바이너리 배포
- 모든 의존성에 대한 라이선스 고지 포함
- LGPL 컴포넌트(libiconv)에 대한 소스 코드 접근 제공
- MIT/BSD/Apache 컴포넌트에 대한 추가 의무 없음

#### 소스 배포
- 원본 라이선스 파일 포함
- 저작권 고지 유지
- LGPL 컴포넌트에 대한 수정 사항 문서화

## 위험 평가

### 낮은 위험 ✅
- **MIT 의존성** (spdlog - 선택적): 동일한 라이선스, 충돌 없음
- **BSD 의존성** (gtest): 매우 허용적, 널리 호환됨
- **Apache 2.0 의존성** (benchmark): 특허 보호, 상업적으로 친화적

### 중간 위험 ⚠️
- **LGPL 의존성** (libiconv): 카피레프트를 피하려면 동적 링킹 필요
  - **완화**: 공유 라이브러리로 사용, 정적 링크 금지
  - **대안**: 플랫폼 네이티브 문자 변환 API 사용 고려

### 권장사항

1. **현재 설정**: 모든 의존성이 MIT 라이선스와 호환됨
2. **동적 링킹**: libiconv가 동적으로 링크되도록 보장
3. **귀속**: 포괄적인 라이선스 고지 포함
4. **문서화**: 이 호환성 분석 유지

## 법적 검토

### 검토 상태
- ✅ **초기 검토**: 2025-09-13 완료
- ✅ **라이선스 스캔**: 모든 의존성 스캔됨
- ✅ **호환성 확인**: 충돌 식별되지 않음
- ⏳ **법률 자문 검토**: 상업적 배포를 위해 권장됨

### 실행 항목
- [ ] 빌드 시스템에서 libiconv 동적 링킹 확인
- [ ] 포괄적인 귀속 문서 작성
- [ ] 라이선스 스캔 자동화 구축
- [ ] 분기별 라이선스 호환성 검토 일정 수립

## 대체 라이선스 전략

### 이중 라이선스 옵션
GPL 호환성이 필요한 경우:
- MIT/GPL 하의 이중 라이선스 고려
- GPL 프로젝트와의 통합 허용
- MIT 하에서 상업적 유연성 유지

### 의존성 대안
더 엄격한 라이선스 요구사항의 경우:
- **libiconv**: 플랫폼 네이티브 API 또는 MIT 라이선스 대안 사용
- **benchmark**: Celero 같은 MIT 라이선스 대안 고려
- **현재 설정 권장**: 모든 현재 의존성이 비즈니스 친화적

## 연락처 정보
**법적 질문**: 프로젝트 관리자에게 연락
**라이선스 준수 담당자**: Backend Developer
**마지막 검토 날짜**: 2025-09-13
**다음 검토 날짜**: 2025-12-13

---

*Last Updated: 2025-10-20*
