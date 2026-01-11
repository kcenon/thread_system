// BSD 3-Clause License
//
// Copyright (c) 2024, DongCheol Shin
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

/**
 * @file policy_queue_test.cpp
 * @brief Unit tests for policy-based queue template
 */

#include <gtest/gtest.h>

#include <kcenon/thread/policies/policies.h>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/core/error_handling.h>

#include <thread>
#include <atomic>
#include <vector>

using namespace kcenon::thread;
using namespace kcenon::thread::policies;

// =============================================================================
// Sync Policy Tests
// =============================================================================

class MutexSyncPolicyTest : public ::testing::Test {
protected:
    mutex_sync_policy policy_;
};

TEST_F(MutexSyncPolicyTest, EnqueueDequeue) {
    auto job = std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; });
    auto result = policy_.enqueue(std::move(job));
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(policy_.size(), 1u);

    auto dequeued = policy_.try_dequeue();
    ASSERT_TRUE(dequeued.is_ok());
    EXPECT_EQ(policy_.size(), 0u);
}

TEST_F(MutexSyncPolicyTest, EnqueueNull) {
    std::unique_ptr<job> null_job;
    auto result = policy_.enqueue(std::move(null_job));
    ASSERT_TRUE(result.is_err());
}

TEST_F(MutexSyncPolicyTest, DequeueEmpty) {
    auto result = policy_.try_dequeue();
    ASSERT_TRUE(result.is_err());
}

TEST_F(MutexSyncPolicyTest, EmptyAndSize) {
    EXPECT_TRUE(policy_.empty());
    EXPECT_EQ(policy_.size(), 0u);

    auto job = std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; });
    policy_.enqueue(std::move(job));

    EXPECT_FALSE(policy_.empty());
    EXPECT_EQ(policy_.size(), 1u);
}

TEST_F(MutexSyncPolicyTest, Capabilities) {
    auto caps = mutex_sync_policy::get_capabilities();
    EXPECT_TRUE(caps.exact_size);
    EXPECT_TRUE(caps.atomic_empty_check);
    EXPECT_FALSE(caps.lock_free);
    EXPECT_TRUE(caps.supports_blocking_wait);
}

TEST_F(MutexSyncPolicyTest, Stop) {
    EXPECT_FALSE(policy_.is_stopped());
    policy_.stop();
    EXPECT_TRUE(policy_.is_stopped());
}

// =============================================================================
// Lock-free Sync Policy Tests
// =============================================================================

class LockfreeSyncPolicyTest : public ::testing::Test {
protected:
    lockfree_sync_policy policy_;
};

TEST_F(LockfreeSyncPolicyTest, EnqueueDequeue) {
    auto job = std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; });
    auto result = policy_.enqueue(std::move(job));
    ASSERT_TRUE(result.is_ok());

    auto dequeued = policy_.dequeue();
    ASSERT_TRUE(dequeued.is_ok());
}

TEST_F(LockfreeSyncPolicyTest, EnqueueNull) {
    std::unique_ptr<job> null_job;
    auto result = policy_.enqueue(std::move(null_job));
    ASSERT_TRUE(result.is_err());
}

TEST_F(LockfreeSyncPolicyTest, DequeueEmpty) {
    auto result = policy_.dequeue();
    ASSERT_TRUE(result.is_err());
}

TEST_F(LockfreeSyncPolicyTest, Capabilities) {
    auto caps = lockfree_sync_policy::get_capabilities();
    EXPECT_FALSE(caps.exact_size);
    EXPECT_FALSE(caps.atomic_empty_check);
    EXPECT_TRUE(caps.lock_free);
    EXPECT_FALSE(caps.supports_blocking_wait);
}

