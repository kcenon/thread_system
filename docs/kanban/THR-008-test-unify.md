# THR-008: Unify unittest and tests Directories

**ID**: THR-008
**Category**: BUILD
**Priority**: MEDIUM
**Status**: DONE
**Estimated Duration**: 5-6 days
**Dependencies**: None
**Completed**: 2025-11-23

---

## Summary

Consolidate the duplicated test directory structure (`unittest/` and `tests/unit/`) into a single unified structure. Reduce maintenance overhead and confusion.

---

## Background

### Current Structure (Duplicated)
```
thread_system/
├── unittest/                    # Original test location
│   ├── CMakeLists.txt
│   ├── interfaces_test/
│   ├── platform_test/
│   ├── thread_base_test/
│   ├── thread_pool_test/
│   ├── typed_thread_pool_test/
│   └── utilities_test/
│
├── tests/                       # Newer test location
│   ├── unit/
│   │   ├── CMakeLists.txt
│   │   ├── interfaces_test/
│   │   ├── platform_test/
│   │   ├── thread_base_test/
│   │   ├── thread_pool_test/
│   │   ├── typed_thread_pool_test/
│   │   ├── utilities_test/
│   │   └── thread_safety_test/
│   └── benchmarks/
```

### Problems
1. **Duplication**: Same tests may exist in both locations
2. **Confusion**: Unclear which location is authoritative
3. **Maintenance**: Changes may need to be made twice
4. **CI Complexity**: Must run tests from multiple locations

---

## Target Structure

```
thread_system/
├── tests/
│   ├── unit/
│   │   ├── CMakeLists.txt
│   │   ├── interfaces/
│   │   ├── platform/
│   │   ├── thread_base/
│   │   ├── thread_pool/
│   │   ├── typed_thread_pool/
│   │   ├── utilities/
│   │   └── thread_safety/
│   ├── integration/
│   │   └── CMakeLists.txt
│   ├── stress/
│   │   └── CMakeLists.txt
│   ├── benchmarks/
│   │   └── CMakeLists.txt
│   └── CMakeLists.txt
```

---

## Acceptance Criteria

- [ ] All tests migrated to `tests/` directory
- [ ] `unittest/` directory removed
- [ ] No duplicate test files
- [ ] All tests pass after migration
- [ ] CMake targets consolidated
- [ ] CI workflows updated
- [ ] Documentation updated

---

## Implementation Tasks

### Phase 1: Audit & Analysis
- [ ] List all test files in both directories
- [ ] Identify duplicates
- [ ] Identify unique tests in each location
- [ ] Document differences

```bash
# Audit script
diff -rq unittest/ tests/unit/ | grep -v "Only in"
```

### Phase 2: Merge Strategy

| unittest/ | tests/unit/ | Action |
|-----------|-------------|--------|
| Unique | - | Move to tests/unit/ |
| - | Unique | Keep |
| Duplicate (same) | Duplicate (same) | Keep tests/unit/ |
| Duplicate (different) | Duplicate (different) | Merge manually |

### Phase 3: Migration

```bash
# Step 1: Backup
cp -r unittest unittest_backup

# Step 2: Move unique tests
for dir in unittest/*_test; do
    target="tests/unit/$(basename $dir)"
    if [ ! -d "$target" ]; then
        mv "$dir" "$target"
    fi
done

# Step 3: Merge duplicates
# Manual review required
```

### Phase 4: CMake Consolidation

```cmake
# tests/CMakeLists.txt (consolidated)
add_subdirectory(unit)
add_subdirectory(integration)
add_subdirectory(stress)
add_subdirectory(benchmarks)

# tests/unit/CMakeLists.txt
set(UNIT_TEST_DIRS
    interfaces
    platform
    thread_base
    thread_pool
    typed_thread_pool
    utilities
    thread_safety
)

foreach(test_dir ${UNIT_TEST_DIRS})
    add_subdirectory(${test_dir})
endforeach()
```

### Phase 5: Cleanup
- [ ] Remove `unittest/` directory
- [ ] Update root CMakeLists.txt
- [ ] Update CI workflows
- [ ] Update documentation references

---

## Risk Mitigation

| Risk | Mitigation |
|------|------------|
| Test loss | Git history preserves all files |
| Build break | Feature branch, extensive testing |
| Coverage drop | Verify coverage before/after |
| CI failures | Run full CI matrix before merge |

---

## Verification Checklist

```bash
# Before migration
./scripts/run_all_tests.sh > before.log
cmake --build build --target coverage
# Record coverage: X%

# After migration
./scripts/run_all_tests.sh > after.log
cmake --build build --target coverage
# Verify coverage: >= X%

# Compare test results
diff before.log after.log
```

---

## Files to Modify/Delete

### Delete
- `unittest/` (entire directory after migration)

### Modify
- `CMakeLists.txt` (root) - Remove unittest reference
- `tests/CMakeLists.txt` - Consolidate
- `tests/unit/CMakeLists.txt` - Update paths
- `.github/workflows/ci.yml` - Update test paths
- `docs/PROJECT_STRUCTURE.md` - Update documentation

---

## Success Metrics

| Metric | Target |
|--------|--------|
| Test count | Same or higher |
| Test coverage | Same or higher |
| Build time | Reduced (no duplicates) |
| CMake files | Reduced count |
| Directory clarity | Single test location |
