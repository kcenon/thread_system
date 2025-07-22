/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
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
#include "../../sources/monitoring/storage/ring_buffer.h"
#include <thread>
#include <vector>
#include <atomic>
#include <random>

namespace monitoring_module {
namespace test {

class RingBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup if needed
    }

    void TearDown() override {
        // Test cleanup if needed
    }
};

TEST_F(RingBufferTest, BasicOperations) {
    ring_buffer<int> buffer(10);
    
    // Test initial state
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0u);
    EXPECT_EQ(buffer.capacity(), 10u);
    
    // Test push and size
    for (int i = 0; i < 5; ++i) {
        buffer.push(i);
        EXPECT_EQ(buffer.size(), static_cast<size_t>(i + 1));
    }
    
    EXPECT_FALSE(buffer.empty());
    EXPECT_FALSE(buffer.full());
}

TEST_F(RingBufferTest, FullBufferBehavior) {
    ring_buffer<int> buffer(5);
    
    // Fill buffer
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(buffer.push(i));
    }
    EXPECT_TRUE(buffer.full());
    
    // Try to push when full - should fail
    EXPECT_FALSE(buffer.push(5));
    EXPECT_EQ(buffer.size(), 5u);
    
    // Pop one and push again
    int item;
    EXPECT_TRUE(buffer.pop(item));
    EXPECT_EQ(item, 0);
    EXPECT_TRUE(buffer.push(5));
}

TEST_F(RingBufferTest, GetRecentItems) {
    ring_buffer<int> buffer(10);
    
    // Add elements in order
    for (int i = 0; i < 7; ++i) {
        buffer.push(i);
    }
    
    // Get recent 5 items
    auto recent = buffer.get_recent_items(5);
    EXPECT_EQ(recent.size(), 5u);
    
    // Verify order is preserved (should get items 2-6)
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(recent[i], i + 2);
    }
    
    // Get more items than available
    auto all_items = buffer.get_recent_items(20);
    EXPECT_EQ(all_items.size(), 7u);
}

TEST_F(RingBufferTest, PopOperation) {
    ring_buffer<int> buffer(10);
    
    // Add some elements
    for (int i = 0; i < 5; ++i) {
        buffer.push(i);
    }
    
    // Pop all elements and verify
    for (int i = 0; i < 5; ++i) {
        int item;
        EXPECT_TRUE(buffer.pop(item));
        EXPECT_EQ(item, i);
    }
    
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0u);
    
    // Try to pop from empty buffer
    int item;
    EXPECT_FALSE(buffer.pop(item));
}

TEST_F(RingBufferTest, EdgeCases) {
    // Zero capacity buffer
    ring_buffer<int> buffer(0);
    EXPECT_EQ(buffer.capacity(), 0u);
    EXPECT_TRUE(buffer.empty());
    EXPECT_TRUE(buffer.full());
    
    // Push to zero capacity buffer should not crash
    buffer.push(42);
    EXPECT_TRUE(buffer.empty());
    
    // Single element buffer
    ring_buffer<int> single_buffer(1);
    EXPECT_TRUE(single_buffer.push(10));
    EXPECT_TRUE(single_buffer.full());
    EXPECT_EQ(single_buffer.size(), 1u);
    
    // Try to push when full
    EXPECT_FALSE(single_buffer.push(20));
    
    // Get recent items
    auto recent = single_buffer.get_recent_items(10);
    EXPECT_EQ(recent.size(), 1u);
    EXPECT_EQ(recent[0], 10);
}

TEST_F(RingBufferTest, ThreadSafeOperations) {
    thread_safe_ring_buffer<int> buffer(100);
    const int num_threads = 4;
    const int items_per_thread = 250;
    
    std::vector<std::thread> producers;
    std::atomic<int> total_pushed{0};
    
    // Launch producer threads
    for (int t = 0; t < num_threads; ++t) {
        producers.emplace_back([&buffer, &total_pushed, t, items_per_thread]() {
            for (int i = 0; i < items_per_thread; ++i) {
                buffer.push(t * 1000 + i);
                total_pushed.fetch_add(1);
            }
        });
    }
    
    // Wait for all producers
    for (auto& t : producers) {
        t.join();
    }
    
    // Buffer should contain at most 100 items (might be 99 due to concurrent push failures)
    EXPECT_LE(buffer.size(), 100u);
    EXPECT_GE(buffer.size(), 99u);
    EXPECT_EQ(total_pushed.load(), num_threads * items_per_thread);
    
    // Verify thread safety of get_all_items
    auto items1 = buffer.get_all_items();
    auto items2 = buffer.get_all_items();
    EXPECT_EQ(items1.size(), items2.size());
}

TEST_F(RingBufferTest, ConcurrentPushAndGet) {
    thread_safe_ring_buffer<int> buffer(1000);
    std::atomic<bool> stop{false};
    std::atomic<int> push_count{0};
    std::atomic<int> get_count{0};
    
    // Producer thread
    std::thread producer([&]() {
        int value = 0;
        while (!stop.load()) {
            buffer.push(value++);
            push_count.fetch_add(1);
            std::this_thread::yield();
        }
    });
    
    // Consumer threads getting items
    std::vector<std::thread> consumers;
    for (int i = 0; i < 3; ++i) {
        consumers.emplace_back([&]() {
            while (!stop.load()) {
                auto items = buffer.get_all_items();
                get_count.fetch_add(1);
                
                // Verify items consistency
                for (size_t j = 1; j < items.size(); ++j) {
                    // Values should be in increasing order
                    EXPECT_GT(items[j], items[j-1]);
                }
                std::this_thread::yield();
            }
        });
    }
    
    // Run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stop.store(true);
    
    producer.join();
    for (auto& t : consumers) {
        t.join();
    }
    
    EXPECT_GT(push_count.load(), 0);
    EXPECT_GT(get_count.load(), 0);
}

TEST_F(RingBufferTest, StressTest) {
    thread_safe_ring_buffer<std::string> buffer(500);
    const int num_threads = 8;
    const int operations_per_thread = 10000;
    
    std::vector<std::thread> threads;
    std::atomic<int> total_operations{0};
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            std::mt19937 rng(t);
            std::uniform_int_distribution<int> op_dist(0, 2);
            
            for (int i = 0; i < operations_per_thread; ++i) {
                int op = op_dist(rng);
                
                switch (op) {
                    case 0: // Push
                        buffer.push("Thread" + std::to_string(t) + "_Item" + std::to_string(i));
                        break;
                    case 1: // Get all items
                        {
                            auto items = buffer.get_all_items();
                            // Just verify we can get items without crashing
                        }
                        break;
                    case 2: // Pop
                        {
                            std::string item;
                            buffer.pop(item); // May fail if empty, that's ok
                        }
                        break;
                }
                total_operations.fetch_add(1);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(total_operations.load(), num_threads * operations_per_thread);
}

} // namespace test
} // namespace monitoring_module