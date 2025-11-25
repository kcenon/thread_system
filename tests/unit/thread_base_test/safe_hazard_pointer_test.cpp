// BSD 3-Clause License
//
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <gtest/gtest.h>

#include "kcenon/thread/core/safe_hazard_pointer.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace kcenon::thread;

class SafeHazardPointerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Collect any lingering retired objects from previous tests
        safe_hazard_pointer_domain::instance().collect();
    }

    void TearDown() override {
        safe_hazard_pointer_domain::instance().collect();
    }
};

// TC-001: Basic protect/clear functionality
TEST_F(SafeHazardPointerTest, BasicProtectClear) {
    int value = 42;
    {
        safe_hazard_guard guard(&value);
        EXPECT_EQ(guard.get(), &value);
    }
    // Guard destroyed, protection released
}

// TC-002: Guard protects pointer during scope
TEST_F(SafeHazardPointerTest, GuardProtectsDuringScope) {
    int* ptr = new int(42);
    std::atomic<bool> deleted{false};

    {
        safe_hazard_guard guard(ptr);

        // Retire the pointer
        safe_hazard_pointer_domain::instance().retire(ptr, [&deleted](void* p) {
            deleted = true;
            delete static_cast<int*>(p);
        });

        // Try to collect - should NOT delete while protected
        safe_hazard_pointer_domain::instance().collect();
        EXPECT_FALSE(deleted.load());
    }

    // Guard destroyed, now collection should work
    safe_hazard_pointer_domain::instance().collect();
    EXPECT_TRUE(deleted.load());
}

// TC-003: Multiple guards on different pointers
TEST_F(SafeHazardPointerTest, MultipleGuards) {
    int* ptr1 = new int(1);
    int* ptr2 = new int(2);

    std::atomic<int> delete_count{0};

    {
        safe_hazard_guard guard1(ptr1);
        safe_hazard_guard guard2(ptr2, 1);  // Use slot 1

        safe_hazard_pointer_domain::instance().retire(ptr1, [&delete_count](void* p) {
            delete_count++;
            delete static_cast<int*>(p);
        });

        safe_hazard_pointer_domain::instance().retire(ptr2, [&delete_count](void* p) {
            delete_count++;
            delete static_cast<int*>(p);
        });

        safe_hazard_pointer_domain::instance().collect();
        EXPECT_EQ(delete_count.load(), 0);  // Both protected
    }

    safe_hazard_pointer_domain::instance().collect();
    EXPECT_EQ(delete_count.load(), 2);  // Both deleted
}

// TC-004: Move semantics
TEST_F(SafeHazardPointerTest, MoveSemantics) {
    int value = 42;
    safe_hazard_guard guard1(&value);
    EXPECT_EQ(guard1.get(), &value);

    // Move construct
    safe_hazard_guard guard2(std::move(guard1));
    EXPECT_EQ(guard1.get(), nullptr);
    EXPECT_EQ(guard2.get(), &value);

    // Move assign
    safe_hazard_guard guard3;
    guard3 = std::move(guard2);
    EXPECT_EQ(guard2.get(), nullptr);
    EXPECT_EQ(guard3.get(), &value);
}

// TC-005: Retire without protection
TEST_F(SafeHazardPointerTest, RetireWithoutProtection) {
    std::atomic<bool> deleted{false};
    int* ptr = new int(42);

    safe_hazard_pointer_domain::instance().retire(ptr, [&deleted](void* p) {
        deleted = true;
        delete static_cast<int*>(p);
    });

    safe_hazard_pointer_domain::instance().collect();
    EXPECT_TRUE(deleted.load());
}

// TC-006: safe_retire_hazard helper
TEST_F(SafeHazardPointerTest, SafeRetireHazardHelper) {
    struct TestObj {
        static std::atomic<int>& destructor_count() {
            static std::atomic<int> count{0};
            return count;
        }

        ~TestObj() {
            destructor_count()++;
        }
    };

    TestObj::destructor_count() = 0;

    TestObj* obj = new TestObj();
    safe_retire_hazard(obj);

    safe_hazard_pointer_domain::instance().collect();
    EXPECT_EQ(TestObj::destructor_count().load(), 1);
}

