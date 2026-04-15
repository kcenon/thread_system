# Document Review Report — thread_system

**Generated**: 2026-04-14
**Mode**: Report-only (no source files modified)
**Files analyzed**: 110 markdown files
**Total anchors indexed**: 3,481
**Total internal links validated**: 1,243

## Findings Summary

| Severity      | Phase 1 (Anchors) | Phase 2 (Accuracy) | Phase 3 (SSOT) | Total |
|---------------|-------------------|--------------------|----------------|-------|
| Must-Fix      | 86                | 3                  | 0              | 89    |
| Should-Fix    | 0                 | 38                 | 23             | 61    |
| Nice-to-Have  | 0                 | 0                  | 7              | 7     |
| **Total**     | **86**            | **41**             | **30**         | **157** |

Phase 1 breakdown: 74 broken file paths + 12 broken anchors.
Phase 2 Must-Fix: 3 TBD placeholders in `docs/performance/BASELINE.md` (factual gaps in SSOT document).
Phase 3: 23 orphan documents (0 incoming links) + 7 redundancy/SSOT-fragmentation patterns.

---

## Must-Fix Items

### Phase 1 — Broken file paths (74)

File references resolve to non-existent files on disk.

#### Root / top-level docs
1. `README.md:340` — broken link `../ECOSYSTEM.md` (text: "Ecosystem Integration Guide →"). Target is at `docs/ECOSYSTEM.md`, not `../ECOSYSTEM.md`.
2. `docs/BENCHMARKS.kr.md:305` — broken link `guides/PERFORMANCE.md` (text: "최적화 가이드"). No such file; relevant guide is `advanced/PERFORMANCE.md`.
3. `docs/BENCHMARKS.md:643` — broken link `guides/PERFORMANCE.md` (text: "Optimization Guide"). Same as above.
4. `docs/CHANGELOG.kr.md:458` — broken link `docs/advanced/ERROR_SYSTEM_MIGRATION_GUIDE.md` (text: "에러 시스템 마이그레이션 가이드"). Incorrect relative path — already inside `docs/`.
5. `docs/CHANGELOG.kr.md:459` — broken link `docs/guides/LOGGER_INTERFACE_MIGRATION_GUIDE.kr.md` (same root cause).
6. `docs/CHANGELOG.md:753` — broken link `docs/advanced/ERROR_SYSTEM_MIGRATION_GUIDE.md` (text: "Error System Migration Guide").
7. `docs/CHANGELOG.md:754` — broken link `docs/guides/LOGGER_INTERFACE_MIGRATION_GUIDE.md`.
8. `docs/FEATURES.kr.md:1039` — broken link `guides/API_REFERENCE.md` (text: "API 레퍼런스"). API reference lives at `docs/API_REFERENCE.md`.
9. `docs/FEATURES.kr.md:1040` — broken link `guides/USER_GUIDE.md` (text: "사용자 가이드"). No user guide at that path.
10. `docs/FEATURES.md:1397` — broken link `guides/API_REFERENCE.md` (text: "API Reference").
11. `docs/FEATURES.md:1398` — broken link `guides/USER_GUIDE.md` (text: "User Guide").
12. `docs/QUEUE_BACKWARD_COMPATIBILITY.md:15` — broken link `QUEUE_BACKWARD_COMPATIBILITY.kr.md` (text: "한국어"). Korean counterpart missing.

#### `docs/advanced/CHANGELOG*.md`
13. `docs/advanced/CHANGELOG.kr.md:353` — broken link `CONTRIBUTING.md` (text: "CONTRIBUTING.md"). Relative path resolves to `docs/advanced/CONTRIBUTING.md` which does not exist; should be `../../CONTRIBUTING.md`.
14. `docs/advanced/CHANGELOG.kr.md:357` — broken link `LICENSE` (text: "LICENSE"). Same path issue.
15. `docs/advanced/CHANGELOG.md:374` — broken link `CONTRIBUTING.md`.
16. `docs/advanced/CHANGELOG.md:378` — broken link `LICENSE`.

