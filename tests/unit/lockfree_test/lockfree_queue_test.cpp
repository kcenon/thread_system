/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.
*****************************************************************************/

#include <kcenon/thread/lockfree/lockfree_queue.h>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace kcenon::thread;
using namespace std::chrono_literals;

class LockfreeQueueTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// =============================================================================
// Basic Operations
// =============================================================================

TEST_F(LockfreeQueueTest, DefaultConstruction) {
    lockfree_queue<int> queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
    EXPECT_FALSE(queue.is_shutdown());
}

TEST_F(LockfreeQueueTest, EnqueueDequeue) {
    lockfree_queue<int> queue;

    queue.enqueue(42);
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1);

    auto value = queue.try_dequeue();
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 42);
    EXPECT_TRUE(queue.empty());
}

TEST_F(LockfreeQueueTest, MultipleEnqueueDequeue) {
    lockfree_queue<int> queue;

    for (int i = 0; i < 100; ++i) {
        queue.enqueue(i);
    }
    EXPECT_EQ(queue.size(), 100);

    for (int i = 0; i < 100; ++i) {
        auto value = queue.try_dequeue();
        ASSERT_TRUE(value.has_value());
        EXPECT_EQ(*value, i);
    }
    EXPECT_TRUE(queue.empty());
}

TEST_F(LockfreeQueueTest, TryDequeueEmpty) {
    lockfree_queue<int> queue;

    auto value = queue.try_dequeue();
    EXPECT_FALSE(value.has_value());
}

TEST_F(LockfreeQueueTest, StringType) {
    lockfree_queue<std::string> queue;

    queue.enqueue("hello");
    queue.enqueue("world");

    auto v1 = queue.try_dequeue();
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(*v1, "hello");

    auto v2 = queue.try_dequeue();
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(*v2, "world");
}

TEST_F(LockfreeQueueTest, MoveOnlyType) {
    lockfree_queue<std::unique_ptr<int>> queue;

    queue.enqueue(std::make_unique<int>(42));

    auto value = queue.try_dequeue();
    ASSERT_TRUE(value.has_value());
    ASSERT_NE(*value, nullptr);
    EXPECT_EQ(**value, 42);
}

// =============================================================================
// Blocking Wait
// =============================================================================

TEST_F(LockfreeQueueTest, WaitDequeueTimeout) {
    lockfree_queue<int> queue;

    auto start = std::chrono::steady_clock::now();
    auto value = queue.wait_dequeue(50ms);
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_FALSE(value.has_value());
    EXPECT_GE(elapsed, 40ms);  // Allow some tolerance
}

TEST_F(LockfreeQueueTest, WaitDequeueSuccess) {
    lockfree_queue<int> queue;

    std::thread producer([&queue]() {
        std::this_thread::sleep_for(10ms);
        queue.enqueue(42);
    });

    auto value = queue.wait_dequeue(1s);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 42);

    producer.join();
}

TEST_F(LockfreeQueueTest, ShutdownWakesWaiters) {
    lockfree_queue<int> queue;

    std::atomic<bool> received{false};
    std::thread consumer([&]() {
        auto value = queue.wait_dequeue(10s);
        received = true;
        EXPECT_FALSE(value.has_value());  // Shutdown returns nullopt
    });

    std::this_thread::sleep_for(10ms);
    queue.shutdown();

    consumer.join();
    EXPECT_TRUE(received);
    EXPECT_TRUE(queue.is_shutdown());
}

// =============================================================================
// Concurrent Access
// =============================================================================

TEST_F(LockfreeQueueTest, SingleProducerSingleConsumer) {
    lockfree_queue<int> queue;
    const int count = 10000;

    std::thread producer([&]() {
        for (int i = 0; i < count; ++i) {
            queue.enqueue(i);
        }
    });

    std::vector<int> received;
    received.reserve(count);

    std::thread consumer([&]() {
        while (received.size() < count) {
            if (auto value = queue.try_dequeue()) {
                received.push_back(*value);
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(received.size(), count);
    for (int i = 0; i < count; ++i) {
        EXPECT_EQ(received[i], i);
    }
}

TEST_F(LockfreeQueueTest, MultipleProducersSingleConsumer) {
    lockfree_queue<int> queue;
    const int producers = 4;
    const int per_producer = 1000;
    const int total = producers * per_producer;

    std::vector<std::thread> producer_threads;
    for (int p = 0; p < producers; ++p) {
        producer_threads.emplace_back([&queue, p, per_producer]() {
            for (int i = 0; i < per_producer; ++i) {
                queue.enqueue(p * per_producer + i);
            }
        });
    }

    std::atomic<int> received_count{0};
    std::thread consumer([&]() {
        while (received_count < total) {
            if (auto value = queue.try_dequeue()) {
                received_count++;
            } else {
                std::this_thread::yield();
            }
        }
    });

    for (auto& t : producer_threads) {
        t.join();
    }
    consumer.join();

    EXPECT_EQ(received_count.load(), total);
    EXPECT_TRUE(queue.empty());
}

TEST_F(LockfreeQueueTest, MultipleProducersMultipleConsumers) {
    lockfree_queue<int> queue;
    const int producers = 4;
    const int consumers = 4;
    const int per_producer = 1000;
    const int total = producers * per_producer;

    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};

    std::vector<std::thread> producer_threads;
    for (int p = 0; p < producers; ++p) {
        producer_threads.emplace_back([&]() {
            for (int i = 0; i < per_producer; ++i) {
                queue.enqueue(i);
                produced++;
            }
        });
    }

    std::atomic<bool> done{false};
    std::vector<std::thread> consumer_threads;
    for (int c = 0; c < consumers; ++c) {
        consumer_threads.emplace_back([&]() {
            while (!done || !queue.empty()) {
                if (auto value = queue.try_dequeue()) {
                    consumed++;
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto& t : producer_threads) {
        t.join();
    }

    // Signal consumers that production is done
    done = true;

    for (auto& t : consumer_threads) {
        t.join();
    }

    EXPECT_EQ(produced.load(), total);
    EXPECT_EQ(consumed.load(), total);
    EXPECT_TRUE(queue.empty());
}

// =============================================================================
// Stress Test
// =============================================================================

TEST_F(LockfreeQueueTest, StressTest) {
    lockfree_queue<int> queue;
    const int threads = 8;
    const int operations = 5000;

    std::atomic<int> enqueued{0};
    std::atomic<int> dequeued{0};

    std::vector<std::thread> worker_threads;
    for (int t = 0; t < threads; ++t) {
        worker_threads.emplace_back([&, t]() {
            for (int i = 0; i < operations; ++i) {
                if (i % 2 == t % 2) {
                    queue.enqueue(i);
                    enqueued++;
                } else {
                    if (queue.try_dequeue()) {
                        dequeued++;
                    }
                }
            }
        });
    }

    for (auto& t : worker_threads) {
        t.join();
    }

    // Drain remaining items
    while (queue.try_dequeue()) {
        dequeued++;
    }

    EXPECT_EQ(enqueued.load(), dequeued.load());
    EXPECT_TRUE(queue.empty());
}
