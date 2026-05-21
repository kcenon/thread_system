---
doc_id: "THR-PROJ-015"
doc_title: "CI Verification Gates"
doc_version: "1.0.0"
doc_date: "2026-05-21"
doc_status: "Released"
project: "thread_system"
category: "PROJ"
---

# CI Verification Gates

> **SSOT**: This document is the single source of truth for the **CI verification gate
> matrix** of Thread System — which sanitizer, stress, and integration checks exist,
> when they run, and which of them block a release.

This document inventories the verification gates that protect Thread System and its
downstream consumers. It does not introduce new CI; every gate referenced here already
exists as a workflow under `.github/workflows/`. The purpose is to give contributors
and reviewers a single map of *what runs, when, and what a failure means*.

## Gate Matrix

| Gate | Tool | Workflow file | Trigger | Scope | Release-blocking |
|------|------|---------------|---------|-------|------------------|
| ASan | AddressSanitizer (`-fsanitize=address`) | `.github/workflows/ci.yml` (`sanitizer` job, matrix `address`) | `push` to `main`/`phase-*`, `pull_request` to `main` | Unit + integration tests, Debug build, clang + libc++ | Yes |
| TSan | ThreadSanitizer (`-fsanitize=thread`) | `.github/workflows/ci.yml` (`sanitizer` job, matrix `thread`) | `push` to `main`/`phase-*`, `pull_request` to `main` | Unit + integration tests, Debug build, clang + libc++ | Yes |
| UBSan | UndefinedBehaviorSanitizer (`-fsanitize=undefined`) | `.github/workflows/ci.yml` (`sanitizer` job, matrix `undefined`) | `push` to `main`/`phase-*`, `pull_request` to `main` | Unit + integration tests, Debug build, clang + libc++ | Yes |
| Stress | Custom sustained/burst/memory stress scenarios | `.github/workflows/stress-tests.yml` | Nightly `schedule` (02:00 UTC), `workflow_dispatch` | Long-running concurrency stress, configurable duration/scenarios | No (nightly signal) |
| Integration | GoogleTest integration suite (`BUILD_INTEGRATION_TESTS=ON`) | `.github/workflows/integration-tests.yml` | `push` to `main`, `pull_request` to `main`, `workflow_dispatch` | Cross-component integration, Debug + Release matrix | Yes |
| Valgrind | Valgrind memcheck | `.github/workflows/valgrind.yml` | `push` to `main`, `pull_request` to `main`, nightly `schedule`, `workflow_dispatch` | Memory leak / invalid access detection | Yes |
| Static analysis | clang-tidy | `.github/workflows/static-analysis.yml` | `push` to `main`/`phase-*`, `pull_request` to `main` | Static lint of source headers/sources | Yes |
| Coverage | gcov / lcov | `.github/workflows/coverage.yml` | `push` to `main`/`phase-*`, `pull_request` to `main` | Line/function coverage thresholds (see workflow `env`) | Yes |
| Performance | Google Benchmark suite | `.github/workflows/performance-benchmarks.yml` | `push` to `main`/`phase-*`, `pull_request` to `main`, nightly `schedule`, `workflow_dispatch` | Throughput/latency regression tracking | No (trend signal) |

> All file paths above are relative to the repository root and match the workflow
> definitions verbatim. If a workflow trigger changes, update this table in the same PR.

## Release Branch Gate Set

Thread System follows a `main` / `develop` branch model. **CI runs only on pull
requests targeting `main`** — the workflows above are scoped with
`pull_request: branches: [ main ]`. Pull requests targeting `develop` (the default
integration branch) do **not** trigger these gates. Verification therefore
concentrates at the `develop` → `main` release PR.

A release pull request into `main` must pass the following minimum gate set before
merge:

- **ASan** — no memory errors.
- **TSan** — no data races (tolerated known flakes excluded; see below).
- **UBSan** — no undefined behavior.
- **Integration** — full integration suite green on Debug and Release.
- **Valgrind** — no leaks or invalid memory access.
- **Static analysis** — clang-tidy clean.
- **Coverage** — line/function coverage at or above the workflow thresholds.

Stress and performance gates are *trend signals*, not release blockers: they run on a
nightly schedule and on demand. A reviewer preparing a release should check the most
recent nightly stress and benchmark runs but is not blocked by them.

## Downstream Consumer Verification

Thread System is consumed by `network_system` and `pacs_system`. Thread System CI does
not trigger downstream workflows automatically; consumer verification is a documented
manual step performed when a change to Thread System could affect a consumer (public
API, threading semantics, or build interface).

Each consumer repository carries its own sanitizer and integration workflows:

| Consumer | Sanitizer workflow | Integration workflow |
|----------|--------------------|----------------------|
| `network_system` | `.github/workflows/sanitizers.yml` | `.github/workflows/integration-tests.yml` |
| `pacs_system` | `.github/workflows/sanitizers.yml` | `.github/workflows/integration-tests.yml` |

To verify a consumer against a Thread System change:

```bash
# Remote: trigger the consumer's gates on its default branch
gh workflow run sanitizers.yml          --repo kcenon/network_system
gh workflow run integration-tests.yml   --repo kcenon/network_system
gh workflow run sanitizers.yml          --repo kcenon/pacs_system
gh workflow run integration-tests.yml   --repo kcenon/pacs_system

# Local: build the consumer with sanitizers enabled
cmake --preset asan  && cmake --build --preset asan  && ctest --preset asan
cmake --preset tsan  && cmake --build --preset tsan  && ctest --preset tsan
cmake --preset ubsan && cmake --build --preset ubsan && ctest --preset ubsan
```

Workflow references:
- network_system: <https://github.com/kcenon/network_system/blob/main/.github/workflows/sanitizers.yml>
- network_system: <https://github.com/kcenon/network_system/blob/main/.github/workflows/integration-tests.yml>
- pacs_system: <https://github.com/kcenon/pacs_system/blob/main/.github/workflows/sanitizers.yml>
- pacs_system: <https://github.com/kcenon/pacs_system/blob/main/.github/workflows/integration-tests.yml>

## Tolerated Warnings / Known Flakes

The TSan gate (`ci.yml` `sanitizer` job, matrix `thread`) excludes a fixed set of tests
via a `--gtest_filter` exclusion. These exclusions are intentional and documented inline
in `ci.yml`; they are reproduced here so the rationale is discoverable without reading
the workflow.

| Excluded test pattern | Reason |
|-----------------------|--------|
| `DataIntegrityDuringModeSwitch`, `DataIntegrityWithMultipleProducersConsumers` | Hang under TSan due to extreme instrumentation overhead on concurrent lock-free / mutex mode-switching loops. |
| `ConcurrentEnableDisable` | Non-atomic write to `policy_.enable_work_stealing` — a known pre-existing issue, not introduced by sanitizer instrumentation. |
| `LinkedWithTimeoutParentCancel` | TSan false positive. libc++ `weak_ptr::lock()` uses `memory_order_relaxed` on the failure path, breaking TSan's happens-before tracking; actual synchronization is established via `__weak_count_` acquire/release operations. |
| `ExecutorAdapterTest.SubmitExecutesTask`, `.SubmitMultipleTasks`, `.ExecuteRunsJob`, `.ExecuteFailingJobReportsError`, `.FactoryAdapterExecutesJob` | libc++ TSan false positive around `std::promise` / `std::future` shared-state teardown. The worker thread releases the last `shared_ptr<promise<void>>` after fulfilling the future while the main thread is still inside `future::get()`; libc++ synchronizes this correctly but TSan reports a spurious race on the promise destruction path. |

Rules for this exclusion list:

- The list is **frozen** — do not add entries to silence a *new* race. A new TSan
  failure must be triaged and fixed, not filtered.
- Removing an entry is encouraged once the underlying flake is fixed (e.g. an
  `std::atomic` correction for `ConcurrentEnableDisable`) or once a libc++ upgrade
  resolves a false positive.
- Any change to the filter must update both `ci.yml` and the table above in the same PR.

ASan and UBSan run the full test set with no exclusions. The sanitizer runtime options
applied by the job are `ASAN_OPTIONS=detect_leaks=1:alloc_dealloc_mismatch=0`,
`UBSAN_OPTIONS=print_stacktrace=1`, and `TSAN_OPTIONS=second_deadlock_stack=1`.

## Retry Policy & Failure Triage

The `sanitizer` job uses `strategy.fail-fast: false`, so a failure in one sanitizer
matrix entry does not cancel the others — ASan, TSan, and UBSan results are always all
reported. Triage a sanitizer failure by category before retrying:

1. **Compilation error** — deterministic. Never retry; fix the source. A sanitizer
   build failing to compile is always a real defect (or a missing dependency).
2. **Runtime race / memory error reported by the sanitizer** — treat as a real defect
   by default. Reproduce locally with the matching preset (`cmake --preset tsan` etc.)
   before concluding it is a flake. Do not retry to make it pass.
3. **Known flake** — only if the failing test matches an entry in the table above. The
   filter should already exclude it; if a known-flake test still fails, the filter is
   stale and must be corrected.
4. **Infrastructure failure** (runner timeout, apt package conflict, network) —
   transient. Re-run the failed job once. If it fails a second time, escalate rather
   than re-running blindly.

Sanitizer artifacts (`build/Testing/Temporary/`, `build/**/*.log`) are uploaded on
`always()` under the name `sanitizer-<sanitizer>-results` — download these first when
triaging a remote failure.

## Missing Gates / Follow-up

Thread System's own verification gates are fully wired: ASan, TSan, UBSan, stress,
integration, valgrind, static analysis, coverage, and performance benchmarks all exist
as workflows and are listed above. There is no missing *self* gate.

The one known gap is **automated downstream verification**. Thread System CI does not
currently trigger `network_system` or `pacs_system` workflows when a Thread System
change lands. Today this is covered by the documented manual procedure in the
*Downstream Consumer Verification* section. A follow-up candidate is to add a
`workflow_dispatch`-based fan-out (or a scheduled consumer-matrix job) so that a
release PR to `main` automatically exercises downstream consumers. This is tracked as a
follow-up rather than implemented here, to keep this change documentation-only.

---

*Last Updated: 2026-05-21*
