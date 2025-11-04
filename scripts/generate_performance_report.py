#!/usr/bin/env python3
"""
성능 벤치마크 결과를 분석하고 Markdown 리포트를 생성합니다.
"""

import json
import argparse
import os
from pathlib import Path
from typing import Dict, List, Any
import statistics

def load_benchmark_results(directory: str) -> Dict[str, Any]:
    """벤치마크 결과 JSON 파일들을 로드합니다."""
    results = {}
    for file_path in Path(directory).glob("*.json"):
        with open(file_path, 'r') as f:
            data = json.load(f)
            results[file_path.stem] = data
    return results

def parse_baseline(baseline_file: str) -> Dict[str, float]:
    """BASELINE.md 파일에서 기준값을 파싱합니다."""
    baselines = {}
    with open(baseline_file, 'r') as f:
        content = f.read()
        import re

        # Throughput 파싱
        throughput_match = re.search(r'Throughput.*?(\d[\d,]+)\s*jobs/second', content)
        if throughput_match:
            baselines['throughput'] = float(throughput_match.group(1).replace(',', ''))

        # Latency 파싱
        latency_match = re.search(r'Latency.*?<(\d+(?:\.\d+)?)\s*μs', content)
        if latency_match:
            baselines['latency_us'] = float(latency_match.group(1))

        # Memory 파싱
        memory_match = re.search(r'Memory.*?(\d+(?:\.\d+)?)\s*MB\s*baseline', content)
        if memory_match:
            baselines['memory_mb'] = float(memory_match.group(1))

    return baselines

def extract_key_metrics(results: Dict[str, Any]) -> Dict[str, float]:
    """벤치마크 결과에서 핵심 지표를 추출합니다."""
    metrics = {}

    # baseline.json에서 핵심 지표 추출
    if 'baseline' in results:
        benchmarks = results['baseline'].get('benchmarks', [])

        for bench in benchmarks:
            name = bench.get('name', '')

            # Throughput 계산
            if 'Throughput' in name:
                items_per_second = bench.get('items_per_second', 0)
                metrics['throughput_jobs_per_sec'] = items_per_second

            # Latency 계산
            if 'TaskSubmission' in name:
                cpu_time = bench.get('cpu_time', 0)  # nanoseconds
                metrics['latency_us'] = cpu_time / 1000.0

    return metrics

def format_number(value: float, unit: str = '') -> str:
    """숫자를 읽기 쉬운 형태로 포맷합니다."""
    if value >= 1_000_000:
        return f"{value/1_000_000:.2f}M{unit}"
    elif value >= 1_000:
        return f"{value/1_000:.2f}K{unit}"
    else:
        return f"{value:.2f}{unit}"

def calculate_change(current: float, baseline: float) -> tuple:
    """변화율을 계산하고 방향을 결정합니다."""
    if baseline == 0:
        return 0.0, '='

    change_pct = ((current - baseline) / baseline) * 100
    if abs(change_pct) < 1.0:
        direction = '='
    elif change_pct > 0:
        direction = '⬆️'
    else:
        direction = '⬇️'

    return change_pct, direction

def generate_report(results: Dict[str, Any], baselines: Dict[str, float],
                   system_info: str) -> str:
    """성능 리포트를 Markdown 형식으로 생성합니다."""

    metrics = extract_key_metrics(results)

    report = []
    report.append("# Performance Benchmark Report\n")

    # 시스템 정보
    if system_info and os.path.exists(system_info):
        with open(system_info, 'r') as f:
            report.append(f.read())
            report.append("\n")

    # 핵심 지표 요약
    report.append("## Key Performance Metrics\n")
    report.append("| Metric | Current | Baseline | Change |\n")
    report.append("|--------|---------|----------|--------|\n")

    # Throughput
    if 'throughput_jobs_per_sec' in metrics and 'throughput' in baselines:
        current_tput = metrics['throughput_jobs_per_sec']
        baseline_tput = baselines['throughput']
        change, direction = calculate_change(current_tput, baseline_tput)

        report.append(f"| Throughput | {format_number(current_tput, ' jobs/s')} | "
                     f"{format_number(baseline_tput, ' jobs/s')} | "
                     f"{direction} {change:+.2f}% |\n")

    # Latency
    if 'latency_us' in metrics and 'latency_us' in baselines:
        current_lat = metrics['latency_us']
        baseline_lat = baselines['latency_us']
        # Latency는 낮을수록 좋으므로 방향 반대
        change, _ = calculate_change(current_lat, baseline_lat)
        if change < 0:
            direction = '⬆️'  # 성능 향상
        elif change > 0:
            direction = '⬇️'  # 성능 저하
        else:
            direction = '='

        report.append(f"| Latency (P50) | {current_lat:.2f} μs | "
                     f"{baseline_lat:.2f} μs | "
                     f"{direction} {change:+.2f}% |\n")

    report.append("\n")

    # 상세 벤치마크 결과
    report.append("## Detailed Benchmark Results\n\n")

    for name, data in results.items():
        if name == 'baseline':
            continue

        report.append(f"### {name}\n\n")
        benchmarks = data.get('benchmarks', [])

        if benchmarks:
            report.append("| Benchmark | Time | Iterations | Items/sec |\n")
            report.append("|-----------|------|------------|----------|\n")

            for bench in benchmarks[:10]:  # 상위 10개만 표시
                bench_name = bench.get('name', 'Unknown')
                cpu_time = bench.get('cpu_time', 0)
                iterations = bench.get('iterations', 0)
                items_per_sec = bench.get('items_per_second', 0)

                report.append(f"| {bench_name} | {cpu_time:.2f} ns | "
                            f"{iterations:,} | {format_number(items_per_sec, '/s')} |\n")

        report.append("\n")

    # 성능 권장사항
    report.append("## Performance Recommendations\n\n")

    if 'throughput_jobs_per_sec' in metrics and 'throughput' in baselines:
        change, _ = calculate_change(metrics['throughput_jobs_per_sec'],
                                     baselines['throughput'])
        if change < -5:
            report.append("⚠️ **Warning**: Throughput has decreased by more than 5%. "
                        "Please review recent changes.\n\n")
        elif change > 10:
            report.append("✅ **Great**: Throughput has improved by more than 10%!\n\n")

    return ''.join(report)

def main():
    parser = argparse.ArgumentParser(description='Generate performance benchmark report')
    parser.add_argument('--input', required=True, help='Directory containing benchmark JSON files')
    parser.add_argument('--baseline', required=True, help='Path to BASELINE.md file')
    parser.add_argument('--output', required=True, help='Output markdown file')
    parser.add_argument('--system-info', help='System information markdown file')

    args = parser.parse_args()

    # 데이터 로드
    results = load_benchmark_results(args.input)
    baselines = parse_baseline(args.baseline)

    # 리포트 생성
    report = generate_report(results, baselines, args.system_info)

    # 파일 저장
    with open(args.output, 'w') as f:
        f.write(report)

    print(f"Performance report generated: {args.output}")

if __name__ == '__main__':
    main()
