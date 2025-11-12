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

#include "kcenon/thread/core/hazard_pointer.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace kcenon::thread;

class HazardPointerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset any global state if needed
    }

    void TearDown() override {
        // Clean up if needed
    }
};

// Test basic protection
TEST_F(HazardPointerTest, BasicProtection) {
    auto hp = hazard_pointer_domain<int>::global().acquire();
    int* ptr = new int(42);

    hp.protect(ptr);
    EXPECT_EQ(hp.get_protected(), ptr);
    EXPECT_TRUE(hp.is_protected());

    hp.reset();
    EXPECT_EQ(hp.get_protected(), nullptr);
    EXPECT_FALSE(hp.is_protected());

    // Clean up manually since we're not retiring
    delete ptr;
}

// Test retirement without protection
TEST_F(HazardPointerTest, RetireWithoutProtection) {
    int* ptr = new int(42);
    hazard_pointer_domain<int>::global().retire(ptr);

    // Force reclamation
    auto reclaimed = hazard_pointer_domain<int>::global().reclaim();
    EXPECT_GE(reclaimed, 1);
}

// Test protection prevents reclamation
TEST_F(HazardPointerTest, ProtectionPreventsReclaim) {
    auto hp = hazard_pointer_domain<int>::global().acquire();
    int* ptr = new int(42);

    hp.protect(ptr);
    hazard_pointer_domain<int>::global().retire(ptr);

    // Should not reclaim protected pointer
    auto reclaimed = hazard_pointer_domain<int>::global().reclaim();
    EXPECT_EQ(reclaimed, 0);

    // Release protection and reclaim
    hp.reset();
    reclaimed = hazard_pointer_domain<int>::global().reclaim();
    EXPECT_GE(reclaimed, 1);
}

// Test multiple hazard pointers
TEST_F(HazardPointerTest, MultipleHazardPointers) {
    auto hp1 = hazard_pointer_domain<int>::global().acquire();
    auto hp2 = hazard_pointer_domain<int>::global().acquire();

    int* ptr1 = new int(1);
    int* ptr2 = new int(2);

    hp1.protect(ptr1);
    hp2.protect(ptr2);

    EXPECT_EQ(hp1.get_protected(), ptr1);
    EXPECT_EQ(hp2.get_protected(), ptr2);

    // Retire both
    hazard_pointer_domain<int>::global().retire(ptr1);
    hazard_pointer_domain<int>::global().retire(ptr2);

    // Neither should be reclaimed
    auto reclaimed = hazard_pointer_domain<int>::global().reclaim();
    EXPECT_EQ(reclaimed, 0);

    // Release one
    hp1.reset();
    reclaimed = hazard_pointer_domain<int>::global().reclaim();
    EXPECT_EQ(reclaimed, 1);

    // Release the other
    hp2.reset();
    reclaimed = hazard_pointer_domain<int>::global().reclaim();
    EXPECT_EQ(reclaimed, 1);
}

// Test move constructor
TEST_F(HazardPointerTest, MoveConstructor) {
    hazard_pointer hp1;
    int* ptr = new int(42);
    hp1.protect(ptr);

    // Move construct
    hazard_pointer hp2(std::move(hp1));

    EXPECT_FALSE(hp1.is_protected());
    EXPECT_TRUE(hp2.is_protected());
    EXPECT_EQ(hp2.get_protected(), ptr);

    // Clean up
    hp2.reset();
    delete ptr;
}

// Test move assignment
TEST_F(HazardPointerTest, MoveAssignment) {
    hazard_pointer hp1;
    int* ptr = new int(42);
    hp1.protect(ptr);

    hazard_pointer hp2;
    hp2 = std::move(hp1);

    EXPECT_FALSE(hp1.is_protected());
    EXPECT_TRUE(hp2.is_protected());
    EXPECT_EQ(hp2.get_protected(), ptr);

    // Clean up
    hp2.reset();
    delete ptr;
}

// Test statistics
TEST_F(HazardPointerTest, Statistics) {
    auto initial_stats = hazard_pointer_domain<int>::global().get_stats();

    int* ptr1 = new int(1);
    int* ptr2 = new int(2);

    hazard_pointer_domain<int>::global().retire(ptr1);
    hazard_pointer_domain<int>::global().retire(ptr2);

    auto after_retire_stats = hazard_pointer_domain<int>::global().get_stats();
    EXPECT_GE(after_retire_stats.objects_retired, initial_stats.objects_retired + 2);

    hazard_pointer_domain<int>::global().reclaim();

    auto after_reclaim_stats = hazard_pointer_domain<int>::global().get_stats();
    EXPECT_GT(after_reclaim_stats.scan_count, initial_stats.scan_count);
    EXPECT_GE(after_reclaim_stats.objects_reclaimed, initial_stats.objects_reclaimed + 2);
}

