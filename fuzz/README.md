# Fuzzing

libFuzzer-based fuzz harnesses for thread_system's lock-free / concurrent queue
and resilience paths.

## Targets

| Target | Area | Source |
|--------|------|--------|
| `concurrent_queue_fuzzer` | The thread-safe queue reachable through the lock-free compatibility include path (`kcenon::thread::detail::concurrent_queue`, the backing for `<kcenon/thread/lockfree/lockfree_queue.h>`): enqueue (copy/move), `try_dequeue` including the empty path, `wait_dequeue(timeout)`, `empty()`/`size()` queries, `shutdown()`/`is_shutdown()` resilience signalling, and FIFO round-trip integrity. | `concurrent_queue_fuzzer.cpp` |

The harness encodes the fuzzer input as an opcode stream: each byte drives one
queue operation (opcode in the high 3 bits, payload in the low 5 bits).

## Requirements

- Clang with the fuzzer and address sanitizers (`-fsanitize=fuzzer,address`).
  GCC is not supported for these targets — the CMake config skips them with a
  warning under non-Clang compilers.

The targets are header-only: they include the queue headers directly and do not
need the full thread_system library link or vcpkg.

## Building and running locally

```bash
cmake -B build-fuzz -G Ninja \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DBUILD_FUZZERS=ON \
  -DTHREAD_BUILD_INTEGRATION_TESTS=OFF \
  -DBUILD_DOCUMENTATION=OFF
cmake --build build-fuzz

# Run for a bounded time, seeded from the checked-in corpus:
./build-fuzz/bin/concurrent_queue_fuzzer -max_total_time=60 fuzz/corpus
```

A crash reproducer (if any) is written to the working directory as
`crash-<hash>`; replay it with:

```bash
./build-fuzz/bin/concurrent_queue_fuzzer crash-<hash>
```

## Seed corpus

`corpus/` contains a few hand-crafted seeds that exercise the major code paths
(enqueue/dequeue round-trips, fill-then-drain, and query/shutdown mixes). Add
interesting inputs discovered during fuzzing here.

## CI

`.github/workflows/fuzzing.yml` builds these targets with Clang+libFuzzer and
runs them for a short bounded time on a weekly schedule and on manual dispatch.
The targets are excluded from the default build and from the regular CI matrix
(`BUILD_FUZZERS` defaults to `OFF`).

## Follow-ups (issue #697)

This PR adds the harness scaffolding only. Still to do under the same issue:

- Phased line-coverage raise (40% -> 50% -> 60%) with the codecov floor bumped
  accordingly.
- A dedicated target for the true lock-free `kcenon::thread::lockfree_job_queue`
  (Michael-Scott + hazard pointers). That one needs the full library link
  (`src/lockfree`, `job`, and the common_system `result<T>` chain), so it is
  deferred until the harness can link the library cleanly in CI.
