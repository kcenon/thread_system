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

#include "kcenon/thread/core/atomic_shared_ptr.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace kcenon::thread;

class AtomicSharedPtrTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// TC-001: Default construction
TEST_F(AtomicSharedPtrTest, DefaultConstruction) {
    atomic_shared_ptr<int> asp;
    EXPECT_EQ(asp.load(), nullptr);
    EXPECT_FALSE(asp);
}

// TC-002: Construction with shared_ptr
TEST_F(AtomicSharedPtrTest, ConstructionWithSharedPtr) {
    auto ptr = std::make_shared<int>(42);
    atomic_shared_ptr<int> asp(ptr);

    auto loaded = asp.load();
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(*loaded, 42);
    EXPECT_TRUE(asp);
}

// TC-003: Store and load
TEST_F(AtomicSharedPtrTest, StoreAndLoad) {
    atomic_shared_ptr<int> asp;

    asp.store(std::make_shared<int>(42));
    auto loaded = asp.load();

    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(*loaded, 42);
}

// TC-004: Exchange
TEST_F(AtomicSharedPtrTest, Exchange) {
    atomic_shared_ptr<int> asp(std::make_shared<int>(10));

    auto old = asp.exchange(std::make_shared<int>(20));
    EXPECT_EQ(*old, 10);
    EXPECT_EQ(*asp.load(), 20);
}

// TC-005: Compare exchange success
TEST_F(AtomicSharedPtrTest, CompareExchangeSuccess) {
    auto initial = std::make_shared<int>(10);
    atomic_shared_ptr<int> asp(initial);

    auto expected = initial;
    auto desired = std::make_shared<int>(20);

    bool result = asp.compare_exchange_strong(expected, desired);

    EXPECT_TRUE(result);
    EXPECT_EQ(*asp.load(), 20);
}

// TC-006: Compare exchange failure
TEST_F(AtomicSharedPtrTest, CompareExchangeFailure) {
    auto initial = std::make_shared<int>(10);
    atomic_shared_ptr<int> asp(initial);

    auto wrong_expected = std::make_shared<int>(999);
    auto desired = std::make_shared<int>(20);

    bool result = asp.compare_exchange_strong(wrong_expected, desired);

    EXPECT_FALSE(result);
    EXPECT_EQ(*asp.load(), 10);  // Unchanged
    EXPECT_EQ(*wrong_expected, 10);  // Updated to actual value
}

// TC-007: Dereference operators
TEST_F(AtomicSharedPtrTest, DereferenceOperators) {
    struct TestStruct {
        int value = 42;
        int getValue() const { return value; }
    };

    atomic_shared_ptr<TestStruct> asp(std::make_shared<TestStruct>());

    EXPECT_EQ((*asp).value, 42);
    EXPECT_EQ(asp->getValue(), 42);
}

// TC-008: Reset
TEST_F(AtomicSharedPtrTest, Reset) {
    atomic_shared_ptr<int> asp(std::make_shared<int>(42));
    EXPECT_TRUE(asp);

    asp.reset();
    EXPECT_FALSE(asp);
    EXPECT_EQ(asp.load(), nullptr);
}

// TC-009: Copy construction
TEST_F(AtomicSharedPtrTest, CopyConstruction) {
    atomic_shared_ptr<int> asp1(std::make_shared<int>(42));
    atomic_shared_ptr<int> asp2(asp1);

    EXPECT_EQ(*asp1.load(), 42);
    EXPECT_EQ(*asp2.load(), 42);
    EXPECT_EQ(asp1.load().get(), asp2.load().get());  // Same object
}

// TC-010: Move construction
TEST_F(AtomicSharedPtrTest, MoveConstruction) {
    atomic_shared_ptr<int> asp1(std::make_shared<int>(42));
    atomic_shared_ptr<int> asp2(std::move(asp1));

    EXPECT_FALSE(asp1);  // Moved from
    EXPECT_EQ(*asp2.load(), 42);
}

// TC-011: Copy assignment
TEST_F(AtomicSharedPtrTest, CopyAssignment) {
    atomic_shared_ptr<int> asp1(std::make_shared<int>(42));
    atomic_shared_ptr<int> asp2;

    asp2 = asp1;

    EXPECT_EQ(*asp1.load(), 42);
    EXPECT_EQ(*asp2.load(), 42);
}

// TC-012: Move assignment
TEST_F(AtomicSharedPtrTest, MoveAssignment) {
    atomic_shared_ptr<int> asp1(std::make_shared<int>(42));
    atomic_shared_ptr<int> asp2;

    asp2 = std::move(asp1);

    EXPECT_FALSE(asp1);
    EXPECT_EQ(*asp2.load(), 42);
}

