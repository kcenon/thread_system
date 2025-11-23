#!/bin/bash
# Valgrind Memory Check Runner
# Usage: ./run_valgrind.sh [--unit-tests|--integration-tests|--stress-test|--generate-report]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/build"
VALGRIND_OPTS="--leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1"
SUPPRESSIONS="${PROJECT_ROOT}/valgrind.supp"

# Add suppressions if file exists
if [ -f "$SUPPRESSIONS" ]; then
    VALGRIND_OPTS="$VALGRIND_OPTS --suppressions=$SUPPRESSIONS"
fi

run_unit_tests() {
    echo "=== Running Valgrind on Unit Tests ==="

    local test_executables=(
        "thread_pool_test"
        "thread_base_test"
        "typed_thread_pool_test"
        "interfaces_test"
        "utilities_test"
        "platform_test"
    )

    for test in "${test_executables[@]}"; do
        local test_path="${BUILD_DIR}/unittest/${test}/${test}"
        if [ -f "$test_path" ]; then
            echo "Running: $test"
            valgrind $VALGRIND_OPTS \
                --xml=yes \
                --xml-file="${BUILD_DIR}/valgrind-${test}.xml" \
                "$test_path" 2>&1 | tee -a "${BUILD_DIR}/valgrind-unit.log"
        else
            echo "Warning: $test not found at $test_path"
        fi
    done
}

run_integration_tests() {
    echo "=== Running Valgrind on Integration Tests ==="

    local test_path="${BUILD_DIR}/integration_tests/integration_tests"
    if [ -f "$test_path" ]; then
        valgrind $VALGRIND_OPTS \
            --xml=yes \
            --xml-file="${BUILD_DIR}/valgrind-integration.xml" \
            "$test_path" 2>&1 | tee -a "${BUILD_DIR}/valgrind-integration.log"
    else
        echo "Warning: integration_tests not found"
    fi
}

run_stress_test() {
    local duration=${1:-3600}  # Default 1 hour
    echo "=== Running Valgrind Stress Test (${duration}s) ==="

    local test_path="${BUILD_DIR}/unittest/thread_pool_test/thread_pool_test"
    if [ -f "$test_path" ]; then
        timeout "${duration}s" valgrind $VALGRIND_OPTS \
            --xml=yes \
            --xml-file="${BUILD_DIR}/valgrind-stress.xml" \
            "$test_path" --gtest_repeat=-1 --gtest_break_on_failure 2>&1 \
            | tee "${BUILD_DIR}/valgrind-stress.log" || true
    fi
}

generate_report() {
    echo "=== Generating Valgrind Report ==="

    local report_file="${BUILD_DIR}/valgrind-report.txt"
    echo "Valgrind Memory Check Report" > "$report_file"
    echo "Generated: $(date)" >> "$report_file"
    echo "========================================" >> "$report_file"
    echo "" >> "$report_file"

    # Parse XML files and extract summary
    for xml_file in "${BUILD_DIR}"/valgrind-*.xml; do
        if [ -f "$xml_file" ]; then
            echo "Results from: $(basename "$xml_file")" >> "$report_file"
            echo "----------------------------------------" >> "$report_file"

            # Extract error counts using grep (simple parsing)
            if grep -q "definitely lost:" "$xml_file" 2>/dev/null; then
                grep "definitely lost:" "$xml_file" >> "$report_file" || true
            fi
            if grep -q "indirectly lost:" "$xml_file" 2>/dev/null; then
                grep "indirectly lost:" "$xml_file" >> "$report_file" || true
            fi
            if grep -q "possibly lost:" "$xml_file" 2>/dev/null; then
                grep "possibly lost:" "$xml_file" >> "$report_file" || true
            fi

            echo "" >> "$report_file"
        fi
    done

    # Also check log files for summary
    for log_file in "${BUILD_DIR}"/valgrind-*.log; do
        if [ -f "$log_file" ]; then
            echo "Log summary from: $(basename "$log_file")" >> "$report_file"
            grep -E "definitely lost:|indirectly lost:|possibly lost:|ERROR SUMMARY:" "$log_file" >> "$report_file" || true
            echo "" >> "$report_file"
        fi
    done

    echo "Report generated: $report_file"
    cat "$report_file"
}

# Parse arguments
DURATION=3600
while [[ $# -gt 0 ]]; do
    case $1 in
        --unit-tests)
            run_unit_tests
            shift
            ;;
        --integration-tests)
            run_integration_tests
            shift
            ;;
        --stress-test)
            shift
            if [[ $1 == --duration=* ]]; then
                DURATION="${1#--duration=}"
                shift
            fi
            run_stress_test "$DURATION"
            ;;
        --generate-report)
            generate_report
            shift
            ;;
        --all)
            run_unit_tests
            run_integration_tests
            generate_report
            shift
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --unit-tests        Run Valgrind on unit tests"
            echo "  --integration-tests Run Valgrind on integration tests"
            echo "  --stress-test       Run extended stress test"
            echo "  --duration=SECONDS  Duration for stress test (default: 3600)"
            echo "  --generate-report   Generate summary report"
            echo "  --all               Run all tests and generate report"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

echo "Valgrind check completed."
