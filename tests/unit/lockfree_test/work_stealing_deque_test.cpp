/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.
*****************************************************************************/

#include <kcenon/thread/lockfree/work_stealing_deque.h>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <random>
#include <thread>
#include <vector>
#include <set>

using namespace kcenon::thread::lockfree;
using namespace std::chrono_literals;

class WorkStealingDequeTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// =============================================================================
// Basic Operations
// =============================================================================

TEST_F(WorkStealingDequeTest, DefaultConstruction) {
    work_stealing_deque<int*> deque;
    EXPECT_TRUE(deque.empty());
    EXPECT_EQ(deque.size(), 0);
}

TEST_F(WorkStealingDequeTest, PushPop) {
    work_stealing_deque<int*> deque;
    int value = 42;

    deque.push(&value);
    EXPECT_FALSE(deque.empty());
    EXPECT_EQ(deque.size(), 1);

    auto result = deque.pop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, &value);
    EXPECT_TRUE(deque.empty());
}

TEST_F(WorkStealingDequeTest, MultiplePushPop) {
    work_stealing_deque<int*> deque;
    std::vector<int> values(100);
    for (int i = 0; i < 100; ++i) {
        values[i] = i;
    }

    // Push all values
    for (int i = 0; i < 100; ++i) {
        deque.push(&values[i]);
    }
    EXPECT_EQ(deque.size(), 100);

    // Pop all values (LIFO order)
    for (int i = 99; i >= 0; --i) {
        auto result = deque.pop();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(**result, i);
    }
    EXPECT_TRUE(deque.empty());
}

TEST_F(WorkStealingDequeTest, PopEmpty) {
    work_stealing_deque<int*> deque;
    auto result = deque.pop();
    EXPECT_FALSE(result.has_value());
}

TEST_F(WorkStealingDequeTest, Steal) {
    work_stealing_deque<int*> deque;
    std::vector<int> values(10);
    for (int i = 0; i < 10; ++i) {
        values[i] = i;
        deque.push(&values[i]);
    }

    // Steal from top (FIFO order)
    auto result = deque.steal();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(**result, 0);  // First pushed item

    result = deque.steal();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(**result, 1);  // Second pushed item
}

TEST_F(WorkStealingDequeTest, StealEmpty) {
    work_stealing_deque<int*> deque;
    auto result = deque.steal();
    EXPECT_FALSE(result.has_value());
}

TEST_F(WorkStealingDequeTest, MixedPushPopSteal) {
    work_stealing_deque<int*> deque;
    std::vector<int> values(10);
    for (int i = 0; i < 10; ++i) {
        values[i] = i;
        deque.push(&values[i]);
    }

    // Steal from top
    auto stolen = deque.steal();
    ASSERT_TRUE(stolen.has_value());
    EXPECT_EQ(**stolen, 0);

    // Pop from bottom
    auto popped = deque.pop();
    ASSERT_TRUE(popped.has_value());
    EXPECT_EQ(**popped, 9);

    EXPECT_EQ(deque.size(), 8);
}

// =============================================================================
// Dynamic Resizing
// =============================================================================

TEST_F(WorkStealingDequeTest, GrowArray) {
    work_stealing_deque<int*> deque(2);  // Start with capacity 4 (2^2)
    std::vector<int> values(100);

    // Push more than initial capacity to trigger growth
    for (int i = 0; i < 100; ++i) {
        values[i] = i;
        deque.push(&values[i]);
    }
    EXPECT_EQ(deque.size(), 100);
    EXPECT_GE(deque.capacity(), 100);

    // Verify all values can be popped (LIFO)
    for (int i = 99; i >= 0; --i) {
        auto result = deque.pop();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(**result, i);
    }
}

// =============================================================================
// Concurrent Stealing
// =============================================================================

TEST_F(WorkStealingDequeTest, ConcurrentSteal) {
    work_stealing_deque<int*> deque;
    constexpr int count = 1000;
    std::vector<int> values(count);

    // Fill the deque
    for (int i = 0; i < count; ++i) {
        values[i] = i;
        deque.push(&values[i]);
    }

    std::atomic<int> stolen_count{0};
    constexpr int num_thieves = 4;
    std::vector<std::thread> thieves;

    // Multiple thieves try to steal concurrently
    for (int t = 0; t < num_thieves; ++t) {
        thieves.emplace_back([&]() {
            while (true) {
                auto result = deque.steal();
                if (!result.has_value()) {
                    // Keep trying until empty
                    if (deque.empty()) break;
                    std::this_thread::yield();
                } else {
                    stolen_count++;
                }
            }
        });
    }

    for (auto& t : thieves) {
        t.join();
    }

    EXPECT_EQ(stolen_count.load(), count);
    EXPECT_TRUE(deque.empty());
}

