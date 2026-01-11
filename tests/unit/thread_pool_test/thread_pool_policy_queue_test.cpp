/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, DongCheol Shin
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

/**
 * @file thread_pool_policy_queue_test.cpp
 * @brief Unit tests for thread_pool with policy_queue support (Issue #450)
 *
 * These tests verify that thread_pool works correctly with the new
 * policy_queue adapter system while maintaining backward compatibility
 * with the legacy job_queue.
 */

#include "gtest/gtest.h"

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/job_queue.h>
#include <kcenon/thread/adapters/job_queue_adapter.h>
#include <kcenon/thread/adapters/policy_queue_adapter.h>

#include <atomic>
#include <chrono>
#include <thread>

using namespace kcenon::thread;

// ============================================
// job_queue_adapter tests
// ============================================

TEST(thread_pool_policy_queue_test, job_queue_adapter_basic_construction) {
    // Test that thread_pool can be constructed with job_queue_adapter
    auto adapter = std::make_unique<job_queue_adapter>();
    auto pool = std::make_shared<thread_pool>("test_pool", std::move(adapter));

    EXPECT_NE(pool, nullptr);
}

TEST(thread_pool_policy_queue_test, job_queue_adapter_with_existing_queue) {
    // Test that thread_pool can use job_queue_adapter wrapping an existing queue
    auto queue = std::make_shared<job_queue>();
    auto adapter = std::make_unique<job_queue_adapter>(queue);
    auto pool = std::make_shared<thread_pool>("test_pool", std::move(adapter));

    EXPECT_NE(pool, nullptr);
}

TEST(thread_pool_policy_queue_test, job_queue_adapter_enqueue_and_execute) {
    // Test that jobs can be enqueued and executed through the adapter
    auto adapter = std::make_unique<job_queue_adapter>();
    auto pool = std::make_shared<thread_pool>("test_pool", std::move(adapter));

    auto worker = std::make_unique<thread_worker>();
    auto result = pool->enqueue(std::move(worker));
    EXPECT_FALSE(result.is_err());

    auto start_result = pool->start();
    EXPECT_FALSE(start_result.is_err());

    std::atomic<int> counter{0};
    auto job = std::make_unique<callback_job>([&counter]() -> kcenon::common::VoidResult {
        counter.fetch_add(1);
        return kcenon::common::ok();
    });

    result = pool->enqueue(std::move(job));
    EXPECT_FALSE(result.is_err());

    // Wait for job to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(counter.load(), 1);

    auto stop_result = pool->stop();
    EXPECT_FALSE(stop_result.is_err());
}

TEST(thread_pool_policy_queue_test, job_queue_adapter_batch_enqueue) {
    // Test batch enqueue through the adapter
    auto adapter = std::make_unique<job_queue_adapter>();
    auto pool = std::make_shared<thread_pool>("test_pool", std::move(adapter));

    auto worker = std::make_unique<thread_worker>();
    pool->enqueue(std::move(worker));
    pool->start();

    std::atomic<int> counter{0};
    std::vector<std::unique_ptr<job>> jobs;
    for (int i = 0; i < 5; ++i) {
        jobs.push_back(std::make_unique<callback_job>([&counter]() -> kcenon::common::VoidResult {
            counter.fetch_add(1);
            return kcenon::common::ok();
        }));
    }

    auto result = pool->enqueue_batch(std::move(jobs));
    EXPECT_FALSE(result.is_err());

    // Wait for jobs to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(counter.load(), 5);

    pool->stop();
}

// ============================================
// Backward compatibility tests
// ============================================

TEST(thread_pool_policy_queue_test, backward_compatibility_default_constructor) {
    // Test that the default constructor still works
    auto pool = std::make_shared<thread_pool>();
    EXPECT_NE(pool, nullptr);

    auto worker = std::make_unique<thread_worker>();
    auto result = pool->enqueue(std::move(worker));
    EXPECT_FALSE(result.is_err());
}

TEST(thread_pool_policy_queue_test, backward_compatibility_custom_job_queue) {
    // Test that the constructor with custom job_queue still works
    auto queue = std::make_shared<job_queue>();
    auto pool = std::make_shared<thread_pool>("test_pool", queue);
    EXPECT_NE(pool, nullptr);

    auto worker = std::make_unique<thread_worker>();
    auto result = pool->enqueue(std::move(worker));
    EXPECT_FALSE(result.is_err());
}

TEST(thread_pool_policy_queue_test, backward_compatibility_get_job_queue) {
    // Test that get_job_queue() still works for default construction
    auto pool = std::make_shared<thread_pool>();
    auto queue = pool->get_job_queue();
    EXPECT_NE(queue, nullptr);
}

// ============================================
// Adapter interface tests
// ============================================

TEST(thread_pool_policy_queue_test, adapter_interface_stop_and_is_stopped) {
    auto adapter = std::make_unique<job_queue_adapter>();

    EXPECT_FALSE(adapter->is_stopped());

    adapter->stop();

    EXPECT_TRUE(adapter->is_stopped());
}

TEST(thread_pool_policy_queue_test, adapter_interface_size_and_empty) {
    auto adapter = std::make_unique<job_queue_adapter>();

    EXPECT_TRUE(adapter->empty());
    EXPECT_EQ(adapter->size(), 0);

    auto job = std::make_unique<callback_job>([]() -> kcenon::common::VoidResult {
        return kcenon::common::ok();
    });
    (void)adapter->enqueue(std::move(job));

    EXPECT_FALSE(adapter->empty());
    EXPECT_EQ(adapter->size(), 1);

    adapter->clear();

    EXPECT_TRUE(adapter->empty());
    EXPECT_EQ(adapter->size(), 0);
}

TEST(thread_pool_policy_queue_test, adapter_interface_get_capabilities) {
    auto adapter = std::make_unique<job_queue_adapter>();
    auto caps = adapter->get_capabilities();

    // job_queue should report these capabilities
    EXPECT_TRUE(caps.supports_batch);
    EXPECT_TRUE(caps.supports_stop);
}

TEST(thread_pool_policy_queue_test, adapter_interface_to_string) {
    auto adapter = std::make_unique<job_queue_adapter>();
    auto str = adapter->to_string();

    // Should return a non-empty string
    EXPECT_FALSE(str.empty());
}
