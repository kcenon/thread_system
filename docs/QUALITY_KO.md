# 품질 및 QA 가이드

> **Language:** [English](QUALITY.md) | **한국어**

## 커버리지

커버리지를 활성화하여 빌드하고 보고서를 생성하세요:

```bash
cmake -S . -B build -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
cmake --build build --target coverage
```

출력물은 `build/coverage/` 디렉토리에 생성됩니다.

참고 사항
- `lcov`와 `genhtml`이 필요합니다.
- macOS의 경우, Homebrew를 통해 설치하세요: `brew install lcov`.

## Sanitizer

CMake 옵션을 통해 sanitizer를 활성화하세요:

```bash
cmake -S . -B build -DENABLE_ASAN=ON -DENABLE_UBSAN=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

데이터 경합 검사를 위해:

```bash
cmake -S . -B build-tsan -DENABLE_TSAN=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-tsan -j
ctest --test-dir build-tsan --output-on-failure
```

## 벤치마크

벤치마크를 빌드하세요:

```bash
cmake -S . -B build -DBUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --target benchmarks
cmake --build build --target run_all_benchmarks
```

JSON 출력을 사용하여 결과를 비교하세요 (Google Benchmark):

```bash
./bin/thread_pool_benchmark --benchmark_format=json --benchmark_out=baseline.json
./bin/thread_pool_benchmark --benchmark_format=json --benchmark_out=current.json
python3 scripts/benchmark_compare.py baseline.json current.json
```

## 정적 분석

```bash
cmake -S . -B build -DENABLE_CLANG_TIDY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```