// Test concurrent retirement
TEST_F(HazardPointerTest, ConcurrentRetirement) {
    constexpr int NUM_THREADS = 4;
    constexpr int OBJECTS_PER_THREAD = 100;

    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([i] {
            for (int j = 0; j < OBJECTS_PER_THREAD; ++j) {
                int* ptr = new int(i * OBJECTS_PER_THREAD + j);
                hazard_pointer_domain<int>::global().retire(ptr);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Force reclamation
    hazard_pointer_domain<int>::global().reclaim();

    // Verify statistics
    auto stats = hazard_pointer_domain<int>::global().get_stats();
    EXPECT_GE(stats.objects_retired, NUM_THREADS * OBJECTS_PER_THREAD);
}

// Test concurrent protection and retirement
TEST_F(HazardPointerTest, ConcurrentProtectionAndRetirement) {
    constexpr int NUM_ITERATIONS = 100;
    std::atomic<bool> stop{false};

    // Thread that retires objects
    std::thread retire_thread([&stop] {
        int counter = 0;
        while (!stop.load(std::memory_order_acquire)) {
            int* ptr = new int(counter++);
            hazard_pointer_domain<int>::global().retire(ptr);

            if (counter % 10 == 0) {
                hazard_pointer_domain<int>::global().reclaim();
            }
        }
    });

    // Thread that protects and releases objects
    std::thread protect_thread([&stop] {
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            auto hp = hazard_pointer_domain<int>::global().acquire();
            int* ptr = new int(1000 + i);

            hp.protect(ptr);
            std::this_thread::yield();
            hp.reset();

            delete ptr;  // We own this, not retired
        }
    });

    protect_thread.join();
    stop.store(true, std::memory_order_release);
    retire_thread.join();

    // Final reclamation
    hazard_pointer_domain<int>::global().reclaim();
}

// Test with custom type
struct TestNode {
    int value;
    std::atomic<TestNode*> next;

    explicit TestNode(int v) : value(v), next(nullptr) {}
};

TEST_F(HazardPointerTest, CustomType) {
    auto hp = hazard_pointer_domain<TestNode>::global().acquire();
    TestNode* node = new TestNode(42);

    hp.protect(node);
    EXPECT_EQ(hp.get_protected(), node);

    hazard_pointer_domain<TestNode>::global().retire(node);

    // Should not be reclaimed while protected
    auto reclaimed = hazard_pointer_domain<TestNode>::global().reclaim();
    EXPECT_EQ(reclaimed, 0);

    hp.reset();
    reclaimed = hazard_pointer_domain<TestNode>::global().reclaim();
    EXPECT_GE(reclaimed, 1);
}

// Test automatic threshold-based reclamation
TEST_F(HazardPointerTest, AutomaticReclamation) {
    auto initial_stats = hazard_pointer_domain<int>::global().get_stats();

    // Retire many objects to trigger automatic reclamation
    constexpr int NUM_OBJECTS = 100;  // Should exceed RECLAIM_THRESHOLD (64)

    for (int i = 0; i < NUM_OBJECTS; ++i) {
        int* ptr = new int(i);
        hazard_pointer_domain<int>::global().retire(ptr);
    }

    // Check that automatic reclamation was triggered
    auto after_stats = hazard_pointer_domain<int>::global().get_stats();
    EXPECT_GT(after_stats.scan_count, initial_stats.scan_count);
}

// Test RAII behavior
TEST_F(HazardPointerTest, RAIIBehavior) {
    auto initial_stats = hazard_pointer_domain<int>::global().get_stats();
    int* ptr = new int(42);

    {
        auto hp = hazard_pointer_domain<int>::global().acquire();
        hp.protect(ptr);
        EXPECT_TRUE(hp.is_protected());

        hazard_pointer_domain<int>::global().retire(ptr);

        // Should not be reclaimed while in scope
        // Note: May reclaim other unprotected objects from previous tests
        hazard_pointer_domain<int>::global().reclaim();

        // Our pointer should still be in the retire list (not reclaimed)
        auto stats_during = hazard_pointer_domain<int>::global().get_stats();
        auto pending = stats_during.objects_retired - stats_during.objects_reclaimed;
        EXPECT_GT(pending, 0);  // At least our object is pending
    }  // hp goes out of scope, releases protection

    // Now our pointer should be reclaimable
    auto reclaimed = hazard_pointer_domain<int>::global().reclaim();
    EXPECT_GE(reclaimed, 1);  // At least our object was reclaimed
}

// Test stress: Many threads protecting and retiring
TEST_F(HazardPointerTest, StressTest) {
    constexpr int NUM_THREADS = 8;
    constexpr int ITERATIONS = 1000;

    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([i] {
            for (int j = 0; j < ITERATIONS; ++j) {
                auto hp = hazard_pointer_domain<int>::global().acquire();
                int* ptr = new int(i * ITERATIONS + j);

                hp.protect(ptr);

                // Simulate some work
                std::this_thread::yield();

                hazard_pointer_domain<int>::global().retire(ptr);

                hp.reset();

                // Occasionally trigger reclamation
                if (j % 100 == 0) {
                    hazard_pointer_domain<int>::global().reclaim();
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Final cleanup
    for (int i = 0; i < 10; ++i) {
        hazard_pointer_domain<int>::global().reclaim();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
