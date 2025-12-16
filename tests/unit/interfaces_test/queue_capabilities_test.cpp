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

/*****************************************************************************
BSD 3-Clause License
*****************************************************************************/

#include <gtest/gtest.h>

#include <kcenon/thread/interfaces/queue_capabilities.h>
#include <kcenon/thread/interfaces/queue_capabilities_interface.h>
#include <kcenon/thread/core/job_queue.h>
#include <kcenon/thread/lockfree/lockfree_job_queue.h>
#include <kcenon/thread/core/callback_job.h>

using namespace kcenon::thread;

// Test queue_capabilities struct default values
TEST(queue_capabilities_test, default_values)
{
    queue_capabilities caps;

    // Verify all default values match mutex-based queue behavior
    EXPECT_TRUE(caps.exact_size);
    EXPECT_TRUE(caps.atomic_empty_check);
    EXPECT_FALSE(caps.lock_free);
    EXPECT_FALSE(caps.wait_free);
    EXPECT_TRUE(caps.supports_batch);
    EXPECT_TRUE(caps.supports_blocking_wait);
    EXPECT_TRUE(caps.supports_stop);
}

// Test queue_capabilities struct equality comparison
TEST(queue_capabilities_test, equality_comparison)
{
    queue_capabilities caps1;
    queue_capabilities caps2;

    // Default-constructed instances should be equal
    EXPECT_EQ(caps1, caps2);

    // Modify one field and verify inequality
    caps2.lock_free = true;
    EXPECT_NE(caps1, caps2);

    // Make them equal again
    caps1.lock_free = true;
    EXPECT_EQ(caps1, caps2);
}

// Test queue_capabilities with custom values (lock-free queue simulation)
TEST(queue_capabilities_test, lockfree_queue_capabilities)
{
    queue_capabilities caps{
        .exact_size = false,            // Lock-free queues may have approximate size
        .atomic_empty_check = false,    // May not be atomically consistent
        .lock_free = true,
        .wait_free = false,
        .supports_batch = false,        // May not support batch operations
        .supports_blocking_wait = false,// Typically non-blocking
        .supports_stop = false          // May not have stop mechanism
    };

    EXPECT_FALSE(caps.exact_size);
    EXPECT_FALSE(caps.atomic_empty_check);
    EXPECT_TRUE(caps.lock_free);
    EXPECT_FALSE(caps.wait_free);
    EXPECT_FALSE(caps.supports_batch);
    EXPECT_FALSE(caps.supports_blocking_wait);
    EXPECT_FALSE(caps.supports_stop);
}

// Test queue_capabilities_interface default implementation
TEST(queue_capabilities_interface_test, default_implementation)
{
    // Create a concrete implementation using default get_capabilities()
    class default_queue : public queue_capabilities_interface {
    public:
        // Uses default get_capabilities() implementation
    };

    default_queue queue;
    auto caps = queue.get_capabilities();

    // Default should match mutex-based queue behavior
    EXPECT_TRUE(caps.exact_size);
    EXPECT_TRUE(caps.atomic_empty_check);
    EXPECT_FALSE(caps.lock_free);
    EXPECT_FALSE(caps.wait_free);
    EXPECT_TRUE(caps.supports_batch);
    EXPECT_TRUE(caps.supports_blocking_wait);
    EXPECT_TRUE(caps.supports_stop);
}

// Test queue_capabilities_interface convenience methods
TEST(queue_capabilities_interface_test, convenience_methods)
{
    class default_queue : public queue_capabilities_interface {
    public:
        // Uses default get_capabilities() implementation
    };

    default_queue queue;

    // Test all convenience methods with default implementation
    EXPECT_TRUE(queue.has_exact_size());
    EXPECT_TRUE(queue.has_atomic_empty());
    EXPECT_FALSE(queue.is_lock_free());
    EXPECT_FALSE(queue.is_wait_free());
    EXPECT_TRUE(queue.supports_batch());
    EXPECT_TRUE(queue.supports_blocking_wait());
    EXPECT_TRUE(queue.supports_stop());
}

// Test queue_capabilities_interface with custom implementation
TEST(queue_capabilities_interface_test, custom_implementation)
{
    class lockfree_queue : public queue_capabilities_interface {
    public:
        [[nodiscard]] auto get_capabilities() const -> queue_capabilities override {
            return queue_capabilities{
                .exact_size = false,
                .atomic_empty_check = false,
                .lock_free = true,
                .wait_free = false,
                .supports_batch = false,
                .supports_blocking_wait = false,
                .supports_stop = false
            };
        }
    };

    lockfree_queue queue;

    // Verify custom implementation values through convenience methods
    EXPECT_FALSE(queue.has_exact_size());
    EXPECT_FALSE(queue.has_atomic_empty());
    EXPECT_TRUE(queue.is_lock_free());
    EXPECT_FALSE(queue.is_wait_free());
    EXPECT_FALSE(queue.supports_batch());
    EXPECT_FALSE(queue.supports_blocking_wait());
    EXPECT_FALSE(queue.supports_stop());
}