TEST_F(LockfreeSyncPolicyTest, ConcurrentEnqueue) {
    constexpr int kNumThreads = 4;
    constexpr int kNumJobsPerThread = 100;
    std::atomic<int> counter{0};

    std::vector<std::thread> threads;
    for (int i = 0; i < kNumThreads; ++i) {
        threads.emplace_back([this, &counter]() {
            for (int j = 0; j < kNumJobsPerThread; ++j) {
                auto job = std::make_unique<callback_job>([&counter]() -> std::optional<std::string> {
                    counter.fetch_add(1);
                    return std::nullopt;
                });
                auto result = policy_.enqueue(std::move(job));
                EXPECT_TRUE(result.is_ok());
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(policy_.size(), kNumThreads * kNumJobsPerThread);
}

// =============================================================================
// Bound Policy Tests
// =============================================================================

TEST(UnboundedPolicyTest, NeverFull) {
    unbounded_policy policy;
    EXPECT_FALSE(policy.is_full(0));
    EXPECT_FALSE(policy.is_full(1000000));
    EXPECT_FALSE(unbounded_policy::is_bounded());
    EXPECT_FALSE(policy.max_size().has_value());
}

TEST(BoundedPolicyTest, Basic) {
    bounded_policy policy(100);
    EXPECT_TRUE(bounded_policy::is_bounded());
    EXPECT_EQ(policy.max_size().value(), 100u);

    EXPECT_FALSE(policy.is_full(0));
    EXPECT_FALSE(policy.is_full(99));
    EXPECT_TRUE(policy.is_full(100));
    EXPECT_TRUE(policy.is_full(101));
}

TEST(BoundedPolicyTest, RemainingCapacity) {
    bounded_policy policy(100);
    EXPECT_EQ(policy.remaining_capacity(0), 100u);
    EXPECT_EQ(policy.remaining_capacity(50), 50u);
    EXPECT_EQ(policy.remaining_capacity(100), 0u);
    EXPECT_EQ(policy.remaining_capacity(150), 0u);
}

TEST(DynamicBoundedPolicyTest, SwitchModes) {
    dynamic_bounded_policy policy(100);
    EXPECT_TRUE(policy.is_bounded());
    EXPECT_EQ(policy.max_size().value(), 100u);

    policy.set_unbounded();
    EXPECT_FALSE(policy.is_bounded());
    EXPECT_FALSE(policy.max_size().has_value());

    policy.set_max_size(50);
    EXPECT_TRUE(policy.is_bounded());
    EXPECT_EQ(policy.max_size().value(), 50u);
}

// =============================================================================
// Overflow Policy Tests
// =============================================================================

TEST(OverflowRejectPolicyTest, Rejects) {
    overflow_reject_policy policy;
    auto job = std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; });
    auto result = policy.handle_overflow(std::move(job));
    EXPECT_TRUE(result.is_err());
    EXPECT_FALSE(policy.blocks());
}

TEST(OverflowDropNewestPolicyTest, DropsNewestSilently) {
    overflow_drop_newest_policy policy;
    auto job = std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; });
    auto result = policy.handle_overflow(std::move(job));
    EXPECT_TRUE(result.is_ok());  // Silent drop
    EXPECT_FALSE(policy.blocks());
    EXPECT_TRUE(policy.drops_newest());
}

TEST(OverflowDropOldestPolicyTest, Metadata) {
    overflow_drop_oldest_policy policy;
    EXPECT_FALSE(policy.blocks());
    EXPECT_TRUE(policy.drops_oldest());
}

TEST(OverflowBlockPolicyTest, Metadata) {
    overflow_block_policy policy;
    EXPECT_TRUE(policy.blocks());
}

TEST(OverflowTimeoutPolicyTest, Timeout) {
    overflow_timeout_policy policy(std::chrono::milliseconds(500));
    EXPECT_EQ(policy.timeout().count(), 500);
    EXPECT_TRUE(policy.blocks());

    policy.set_timeout(std::chrono::milliseconds(1000));
    EXPECT_EQ(policy.timeout().count(), 1000);
}

// =============================================================================
// Policy Queue Integration Tests
// =============================================================================

class PolicyQueueTest : public ::testing::Test {
protected:
    using standard_queue_type = policy_queue<
        mutex_sync_policy,
        unbounded_policy,
        overflow_reject_policy
    >;

    standard_queue_type queue_;
};

TEST_F(PolicyQueueTest, BasicEnqueueDequeue) {
    auto job = std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; });
    auto result = queue_.enqueue(std::move(job));
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(queue_.size(), 1u);

    auto dequeued = queue_.try_dequeue();
    ASSERT_TRUE(dequeued.is_ok());
    EXPECT_EQ(queue_.size(), 0u);
}

TEST_F(PolicyQueueTest, SchedulerInterface) {
    auto job = std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; });
    auto result = queue_.schedule(std::move(job));
    ASSERT_TRUE(result.is_ok());

    auto next = queue_.get_next_job();
    ASSERT_TRUE(next.is_ok());
}

TEST_F(PolicyQueueTest, Capabilities) {
    auto caps = queue_.get_capabilities();
    EXPECT_TRUE(caps.exact_size);
    EXPECT_TRUE(caps.atomic_empty_check);
    EXPECT_FALSE(caps.lock_free);
}

TEST_F(PolicyQueueTest, StopBehavior) {
    EXPECT_FALSE(queue_.is_stopped());
    queue_.stop();
    EXPECT_TRUE(queue_.is_stopped());
}