// TC-007: Concurrent protection and retirement
TEST_F(SafeHazardPointerTest, ConcurrentProtectionAndRetirement) {
    constexpr int NUM_ITERATIONS = 100;
    std::atomic<bool> stop{false};
    std::atomic<int> protected_accesses{0};

    // Shared data
    std::atomic<int*> shared_ptr{new int(0)};

    // Reader thread - protects and reads
    std::thread reader([&]() {
        while (!stop.load(std::memory_order_acquire)) {
            safe_hazard_guard guard;
            int* p = shared_ptr.load(std::memory_order_acquire);
            guard.protect(p);

            // Double-check pattern
            if (shared_ptr.load(std::memory_order_acquire) == p && p) {
                [[maybe_unused]] int val = *p;  // Safe access
                protected_accesses++;
            }
        }
    });

    // Writer thread - replaces and retires
    std::thread writer([&]() {
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            int* new_ptr = new int(i);
            int* old_ptr = shared_ptr.exchange(new_ptr, std::memory_order_acq_rel);
            if (old_ptr) {
                safe_retire_hazard(old_ptr);
            }
            std::this_thread::yield();
        }
    });

    writer.join();
    stop = true;
    reader.join();

    // Cleanup
    int* final_ptr = shared_ptr.load();
    if (final_ptr) {
        delete final_ptr;
    }
    safe_hazard_pointer_domain::instance().collect();

    EXPECT_GT(protected_accesses.load(), 0);
}

// TC-008: Multi-threaded stress test
TEST_F(SafeHazardPointerTest, MultiThreadedStress) {
    constexpr int NUM_THREADS = 4;
    constexpr int ITERATIONS = 500;

    std::atomic<int> retire_count{0};
    std::atomic<int> delete_count{0};

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < ITERATIONS; ++j) {
                int* ptr = new int(i * ITERATIONS + j);

                {
                    safe_hazard_guard guard(ptr);
                    std::this_thread::yield();
                }

                safe_hazard_pointer_domain::instance().retire(ptr, [&delete_count](void* p) {
                    delete_count++;
                    delete static_cast<int*>(p);
                });
                retire_count++;

                if (j % 50 == 0) {
                    safe_hazard_pointer_domain::instance().collect();
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Final collection
    for (int i = 0; i < 10; ++i) {
        safe_hazard_pointer_domain::instance().collect();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(retire_count.load(), NUM_THREADS * ITERATIONS);
    // All retired objects should eventually be deleted
    EXPECT_EQ(delete_count.load(), NUM_THREADS * ITERATIONS);
}

// TC-009: Statistics tracking
TEST_F(SafeHazardPointerTest, StatisticsTracking) {
    auto& domain = safe_hazard_pointer_domain::instance();

    size_t initial_retired = domain.retired_count();

    // Retire some objects
    for (int i = 0; i < 10; ++i) {
        int* ptr = new int(i);
        domain.retire(ptr, [](void* p) { delete static_cast<int*>(p); });
    }

    EXPECT_GE(domain.retired_count(), initial_retired + 10);

    // Collect
    domain.collect();

    // Retired count should decrease (objects deleted)
    EXPECT_LT(domain.retired_count(), initial_retired + 10);
}

// TC-010: Guard re-protect
TEST_F(SafeHazardPointerTest, GuardReprotect) {
    int val1 = 1;
    int val2 = 2;

    safe_hazard_guard guard(&val1);
    EXPECT_EQ(guard.get(), &val1);

    guard.protect(&val2);
    EXPECT_EQ(guard.get(), &val2);

    guard.clear();
    EXPECT_EQ(guard.get(), nullptr);
}

// TC-011: Typed domain
TEST_F(SafeHazardPointerTest, TypedDomain) {
    struct Node {
        int value;
        explicit Node(int v) : value(v) {}
    };

    auto& domain = typed_safe_hazard_domain<Node>::instance();

    Node* node = new Node(42);
    domain.retire(node);
    domain.collect();
    // Node should be deleted
}
