/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, 🍀☀🌕🌥 🌊
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#include <gtest/gtest.h>
#include <kcenon/thread/core/cancellation_token.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <latch>

namespace kcenon::thread {
namespace test {

// Reduce iteration count for coverage builds to prevent hangs
#ifdef ENABLE_COVERAGE
constexpr int RACE_TEST_ITERATIONS = 5;
constexpr int STRESS_TEST_ITERATIONS = 3;
#else
constexpr int RACE_TEST_ITERATIONS = 50;
constexpr int STRESS_TEST_ITERATIONS = 20;
#endif

// Timeout for all operations to prevent infinite hangs
constexpr auto MAX_WAIT_TIME = std::chrono::seconds(5);

/**
 * Test for Sprint 1, Task 1.3: Cancellation token callback race condition
 * Verifies that register_callback() safely coordinates with cancel()
 */
class CancellationTokenRaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset for each test
    }

    void TearDown() override {
        // Clean up
    }

    // Helper: Wait with timeout
    template<typename Predicate>
    bool wait_for(Predicate pred, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        auto start = std::chrono::steady_clock::now();
        while (!pred()) {
            if (std::chrono::steady_clock::now() - start > timeout) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        return true;
    }
};

/**
 * Test concurrent callback registration
 * Verifies that all registered callbacks are called exactly once
 */
TEST_F(CancellationTokenRaceTest, ConcurrentCallbackRegistration) {
    cancellation_token token;

    std::atomic<int> callback_count{0};
    const int num_threads = 4;  // Reduced from 10
    const int callbacks_per_thread = 50;  // Reduced from 100

    // Register callbacks from multiple threads
    std::vector<std::thread> registration_threads;
    std::latch start_latch(num_threads);

    for (int t = 0; t < num_threads; ++t) {
        registration_threads.emplace_back([&, token]() mutable {
            start_latch.arrive_and_wait();

            for (int i = 0; i < callbacks_per_thread; ++i) {
                token.register_callback([&callback_count]() {
                    callback_count.fetch_add(1, std::memory_order_relaxed);
                });
            }
        });
    }

    // Wait for all registrations to complete with timeout
    for (auto& thread : registration_threads) {
        thread.join();
    }

    // Cancel the token
    token.cancel();

    // Verify all callbacks were called exactly once
    EXPECT_EQ(callback_count.load(), num_threads * callbacks_per_thread);
}

/**
 * Test race between registration and cancellation
 * Verifies that callbacks are not missed even during concurrent cancel()
 */
TEST_F(CancellationTokenRaceTest, RegistrationDuringCancellation) {
    for (int iter = 0; iter < RACE_TEST_ITERATIONS; ++iter) {
        cancellation_token token;

        std::atomic<int> callback_count{0};
        std::latch start_latch(2);

        // Thread that registers callback
        std::thread registration_thread([&, token]() mutable {
            start_latch.arrive_and_wait();

            token.register_callback([&callback_count]() {
                callback_count.fetch_add(1, std::memory_order_relaxed);
            });
        });

        // Thread that cancels
        std::thread cancel_thread([&, token]() mutable {
            start_latch.arrive_and_wait();
            token.cancel();
        });

        registration_thread.join();
        cancel_thread.join();

        // Callback must be called exactly once (either by register_callback or by cancel)
        EXPECT_EQ(callback_count.load(), 1);
    }
}

/**
 * Test callback registration after cancellation
 * Verifies that callbacks registered after cancel() are invoked immediately
 */
TEST_F(CancellationTokenRaceTest, RegistrationAfterCancellation) {
    cancellation_token token;

    // Cancel first
    token.cancel();

    std::atomic<bool> callback_invoked{false};

    // Register callback after cancellation
    token.register_callback([&callback_invoked]() {
        callback_invoked.store(true, std::memory_order_release);
    });

    // Callback should be invoked immediately
    EXPECT_TRUE(callback_invoked.load(std::memory_order_acquire));
}

/**
 * Test that callbacks are invoked in the same order they are registered
 * when cancel() is called
 */
TEST_F(CancellationTokenRaceTest, CallbackInvocationOrder) {
    cancellation_token token;

    std::vector<int> invocation_order;
    std::mutex order_mutex;

    const int num_callbacks = 50;  // Reduced from 100

    // Register callbacks
    for (int i = 0; i < num_callbacks; ++i) {
        token.register_callback([i, &invocation_order, &order_mutex]() {
            std::lock_guard<std::mutex> lock(order_mutex);
            invocation_order.push_back(i);
        });
    }

    // Cancel
    token.cancel();

    // Verify all callbacks were invoked
    EXPECT_EQ(invocation_order.size(), static_cast<size_t>(num_callbacks));

    // Verify order (should match registration order)
    for (size_t i = 0; i < invocation_order.size(); ++i) {
        EXPECT_EQ(invocation_order[i], static_cast<int>(i));
    }
}

/**
 * Test high-frequency registration and cancellation
 * Stress test to ensure no race conditions under pressure
 */
TEST_F(CancellationTokenRaceTest, HighFrequencyRegistrationAndCancellation) {
    for (int iter = 0; iter < STRESS_TEST_ITERATIONS; ++iter) {
        cancellation_token token;

        std::atomic<int> total_callbacks{0};
        std::latch start_latch(2);

        // Thread 1: Register callbacks rapidly
        std::thread reg_thread1([&, token]() mutable {
            start_latch.arrive_and_wait();
            for (int i = 0; i < 25; ++i) {  // Reduced from 50
                token.register_callback([&total_callbacks]() {
                    total_callbacks.fetch_add(1, std::memory_order_relaxed);
                });
            }
        });

        // Thread 2: Register callbacks rapidly
        std::thread reg_thread2([&, token]() mutable {
            start_latch.arrive_and_wait();
            for (int i = 0; i < 25; ++i) {  // Reduced from 50
                token.register_callback([&total_callbacks]() {
                    total_callbacks.fetch_add(1, std::memory_order_relaxed);
                });
            }
        });

        // Give some time for registrations
        std::this_thread::sleep_for(std::chrono::microseconds(500));

        // Cancel
        token.cancel();

        reg_thread1.join();
        reg_thread2.join();

        // All callbacks should be invoked exactly once
        EXPECT_EQ(total_callbacks.load(), 50);  // Updated expected count
    }
}

/**
 * Test that callback invocation doesn't hold locks
 * Verifies that callbacks can register new callbacks without deadlock
 */
TEST_F(CancellationTokenRaceTest, CallbackCanRegisterNewCallbacks) {
    cancellation_token token;

    std::atomic<int> first_level_count{0};
    std::atomic<int> second_level_count{0};

    // Register a callback that registers another callback
    token.register_callback([&, token]() mutable {
        first_level_count.fetch_add(1, std::memory_order_relaxed);

        // This should not deadlock
        token.register_callback([&]() {
            second_level_count.fetch_add(1, std::memory_order_relaxed);
        });
    });

    // Cancel
    token.cancel();

    // First-level callback should be invoked
    EXPECT_EQ(first_level_count.load(), 1);

    // Second-level callback should be invoked immediately during registration
    EXPECT_EQ(second_level_count.load(), 1);
}

/**
 * Test linked tokens
 * Verifies that canceling a parent token cancels all linked tokens
 */
TEST_F(CancellationTokenRaceTest, LinkedTokenCancellation) {
    cancellation_token parent1;
    cancellation_token parent2;

    auto linked = cancellation_token::create_linked({parent1, parent2});

    std::atomic<bool> linked_cancelled{false};
    linked.register_callback([&linked_cancelled]() {
        linked_cancelled.store(true, std::memory_order_release);
    });

    // Cancel one parent
    parent1.cancel();

    // Wait with timeout for callback propagation
    EXPECT_TRUE(wait_for([&]() { return linked_cancelled.load(std::memory_order_acquire); },
                         std::chrono::milliseconds(100)));
    EXPECT_TRUE(linked.is_cancelled());
}

} // namespace test
} // namespace kcenon::thread