TEST_F(PolicyQueueTest, Clear) {
    for (int i = 0; i < 5; ++i) {
        queue_.enqueue(std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; }));
    }
    EXPECT_EQ(queue_.size(), 5u);

    queue_.clear();
    EXPECT_EQ(queue_.size(), 0u);
    EXPECT_TRUE(queue_.empty());
}

// =============================================================================
// Bounded Queue Tests
// =============================================================================

TEST(BoundedQueueTest, RejectOnFull) {
    using bounded_queue = policy_queue<
        mutex_sync_policy,
        bounded_policy,
        overflow_reject_policy
    >;

    bounded_queue queue(bounded_policy(3));

    for (int i = 0; i < 3; ++i) {
        auto result = queue.enqueue(std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; }));
        ASSERT_TRUE(result.is_ok());
    }
    EXPECT_EQ(queue.size(), 3u);
    EXPECT_TRUE(queue.is_full());

    // Fourth enqueue should fail
    auto result = queue.enqueue(std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; }));
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(queue.size(), 3u);
}

TEST(BoundedQueueTest, DropOldestOnFull) {
    using ring_queue = policy_queue<
        mutex_sync_policy,
        bounded_policy,
        overflow_drop_oldest_policy
    >;

    ring_queue queue(bounded_policy(3));

    for (int i = 0; i < 3; ++i) {
        auto result = queue.enqueue(std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; }));
        ASSERT_TRUE(result.is_ok());
    }
    EXPECT_EQ(queue.size(), 3u);

    // Fourth enqueue should succeed by dropping oldest
    auto result = queue.enqueue(std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; }));
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(queue.size(), 3u);
}

TEST(BoundedQueueTest, DropNewestOnFull) {
    using drop_newest_queue = policy_queue<
        mutex_sync_policy,
        bounded_policy,
        overflow_drop_newest_policy
    >;

    drop_newest_queue queue(bounded_policy(3));

    for (int i = 0; i < 3; ++i) {
        auto result = queue.enqueue(std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; }));
        ASSERT_TRUE(result.is_ok());
    }
    EXPECT_EQ(queue.size(), 3u);

    // Fourth enqueue should succeed by dropping (new one is silently dropped)
    auto result = queue.enqueue(std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; }));
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(queue.size(), 3u);
}

// =============================================================================
// Lock-free Queue Integration Tests
// =============================================================================

TEST(LockfreeQueueTest, Basic) {
    using lockfree_queue_type = policy_queue<
        lockfree_sync_policy,
        unbounded_policy,
        overflow_reject_policy
    >;

    lockfree_queue_type queue;

    auto result = queue.enqueue(std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; }));
    ASSERT_TRUE(result.is_ok());

    auto dequeued = queue.dequeue();
    ASSERT_TRUE(dequeued.is_ok());
}

TEST(LockfreeQueueTest, Capabilities) {
    using lockfree_queue_type = policy_queue<
        lockfree_sync_policy,
        unbounded_policy,
        overflow_reject_policy
    >;

    lockfree_queue_type queue;
    auto caps = queue.get_capabilities();
    EXPECT_FALSE(caps.exact_size);
    EXPECT_TRUE(caps.lock_free);
}

TEST(LockfreeQueueTest, ConcurrentOperations) {
    using lockfree_queue_type = policy_queue<
        lockfree_sync_policy,
        unbounded_policy,
        overflow_reject_policy
    >;

    lockfree_queue_type queue;
    constexpr int kNumProducers = 4;
    constexpr int kNumJobsPerProducer = 100;
    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    // Start producers
    for (int i = 0; i < kNumProducers; ++i) {
        producers.emplace_back([&queue, &produced]() {
            for (int j = 0; j < kNumJobsPerProducer; ++j) {
                auto result = queue.enqueue(std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; }));
                if (result.is_ok()) {
                    produced.fetch_add(1);
                }
            }
        });
    }

    // Wait for producers
    for (auto& t : producers) {
        t.join();
    }

    // Consume all
    while (true) {
        auto result = queue.dequeue();
        if (result.is_err()) break;
        consumed.fetch_add(1);
    }

    EXPECT_EQ(produced.load(), kNumProducers * kNumJobsPerProducer);
    EXPECT_EQ(consumed.load(), produced.load());
}

// =============================================================================
// Type Alias Tests
// =============================================================================

TEST(TypeAliasTest, StandardQueue) {
    standard_queue queue;
    auto result = queue.enqueue(std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; }));
    ASSERT_TRUE(result.is_ok());
}

TEST(TypeAliasTest, LockfreeQueue) {
    lockfree_queue queue;
    auto result = queue.enqueue(std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; }));
    ASSERT_TRUE(result.is_ok());
}
