#!/usr/bin/env python3
"""
README.md의 성능 지표 섹션을 자동으로 업데이트합니다.
"""

import json
import argparse
import re
from pathlib import Path
from datetime import datetime

def load_benchmark_results(directory: str) -> dict:
    """벤치마크 결과를 로드합니다."""
    baseline_file = Path(directory) / 'baseline.json'
    if baseline_file.exists():
        with open(baseline_file, 'r') as f:
            return json.load(f)
    return {}

def extract_metrics(data: dict) -> dict:
    """벤치마크 데이터에서 핵심 지표를 추출합니다."""
    metrics = {
        'throughput': 0,
        'latency_p50': 0,
        'workers': 0,
        'date': datetime.utcnow().strftime('%Y-%m-%d')
    }

    for bench in data.get('benchmarks', []):
        name = bench.get('name', '')

        if 'Throughput' in name:
            metrics['throughput'] = bench.get('items_per_second', 0)

        if 'TaskSubmission' in name:
            cpu_time = bench.get('cpu_time', 0)  # nanoseconds
            metrics['latency_p50'] = cpu_time / 1000.0  # microseconds

    return metrics

def format_throughput(value: float) -> str:
    """Throughput 포맷"""
    if value >= 1_000_000:
        return f"{value/1_000_000:.2f}M jobs/s"
    elif value >= 1_000:
        return f"{value/1_000:.2f}K jobs/s"
    else:
        return f"{value:.0f} jobs/s"

def update_readme(readme_path: str, metrics: dict) -> str:
    """README.md 파일의 성능 지표 섹션을 업데이트합니다."""

    with open(readme_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # 성능 지표 섹션을 찾아서 업데이트
    marker_start = "<!-- PERFORMANCE_METRICS_START -->"
    marker_end = "<!-- PERFORMANCE_METRICS_END -->"

    if marker_start in content and marker_end in content:
        # 새로운 성능 지표 테이블 생성
        new_metrics = f"""{marker_start}

**Latest CI Performance (Ubuntu 22.04, Release build)**

*Last updated: {metrics['date']}*

| Metric | Value | Notes |
|--------|-------|-------|
| **Throughput** | {format_throughput(metrics['throughput'])} | Jobs processed per second |
| **Latency (P50)** | {metrics['latency_p50']:.2f} μs | Task submission latency |
| **Platform** | Ubuntu 22.04 | CI environment |
| **Build** | Release (-O3) | Optimized build |

> 📊 These metrics are automatically measured in our CI pipeline. See [BASELINE.md](BASELINE.md) for detailed performance analysis.

{marker_end}"""

        # 기존 섹션 교체
        pattern = re.compile(f"{re.escape(marker_start)}.*?{re.escape(marker_end)}",
                           re.DOTALL)
        content = pattern.sub(new_metrics, content)
    else:
        # 마커가 없으면 경고
        print("Warning: Performance metrics markers not found in README.md")
        print(f"Please add {marker_start} and {marker_end} markers to enable auto-update")

    return content

def main():
    parser = argparse.ArgumentParser(description='Update README performance metrics')
    parser.add_argument('--benchmark-results', required=True,
                       help='Directory with benchmark results')
    parser.add_argument('--readme', required=True, help='Path to README.md')
    parser.add_argument('--output', required=True, help='Output README.md path')

    args = parser.parse_args()

    # 벤치마크 결과 로드
    data = load_benchmark_results(args.benchmark_results)

    if not data:
        print("No benchmark data found")
        return

    # 지표 추출
    metrics = extract_metrics(data)

    # README 업데이트
    updated_content = update_readme(args.readme, metrics)

    # 파일 저장
    with open(args.output, 'w', encoding='utf-8') as f:
        f.write(updated_content)

    print(f"README updated with latest performance metrics")
    print(f"  Throughput: {format_throughput(metrics['throughput'])}")
    print(f"  Latency: {metrics['latency_p50']:.2f} μs")

if __name__ == '__main__':
    main()