TEST_F(WorkStealingDequeTest, OwnerAndThieves) {
    work_stealing_deque<int*> deque;
    constexpr int count = 10000;
    std::vector<int> values(count);

    std::atomic<int> owner_count{0};
    std::atomic<int> stolen_count{0};
    std::atomic<bool> done{false};

    // Start thieves first
    constexpr int num_thieves = 3;
    std::vector<std::thread> thieves;
    for (int t = 0; t < num_thieves; ++t) {
        thieves.emplace_back([&]() {
            while (!done || !deque.empty()) {
                auto result = deque.steal();
                if (result.has_value()) {
                    stolen_count++;
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    // Owner pushes and pops
    std::thread owner([&]() {
        for (int i = 0; i < count; ++i) {
            values[i] = i;
            deque.push(&values[i]);

            // Occasionally pop (simulating local work)
            if (i % 5 == 0) {
                auto result = deque.pop();
                if (result.has_value()) {
                    owner_count++;
                }
            }
        }

        // Pop remaining local work
        while (auto result = deque.pop()) {
            owner_count++;
        }
    });

    owner.join();
    done = true;

    for (auto& t : thieves) {
        t.join();
    }

    // All items should be accounted for
    EXPECT_EQ(owner_count.load() + stolen_count.load(), count);
    EXPECT_TRUE(deque.empty());
}

// =============================================================================
// Single Element Contention
// =============================================================================

TEST_F(WorkStealingDequeTest, LastElementContention) {
    // Test race condition when owner and thief compete for last element
    for (int trial = 0; trial < 100; ++trial) {
        work_stealing_deque<int*> deque;
        int value = 42;
        deque.push(&value);

        std::atomic<int> winner_count{0};
        std::atomic<bool> start{false};

        std::thread owner([&]() {
            while (!start) std::this_thread::yield();
            if (deque.pop().has_value()) {
                winner_count++;
            }
        });

        std::thread thief([&]() {
            while (!start) std::this_thread::yield();
            if (deque.steal().has_value()) {
                winner_count++;
            }
        });

        start = true;
        owner.join();
        thief.join();

        // Exactly one should win
        EXPECT_EQ(winner_count.load(), 1);
        EXPECT_TRUE(deque.empty());
    }
}

// =============================================================================
// Stress Test
// =============================================================================

TEST_F(WorkStealingDequeTest, StressTest) {
    work_stealing_deque<int*> deque;
    constexpr int operations = 50000;
    constexpr int num_thieves = 4;

    std::vector<int> values(operations);
    std::atomic<int> pushed{0};
    std::atomic<int> popped{0};
    std::atomic<int> stolen{0};
    std::atomic<bool> done{false};

    // Thieves
    std::vector<std::thread> thieves;
    for (int t = 0; t < num_thieves; ++t) {
        thieves.emplace_back([&]() {
            while (!done || !deque.empty()) {
                if (deque.steal().has_value()) {
                    stolen++;
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    // Owner: push, pop, push, pop...
    std::thread owner([&]() {
        std::mt19937 rng(42);
        for (int i = 0; i < operations; ++i) {
            values[i] = i;
            deque.push(&values[i]);
            pushed++;

            // Randomly pop some
            if (rng() % 3 == 0) {
                if (deque.pop().has_value()) {
                    popped++;
                }
            }
        }

        // Drain remaining
        while (deque.pop().has_value()) {
            popped++;
        }
    });

    owner.join();
    done = true;

    for (auto& t : thieves) {
        t.join();
    }

    EXPECT_EQ(pushed.load(), popped.load() + stolen.load());
    EXPECT_TRUE(deque.empty());
}

// =============================================================================
// LIFO/FIFO Order Verification
// =============================================================================

TEST_F(WorkStealingDequeTest, LIFOOrderForOwner) {
    work_stealing_deque<int*> deque;
    std::vector<int> values{1, 2, 3, 4, 5};

    for (auto& v : values) {
        deque.push(&v);
    }

    // Owner pops in LIFO order
    std::vector<int> popped_order;
    while (auto result = deque.pop()) {
        popped_order.push_back(**result);
    }

    std::vector<int> expected{5, 4, 3, 2, 1};
    EXPECT_EQ(popped_order, expected);
}

TEST_F(WorkStealingDequeTest, FIFOOrderForThief) {
    work_stealing_deque<int*> deque;
    std::vector<int> values{1, 2, 3, 4, 5};

    for (auto& v : values) {
        deque.push(&v);
    }

    // Thief steals in FIFO order
    std::vector<int> stolen_order;
    while (auto result = deque.steal()) {
        stolen_order.push_back(**result);
    }

    std::vector<int> expected{1, 2, 3, 4, 5};
    EXPECT_EQ(stolen_order, expected);
}

// =============================================================================
// Capacity and Memory
// =============================================================================

TEST_F(WorkStealingDequeTest, InitialCapacity) {
    work_stealing_deque<int*> deque(3);  // 2^3 = 8
    EXPECT_EQ(deque.capacity(), 8);
}

TEST_F(WorkStealingDequeTest, CleanupOldArrays) {
    work_stealing_deque<int*> deque(2);  // Start small
    std::vector<int> values(100);

    // Force multiple growths
    for (int i = 0; i < 100; ++i) {
        values[i] = i;
        deque.push(&values[i]);
    }

    // Cleanup should not crash
    deque.cleanup_old_arrays();
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(WorkStealingDequeTest, RapidPushPop) {
    work_stealing_deque<int*> deque;
    int value = 42;

    // Rapid push/pop cycles
    for (int i = 0; i < 10000; ++i) {
        deque.push(&value);
        auto result = deque.pop();
        ASSERT_TRUE(result.has_value());
    }
    EXPECT_TRUE(deque.empty());
}

TEST_F(WorkStealingDequeTest, StealDuringGrow) {
    work_stealing_deque<int*> deque(2);  // Start with capacity 4
    std::vector<int> values(1000);
    std::atomic<int> stolen_count{0};
    std::atomic<bool> done{false};

    // Thief continuously steals
    std::thread thief([&]() {
        while (!done || !deque.empty()) {
            if (deque.steal().has_value()) {
                stolen_count++;
            }
        }
    });

    // Owner pushes (triggering grows)
    for (int i = 0; i < 1000; ++i) {
        values[i] = i;
        deque.push(&values[i]);
    }

    // Pop remaining
    int popped = 0;
    while (deque.pop().has_value()) {
        popped++;
    }

    done = true;
    thief.join();

    EXPECT_EQ(popped + stolen_count.load(), 1000);
}
