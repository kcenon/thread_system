#!/usr/bin/env python3
"""
성능 배지를 업데이트합니다 (shields.io endpoint용).
"""

import json
import argparse
from pathlib import Path

def extract_throughput(benchmark_json: str) -> float:
    """벤치마크 결과에서 throughput을 추출합니다."""
    with open(benchmark_json, 'r') as f:
        data = json.load(f)

    for bench in data.get('benchmarks', []):
        if 'Throughput' in bench.get('name', ''):
            return bench.get('items_per_second', 0)

    return 0

def format_throughput(value: float) -> str:
    """Throughput을 읽기 쉬운 형태로 포맷합니다."""
    if value >= 1_000_000:
        return f"{value/1_000_000:.2f}M jobs/s"
    elif value >= 1_000:
        return f"{value/1_000:.0f}K jobs/s"
    else:
        return f"{value:.0f} jobs/s"

def determine_color(value: float, baseline: float = 1_240_000) -> str:
    """성능에 따라 배지 색상을 결정합니다."""
    if value >= baseline:
        return 'brightgreen'
    elif value >= baseline * 0.95:
        return 'green'
    elif value >= baseline * 0.90:
        return 'yellow'
    elif value >= baseline * 0.80:
        return 'orange'
    else:
        return 'red'

def main():
    parser = argparse.ArgumentParser(description='Update performance badge')
    parser.add_argument('--input', required=True, help='Benchmark JSON file')
    parser.add_argument('--output', required=True, help='Output badge JSON file')

    args = parser.parse_args()

    throughput = extract_throughput(args.input)

    badge_data = {
        'schemaVersion': 1,
        'label': 'performance',
        'message': format_throughput(throughput),
        'color': determine_color(throughput)
    }

    # 출력 디렉토리 생성
    Path(args.output).parent.mkdir(parents=True, exist_ok=True)

    with open(args.output, 'w') as f:
        json.dump(badge_data, f, indent=2)

    print(f"Badge updated: {format_throughput(throughput)}")

if __name__ == '__main__':
    main()
