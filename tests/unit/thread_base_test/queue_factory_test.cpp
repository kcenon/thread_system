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
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/interfaces/queue_capabilities_interface.h>
#include <kcenon/thread/queue/queue_factory.h>

#include <atomic>
#include <thread>
#include <type_traits>

using namespace kcenon::thread;

class QueueFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {
        // Hazard pointer cleanup happens deterministically when pointers go out of scope
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

    // Verify it's a job_queue with exact size capability
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

    // Verify it's a lockfree_job_queue
    auto caps = queue->get_capabilities();
    EXPECT_FALSE(caps.exact_size);
    EXPECT_FALSE(caps.atomic_empty_check);
    EXPECT_TRUE(caps.lock_free);
    EXPECT_FALSE(caps.supports_batch);
    EXPECT_FALSE(caps.supports_blocking_wait);
    EXPECT_FALSE(caps.supports_stop);
}

TEST_F(QueueFactoryTest, CreateAdaptiveQueueDefaultPolicy) {
    auto queue = queue_factory::create_adaptive_queue();

    ASSERT_NE(queue, nullptr);
    EXPECT_TRUE(queue->empty());
    EXPECT_EQ(queue->current_policy(), adaptive_job_queue::policy::balanced);
}

TEST_F(QueueFactoryTest, CreateAdaptiveQueueWithPolicy) {
    auto accuracy_queue =
        queue_factory::create_adaptive_queue(adaptive_job_queue::policy::accuracy_first);
    EXPECT_EQ(accuracy_queue->current_policy(), adaptive_job_queue::policy::accuracy_first);
    EXPECT_EQ(accuracy_queue->current_mode(), adaptive_job_queue::mode::mutex);

    auto perf_queue =
        queue_factory::create_adaptive_queue(adaptive_job_queue::policy::performance_first);
    EXPECT_EQ(perf_queue->current_policy(), adaptive_job_queue::policy::performance_first);
    EXPECT_EQ(perf_queue->current_mode(), adaptive_job_queue::mode::lock_free);

    auto manual_queue = queue_factory::create_adaptive_queue(adaptive_job_queue::policy::manual);
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

    // Should be a job_queue (exact size requires mutex-based)
    auto* caps_interface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_interface, nullptr);

    auto caps = caps_interface->get_capabilities();
    EXPECT_TRUE(caps.exact_size);
    EXPECT_FALSE(caps.lock_free);
}

TEST_F(QueueFactoryTest, CreateForRequirementsAtomicEmpty) {
    queue_factory::requirements reqs;
    reqs.need_atomic_empty = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_interface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_interface, nullptr);

    auto caps = caps_interface->get_capabilities();
    EXPECT_TRUE(caps.atomic_empty_check);
}

TEST_F(QueueFactoryTest, CreateForRequirementsBatchOperations) {
    queue_factory::requirements reqs;
    reqs.need_batch_operations = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_interface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_interface, nullptr);

    auto caps = caps_interface->get_capabilities();
    EXPECT_TRUE(caps.supports_batch);
}

TEST_F(QueueFactoryTest, CreateForRequirementsBlockingWait) {
    queue_factory::requirements reqs;
    reqs.need_blocking_wait = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_interface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_interface, nullptr);

    auto caps = caps_interface->get_capabilities();
    EXPECT_TRUE(caps.supports_blocking_wait);
}

TEST_F(QueueFactoryTest, CreateForRequirementsPreferLockFree) {
    queue_factory::requirements reqs;
    reqs.prefer_lock_free = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_interface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_interface, nullptr);

    auto caps = caps_interface->get_capabilities();
    EXPECT_TRUE(caps.lock_free);
}

TEST_F(QueueFactoryTest, CreateForRequirementsDefault) {
    queue_factory::requirements reqs;  // All defaults (false)

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    // Should be adaptive_job_queue (default)
    auto* adaptive = dynamic_cast<adaptive_job_queue*>(queue.get());
    EXPECT_NE(adaptive, nullptr);
}

