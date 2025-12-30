# ADR-002: typed_pool Template Hierarchy Evaluation

**Status:** Accepted
**Date:** 2025-12-31
**Authors:** thread_system maintainers
**Related Issues:** #354

## Context

The `typed_pool` module provides a parallel template hierarchy for priority-based job scheduling:

| Standard Pool | Typed Pool |
|---------------|------------|
| `thread_pool` | `typed_thread_pool_t<JobType>` |
| `thread_worker` | `typed_thread_worker_t<JobType>` |
| `job` | `typed_job_t<JobType>` |
| `job_queue` | `typed_job_queue_t<JobType>` |

Issue #354 raised concerns about:
- Parallel hierarchies increasing codebase complexity
- Potential over-engineering for use cases
- Violation of "Fewest Elements" principle from Simple Design

This ADR documents the analysis and decision regarding the typed_pool hierarchy.

---

## Analysis

### 1. Functional Differences

The two hierarchies serve fundamentally different purposes:

| Feature | thread_pool | typed_thread_pool_t |
|---------|-------------|---------------------|
| Scheduling | FIFO | Priority-based |
| Job Priority | Not supported | Template parameter |
| Worker Specialization | All jobs | Specific priority levels |
| Work Stealing | Supported | Not implemented |
| Metrics Collection | ThreadPoolMetrics | Not implemented |
| Health Checks | Supported | Not implemented |

**Key Finding:** The typed_pool provides **priority-based scheduling** that cannot be replicated by the standard thread_pool without significant runtime overhead.

### 2. Code Metrics

```
typed_pool headers:  3,276 lines (13 files)
core headers:        8,061 lines (28 files)
```

The typed_pool represents approximately 29% of the header codebase but provides a distinct feature set.

### 3. Usage Analysis

#### Examples
- `typed_thread_pool_sample/` - Demonstrates priority worker assignment
- `typed_thread_pool_sample_2/` - Advanced typed pool usage
- `typed_job_queue_sample/` - Priority queue operations

#### Tests
- `typed_thread_pool_test/` - Unit tests for typed pool
- `typed_thread_pool_benchmarks/` - Performance benchmarks

#### Documentation
- Referenced in API_REFERENCE.md, ARCHITECTURE.md, USER_GUIDE.md
- Included in ADR-001 v3.0 API surface

### 4. Design Benefits

1. **Compile-time Type Safety**: Priority types are checked at compile time
2. **Zero Runtime Overhead**: Template instantiation eliminates virtual dispatch for priority handling
3. **Worker Specialization**: Workers can be assigned to handle specific priority levels
4. **Custom Priority Types**: Users can define their own priority enums

### 5. Alternative Evaluation

| Option | Pros | Cons |
|--------|------|------|
| **A: Keep and document** | No breaking changes, clear separation | Two hierarchies to maintain |
| **B: Merge via template specialization** | Single hierarchy | Complex template metaprogramming, potential performance impact |
| **C: Remove, use std::any** | Simpler codebase | Runtime overhead, loss of type safety |

---

## Decision

**Decision: Option A - Keep typed_pool and improve documentation**

### Rationale

1. **Distinct Purpose**: The typed_pool provides priority-based scheduling, a fundamentally different feature from the standard thread_pool's FIFO scheduling.

2. **No Code Duplication**: The two hierarchies share `thread_base` as a common base but have independent implementations for their distinct scheduling behaviors.

3. **Active Usage**: Tests, examples, and benchmarks demonstrate real-world usage patterns.

4. **Type Safety Value**: Template-based priority handling provides compile-time guarantees that runtime alternatives cannot offer.

5. **Minimal Maintenance Burden**: The typed_pool is stable with well-defined interfaces and doesn't frequently require changes.

---

## Actions

### Documentation Improvements

1. **Add When to Use guide**: Create clear guidance on choosing between `thread_pool` and `typed_thread_pool_t`

2. **Update FEATURES.md**: Add comparison table between the two implementations

3. **Enhance API Reference**: Document the performance characteristics and use cases

### Code Quality

1. **Verify header inclusion**: Ensure all typed_pool headers include necessary dependencies

2. **Maintain test coverage**: Keep existing test coverage for typed_pool

---

## Consequences

### Positive

1. **No Breaking Changes**: Existing users continue to work without modification
2. **Clear Separation**: Each pool type optimized for its use case
3. **Performance**: Priority scheduling without runtime overhead

### Negative

1. **Two Hierarchies**: Developers must understand when to use each
2. **Documentation Debt**: Requires clear guidance on selection criteria

### Neutral

1. **Codebase Size**: The additional ~3K lines are justified by the distinct functionality

---

## References

- [Issue #354](https://github.com/kcenon/thread_system/issues/354)
- [Simple Design Principles](https://martinfowler.com/bliki/BeckDesignRules.html)
- [ADR-001: v3.0 API Surface](./ADR-001-v3-api-surface.md)

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2025-12-31 | thread_system maintainers | Initial decision |