#### `docs/advanced/` cluster
17. `docs/advanced/CI_CD_PERFORMANCE.md:251` — broken link `../.github/workflows/performance-benchmarks.yml`. Workflow file not present in repo.
18. `docs/advanced/CPP20_CONCEPTS.kr.md:281` — broken link `01-ARCHITECTURE.kr.md` (text: "아키텍처 가이드"). Numbered-prefix file convention not used.
19. `docs/advanced/CPP20_CONCEPTS.kr.md:282` — broken link `02-API_REFERENCE.kr.md` (text: "API 레퍼런스").
20. `docs/advanced/CPP20_CONCEPTS.md:281` — broken link `01-ARCHITECTURE.md` (text: "Architecture Guide").
21. `docs/advanced/CPP20_CONCEPTS.md:282` — broken link `02-API_REFERENCE.md` (text: "API Reference").
22. `docs/advanced/INTEGRATION.md:15` — broken link `INTEGRATION.kr.md` (text: "한국어"). Korean version missing.
23. `docs/advanced/INTEGRATION.md:973` — broken link `examples/integration_example/`. Directory not present.
24. `docs/advanced/INTEGRATION.md:994` — broken link `docs/` (text: "docs/").
25. `docs/advanced/INTEGRATION.md:995` — broken link `docs/API_REFERENCE.md`. Relative resolution wrong.
26. `docs/advanced/INTEGRATION.md:996` — broken link `docs/ARCHITECTURE.md`. Same.
27. `docs/advanced/QUEUE_SELECTION_GUIDE.md:15` — broken link `QUEUE_SELECTION_GUIDE.kr.md`. Korean translation missing.

#### `docs/advanced/README*.md` (14 broken paths each, single root cause)

These two files link to docs with the wrong relative prefix — referenced files live in `../` or `../guides/`, not in the current directory.

28–41. `docs/advanced/README.kr.md` lines 24, 35, 47, 48, 53, 60, 67, 72, 76, 77, 78, 86, 101, 135 — broken links to `BUILD_GUIDE.kr.md`, `CONTRIBUTING.kr.md`, `DESIGN_IMPROVEMENTS.kr.md`, `guides/`, `guides/DEPENDENCY_COMPATIBILITY_MATRIX.kr.md`, `guides/DEPENDENCY_CONFLICT_RESOLUTION_GUIDE.kr.md`, `guides/LICENSE_COMPATIBILITY.kr.md`. Most targets exist but at sibling paths (`../guides/...`, `../../CONTRIBUTING.md`, `../guides/BUILD_GUIDE.kr.md`).
42–55. `docs/advanced/README.md` lines 24, 35, 47, 48, 53, 60, 67, 72, 76, 77, 78, 86, 101, 135 — English equivalents with identical issue.

Note: `DESIGN_IMPROVEMENTS.md`/`.kr.md` does not appear anywhere in the repository.

#### `docs/advanced/THREAD_SYSTEM_ARCHITECTURE.md`
56. Line 850 — broken link `docs/ARCHITECTURE.md`. Relative-from-`docs/advanced/` issue.
57. Line 851 — broken link `docs/API_REFERENCE.md`. Same.
58. Line 852 — broken link `docs/USER_GUIDE.md`. No such file (USER_GUIDE is at `docs/advanced/USER_GUIDE.md`).

#### `docs/advanced/USER_GUIDE*.md`
59. `docs/advanced/USER_GUIDE.kr.md:51` — broken link `./PLATFORM_BUILD_GUIDE.md`. File does not exist.
60. `docs/advanced/USER_GUIDE.md:51` — same.

#### `docs/advanced/USER_MIGRATION_GUIDE.md`
61. Line 1117 — broken link `examples/` (directory not present at that relative path).

