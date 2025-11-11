# CI/CD Performance Metrics Integration

## Overview

This document describes the automated performance metrics system integrated into the thread_system CI/CD pipeline. The system automatically measures, tracks, and reports performance metrics for every commit and pull request.

## Architecture

### Components

1. **Performance Benchmarks Workflow** (`.github/workflows/performance-benchmarks.yml`)
   - Triggered on: push, pull_request, schedule (daily), manual dispatch
   - Runs on: Ubuntu 22.04
   - Build: Release mode with optimizations (-O3)

2. **Scripts**
   - `generate_performance_report.py`: Analyzes benchmark results and generates Markdown reports
   - `check_performance_regression.py`: Detects performance regressions (>5% threshold)
   - `update_performance_badge.py`: Updates shields.io badge data
   - `update_readme_performance.py`: Updates README.md with latest metrics
   - `generate_performance_dashboard.py`: Creates interactive HTML dashboard

3. **Workflows**
   - `performance-benchmarks.yml`: Main benchmark execution
   - `update-readme-performance.yml`: README auto-update after successful benchmarks

## Features

### Automated Benchmark Execution

- Runs on every push and pull request
- Daily scheduled runs for trend tracking
- Collects system information (CPU, memory, OS, compiler)
- Executes all available benchmarks with 3 repetitions
- Aggregates results for consistency

### Performance Regression Detection

- Compares current results against baseline
- 5% threshold for regression alerts
- Automatic PR comments with detected regressions
- Highlights both regressions and improvements

### README Auto-Update

The README.md is automatically updated with the latest CI metrics:

```markdown
<!-- PERFORMANCE_METRICS_START -->

**Latest CI Performance (Ubuntu 22.04, Release build)**

*Last updated: 2024-11-04*

| Metric | Value | Notes |
|--------|-------|-------|
| **Throughput** | 1.24M jobs/s | Jobs processed per second |
| **Latency (P50)** | 0.8 μs | Task submission latency |
| **Platform** | Ubuntu 22.04 | CI environment |
| **Build** | Release (-O3) | Optimized build |

<!-- PERFORMANCE_METRICS_END -->
```

### Performance Dashboard

Interactive HTML dashboard published to GitHub Pages:
- Real-time performance metrics
- Historical trend charts
- Throughput and latency tracking
- Performance comparison graphs

**URL**: `https://kcenon.github.io/thread_system/performance/`

## Usage

### Viewing Performance Metrics

1. **In README.md**: Check the "Performance Benchmarks" section
2. **In Pull Requests**: Automated comment with benchmark results
3. **Dashboard**: Visit the GitHub Pages dashboard
4. **Artifacts**: Download from GitHub Actions artifacts

### Triggering Benchmarks

**Automatic**:
- Every push to main or phase-* branches
- Every pull request
- Daily at midnight UTC

**Manual**:
```bash
# Via GitHub UI: Actions -> Performance Benchmarks -> Run workflow
# Or using GitHub CLI:
gh workflow run performance-benchmarks.yml
```

### Understanding Results

**Performance Report**:
- System information section
- Key metrics comparison (current vs baseline)
- Detailed benchmark results
- Performance recommendations

**Regression Report**:
- ⚠️ Regressions (>5% slower)
- ✅ Improvements (>5% faster)
- Comparison with baseline

## Configuration

### Baseline Reference

The `baseline-reference.json` file stores the reference performance metrics:
- Updated automatically on main branch
- Used for regression detection
- Committed with `[skip ci]` tag

### Thresholds

Current regression detection threshold: **5%**

To modify, edit the workflow:
```yaml
--threshold 5.0
```

### Benchmark Repetitions

Current setting: **3 repetitions**

To modify, edit the workflow:
```yaml
--benchmark_repetitions=3
```

## Maintenance

### Adding New Benchmarks

1. Create benchmark executable in `benchmarks/` directory
2. Ensure it outputs JSON format (`--benchmark_format=json`)
3. It will be automatically discovered and executed

### Updating Scripts

All scripts are in the `scripts/` directory:
- Make changes to the Python scripts
- Test locally before committing
- Scripts are automatically used by workflows

### Troubleshooting

**No benchmark results**:
- Check if benchmarks are built (`cmake --build build`)
- Verify benchmark executables exist in `build/bin/`
- Check workflow logs for errors

**README not updating**:
- Verify markers exist in README.md
- Check `update-readme-performance.yml` workflow logs
- Ensure main branch triggers workflow_run

**Dashboard not showing**:
- Verify GitHub Pages is enabled
- Check `peaceiris/actions-gh-pages` action logs
- Ensure `gh-pages` branch exists

## Performance Metrics

### Key Metrics Tracked

1. **Throughput**: Jobs processed per second
2. **Latency P50**: Median task submission latency
3. **Scaling Efficiency**: Multi-core scaling performance
4. **Memory Usage**: Baseline memory footprint

### Benchmark Types

- **baseline_metrics**: Core performance benchmarks
- **throughput_detailed_benchmark**: Detailed throughput analysis
- **scalability_benchmark**: Worker scaling tests
- **contention_benchmark**: High-contention scenarios
- **memory_benchmark**: Memory usage patterns

## Integration with Development Workflow

### Pull Request Flow

1. Developer creates PR
2. CI runs performance benchmarks
3. Bot comments with results:
   - Performance report
   - Regression detection
   - Comparison with baseline
4. Reviewer checks for regressions
5. Merge if acceptable

### Main Branch Flow

1. PR merged to main
2. Benchmark workflow runs
3. Results published to:
   - GitHub Pages dashboard
   - README.md (auto-update)
   - Baseline reference file
4. Badge updated

## Best Practices

### For Developers

1. **Check PR comments**: Review performance impact before merge
2. **Investigate regressions**: >5% regression requires explanation
3. **Celebrate improvements**: Document significant performance gains
4. **Use dashboard**: Track long-term trends

### For Maintainers

1. **Monitor dashboard**: Regular performance health checks
2. **Update baselines**: When architecture changes significantly
3. **Review thresholds**: Adjust if too sensitive or permissive
4. **Archive artifacts**: Keep historical data for analysis

## Future Enhancements

Potential improvements:

1. **Multi-platform benchmarks**: macOS, Windows results
2. **Compiler comparison**: GCC vs Clang performance
3. **Historical analysis**: Long-term trend detection
4. **Performance budgets**: Per-feature performance limits
5. **Automated optimization**: Suggest performance improvements

## References

- [BASELINE.md](../BASELINE.md): Performance baseline specification
- [Performance Benchmarks Workflow](../.github/workflows/performance-benchmarks.yml)
- [GitHub Pages Dashboard](https://kcenon.github.io/thread_system/performance/)

## Support

For issues or questions:
- Create GitHub issue with label `performance`
- Tag @kcenon for critical performance regressions
- Check workflow logs for detailed error messages

---

*Last updated: 2024-11-04*