TEST_F(QueueFactoryTest, CreateForRequirementsExactSizeOverridesLockFree) {
    queue_factory::requirements reqs;
    reqs.need_exact_size = true;
    reqs.prefer_lock_free = true;  // Should be ignored due to exact_size

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_interface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_interface, nullptr);

    auto caps = caps_interface->get_capabilities();
    EXPECT_TRUE(caps.exact_size);
    EXPECT_FALSE(caps.lock_free);  // Lock-free cannot provide exact size
}

// ============================================
// Environment-based auto-selection tests
// ============================================

TEST_F(QueueFactoryTest, CreateOptimal) {
    auto queue = queue_factory::create_optimal();
    ASSERT_NE(queue, nullptr);

    // Verify it implements scheduler_interface
    std::atomic<int> counter{0};
    auto job = std::make_unique<callback_job>([&counter]() -> result_void {
        counter.fetch_add(1, std::memory_order_relaxed);
        return result_void();
    });

    auto schedule_result = queue->schedule(std::move(job));
    EXPECT_FALSE(schedule_result.has_error());

    auto get_result = queue->get_next_job();
    EXPECT_TRUE(get_result.has_value());

    auto exec_result = get_result.value()->do_work();
    EXPECT_FALSE(exec_result.has_error());
    EXPECT_EQ(counter.load(), 1);
}

// ============================================
// Compile-time selection tests
// ============================================

TEST_F(QueueFactoryTest, CompileTimeSelectionAccurate) {
    // queue_type_selector<true, false> should be job_queue
    using selected_type = queue_type_selector<true, false>::type;
    static_assert(std::is_same_v<selected_type, job_queue>,
                  "queue_type_selector<true, false> should select job_queue");

    // Using queue_t alias
    static_assert(std::is_same_v<queue_t<true, false>, job_queue>,
                  "queue_t<true, false> should be job_queue");
}

TEST_F(QueueFactoryTest, CompileTimeSelectionFast) {
    // queue_type_selector<false, true> should be lockfree_job_queue
    using selected_type = queue_type_selector<false, true>::type;
    static_assert(std::is_same_v<selected_type, lockfree_job_queue>,
                  "queue_type_selector<false, true> should select lockfree_job_queue");

    // Using queue_t alias
    static_assert(std::is_same_v<queue_t<false, true>, lockfree_job_queue>,
                  "queue_t<false, true> should be lockfree_job_queue");
}

TEST_F(QueueFactoryTest, CompileTimeSelectionBalanced) {
    // queue_type_selector<false, false> should be adaptive_job_queue
    using selected_type = queue_type_selector<false, false>::type;
    static_assert(std::is_same_v<selected_type, adaptive_job_queue>,
                  "queue_type_selector<false, false> should select adaptive_job_queue");

    // Using queue_t alias
    static_assert(std::is_same_v<queue_t<false, false>, adaptive_job_queue>,
                  "queue_t<false, false> should be adaptive_job_queue");
}

TEST_F(QueueFactoryTest, TypeAliases) {
    // Verify pre-defined type aliases
    static_assert(std::is_same_v<accurate_queue_t, job_queue>,
                  "accurate_queue_t should be job_queue");
    static_assert(std::is_same_v<fast_queue_t, lockfree_job_queue>,
                  "fast_queue_t should be lockfree_job_queue");
    static_assert(std::is_same_v<balanced_queue_t, adaptive_job_queue>,
                  "balanced_queue_t should be adaptive_job_queue");
}

// ============================================
// Functional tests - verify queues work correctly
// ============================================

TEST_F(QueueFactoryTest, StandardQueueFunctional) {
    auto queue = queue_factory::create_standard_queue();

    std::atomic<int> counter{0};
    for (int i = 0; i < 10; ++i) {
        auto job = std::make_unique<callback_job>([&counter]() -> result_void {
            counter.fetch_add(1, std::memory_order_relaxed);
            return result_void();
        });
        auto result = queue->enqueue(std::move(job));
        EXPECT_FALSE(result.has_error());
    }

    EXPECT_EQ(queue->size(), 10);

    for (int i = 0; i < 10; ++i) {
        auto result = queue->dequeue();
        EXPECT_TRUE(result.has_value());
        (void)result.value()->do_work();
    }

    EXPECT_EQ(counter.load(), 10);
    EXPECT_TRUE(queue->empty());
}