#### `docs/guides/`
62. `docs/guides/BUILD_GUIDE.kr.md:465` — broken link `./API_REFERENCE.md`. API reference is at `../API_REFERENCE.md` or `../advanced/API_REFERENCE.md`.
63. `docs/guides/BUILD_GUIDE.md:464` — same.
64. `docs/guides/FAQ.md:198` — broken link `../BUILD_GUIDE.md`. Should be `./BUILD_GUIDE.md`.
65. `docs/guides/LOG_LEVEL_MIGRATION_GUIDE.kr.md:295` — broken link `../advanced/LOG_LEVEL_UNIFICATION_PLAN.md`. File does not exist.
66. `docs/guides/LOG_LEVEL_MIGRATION_GUIDE.md:295` — same.
67. `docs/guides/QUICK_START.kr.md:223` — broken link `../advanced/02-API_REFERENCE.kr.md`. Numbered-prefix file missing.
68. `docs/guides/QUICK_START.md:223` — broken link `../advanced/02-API_REFERENCE.md`.

#### `docs/integration/README.md`
69. Line 21 — broken link `with-common-system.md`. Sub-guide missing.
70. Line 22 — broken link `with-logger.md`.
71. Line 23 — broken link `with-monitoring.md`.
72. Line 24 — broken link `with-network-system.md`.
73. Line 25 — broken link `with-database-system.md`.
74. Line 150 — broken link `../../../ECOSYSTEM.md`. Resolves outside the repository.

### Phase 1 — Broken anchors (12)

75. `docs/advanced/API_REFERENCE.kr.md:55` — intra-file anchor `#formatter-macros` not found (heading is `### formatter_macros`, slug uses underscore).
76. `docs/advanced/API_REFERENCE.md:58` — identical issue.
77. `docs/advanced/FAQ.md:45` — anchor `#worker-thread-pattern` not found in `docs/advanced/PATTERNS.md`. Actual heading is `### 1. Worker Thread Pattern` (slug `1-worker-thread-pattern`).
78. `docs/advanced/FAQ.md:85` — anchor `#type-based-job-execution-pattern` not found in `docs/advanced/PATTERNS.md` (heading prefixed `### 3.`).
79. `docs/advanced/faq.kr.md:45` — same as #77.
80. `docs/advanced/faq.kr.md:85` — same as #78.
81. `docs/advanced/PERFORMANCE.kr.md:26` — intra-file anchor `#typed-lock-free-thread-pool-benchmark` has no matching heading in the document.
82. `docs/guides/FAQ.md:26` — intra-file anchor `#thread-pool-basics` has no matching heading (TOC references sections that were never written).
83. `docs/guides/FAQ.md:27` — anchor `#job-scheduling` missing.
84. `docs/guides/FAQ.md:28` — anchor `#synchronization` missing.
85. `docs/guides/FAQ.md:29` — anchor `#performance` missing.
86. `docs/guides/FAQ.md:30` — anchor `#integration` missing.

### Phase 2 — TBD placeholders in SSOT document

87. `docs/performance/BASELINE.md:56` — benchmark cell `| Mean | TBD | μs | Average submission time |`. Baseline document is authoritative; unfilled numerics make it unusable.
88. `docs/performance/BASELINE.md:57–60` — `Median`, `P95`, `P99`, `Min` all `TBD`.
89. `docs/performance/BASELINE.md` (22 total TBD rows across submission-latency and throughput tables). Reported as three representative Must-Fix items; remaining 19 tracked as Should-Fix below.

---

## Should-Fix Items

### Phase 2 — Factual / Version References (15)

These references are still valid as *fallback* mentions, but appear in contexts where a stronger current-baseline statement would reduce ambiguity.

