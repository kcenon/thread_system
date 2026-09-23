// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/**
 * @file concurrent_queue_fuzzer.cpp
 * @brief libFuzzer harness for the lock-free / resilience queue surface.
 *
 * This harness drives kcenon::thread::detail::concurrent_queue<T> with a
 * fuzzer-controlled opcode stream. concurrent_queue is the implementation that
 * backs the legacy <kcenon/thread/lockfree/lockfree_queue.h> include path, and
 * it is the queue's input-handling and resilience surface (enqueue/dequeue,
 * empty/size queries, the wait-with-timeout path, and shutdown signalling).
 *
 * The harness explores:
 *   - enqueue (copy and move overloads),
 *   - try_dequeue including the empty-queue resilience path,
 *   - wait_dequeue(timeout) with a tiny timeout (non-blocking probe),
 *   - empty()/size() state queries,
 *   - shutdown()/is_shutdown() resilience signalling,
 *   - FIFO round-trip integrity against a shadow model.
 *
 * It is intentionally single-threaded: libFuzzer requires deterministic replay,
 * so this targets input handling and state-machine edge cases rather than
 * multi-threaded interleavings. Concurrency stress remains the job of the unit
 * tests under tests/unit/lockfree_test/.
 *
 * The true lock-free queue (kcenon::thread::lockfree_job_queue, Michael-Scott +
 * hazard pointers) requires the full library link (src/impl/lockfree + job +
 * common_system result<T>); a dedicated target for it is tracked as a follow-up
 * under issue #697 (see fuzz/README.md).
 *
 * Build via the BUILD_FUZZERS CMake option (clang -fsanitize=fuzzer,address).
 */

// Silence the legacy-include deprecation pragma; we deliberately fuzz the
// queue reachable through the lock-free compatibility path.
#define THREAD_SUPPRESS_LEGACY_LOCKFREE_QUEUE_WARNING 1
#include <kcenon/thread/lockfree/lockfree_queue.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

using kcenon::thread::detail::concurrent_queue;

// Opcodes encoded in the high bits of each input byte; the low bits feed the
// payload (the value to enqueue), so a single byte fully drives one step.
enum class op : std::uint8_t {
    enqueue_copy = 0,
    enqueue_move = 1,
    try_dequeue = 2,
    wait_dequeue = 3,
    query = 4,
    shutdown = 5,
};

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    concurrent_queue<std::uint32_t> queue;

    // Shadow model of values enqueued but not yet dequeued. For a FIFO queue,
    // successful dequeues must return values in enqueue order, so we can assert
    // round-trip integrity on the dequeue paths.
    std::vector<std::uint32_t> shadow;
    bool shut = false;

    for (std::size_t i = 0; i < size; ++i) {
        const std::uint8_t byte = data[i];
        const auto code = static_cast<op>((byte >> 5) % 6u);
        const std::uint32_t payload = static_cast<std::uint32_t>(byte & 0x1Fu);

        switch (code) {
            case op::enqueue_copy: {
                queue.enqueue(payload);
                shadow.push_back(payload);
                break;
            }
            case op::enqueue_move: {
                std::uint32_t movable = payload;
                queue.enqueue(std::move(movable));
                shadow.push_back(payload);
                break;
            }
            case op::try_dequeue: {
                auto maybe = queue.try_dequeue();
                if (maybe.has_value()) {
                    if (shadow.empty() || *maybe != shadow.front()) {
                        __builtin_trap();  // FIFO integrity violation
                    }
                    shadow.erase(shadow.begin());
                }
                break;
            }
            case op::wait_dequeue: {
                // Tiny timeout so the harness stays deterministic and fast even
                // when the queue is empty (exercises the timeout resilience
                // path without actually blocking the fuzzer for long).
                auto maybe = queue.wait_dequeue(std::chrono::milliseconds(0));
                if (maybe.has_value()) {
                    if (shadow.empty() || *maybe != shadow.front()) {
                        __builtin_trap();
                    }
                    shadow.erase(shadow.begin());
                }
                break;
            }
            case op::query: {
                // Exercise the read-only surface; sink results so the optimizer
                // cannot elide the calls.
                volatile bool e = queue.empty();
                volatile std::size_t n = queue.size();
                volatile bool s = queue.is_shutdown();
                (void)e;
                (void)n;
                (void)s;
                break;
            }
            case op::shutdown: {
                queue.shutdown();
                shut = true;
                if (!queue.is_shutdown()) {
                    __builtin_trap();  // shutdown must be observable
                }
                break;
            }
        }
    }

    (void)shut;
    return 0;
}
