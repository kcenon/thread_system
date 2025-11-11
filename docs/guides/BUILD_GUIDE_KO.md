# Platform-Specific Build Guide

> **Language:** [English](BUILD_GUIDE.md) | **한국어**

## 개요

본 가이드는 다양한 플랫폼과 컴파일러에서 Thread System을 빌드하기 위한 상세한 지침을 제공합니다.

## 목차

1. [사전 요구사항](#사전-요구사항)
2. [Linux 빌드 가이드](#linux-빌드-가이드)
3. [macOS 빌드 가이드](#macos-빌드-가이드)
4. [Windows 빌드 가이드](#windows-빌드-가이드)
5. [크로스 플랫폼 CMake 옵션](#크로스-플랫폼-cmake-옵션)
6. [문제 해결](#문제-해결)

## 사전 요구사항

### 모든 플랫폼

- CMake 3.16 이상
- C++20 지원 컴파일러
- vcpkg 패키지 매니저 (의존성 스크립트에 의해 자동 설치)
- Git (버전 관리용)

### 플랫폼별 요구사항

| Platform | Compiler | Minimum Version | Notes |
|----------|----------|-----------------|-------|
| Linux | GCC | 9.0+ | GCC 10+부터 완전한 C++20 지원 |
| Linux | Clang | 10.0+ | Clang 12+부터 완전한 C++20 지원 |
| macOS | Apple Clang | 12.0+ | Xcode 12+ 권장 |
| macOS | Homebrew GCC | 10.0+ | 선택적 대체 컴파일러 |
| Windows | MSVC | 2019 (16.8+) | Visual Studio 2019/2022 |
| Windows | MinGW-w64 | 10.0+ | MSYS2 환경 권장 |
| Windows | Clang | 12.0+ | Visual Studio 또는 LLVM을 통해 사용 |

## Linux 빌드 가이드

### Ubuntu/Debian

```bash
# 빌드 필수 도구 설치
sudo apt-get update
sudo apt-get install -y build-essential cmake git curl zip unzip tar

# 특정 컴파일러 설치 (선택사항)
# GCC 11의 경우:
sudo apt-get install -y gcc-11 g++-11
export CC=gcc-11
export CXX=g++-11

# Clang 14의 경우:
sudo apt-get install -y clang-14
export CC=clang-14
export CXX=clang++-14

# 의존성 설치
./dependency.sh

# 빌드
./build.sh --clean --release -j$(nproc)

# 또는 수동 빌드
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=g++-11
cmake --build build -j$(nproc)

# 테스트 실행
cd build && ctest --verbose
```

### Fedora/RHEL/CentOS

```bash
# 빌드 도구 설치
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake git

# 필요시 최신 GCC 설치
sudo dnf install gcc-toolset-11
scl enable gcc-toolset-11 bash

# 위와 동일한 빌드 절차
./dependency.sh
./build.sh --clean --release
```

### Arch Linux

```bash
# 빌드 도구 설치
sudo pacman -S base-devel cmake git

# 빌드
./dependency.sh
./build.sh --clean --release
```

## macOS 빌드 가이드

### Apple Clang 사용 (기본)

```bash
# Xcode Command Line Tools 설치
xcode-select --install

# Homebrew 설치 (설치되지 않은 경우)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# CMake 설치
brew install cmake

# 의존성 설치
./dependency.sh

# Apple Clang으로 빌드
./build.sh --clean --release -j$(sysctl -n hw.ncpu)

# 또는 수동 빌드
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build -j$(sysctl -n hw.ncpu)
```

### Homebrew GCC 사용

```bash
# GCC 설치
brew install gcc@12

# 컴파일러 설정
export CC=gcc-12
export CXX=g++-12

# 빌드
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=gcc-12 \
    -DCMAKE_CXX_COMPILER=g++-12
cmake --build build -j$(sysctl -n hw.ncpu)
```

### macOS 관련 참고사항

- CI 환경 제약으로 macOS에서는 테스트가 기본적으로 비활성화됨
- `CMAKE_OSX_DEPLOYMENT_TARGET`를 사용하여 최소 macOS 버전 설정
- libiconv는 문자 인코딩 지원을 위해 자동으로 포함됨

## Windows 빌드 가이드

### Visual Studio (MSVC)

```batch
:: C++ 워크로드가 포함된 Visual Studio 2019/2022 설치

:: Developer Command Prompt 사용
:: 의존성 설치
dependency.bat

:: Visual Studio로 빌드
build.bat --clean --release

:: 또는 수동 빌드
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release -j %NUMBER_OF_PROCESSORS%

:: 특정 툴셋 사용
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -T v142
```

### MinGW-w64 (MSYS2)

```bash
# https://www.msys2.org/에서 MSYS2 설치

# MSYS2 터미널에서 도구 설치
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make git

# PATH에 추가
export PATH=/mingw64/bin:$PATH

# 빌드
./dependency.sh
cmake -S . -B build \
    -G "MinGW Makefiles" \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Windows에서 Clang 사용

```batch
:: https://llvm.org/에서 LLVM 설치
:: 또는 Visual Studio의 Clang 사용

:: Clang으로 빌드
cmake -S . -B build -G "Visual Studio 17 2022" -T ClangCL
cmake --build build --config Release

:: 또는 ninja 사용
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++
cmake --build build
```

### Windows 관련 참고사항

- 경로에 슬래시(/) 또는 이스케이프된 백슬래시(\\) 사용
- libiconv는 Windows에서 제외됨 (필요하지 않음)
- DLL 빌드 시 `CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS` 사용 고려

## 크로스 플랫폼 CMake 옵션

### 빌드 구성

```bash
# Release 빌드 (최적화)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Debug 빌드 (디버그 심볼 포함)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# RelWithDebInfo (디버그 정보 포함 최적화)
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo

# MinSizeRel (크기 최적화)
cmake -S . -B build -DCMAKE_BUILD_TYPE=MinSizeRel
```

### 기능 플래그

```bash
# 서브모듈로 빌드 (샘플/테스트 제외)
cmake -S . -B build -DBUILD_THREADSYSTEM_AS_SUBMODULE=ON

# 문서 생성 활성화
cmake -S . -B build -DBUILD_DOCUMENTATION=ON

# Sanitizer 활성화 (Debug 빌드)
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_ASAN=ON \
    -DENABLE_UBSAN=ON

# 코드 커버리지 활성화
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_COVERAGE=ON

# 특정 C++ 기능 비활성화
cmake -S . -B build \
    -DDISABLE_STD_FORMAT=ON \
    -DDISABLE_STD_JTHREAD=ON
```

### 컴파일러 선택

```bash
# 컴파일러 명시적 지정
cmake -S . -B build \
    -DCMAKE_C_COMPILER=/usr/bin/gcc-11 \
    -DCMAKE_CXX_COMPILER=/usr/bin/g++-11

# 컴파일러 런처 사용 (ccache)
cmake -S . -B build \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

# 컴파일러 플래그 설정
cmake -S . -B build \
    -DCMAKE_CXX_FLAGS="-march=native -mtune=native"
```

## 문제 해결

### 일반적인 문제와 해결책

#### vcpkg 설치 실패

```bash
# vcpkg 정리 후 재시도
rm -rf vcpkg
./dependency.sh

# 또는 수동으로 vcpkg 설치
git clone https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=$PWD/vcpkg
```

#### CMake가 컴파일러를 찾을 수 없음

```bash
# 컴파일러 경로 명시적 설정
export CC=/full/path/to/gcc
export CXX=/full/path/to/g++

# 또는 CMake 변수 사용
cmake -S . -B build \
    -DCMAKE_C_COMPILER=/full/path/to/gcc \
    -DCMAKE_CXX_COMPILER=/full/path/to/g++
```

#### C++20 기능 사용 불가

```bash
# 컴파일러 버전 확인
g++ --version
clang++ --version

# C++ 표준 강제 지정
cmake -S . -B build -DCMAKE_CXX_STANDARD=20

# 지원되지 않는 기능 비활성화
cmake -S . -B build \
    -DDISABLE_STD_FORMAT=ON \
    -DDISABLE_STD_JTHREAD=ON
```

#### Windows에서 링크 에러

```batch
:: 정적 런타임 사용
cmake -S . -B build -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded

:: 또는 동적 런타임
cmake -S . -B build -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL
```

#### 테스트 빌드 실패

```bash
# 테스트 비활성화
cmake -S . -B build -DBUILD_TESTING=OFF

# 또는 Google Test 문제 해결
vcpkg install gtest
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
```

### 플랫폼별 환경 변수

```bash
# Linux/macOS
export MAKEFLAGS="-j$(nproc)"
export CMAKE_BUILD_PARALLEL_LEVEL=$(nproc)

# Windows
set CMAKE_BUILD_PARALLEL_LEVEL=%NUMBER_OF_PROCESSORS%
```

## 성능 팁

### 플랫폼별 최적화 플래그

#### Linux/macOS with GCC/Clang
```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native -flto"
```

#### Windows with MSVC
```batch
cmake -S . -B build -DCMAKE_CXX_FLAGS="/O2 /GL /arch:AVX2"
```

### Link-Time Optimization (LTO)
```bash
# LTO 활성화
cmake -S . -B build \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

### Debug vs Release 성능

| Build Type | 상대 성능 | 사용 용도 |
|------------|---------------------|----------|
| Debug | 1x (기준) | 개발, 디버깅 |
| RelWithDebInfo | 5-10x | 심볼이 포함된 프로파일링 |
| Release | 10-20x | 프로덕션 배포 |
| MinSizeRel | 8-15x | 임베디드 시스템 |

## CI/CD 통합

### GitHub Actions
```yaml
- name: Build Thread System
  run: |
    ./dependency.sh
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j
    cd build && ctest --verbose
```

### Docker Build
```dockerfile
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y \
    build-essential cmake git curl zip unzip tar
WORKDIR /app
COPY . .
RUN ./dependency.sh && ./build.sh --release
```

## 지원 매트릭스

| Platform | Architecture | Status | Notes |
|----------|-------------|--------|-------|
| Ubuntu 20.04+ | x64, ARM64 | ✅ 완전 지원 | 주요 개발 플랫폼 |
| macOS 11+ | x64, Apple Silicon | ✅ 완전 지원 | CI에서 테스트 비활성화 |
| Windows 10/11 | x64 | ✅ 완전 지원 | MSVC 권장 |
| Alpine Linux | x64 | ⚠️ 실험적 | musl libc 호환성 |
| FreeBSD | x64 | ⚠️ 실험적 | 유지보수 지원 |

## 추가 리소스

- [CMake Documentation](https://cmake.org/documentation/)
- [vcpkg Documentation](https://vcpkg.io/)
- [Compiler Support for C++20](https://en.cppreference.com/w/cpp/compiler_support)
- [Thread System README](../README.md)
- [API Reference](./API_REFERENCE.md)

## 변경 이력

- **2025-09-13**: 플랫폼 빌드 가이드 초기 작성
- **2025-09-13**: 문제 해결 섹션 추가
- **2025-09-13**: 성능 최적화 팁 추가

---

*Last Updated: 2025-10-20*