- `CLAUDE.md:93` — compiler baseline mentions "MSVC 2022+". Valid but consider pinning MSVC 17.x explicitly to align with `README.md:350`.
- `CONTRIBUTING.md:17` — "GCC 13+, Clang 16+" conflicts with `README.md:57` which says "GCC 13+ / Clang 17+". Choose one authoritative baseline.
- `README.kr.md:77` vs `README.md:57` vs `CLAUDE.md:93` — Clang minimum varies: `16+`, `17+` across documents; unify.
- `docs/GETTING_STARTED.md:15` — MSVC row shows `2022 (17.0)` minimum, conflicting with `README.md:350` which says `17.4+`.
- `docs/PRODUCTION_QUALITY.kr.md:368` — "MSVC 2019, 2022 / MSYS2 GCC 11, 12" — out-of-date legacy reference; should note this section describes prior validation only.
- `docs/PRODUCTION_QUALITY.md:747` — "MSVC 2019 (v142)" — stale, conflicts with current baseline.
- `docs/PROJECT_STRUCTURE.md:518` — `C++17/20 compatibility` section; no longer applies since `C++20 required` per `CLAUDE.md:93`.
- `docs/PROJECT_STRUCTURE.kr.md:532` — "GCC 9+, Clang 10+, MSVC 2019+" contradicts current C++20 baseline.
- `docs/PROJECT_STRUCTURE.md:916` — same contradictions as above.
- `docs/ARCHITECTURE.kr.md:457`, `docs/ARCHITECTURE.md:468` — "Execution policy integration (C++17)" roadmap item; confirm still targeted or remove.
- `docs/advanced/CPP20_CONCEPTS.kr.md:26–220` (multiple) — heavy C++17 fallback documentation appropriate for its topic; flag only if dropping C++17 fallback support.
- `docs/advanced/CPP20_CONCEPTS.md:26–220` — same.

### Phase 2 — TBD placeholders (19 remaining)

All in `docs/performance/BASELINE.md`:
- Line 56–60 (submission latency table — first 5 flagged as Must-Fix above).
- Lines 64–73 (throughput table: Mean, Median, P95, P99, Min, Max cells all TBD).
- Lines 77–86 (memory/queue-depth rows, further TBD cells).

Taken together, the SSOT baseline file currently carries no benchmark numbers; its "Releasable" status should be re-evaluated.

### Phase 2 — Terminology Consistency (27)

Project-wide preferred form is `lock-free` (hyphenated) and `thread pool` (two words, space). Deviations:

- `lockfree` (one word) used 27 times across 14 files, most frequently:
  - `docs/POLICY_QUEUE_GUIDE.md` (4 occurrences)
  - `docs/TRACEABILITY.md` (3 occurrences)
  - `docs/design/QUEUE_POLICY_DESIGN.md` (3 occurrences)
  - `CLAUDE.md` (2 occurrences)
  - `docs/API_QUICK_REFERENCE.md`, `docs/CHANGELOG.md`, `docs/CHANGELOG.kr.md`, `docs/advanced/KNOWN_ISSUES.md`, `docs/advanced/MIGRATION.md` (2 each)
  - `docs/API_REFERENCE.md`, `docs/API_REFERENCE.kr.md`, `docs/advanced/MIGRATION.kr.md`, `docs/advanced/QUEUE_SELECTION_GUIDE.md`, `docs/design/QUEUE_MIGRATION_GUIDE.md` (1 each)

- `thread-pool` (hyphenated) used 19 times across 13 files: `docs/advanced/API_REFERENCE.md` (3), `docs/advanced/API_REFERENCE.kr.md` (3), `docs/BENCHMARKS.md` (2), `docs/FEATURES.md` (2), and others (1 each).

- `threadpool` (one word) used 1 time — likely a typo.

### Phase 3 — Orphan Documents (23)

These files are not linked from any other markdown document in the repository. Many are valuable references that should be surfaced in an index:

- `README_KO_UPDATE_NOTES.md`
- `docs/ECOSYSTEM.md`
- `docs/GETTING_STARTED.md`
- `docs/SOUP.md`
- `docs/TRACEABILITY.md`
- `docs/adr/ADR-002-typed-pool-evaluation.md`
- `docs/adr/ADR-003-queue-architecture-selection.md`
- `docs/advanced/CI_CD_PERFORMANCE.md`
- `docs/advanced/CPP20_CONCEPTS.md`
- `docs/advanced/HAZARD_POINTER_DESIGN.md`
- `docs/advanced/JOB_CANCELLATION.md`
- `docs/advanced/KNOWN_ISSUES.md`
- `docs/advanced/QUEUE_SELECTION_GUIDE.md`
- `docs/advanced/STRUCTURE.md`
- `docs/advanced/faq.kr.md`
- `docs/guides/COVERAGE_GUIDE.md`
- `docs/guides/DEPENDENCY_COMPATIBILITY_MATRIX.md`
- `docs/guides/LICENSE_COMPATIBILITY.md`
- `docs/guides/LOGGER_INTERFACE_MIGRATION_GUIDE.md`
- `docs/guides/LOG_LEVEL_MIGRATION_GUIDE.md`
- `docs/guides/QUICK_START.md`
- `docs/guides/THREAD_POOL_API_MIGRATION_GUIDE.md`
- `docs/metrics_audit.md`

Recommend cataloguing these in `docs/README.md` or the top-level `README.md` "Documentation" section.

---

## Nice-to-Have Items

### Phase 3 — SSOT Redundancy / Architecture Doc Fragmentation (7)

Multiple documents each describe overlapping aspects of system architecture without an explicit delegation; readers may encounter contradictory material. None currently contradict each other explicitly, but consolidation would reduce maintenance load:

1. `docs/ARCHITECTURE.md` (515 lines) claims SSOT for "Thread System Architecture".
2. `docs/ARCHITECTURE_OVERVIEW.md` (228 lines) — covers System Architecture Overview, Core Module Hierarchy, Threading & Job Execution Flow.
3. `docs/ARCHITECTURE_DETAILS.md` (457 lines) — covers Queue, Type-Based, Hazard Pointer, Cancellation, Error architecture details.
4. `docs/ARCHITECTURE_DIAGRAM.md` (36 lines) — visual-only index pointing at overview/details.
5. `docs/advanced/ARCHITECTURE.md` (287 lines) — SSOT claim for "Threading Ecosystem Architecture" (cross-project scope).
6. `docs/advanced/THREAD_SYSTEM_ARCHITECTURE.md` (859 lines) — parallel long-form architecture reference.
7. Similar fragmentation exists for benchmarks: `docs/BENCHMARKS.md` (649L) + `docs/advanced/PERFORMANCE_BENCHMARKS.md` (820L) + `docs/advanced/PERFORMANCE.md` (43L, stub) + `docs/advanced/PERFORMANCE.kr.md` (1317L!) + `docs/performance/BASELINE.md` — five documents covering benchmarks with no single SSOT declaration.

Recommendation: Add explicit `> **See also (authoritative source):** ...` pointers where content overlaps, and move long-form technical material under one canonical tree.

---

## Score

- **Overall**: **6.2 / 10**
- **Anchors (Phase 1)**: **5.5 / 10** — 86 broken links across 28 files; `docs/advanced/README*.md` is almost entirely broken (14 links each), `docs/integration/README.md` points at pages that don't exist yet, and 5 TOC anchors in `docs/guides/FAQ.md` reference never-written sections.
- **Accuracy (Phase 2)**: **6.8 / 10** — C++ compiler baselines disagree across `README.md`, `CLAUDE.md`, `CONTRIBUTING.md`, and `PROJECT_STRUCTURE.md`. Authoritative performance baseline (`BASELINE.md`) carries 22 TBD cells. Terminology drift on `lock-free`/`lockfree` is the dominant minor-consistency issue.
- **SSOT (Phase 3)**: **6.5 / 10** — No hard contradictions between SSOT-declared docs, but architecture-related content is spread across 8 files (none designated as the single index); benchmark coverage is spread across 5 files; 23 documents are orphaned with no inbound references.

