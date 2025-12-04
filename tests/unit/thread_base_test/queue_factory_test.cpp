/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, DongCheol Shin
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

#include <kcenon/thread/queue/queue_factory.h>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/interfaces/queue_capabilities_interface.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <type_traits>

using namespace kcenon::thread;

class QueueFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset any global state if needed
    }

    void TearDown() override {
        // Allow cleanup
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
};

// ============================================
// Convenience factory method tests
// ============================================

TEST_F(QueueFactoryTest, CreateStandardQueue) {
    auto queue = queue_factory::create_standard_queue();

    ASSERT_NE(queue, nullptr);
    EXPECT_TRUE(queue->empty());
    EXPECT_EQ(queue->size(), 0);

    // Verify it's a job_queue with expected capabilities
    auto caps = queue->get_capabilities();
    EXPECT_TRUE(caps.exact_size);
    EXPECT_TRUE(caps.atomic_empty_check);
    EXPECT_FALSE(caps.lock_free);
    EXPECT_TRUE(caps.supports_batch);
    EXPECT_TRUE(caps.supports_blocking_wait);
    EXPECT_TRUE(caps.supports_stop);
}

TEST_F(QueueFactoryTest, CreateLockfreeQueue) {
    auto queue = queue_factory::create_lockfree_queue();

    ASSERT_NE(queue, nullptr);
    EXPECT_TRUE(queue->empty());

    // Verify it's a lockfree_job_queue with expected capabilities
    auto caps = queue->get_capabilities();
    EXPECT_FALSE(caps.exact_size);
    EXPECT_FALSE(caps.atomic_empty_check);
    EXPECT_TRUE(caps.lock_free);
    EXPECT_FALSE(caps.supports_batch);
    EXPECT_FALSE(caps.supports_blocking_wait);
}

TEST_F(QueueFactoryTest, CreateAdaptiveQueue) {
    auto queue = queue_factory::create_adaptive_queue();

    ASSERT_NE(queue, nullptr);
    EXPECT_TRUE(queue->empty());
    EXPECT_EQ(queue->size(), 0);
    EXPECT_EQ(queue->current_policy(), adaptive_job_queue::policy::balanced);
}

TEST_F(QueueFactoryTest, CreateAdaptiveQueueWithPolicy) {
    auto accuracy_queue = queue_factory::create_adaptive_queue(
        adaptive_job_queue::policy::accuracy_first);
    EXPECT_EQ(accuracy_queue->current_policy(), adaptive_job_queue::policy::accuracy_first);

    auto perf_queue = queue_factory::create_adaptive_queue(
        adaptive_job_queue::policy::performance_first);
    EXPECT_EQ(perf_queue->current_policy(), adaptive_job_queue::policy::performance_first);

    auto manual_queue = queue_factory::create_adaptive_queue(
        adaptive_job_queue::policy::manual);
    EXPECT_EQ(manual_queue->current_policy(), adaptive_job_queue::policy::manual);
}

// ============================================
// Requirements-based factory tests
// ============================================

TEST_F(QueueFactoryTest, CreateForRequirementsExactSize) {
    queue_factory::requirements reqs;
    reqs.need_exact_size = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    // Should select job_queue for exact size requirement
    auto* caps_iface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_iface, nullptr);
    EXPECT_TRUE(caps_iface->has_exact_size());
}

TEST_F(QueueFactoryTest, CreateForRequirementsAtomicEmpty) {
    queue_factory::requirements reqs;
    reqs.need_atomic_empty = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_iface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_iface, nullptr);
    EXPECT_TRUE(caps_iface->has_atomic_empty());
}

TEST_F(QueueFactoryTest, CreateForRequirementsBatchOperations) {
    queue_factory::requirements reqs;
    reqs.need_batch_operations = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_iface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_iface, nullptr);
    EXPECT_TRUE(caps_iface->supports_batch());
}

TEST_F(QueueFactoryTest, CreateForRequirementsBlockingWait) {
    queue_factory::requirements reqs;
    reqs.need_blocking_wait = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_iface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_iface, nullptr);
    EXPECT_TRUE(caps_iface->supports_blocking_wait());
}

TEST_F(QueueFactoryTest, CreateForRequirementsPreferLockFree) {
    queue_factory::requirements reqs;
    reqs.prefer_lock_free = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_iface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_iface, nullptr);
    EXPECT_TRUE(caps_iface->is_lock_free());
}

TEST_F(QueueFactoryTest, CreateForRequirementsDefault) {
    queue_factory::requirements reqs;
    // All defaults (false)

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    // Should return adaptive_job_queue by default
    auto* adaptive_queue = dynamic_cast<adaptive_job_queue*>(queue.get());
    EXPECT_NE(adaptive_queue, nullptr);
}