TEST_F(QueueFactoryTest, LockfreeQueueFunctional) {
    auto queue = queue_factory::create_lockfree_queue();

    std::atomic<int> counter{0};
    for (int i = 0; i < 10; ++i) {
        auto job = std::make_unique<callback_job>([&counter]() -> result_void {
            counter.fetch_add(1, std::memory_order_relaxed);
            return result_void();
        });
        auto result = queue->enqueue(std::move(job));
        EXPECT_FALSE(result.has_error());
    }

    int dequeued = 0;
    while (auto result = queue->dequeue()) {
        (void)result.value()->do_work();
        ++dequeued;
    }

    EXPECT_EQ(dequeued, 10);
    EXPECT_EQ(counter.load(), 10);
}

TEST_F(QueueFactoryTest, AdaptiveQueueFunctional) {
    auto queue = queue_factory::create_adaptive_queue();

    std::atomic<int> counter{0};
    for (int i = 0; i < 10; ++i) {
        auto job = std::make_unique<callback_job>([&counter]() -> result_void {
            counter.fetch_add(1, std::memory_order_relaxed);
            return result_void();
        });
        auto result = queue->enqueue(std::move(job));
        EXPECT_FALSE(result.has_error());
    }

    int dequeued = 0;
    while (auto result = queue->dequeue()) {
        (void)result.value()->do_work();
        ++dequeued;
    }

    EXPECT_EQ(dequeued, 10);
    EXPECT_EQ(counter.load(), 10);
}

// ============================================
// Scheduler interface compatibility tests
// ============================================

TEST_F(QueueFactoryTest, AllQueuesImplementSchedulerInterface) {
    // Standard queue
    {
        auto queue = queue_factory::create_standard_queue();
        scheduler_interface* scheduler = queue.get();
        ASSERT_NE(scheduler, nullptr);

        auto job = std::make_unique<callback_job>([]() -> result_void { return result_void(); });
        EXPECT_FALSE(scheduler->schedule(std::move(job)).has_error());
        EXPECT_TRUE(scheduler->get_next_job().has_value());
    }

    // Lockfree queue
    {
        auto queue = queue_factory::create_lockfree_queue();
        scheduler_interface* scheduler = queue.get();
        ASSERT_NE(scheduler, nullptr);

        auto job = std::make_unique<callback_job>([]() -> result_void { return result_void(); });
        EXPECT_FALSE(scheduler->schedule(std::move(job)).has_error());
        EXPECT_TRUE(scheduler->get_next_job().has_value());
    }

    // Adaptive queue
    {
        auto queue = queue_factory::create_adaptive_queue();
        scheduler_interface* scheduler = queue.get();
        ASSERT_NE(scheduler, nullptr);

        auto job = std::make_unique<callback_job>([]() -> result_void { return result_void(); });
        EXPECT_FALSE(scheduler->schedule(std::move(job)).has_error());
        EXPECT_TRUE(scheduler->get_next_job().has_value());
    }
}

// ============================================
// Backward compatibility test
// ============================================

TEST_F(QueueFactoryTest, ExistingCodeStillWorks) {
    // Verify that direct queue construction still works
    // This is the backward compatibility guarantee

    // Direct job_queue construction
    auto q1 = std::make_shared<job_queue>();
    EXPECT_TRUE(q1->empty());

    // Direct lockfree_job_queue construction
    auto q2 = std::make_unique<lockfree_job_queue>();
    EXPECT_TRUE(q2->empty());

    // Direct adaptive_job_queue construction
    auto q3 = std::make_unique<adaptive_job_queue>();
    EXPECT_TRUE(q3->empty());

    // All existing methods work
    auto job = std::make_unique<callback_job>([]() -> result_void { return result_void(); });
    EXPECT_FALSE(q1->enqueue(std::move(job)).has_error());
    EXPECT_TRUE(q1->dequeue().has_value());
}
