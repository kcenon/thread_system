#!/usr/bin/env python3
"""
성능 회귀를 감지하고 리포트를 생성합니다.
"""

import json
import argparse
import sys

def load_json(file_path: str) -> dict:
    """JSON 파일을 로드합니다."""
    try:
        with open(file_path, 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        return {}

def check_regression(current: dict, baseline: dict, threshold: float) -> tuple:
    """성능 회귀를 확인합니다."""
    regressions = []
    improvements = []

    current_benchmarks = {b['name']: b for b in current.get('benchmarks', [])}
    baseline_benchmarks = {b['name']: b for b in baseline.get('benchmarks', [])}

    for name, curr_bench in current_benchmarks.items():
        if name not in baseline_benchmarks:
            continue

        base_bench = baseline_benchmarks[name]

        # CPU 시간 비교 (낮을수록 좋음)
        curr_time = curr_bench.get('cpu_time', 0)
        base_time = base_bench.get('cpu_time', 0)

        if base_time == 0:
            continue

        change_pct = ((curr_time - base_time) / base_time) * 100

        if change_pct > threshold:
            regressions.append({
                'name': name,
                'current': curr_time,
                'baseline': base_time,
                'change_pct': change_pct
            })
        elif change_pct < -threshold:
            improvements.append({
                'name': name,
                'current': curr_time,
                'baseline': base_time,
                'change_pct': change_pct
            })

    return regressions, improvements

def generate_regression_report(regressions: list, improvements: list,
                               threshold: float) -> str:
    """회귀 리포트를 생성합니다."""
    report = []

    if regressions:
        report.append(f"## ⚠️ Performance Regressions (>{threshold}% slower)\n\n")
        report.append("| Benchmark | Current | Baseline | Change |\n")
        report.append("|-----------|---------|----------|--------|\n")

        for reg in sorted(regressions, key=lambda x: x['change_pct'], reverse=True):
            report.append(f"| {reg['name']} | {reg['current']:.2f} ns | "
                        f"{reg['baseline']:.2f} ns | "
                        f"🔴 +{reg['change_pct']:.2f}% |\n")

        report.append("\n")

    if improvements:
        report.append(f"## ✅ Performance Improvements (>{threshold}% faster)\n\n")
        report.append("| Benchmark | Current | Baseline | Change |\n")
        report.append("|-----------|---------|----------|--------|\n")

        for imp in sorted(improvements, key=lambda x: x['change_pct']):
            report.append(f"| {imp['name']} | {imp['current']:.2f} ns | "
                        f"{imp['baseline']:.2f} ns | "
                        f"🟢 {imp['change_pct']:.2f}% |\n")

        report.append("\n")

    if not regressions and not improvements:
        report.append("## ✅ No significant performance changes detected\n\n")

    return ''.join(report)

def main():
    parser = argparse.ArgumentParser(description='Check for performance regressions')
    parser.add_argument('--current', required=True, help='Current benchmark JSON')
    parser.add_argument('--baseline', required=True, help='Baseline benchmark JSON')
    parser.add_argument('--threshold', type=float, default=5.0,
                       help='Regression threshold percentage (default: 5.0)')
    parser.add_argument('--output', required=True, help='Output markdown file')

    args = parser.parse_args()

    current = load_json(args.current)
    baseline = load_json(args.baseline)

    regressions, improvements = check_regression(current, baseline, args.threshold)

    report = generate_regression_report(regressions, improvements, args.threshold)

    with open(args.output, 'w') as f:
        f.write(report)

    # Exit with error if regressions detected
    if regressions:
        print(f"⚠️ {len(regressions)} performance regression(s) detected!")
        sys.exit(1)
    else:
        print("✅ No performance regressions detected")
        sys.exit(0)

if __name__ == '__main__':
    main()