TEST_F(QueueFactoryTest, CreateForRequirementsConflictResolution) {
    queue_factory::requirements reqs;
    // Set conflicting requirements: accuracy requirements take priority
    reqs.need_exact_size = true;
    reqs.prefer_lock_free = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    // Accuracy requirements should win over lock-free preference
    auto* caps_iface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_iface, nullptr);
    EXPECT_TRUE(caps_iface->has_exact_size());
    EXPECT_FALSE(caps_iface->is_lock_free());
}

// ============================================
// Environment-based auto-selection tests
// ============================================

TEST_F(QueueFactoryTest, CreateOptimal) {
    auto queue = queue_factory::create_optimal();
    ASSERT_NE(queue, nullptr);

    // The queue should be usable via scheduler_interface
    std::atomic<int> counter{0};
    auto job = std::make_unique<callback_job>([&counter]() -> result_void {
        counter.fetch_add(1, std::memory_order_relaxed);
        return result_void();
    });

    auto schedule_result = queue->schedule(std::move(job));
    EXPECT_FALSE(schedule_result.has_error());

    auto get_result = queue->get_next_job();
    EXPECT_TRUE(get_result.has_value());
    ASSERT_TRUE(get_result.value() != nullptr);

    // Execute the job
    get_result.value()->do_work();
    EXPECT_EQ(counter.load(), 1);
}

// ============================================
// Compile-time selection tests
// ============================================

TEST_F(QueueFactoryTest, CompileTimeSelection) {
    // Verify type aliases resolve to expected types
    static_assert(std::is_same_v<accurate_queue_t, job_queue>,
        "accurate_queue_t should be job_queue");
    static_assert(std::is_same_v<fast_queue_t, lockfree_job_queue>,
        "fast_queue_t should be lockfree_job_queue");
    static_assert(std::is_same_v<balanced_queue_t, adaptive_job_queue>,
        "balanced_queue_t should be adaptive_job_queue");

    // Verify queue_t template works correctly
    static_assert(std::is_same_v<queue_t<true, false>, job_queue>,
        "queue_t<true, false> should be job_queue");
    static_assert(std::is_same_v<queue_t<false, true>, lockfree_job_queue>,
        "queue_t<false, true> should be lockfree_job_queue");
    static_assert(std::is_same_v<queue_t<false, false>, adaptive_job_queue>,
        "queue_t<false, false> should be adaptive_job_queue");

    // Test succeeded if it compiled
    SUCCEED();
}

TEST_F(QueueFactoryTest, TypeAliasesAreUsable) {
    // Verify type aliases can be instantiated
    accurate_queue_t accurate_queue;
    EXPECT_TRUE(accurate_queue.empty());

    fast_queue_t fast_queue;
    EXPECT_TRUE(fast_queue.empty());

    balanced_queue_t balanced_queue;
    EXPECT_TRUE(balanced_queue.empty());
}

// ============================================
// Integration tests
// ============================================

TEST_F(QueueFactoryTest, FactoryCreatedQueuesWorkWithJobs) {
    auto standard = queue_factory::create_standard_queue();
    auto lockfree = queue_factory::create_lockfree_queue();
    auto adaptive = queue_factory::create_adaptive_queue();

    std::atomic<int> counter{0};

    // Test each queue type
    for (auto* queue : {static_cast<scheduler_interface*>(standard.get()),
                        static_cast<scheduler_interface*>(lockfree.get()),
                        static_cast<scheduler_interface*>(adaptive.get())}) {
        auto job = std::make_unique<callback_job>([&counter]() -> result_void {
            counter.fetch_add(1, std::memory_order_relaxed);
            return result_void();
        });

        auto schedule_result = queue->schedule(std::move(job));
        EXPECT_FALSE(schedule_result.has_error());

        auto get_result = queue->get_next_job();
        EXPECT_TRUE(get_result.has_value());
        ASSERT_TRUE(get_result.value() != nullptr);

        get_result.value()->do_work();
    }

    EXPECT_EQ(counter.load(), 3);
}

TEST_F(QueueFactoryTest, MultipleJobsRoundTrip) {
    auto queue = queue_factory::create_standard_queue();

    constexpr int job_count = 100;
    std::atomic<int> counter{0};

    // Enqueue all jobs
    for (int i = 0; i < job_count; ++i) {
        auto job = std::make_unique<callback_job>([&counter]() -> result_void {
            counter.fetch_add(1, std::memory_order_relaxed);
            return result_void();
        });
        auto result = queue->schedule(std::move(job));
        EXPECT_FALSE(result.has_error());
    }

    EXPECT_EQ(queue->size(), job_count);

    // Dequeue and execute all jobs
    for (int i = 0; i < job_count; ++i) {
        auto result = queue->get_next_job();
        EXPECT_TRUE(result.has_value());
        ASSERT_TRUE(result.value() != nullptr);
        result.value()->do_work();
    }

    EXPECT_EQ(counter.load(), job_count);
    EXPECT_TRUE(queue->empty());
}