---

## Notes

### Recurring Patterns (most severe → least)

1. **Wrong relative depth in `docs/advanced/` links** — many files in `docs/advanced/` link with paths that assume a flat `docs/` layout (e.g., `BUILD_GUIDE.md` instead of `../guides/BUILD_GUIDE.md`, or `docs/API_REFERENCE.md` instead of `../API_REFERENCE.md`). Accounts for roughly half of all broken file links (42+ of 74). Concentrated in `docs/advanced/README*.md`, `docs/advanced/INTEGRATION.md`, `docs/advanced/THREAD_SYSTEM_ARCHITECTURE.md`, `docs/advanced/CHANGELOG*.md`.

2. **Renamed-file or reorganisation debris** — Links point at numbered-prefix files (`01-ARCHITECTURE.md`, `02-API_REFERENCE.md`), and at `DESIGN_IMPROVEMENTS.md`, `PLATFORM_BUILD_GUIDE.md`, `LOG_LEVEL_UNIFICATION_PLAN.md` that are not in the tree. Hints at a past refactor whose link-cleanup pass was incomplete.

3. **Missing Korean counterparts** — `docs/QUEUE_BACKWARD_COMPATIBILITY.md`, `docs/advanced/INTEGRATION.md`, `docs/advanced/QUEUE_SELECTION_GUIDE.md` all advertise Korean versions that do not exist. Either create stubs or remove the "한국어" chips.

4. **SSOT claim without delegation from peer docs** — Many documents carry `> **SSOT**: This document is the single source of truth for X` but related docs elsewhere don't cross-reference them. E.g., `docs/advanced/ARCHITECTURE.md` (ecosystem SSOT) is not linked from `docs/ARCHITECTURE.md` (thread_system SSOT) despite overlapping scope.

5. **TOC-vs-headings drift in FAQs** — `docs/guides/FAQ.md` TOC lists 6 sections; only 1 matches an actual heading. Similar TOC drift found in `docs/advanced/PERFORMANCE.kr.md` (line 26). Likely a template that was imported but not filled.

### System-Specific Observations

- The repository has a **dual structure** — `docs/` and `docs/advanced/` — with partial duplication (ARCHITECTURE, API_REFERENCE, CHANGELOG, FAQ, README exist in both). Neither hierarchy explicitly designates the other as authoritative. A top-level `docs/README.md` index covers most of the `docs/` layer but not the advanced layer.
- Bilingual documentation is mostly complete, but 3 SSOT docs lack a `.kr.md` twin and 1 Korean doc (`docs/advanced/faq.kr.md`) has lowercase filename inconsistent with the project convention (`FAQ.md` capitalization).
- `docs/performance/BASELINE.md` is marked "Released" in its front-matter (`doc_status: "Released"`) yet contains only TBD numeric cells — the status-vs-content mismatch is the single most impactful accuracy issue.
- `docs/advanced/` has 14 orphan documents; the README for that directory is itself one of the most broken files in the repo (14 dead links each in en/kr).
- The repository enforces that Korean `.kr.md` files are legitimate translations of their English counterparts and the anchor-slug algorithm handles Korean headings correctly (Unicode letters preserved through slugification).

### Recommendations (prioritised)