// TC-013: Conversion to shared_ptr
TEST_F(AtomicSharedPtrTest, ConversionToSharedPtr) {
    atomic_shared_ptr<int> asp(std::make_shared<int>(42));

    std::shared_ptr<int> sp = asp;  // Implicit conversion
    EXPECT_EQ(*sp, 42);
}

// TC-014: make_atomic_shared
TEST_F(AtomicSharedPtrTest, MakeAtomicShared) {
    struct TestStruct {
        int a;
        double b;
        TestStruct(int x, double y) : a(x), b(y) {}
    };

    auto asp = make_atomic_shared<TestStruct>(10, 3.14);
    EXPECT_EQ(asp->a, 10);
    EXPECT_DOUBLE_EQ(asp->b, 3.14);
}

// TC-015: Multi-threaded read
TEST_F(AtomicSharedPtrTest, MultiThreadedRead) {
    atomic_shared_ptr<int> asp(std::make_shared<int>(42));
    std::atomic<int> read_sum{0};

    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 1000; ++j) {
                auto p = asp.load();
                if (p) {
                    read_sum.fetch_add(*p, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(read_sum.load(), 8 * 1000 * 42);
}

// TC-016: Multi-threaded write
TEST_F(AtomicSharedPtrTest, MultiThreadedWrite) {
    atomic_shared_ptr<int> asp(std::make_shared<int>(0));

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < 250; ++j) {
                asp.store(std::make_shared<int>(i * 1000 + j));
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Final value should be from one of the threads
    EXPECT_NE(asp.load(), nullptr);
}

// TC-017: Multi-threaded read-write
TEST_F(AtomicSharedPtrTest, MultiThreadedReadWrite) {
    atomic_shared_ptr<int> asp(std::make_shared<int>(0));
    std::atomic<bool> stop{false};
    std::atomic<int> read_count{0};
    std::atomic<int> write_count{0};

    // Readers
    std::vector<std::thread> readers;
    for (int i = 0; i < 4; ++i) {
        readers.emplace_back([&]() {
            while (!stop.load(std::memory_order_acquire)) {
                auto p = asp.load();
                if (p) {
                    [[maybe_unused]] int val = *p;
                    read_count++;
                }
            }
        });
    }

    // Writers
    std::vector<std::thread> writers;
    for (int i = 0; i < 2; ++i) {
        writers.emplace_back([&, i]() {
            for (int j = 0; j < 500; ++j) {
                asp.store(std::make_shared<int>(i * 1000 + j));
                write_count++;
            }
        });
    }

    for (auto& t : writers) {
        t.join();
    }

    stop = true;

    for (auto& t : readers) {
        t.join();
    }

    EXPECT_EQ(write_count.load(), 2 * 500);
    EXPECT_GT(read_count.load(), 0);
}

// TC-018: CAS loop pattern
TEST_F(AtomicSharedPtrTest, CASLoopPattern) {
    atomic_shared_ptr<int> asp(std::make_shared<int>(0));

    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 100; ++j) {
                std::shared_ptr<int> expected;
                std::shared_ptr<int> desired;

                do {
                    expected = asp.load();
                    int new_val = expected ? *expected + 1 : 1;
                    desired = std::make_shared<int>(new_val);
                } while (!asp.compare_exchange_weak(expected, desired));
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(*asp.load(), 8 * 100);
}

// TC-019: Memory safety under concurrent access
TEST_F(AtomicSharedPtrTest, MemorySafetyStress) {
    constexpr int NUM_ITERATIONS = 1000;

    struct Node {
        std::atomic<int> value{0};
        std::vector<int> data;

        explicit Node(int v) : value(v), data(100, v) {}
    };

    atomic_shared_ptr<Node> asp(std::make_shared<Node>(0));
    std::atomic<bool> stop{false};

    // Readers - verify data integrity
    std::thread reader([&]() {
        while (!stop.load(std::memory_order_acquire)) {
            auto p = asp.load();
            if (p) {
                int expected = p->value.load();
                for (int d : p->data) {
                    EXPECT_EQ(d, expected);  // Data should be consistent
                }
            }
        }
    });

    // Writer - replace with new nodes
    std::thread writer([&]() {
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            asp.store(std::make_shared<Node>(i));
        }
    });

    writer.join();
    stop = true;
    reader.join();
}

// TC-020: get_unsafe
TEST_F(AtomicSharedPtrTest, GetUnsafe) {
    auto sp = std::make_shared<int>(42);
    int* raw_ptr = sp.get();

    atomic_shared_ptr<int> asp(sp);
    EXPECT_EQ(asp.get_unsafe(), raw_ptr);
}
