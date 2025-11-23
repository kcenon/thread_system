# THR-001: Valgrind Memory Leak Verification & CI/CD Integration

**ID**: THR-001
**Category**: CORE
**Priority**: HIGH
**Status**: TODO
**Estimated Duration**: 2-3 days
**Dependencies**: None

---

## Summary

Implement comprehensive Valgrind memory leak verification on Linux CI/CD pipeline. macOS Valgrind support is limited, requiring Linux-based verification infrastructure.

---

## Background

- **Current State**: ThreadSanitizer and AddressSanitizer are clean
- **Gap**: Valgrind verification pending (macOS not supported)
- **Reference**: `docs/advanced/KNOWN_ISSUES.md` - P2 item

---

## Acceptance Criteria

- [ ] Linux CI runner configured with Valgrind
- [ ] Valgrind runs on all unit tests
- [ ] Valgrind runs on integration tests
- [ ] Extended stress test verification (1+ hour runs)
- [ ] Memory leak report generated as CI artifact
- [ ] CI fails on detected leaks
- [ ] Documentation updated with Valgrind results

---

## Implementation Tasks

### 1. CI/CD Configuration
```yaml
# .github/workflows/valgrind.yml
jobs:
  valgrind:
    runs-on: ubuntu-latest
    steps:
      - name: Install Valgrind
        run: sudo apt-get install -y valgrind
      - name: Run Valgrind
        run: |
          valgrind --leak-check=full \
                   --show-leak-kinds=all \
                   --track-origins=yes \
                   --error-exitcode=1 \
                   ./build/bin/thread_system_tests
```

### 2. Test Runner Script
- Create `scripts/run_valgrind.sh`
- Configure suppressions file for known false positives
- Generate XML report for CI parsing

### 3. Extended Stress Tests
- 1-hour continuous operation test
- Memory growth monitoring
- Peak memory usage reporting

---

## Success Metrics

| Metric | Target |
|--------|--------|
| Memory leaks | 0 |
| Memory errors | 0 |
| False positives | Suppressed |
| CI execution time | < 30 min |

---

## Files to Modify

- `.github/workflows/` - Add valgrind.yml
- `scripts/` - Add run_valgrind.sh
- `docs/advanced/KNOWN_ISSUES.md` - Update P2 status

---

## Notes

- Valgrind can be slow; consider running only on merge to main
- Use `--gen-suppressions=all` to create suppression file for STL/GTest false positives