1. Run a bulk-replace in `docs/advanced/README*.md` to prefix relative paths correctly (`../guides/...`, `../../CONTRIBUTING.md`). This single batch fix resolves ~30 Must-Fix items.
2. Fill `docs/performance/BASELINE.md` with actual benchmark numbers or downgrade `doc_status` to `Draft`.
3. Write the 5 missing FAQ sections in `docs/guides/FAQ.md` or remove the TOC entries.
4. Unify C++ compiler baseline into one single-source table (suggest `CONTRIBUTING.md`) and have other docs link to it rather than restate.
5. Create a `docs/INDEX.md` (or extend `docs/README.md`) that links every doc currently orphaned, eliminating the "23 orphans" finding without touching content.
6. For architecture docs — declare `docs/advanced/THREAD_SYSTEM_ARCHITECTURE.md` (or one chosen file) as canonical; mark the others as scoped views with `> Scope: X (authoritative: Y)` banners.

---

## Post-Fix Re-Validation (2026-04-15)

**Fix commit**: `ca09c3dd` — "docs: fix 74 broken links, 5 anchors, 4 version refs"
**Re-run scope**: Phase 1 only (anchors + inter-file links).
**Method**: Python validator replays the Phase 1 rules (GitHub-style slugification with duplicate suffixes, fenced-code and HTML-comment skipping, external-URL exclusion) against both the pre-fix commit (`2b210572`) and the fix commit (`ca09c3dd`); classifies each current break as Residual (existed pre-fix) or Regression (new since fix).
**Files scanned**: 111 markdown files (excluding `build*/`, `.git/`, `node_modules/`, `vcpkg_installed/`).

### Before / After

| Metric | Pre-fix (`2b210572`) | Post-fix (`ca09c3dd`) | Delta |
|--------|-----------|------------|-------|
| Broken intra-file anchors | 15 | 13 | -2 |
| Broken inter-file links | 73 | 19 | -54 |
| Total Phase-1 Must-Fix (validator-observed) | 88 | 32 | -56 |
| Fixed by this commit | — | 56 | — |
| Residual (still broken, pre-existing) | — | 32 | — |
| Regressions (broken, new since fix) | — | **0** | — |

> The prior report enumerated 86 Phase-1 Must-Fix items; the validator now observes 88 pre-fix breaks. The +2 delta comes from items the prior enumeration missed (e.g., `docs/advanced/API_REFERENCE.{md,kr.md}` `#resultt-template`, `docs/advanced/MIGRATION.kr.md` Korean phase-heading anchors, `docs/ARCHITECTURE_DIAGRAM.md:27` `#3-threading--job-execution-flow`). These are pre-existing defects, not regressions.

### Residual Must-Fix (32)

Intra-file anchor residuals (13):

1. `docs/advanced/API_REFERENCE.kr.md:26` — `#resultt-template` (heading is `### result<T> Template`; slug is `#resultt-template` only if `<` / `>` are stripped — actual slug generated is `resultt-template` which MATCHES; however the validator finds no match because the actual heading slug currently renders as `result-template`; TOC entry is inconsistent with heading normalization — prior report did not enumerate this).
2. `docs/advanced/API_REFERENCE.md:29` — same as #1.
3. `docs/advanced/MIGRATION.kr.md:22` — `#phase-1-interface-추출-및-정리--완료` (Korean heading with `✅` emoji; GitHub slug drops emoji then collapses double hyphens differently).
4. `docs/advanced/MIGRATION.kr.md:23` — `#phase-2-새로운-repository-구조-생성--완료`.
5. `docs/advanced/MIGRATION.kr.md:24` — `#phase-3-component-마이그레이션--완료`.
6. `docs/advanced/MIGRATION.kr.md:25` — `#phase-4-통합-테스트--완료`.
7. `docs/advanced/MIGRATION.kr.md:27` — `#phase-5-점진적-배포--대기-중`.
8. `docs/advanced/PERFORMANCE.kr.md:27` — `#typed-lock-free-thread-pool-benchmark` (TOC drift; fix commit added TODO marker at line 26 only).
9. `docs/guides/FAQ.md:27` — `#thread-pool-basics`.
10. `docs/guides/FAQ.md:28` — `#job-scheduling`.
11. `docs/guides/FAQ.md:29` — `#synchronization`.
12. `docs/guides/FAQ.md:30` — `#performance`.
13. `docs/guides/FAQ.md:31` — `#integration`.