// Test dynamic_cast usage pattern as documented
TEST(queue_capabilities_interface_test, dynamic_cast_pattern)
{
    class mutex_queue : public queue_capabilities_interface {
    public:
        // Uses default capabilities
    };

    // Simulate usage pattern from documentation
    auto queue = std::make_unique<mutex_queue>();

    // Cast to base interface pointer
    queue_capabilities_interface* cap = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(cap, nullptr);

    // Query capabilities
    EXPECT_TRUE(cap->has_exact_size());
    EXPECT_FALSE(cap->is_lock_free());
}

// Test wait-free queue capabilities (stronger than lock-free)
TEST(queue_capabilities_test, waitfree_implies_lockfree)
{
    queue_capabilities caps{
        .exact_size = false,
        .atomic_empty_check = true,
        .lock_free = true,      // Wait-free implies lock-free
        .wait_free = true,
        .supports_batch = false,
        .supports_blocking_wait = false,
        .supports_stop = false
    };

    // A wait-free queue should also be lock-free
    EXPECT_TRUE(caps.wait_free);
    EXPECT_TRUE(caps.lock_free);
}

// Test capabilities struct can be constexpr constructed
TEST(queue_capabilities_test, constexpr_construction)
{
    constexpr queue_capabilities caps{};

    static_assert(caps.exact_size == true);
    static_assert(caps.atomic_empty_check == true);
    static_assert(caps.lock_free == false);
    static_assert(caps.wait_free == false);
    static_assert(caps.supports_batch == true);
    static_assert(caps.supports_blocking_wait == true);
    static_assert(caps.supports_stop == true);

    // Runtime check to ensure constexpr works
    EXPECT_TRUE(caps.exact_size);
}

// =============================================================================
// Phase 2: job_queue capabilities tests
// =============================================================================

// Test job_queue returns correct capabilities
TEST(job_queue_capabilities_test, get_capabilities_returns_correct_values)
{
    auto queue = std::make_shared<job_queue>();
    auto caps = queue->get_capabilities();

    // job_queue provides exact, mutex-based operations
    EXPECT_TRUE(caps.exact_size);
    EXPECT_TRUE(caps.atomic_empty_check);
    EXPECT_FALSE(caps.lock_free);
    EXPECT_FALSE(caps.wait_free);
    EXPECT_TRUE(caps.supports_batch);
    EXPECT_TRUE(caps.supports_blocking_wait);
    EXPECT_TRUE(caps.supports_stop);
}

// Test job_queue convenience methods work correctly
TEST(job_queue_capabilities_test, convenience_methods_work)
{
    auto queue = std::make_shared<job_queue>();

    EXPECT_TRUE(queue->has_exact_size());
    EXPECT_TRUE(queue->has_atomic_empty());
    EXPECT_FALSE(queue->is_lock_free());
    EXPECT_FALSE(queue->is_wait_free());
    EXPECT_TRUE(queue->supports_batch());
    EXPECT_TRUE(queue->supports_blocking_wait());
    EXPECT_TRUE(queue->supports_stop());
}

// Test job_queue can be cast to queue_capabilities_interface
TEST(job_queue_capabilities_test, dynamic_cast_to_interface)
{
    auto queue = std::make_shared<job_queue>();

    // Cast to capabilities interface
    auto* cap = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(cap, nullptr);

    // Verify capabilities through interface
    EXPECT_TRUE(cap->has_exact_size());
    EXPECT_FALSE(cap->is_lock_free());
}

// Test job_queue still works as scheduler_interface
TEST(job_queue_capabilities_test, scheduler_interface_still_works)
{
    auto queue = std::make_shared<job_queue>();

    // Cast to scheduler_interface (backward compatibility)
    scheduler_interface* scheduler = queue.get();
    ASSERT_NE(scheduler, nullptr);

    // Basic operations should still work
    EXPECT_TRUE(queue->empty());
    EXPECT_EQ(queue->size(), 0);
}

// Test that job_queue capabilities are consistent across multiple calls
TEST(job_queue_capabilities_test, capabilities_are_consistent)
{
    auto queue = std::make_shared<job_queue>();

    auto caps1 = queue->get_capabilities();
    auto caps2 = queue->get_capabilities();

    EXPECT_EQ(caps1, caps2);
}

// =============================================================================
// Phase 3: lockfree_job_queue capabilities tests
// =============================================================================

// Test lockfree_job_queue returns correct capabilities
TEST(lockfree_queue_capabilities_test, get_capabilities_returns_correct_values)
{
    lockfree_job_queue queue;
    auto caps = queue.get_capabilities();

    // Lock-free queues have approximate size
    EXPECT_FALSE(caps.exact_size);

    // Empty check is non-atomic
    EXPECT_FALSE(caps.atomic_empty_check);

    // Is lock-free
    EXPECT_TRUE(caps.lock_free);

    // Not wait-free (dequeue is lock-free, not wait-free)
    EXPECT_FALSE(caps.wait_free);

    // No batch operations
    EXPECT_FALSE(caps.supports_batch);

    // No blocking wait (spin-wait only)
    EXPECT_FALSE(caps.supports_blocking_wait);

    // No stop() method
    EXPECT_FALSE(caps.supports_stop);
}

