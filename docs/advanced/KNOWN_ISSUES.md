# Known Issues

**Version**: 0.3.0.0
**Last Updated**: 2026-01-11
**Status**: Active Tracking

This document tracks known issues in thread_system that require attention.

## ✅ Production Deployment Checklist

Before deploying thread_system to production, verify the following:

- [x] **Lock-Free Queue Safe**: Hazard Pointer implementation completed
- [x] **Multiple Queue Options**: Both mutex-based and lock-free queues available
- [x] **Test Suite Passes**: All tests pass in sequence without segfaults
- [x] **ThreadSanitizer Clean**: No data races detected by ThreadSanitizer
- [x] **AddressSanitizer Clean**: No memory errors detected by AddressSanitizer
- [x] **Valgrind Clean**: CI/CD pipeline configured for Linux verification (see `.github/workflows/valgrind.yml`)

## ✅ P0 Critical Issues (Resolved)

### 1. MPMC Queue Thread-Local Storage Segfault ✅ RESOLVED

**Component**: Lock-free MPMC queue
**Severity**: P0 (Critical - Blocks Production Use)
**Status**: ✅ **RESOLVED** (2025-11-09)
**Discovered**: Test suite execution
**Resolution**: Implemented Hazard Pointer-based memory reclamation

**Original Description**:
The original lock-free MPMC queue implementation used thread-local storage (TLS) for node pool caching, which caused segmentation faults when TLS destructors accessed already-freed memory.

**Resolution Details**:
1. **Implemented Hazard Pointer-based memory reclamation** (include/kcenon/thread/core/hazard_pointer.h)
   - Thread-local hazard arrays (4 slots per thread)
   - Automatic reclamation with configurable threshold (default: 64 objects)
   - Statistics tracking for monitoring

2. **Rebuilt lock-free queue with Hazard Pointers** (include/kcenon/thread/lockfree/lockfree_job_queue.h)
   - Michael-Scott algorithm with HP-based memory reclamation
   - Protect-then-verify pattern for safe concurrent access
   - CAS-before-read to prevent data races
   - No TLS node pool (eliminates the root cause)

3. **Comprehensive Testing**:
   - All 10 lock-free queue tests passing (100% success rate)
   - ThreadSanitizer clean (no data races)
   - AddressSanitizer clean (no memory errors)
   - Performance benchmarks: **4x faster than mutex-based** (71 μs vs 291 μs per operation)

**Performance Impact**:
- Lock-free queue: 71 μs/operation
- Mutex-based queue: 291 μs/operation
- **Improvement**: 4x throughput increase

**Production Status**: ✅ **Safe for production use**

**References**:
- Hazard Pointer Implementation: `include/kcenon/thread/core/hazard_pointer.h`
- Lock-Free Queue: `include/kcenon/thread/lockfree/lockfree_job_queue.h`
- Design Document: `docs/HAZARD_POINTER_DESIGN.md`
- Tests: `unittest/thread_base_test/hazard_pointer_test.cpp`, `unittest/thread_base_test/lockfree_job_queue_test.cpp`

### 2. Typed Lock-Free Queue TLS Bug ✅ RESOLVED → 🗑️ REMOVED

**Component**: typed_lockfree_job_queue_t
**Severity**: P0 (Critical - Blocks Production Use)
**Status**: 🗑️ **REMOVED** (2026-01-11) - Component deleted in Phase 1.4.2
**Previously**: ✅ RESOLVED (2025-12-02)
**Discovered**: GitHub Issue #217
**Resolution**: Refactored to use lockfree_job_queue internally, then **REMOVED** in favor of `policy_lockfree_queue`

**Original Description**:
The typed_lockfree_job_queue_t implementation was blocked by `#error` directive due to TLS destructor ordering bugs. The class had incomplete implementation and could not be safely used.

**Final Resolution**:
The entire `typed_lockfree_job_queue_t` class has been **removed** from the codebase as part of the Phase 1.4 queue consolidation effort (#440). Users should migrate to:
- `policy_queue<lockfree_sync_policy, unbounded_policy>` (alias: `policy_lockfree_queue`)

**Migration Guide**: See [QUEUE_MIGRATION_GUIDE.md](../design/QUEUE_MIGRATION_GUIDE.md)

**References**:
- Removal PR: Phase 1.4.2 (#457)
- Migration Guide: `docs/design/QUEUE_MIGRATION_GUIDE.md`
- New implementation: `include/kcenon/thread/policies/policy_queue.h`

---

## P0 Critical Issues

None currently identified.

---

## P1 High Priority Issues

None currently identified.

---

## P2 Medium Priority Issues

### 1. Valgrind Verification on Linux ✅ RESOLVED

**Component**: Memory leak verification
**Severity**: P2 (Medium - Quality Assurance)
**Status**: ✅ **RESOLVED** (2025-11-23)
**Discovered**: 2025-11-09

**Description**:
Valgrind support on macOS is limited. Full memory leak verification needs to be performed on Linux systems.

**Resolution**:
- [x] Run Valgrind on Linux CI/CD pipeline - `.github/workflows/valgrind.yml`
- [x] Verify no memory leaks in extended stress tests - `scripts/run_valgrind.sh --stress-test`
- [x] Suppressions file for false positives - `valgrind.supp`

**References**:
- CI Workflow: `.github/workflows/valgrind.yml`
- Runner Script: `scripts/run_valgrind.sh`
- Suppressions: `valgrind.supp`

---

## Issue Severity Classification

**P0 (Critical)**: Blocks production use, causes crashes or data corruption
**P1 (High)**: Significant functionality impact, workaround available
**P2 (Medium)**: Minor functionality impact, low user impact
**P3 (Low)**: Cosmetic issues, documentation gaps

---

## Reporting New Issues

When reporting a new issue, please include:
1. Severity classification (P0-P3)
2. Component affected
3. Detailed description with reproduction steps
4. Root cause analysis (if known)
5. Proposed solutions
6. Current workarounds
7. File references

---

Last Updated: 2026-01-11
Maintainer: Architecture Review Team