Inter-file link residuals (19):

14. `docs/ARCHITECTURE_DIAGRAM.md:27` — `ARCHITECTURE_OVERVIEW.md#3-threading--job-execution-flow` (anchor does not exist in target; actual heading slug is `3-threading-job-execution-flow` — single hyphen).
15. `docs/QUEUE_BACKWARD_COMPATIBILITY.md:15` — `QUEUE_BACKWARD_COMPATIBILITY.kr.md` (Korean counterpart still missing).
16. `docs/advanced/INTEGRATION.md:15` — `INTEGRATION.kr.md` (Korean counterpart still missing).
17. `docs/advanced/QUEUE_SELECTION_GUIDE.md:15` — `QUEUE_SELECTION_GUIDE.kr.md` (Korean counterpart still missing).
18. `docs/advanced/README.kr.md:49` — `DESIGN_IMPROVEMENTS.kr.md` (target file does not exist).
19. `docs/advanced/README.kr.md:69` — `DESIGN_IMPROVEMENTS.kr.md`.
20. `docs/advanced/README.md:49` — `DESIGN_IMPROVEMENTS.md` (target file does not exist).
21. `docs/advanced/README.md:69` — `DESIGN_IMPROVEMENTS.md`.
22. `docs/advanced/USER_GUIDE.kr.md:52` — `./PLATFORM_BUILD_GUIDE.md` (target file does not exist).
23. `docs/advanced/USER_GUIDE.md:52` — `./PLATFORM_BUILD_GUIDE.md`.
24. `docs/guides/LOG_LEVEL_MIGRATION_GUIDE.kr.md:296` — `../advanced/LOG_LEVEL_UNIFICATION_PLAN.md` (target file does not exist).
25. `docs/guides/LOG_LEVEL_MIGRATION_GUIDE.kr.md:298` — `../../../logger_system/docs/advanced/LOG_LEVEL_SEMANTIC_STANDARD.md` (resolves outside repo).
26. `docs/guides/LOG_LEVEL_MIGRATION_GUIDE.md:296` — `../advanced/LOG_LEVEL_UNIFICATION_PLAN.md`.
27. `docs/guides/LOG_LEVEL_MIGRATION_GUIDE.md:298` — `../../../logger_system/docs/advanced/LOG_LEVEL_SEMANTIC_STANDARD.md`.
28. `docs/integration/README.md:22` — `with-common-system.md` (integration sub-guide missing).
29. `docs/integration/README.md:23` — `with-logger.md`.
30. `docs/integration/README.md:24` — `with-monitoring.md`.
31. `docs/integration/README.md:25` — `with-network-system.md`.
32. `docs/integration/README.md:26` — `with-database-system.md`.

### Regressions (0)

No Phase-1 regressions detected. Every post-fix broken anchor or link was verified to also exist in the pre-fix tree (`2b210572`).

### Verdict

**PARTIAL-PASS** — 0 regressions, but 32 residual Phase-1 Must-Fix items remain. The fix commit resolved 56 of 88 validator-observed broken references (64%). Remaining work is concentrated in three areas: (a) missing target files that the fix marked with `<!-- TODO -->` markers rather than deleting/creating (`DESIGN_IMPROVEMENTS*`, `PLATFORM_BUILD_GUIDE.md`, `LOG_LEVEL_UNIFICATION_PLAN.md`, `docs/integration/with-*.md`, Korean counterparts); (b) FAQ TOC drift in `docs/guides/FAQ.md` and `docs/advanced/PERFORMANCE.kr.md`; (c) anchor-slug mismatches in `docs/advanced/API_REFERENCE*.md`, `docs/advanced/MIGRATION.kr.md`, and `docs/ARCHITECTURE_DIAGRAM.md`.
