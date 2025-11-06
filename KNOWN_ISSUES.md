# Known Issues

This document tracks critical known issues in thread_system that require resolution before production deployment.

## P0 Critical Issues

### 1. MPMC Queue Thread-Local Storage Segfault

**Component**: Lock-free MPMC queue
**Severity**: P0 (Critical - Blocks Production Use)
**Status**: Open
**Discovered**: Test suite execution

**Description**:
The lock-free MPMC queue implementation uses thread-local storage (TLS) for node pool caching. When test fixtures are destroyed, TLS destructors may access already-freed memory from the node pool, causing segmentation faults.

**Impact**:
- Multiple tests cannot run reliably in sequence
- Individual tests pass when isolated
- Production use may encounter similar race conditions during shutdown
- **Do NOT use MPMC queue in production until resolved**

**Reproduction**:
```bash
# Run all MPMC queue tests - may segfault
./build/tests/thread_base_test --gtest_filter=MPMCQueueTest.*

# Run individual test - passes reliably
./build/tests/thread_base_test --gtest_filter=MPMCQueueTest.BasicEnqueueDequeue
```

**Root Cause Analysis**:
1. Lock-free node pool allocates thread-local caches for performance
2. Test fixture destructor runs before TLS destructors
3. TLS destructor attempts to return nodes to already-freed pool
4. Access to freed memory → segmentation fault

**Recommended Solutions** (Priority Order):

1. **Hazard Pointers** (Preferred)
   - Effort: 3 weeks
   - Provides guaranteed memory safety
   - Industry-standard solution for lock-free data structures
   - Implementation: Add hazard pointer domain to node pool

2. **Epoch-Based Reclamation (EBR)**
   - Effort: 2 weeks
   - Deterministic cleanup with grace periods
   - Good for controlled environments
   - Implementation: Add epoch counter and grace period tracking

3. **Node Pool Lifetime Redesign**
   - Effort: 1 week (fastest)
   - Make node pool outlive all threads
   - Use static/global lifetime or reference counting
   - Trade-off: Less elegant design, but safe

**Current Workaround**:
Test suite includes forced delays in `TearDown()`:
```cpp
void TearDown() override {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    for (int i = 0; i < 3; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::this_thread::yield();
    }
}
```

**WARNING**: This is NOT a production-safe solution. Delays only reduce probability, do not eliminate the race condition.

**Action Items**:
- [ ] Choose solution approach (recommend hazard pointers)
- [ ] Implement chosen solution
- [ ] Add comprehensive concurrency tests
- [ ] Run under ThreadSanitizer to verify fix
- [ ] Remove workaround delays from tests

**References**:
- File: `unittest/thread_base_test/mpmc_queue_test.cpp:7-30`
- Original Analysis: Architecture review 2025-11-07

---

## P1 High Priority Issues

None currently identified.

---

## P2 Medium Priority Issues

None currently identified.

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

Last Updated: 2025-11-07
Maintainer: Architecture Review Team
