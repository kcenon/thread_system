# Thread System Kanban Board

This folder contains tickets for tracking improvement work on the Thread System.

**Last Updated**: 2025-11-23

---

## Ticket Status

### Summary

| Category | Total | Done | In Progress | Pending |
|----------|-------|------|-------------|---------|
| CORE | 4 | 4 | 0 | 0 |
| TEST | 3 | 0 | 0 | 3 |
| BUILD | 3 | 0 | 0 | 3 |
| **Total** | **10** | **4** | **0** | **6** |

> Note: 10 actionable tickets have been created. Additional DOC/MAINT tickets can be created as needed.

---

## Ticket List

### CORE: Complete Core Features

Complete Valgrind verification and migration plans.

| ID | Title | Priority | Est. Duration | Dependencies | Status |
|----|-------|----------|---------------|--------------|--------|
| [THR-001](THR-001-valgrind.md) | Valgrind Memory Leak Verification & CI/CD Integration | HIGH | 2-3d | - | DONE |
| [THR-002](THR-002-platform-tests.md) | Platform-specific Edge Case Tests | HIGH | 3-4d | - | DONE |
| [THR-003](THR-003-error-migration.md) | Complete ERROR_SYSTEM_MIGRATION Plan | HIGH | 4-5d | - | DONE |
| [THR-004](THR-004-log-level.md) | Execute LOG_LEVEL Unification Plan | HIGH | 3-4d | - | DONE |

**Recommended Execution Order**: THR-001 → THR-002 → THR-003 → THR-004

---

### TEST: Test Enhancement

Enhance test coverage and stress tests.

| ID | Title | Priority | Est. Duration | Dependencies | Status |
|----|-------|----------|---------------|--------------|--------|
| [THR-005](THR-005-coverage-80.md) | Achieve 80% Integration Test Coverage | HIGH | 5-6d | - | TODO |
| [THR-006](THR-006-stress-test.md) | Automate Stress Tests & Result Collection | HIGH | 4-5d | - | TODO |
| [THR-007](THR-007-perf-regression.md) | Automate Performance Regression Detection | HIGH | 3-4d | - | TODO |

---

### BUILD: Build & CI/CD Improvements

Unify test structure and optimize CI.

| ID | Title | Priority | Est. Duration | Dependencies | Status |
|----|-------|----------|---------------|--------------|--------|
| [THR-008](THR-008-test-unify.md) | Unify unittest and tests Directories | MEDIUM | 5-6d | - | TODO |
| [THR-009](THR-009-cmake-refactor.md) | Refactor CMake Build System | MEDIUM | 6-8d | THR-008 | TODO |
| [THR-010](THR-010-platform-code.md) | Separate & Clean Platform-specific Code | MEDIUM | 4-5d | - | TODO |

---

### Future Tickets (Not Yet Created)

The following tickets are planned but not yet detailed:

#### DOC: Documentation Improvement
- THR-014: Verify & Document Windows ARM64 Support
- THR-015: Automate Documentation Link & Consistency Verification
- THR-016: Organize Korean/English Document Version Management
- THR-017: Automate Doxygen Documentation Generation & Hosting

#### BUILD (Additional)
- THR-011: Organize & Document Example Projects
- THR-012: Optimize GitHub Actions Workflow Performance
- THR-013: Automate Cross-platform Performance Benchmark Comparison

#### MAINT: Maintenance
- THR-021: Type-based Thread Pool Routing Optimization Guide
- THR-022: Strengthen common_system Integration
- THR-023: Regular Performance Benchmark Baseline Updates

---

## Execution Plan

### Phase 1: Production Completion (HIGH Priority)
1. **THR-001**: Valgrind Verification & CI Integration
2. **THR-002**: Platform Edge Case Tests
3. **THR-003**: Error System Migration
4. **THR-004**: Log Level Unification

### Phase 2: Test Enhancement (HIGH Priority)
1. **THR-005**: Achieve 80% Coverage
2. **THR-006**: Stress Test Automation
3. **THR-007**: Performance Regression Detection

### Phase 3: Structural Improvement (MEDIUM Priority)
1. **THR-008**: Unify Test Directories
2. **THR-009**: CMake Refactoring (depends on THR-008)
3. **THR-010**: Platform Code Separation

---

## Status Definitions

- **TODO**: Not yet started
- **IN_PROGRESS**: Work in progress
- **REVIEW**: Awaiting code review
- **DONE**: Completed

---

## Created Ticket Files

```
kanban/
├── README.md
├── PRIORITY_DIRECTIVE.md
├── THR-001-valgrind.md
├── THR-002-platform-tests.md
├── THR-003-error-migration.md
├── THR-004-log-level.md
├── THR-005-coverage-80.md
├── THR-006-stress-test.md
├── THR-007-perf-regression.md
├── THR-008-test-unify.md
├── THR-009-cmake-refactor.md
└── THR-010-platform-code.md
```

---

**Maintainer**: TBD
**Contact**: Use issue tracker