// Test lockfree_job_queue convenience methods work correctly
TEST(lockfree_queue_capabilities_test, convenience_methods_work)
{
    lockfree_job_queue queue;

    EXPECT_FALSE(queue.has_exact_size());
    EXPECT_FALSE(queue.has_atomic_empty());
    EXPECT_TRUE(queue.is_lock_free());
    EXPECT_FALSE(queue.is_wait_free());
    EXPECT_FALSE(queue.supports_batch());
    EXPECT_FALSE(queue.supports_blocking_wait());
    EXPECT_FALSE(queue.supports_stop());
}

// Test lockfree_job_queue can be cast to interfaces
TEST(lockfree_queue_capabilities_test, dynamic_cast_to_interface)
{
    lockfree_job_queue queue;

    // Should be castable to queue_capabilities_interface
    auto* cap_interface = dynamic_cast<queue_capabilities_interface*>(&queue);
    ASSERT_NE(cap_interface, nullptr);

    // And to scheduler_interface
    auto* sched_interface = dynamic_cast<scheduler_interface*>(&queue);
    ASSERT_NE(sched_interface, nullptr);
}

// Test lockfree_job_queue capabilities are consistent across multiple calls
TEST(lockfree_queue_capabilities_test, capabilities_are_consistent)
{
    lockfree_job_queue queue;

    // Get capabilities multiple times - should be consistent
    auto caps1 = queue.get_capabilities();
    auto caps2 = queue.get_capabilities();

    EXPECT_EQ(caps1.exact_size, caps2.exact_size);
    EXPECT_EQ(caps1.atomic_empty_check, caps2.atomic_empty_check);
    EXPECT_EQ(caps1.lock_free, caps2.lock_free);
    EXPECT_EQ(caps1.wait_free, caps2.wait_free);
    EXPECT_EQ(caps1.supports_batch, caps2.supports_batch);
    EXPECT_EQ(caps1.supports_blocking_wait, caps2.supports_blocking_wait);
    EXPECT_EQ(caps1.supports_stop, caps2.supports_stop);

    // Also use the equality operator
    EXPECT_EQ(caps1, caps2);
}

// Test lockfree_job_queue scheduler interface still works
TEST(lockfree_queue_capabilities_test, scheduler_interface_still_works)
{
    lockfree_job_queue queue;
    scheduler_interface* scheduler = &queue;

    // Schedule via interface
    auto result = scheduler->schedule(
        std::make_unique<callback_job>(
            []() -> std::optional<std::string> { return std::nullopt; }));
    EXPECT_FALSE(result.is_err());

    // Get via interface
    auto job = scheduler->get_next_job();
    EXPECT_TRUE(job.is_ok());
    EXPECT_NE(job.value(), nullptr);
}

// =============================================================================
// Phase 4: Comparison tests between queue types
// =============================================================================

// Test lockfree vs mutex-based queue capabilities
TEST(queue_capabilities_comparison_test, lockfree_vs_mutex_capabilities)
{
    auto mutex_queue = std::make_shared<job_queue>();
    lockfree_job_queue lockfree_queue;

    auto mutex_caps = mutex_queue->get_capabilities();
    auto lockfree_caps = lockfree_queue.get_capabilities();

    // Mutex queue has exact size, lockfree doesn't
    EXPECT_TRUE(mutex_caps.exact_size);
    EXPECT_FALSE(lockfree_caps.exact_size);

    // Mutex queue has atomic empty check, lockfree doesn't
    EXPECT_TRUE(mutex_caps.atomic_empty_check);
    EXPECT_FALSE(lockfree_caps.atomic_empty_check);

    // Lockfree is lock-free, mutex isn't
    EXPECT_FALSE(mutex_caps.lock_free);
    EXPECT_TRUE(lockfree_caps.lock_free);

    // Neither is wait-free
    EXPECT_FALSE(mutex_caps.wait_free);
    EXPECT_FALSE(lockfree_caps.wait_free);

    // Mutex supports batch, lockfree doesn't
    EXPECT_TRUE(mutex_caps.supports_batch);
    EXPECT_FALSE(lockfree_caps.supports_batch);

    // Mutex supports blocking wait, lockfree doesn't
    EXPECT_TRUE(mutex_caps.supports_blocking_wait);
    EXPECT_FALSE(lockfree_caps.supports_blocking_wait);

    // Mutex supports stop, lockfree doesn't
    EXPECT_TRUE(mutex_caps.supports_stop);
    EXPECT_FALSE(lockfree_caps.supports_stop);

    // Capabilities should not be equal
    EXPECT_NE(mutex_caps, lockfree_caps);
}
