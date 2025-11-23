# THR-007: Automate Performance Regression Detection

**ID**: THR-007
**Category**: TEST
**Priority**: HIGH
**Status**: TODO
**Estimated Duration**: 3-4 days
**Dependencies**: THR-006 (optional)

---

## Summary

Implement automated performance regression detection in CI/CD. Alert on significant performance degradation between commits.

---

## Background

- **Current Performance** (Apple M1):
  - Production throughput: 1.16M tasks/sec
  - Typed pool: 1.24M tasks/sec
  - Lock-free queue: 71 μs/op
  - Task latency P50: 77 ns
- **Gap**: No automated regression detection
- **Reference**: `docs/BENCHMARKS.md`

---

## Acceptance Criteria

- [ ] Baseline metrics stored per platform
- [ ] CI runs benchmarks on every PR
- [ ] Statistical comparison against baseline
- [ ] Configurable regression threshold (default: 10%)
- [ ] Automatic alerts on regression
- [ ] Performance trend visualization
- [ ] Easy baseline update process

---

## Performance Metrics to Track

| Metric | Unit | Threshold |
|--------|------|-----------|
| Throughput | tasks/sec | -10% |
| Latency P50 | ns | +20% |
| Latency P99 | ns | +30% |
| Memory per task | bytes | +15% |
| Queue ops/sec | ops/sec | -10% |

---

## Implementation Tasks

### 1. Benchmark Runner
```cpp
// benchmarks/perf_runner.h
struct BenchmarkResult {
    std::string name;
    double value;
    std::string unit;
    std::chrono::system_clock::time_point timestamp;
    std::string commit_sha;
    std::string platform;
};

class PerfRunner {
public:
    std::vector<BenchmarkResult> run_all();
    void save_results(const std::string& path);
    static std::vector<BenchmarkResult> load_baseline(const std::string& path);
};
```

### 2. Regression Detection
```cpp
// benchmarks/regression_detector.h
struct RegressionReport {
    bool has_regression;
    std::vector<std::string> regressions;
    std::vector<std::string> improvements;
};

class RegressionDetector {
public:
    struct Threshold {
        double regression_percent = 10.0;  // Alert if worse by 10%
        double improvement_percent = 5.0;  // Note if better by 5%
        int min_samples = 3;               // Statistical significance
    };

    RegressionReport compare(
        const std::vector<BenchmarkResult>& current,
        const std::vector<BenchmarkResult>& baseline,
        const Threshold& threshold
    );
};
```

### 3. CI Integration
```yaml
# .github/workflows/performance-benchmarks.yml
name: Performance Regression Check

on:
  pull_request:
    branches: [main]

jobs:
  benchmark:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Build Release
        run: |
          cmake -B build -DCMAKE_BUILD_TYPE=Release
          cmake --build build --target benchmarks

      - name: Run Benchmarks
        run: ./build/bin/perf_runner --output=results.json

      - name: Download Baseline
        uses: actions/download-artifact@v4
        with:
          name: perf-baseline-${{ runner.os }}

      - name: Check Regression
        run: |
          python scripts/check_regression.py \
            --current=results.json \
            --baseline=baseline.json \
            --threshold=10

      - name: Comment PR
        if: failure()
        uses: actions/github-script@v7
        with:
          script: |
            github.rest.issues.createComment({
              issue_number: context.issue.number,
              body: '⚠️ Performance regression detected! See workflow for details.'
            })
```

### 4. Baseline Management
```bash
# scripts/update_baseline.sh
#!/bin/bash
# Run on main branch after performance improvements

./build/bin/perf_runner --output=baselines/${PLATFORM}.json

git add baselines/
git commit -m "perf: update baseline for ${PLATFORM}"
```

### 5. Trend Visualization
```python
# scripts/visualize_trends.py
import matplotlib.pyplot as plt
import json

def plot_metric_trend(metric_name, data_files):
    """Generate trend chart for a metric across commits"""
    # Load historical data
    # Plot with matplotlib
    # Save as artifact
```

---

## Baseline Storage

```
baselines/
├── ubuntu-latest.json
├── macos-latest.json
├── windows-latest.json
└── history/
    ├── 2025-11-01.json
    ├── 2025-11-08.json
    └── 2025-11-15.json
```

---

## Report Format

```json
{
  "summary": "2 regressions, 1 improvement",
  "status": "FAIL",
  "details": [
    {
      "metric": "throughput",
      "baseline": 1160000,
      "current": 1020000,
      "change": -12.1,
      "status": "REGRESSION"
    },
    {
      "metric": "latency_p50",
      "baseline": 77,
      "current": 65,
      "change": -15.6,
      "status": "IMPROVEMENT"
    }
  ]
}
```

---

## Files to Create/Modify

- `benchmarks/perf_runner.h` - Benchmark orchestration
- `benchmarks/regression_detector.h` - Comparison logic
- `scripts/check_regression.py` - CI script
- `scripts/update_baseline.sh` - Baseline management
- `.github/workflows/performance-benchmarks.yml` - Update
- `baselines/` - New directory for baselines

---

## Success Metrics

| Metric | Target |
|--------|--------|
| False positive rate | < 5% |
| Detection accuracy | > 95% |
| CI overhead | < 5 min |
| Baseline freshness | Weekly updates |
