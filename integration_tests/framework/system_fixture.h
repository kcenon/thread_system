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

#pragma once

#include <gtest/gtest.h>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <functional>

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/typed_thread_pool.h>
#include <kcenon/thread/core/job_queue.h>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/core/error_handling.h>
#include <kcenon/thread/interfaces/thread_context.h>

namespace integration_tests {

/**
 * @class SystemFixture
 * @brief Base fixture for integration tests providing common setup and teardown
 *
 * This fixture provides:
 * - Thread pool creation and management
 * - Job queue initialization
 * - Common test utilities and helpers
 * - Cleanup and verification
 */
class SystemFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize default context
        context_ = kcenon::thread::thread_context();

        // Reset counters
        completed_jobs_.store(0);
        failed_jobs_.store(0);
    }

    void TearDown() override {
        // Clean up thread pool if still running
        if (pool_ && pool_->is_running()) {
            auto result = pool_->stop(true);
            EXPECT_TRUE(result) << "Failed to stop pool: " << result.get_error().to_string();
        }

        pool_.reset();
        job_queue_.reset();
    }

    /**
     * @brief Create a standard thread pool with specified number of workers
     */
    void CreateThreadPool(size_t worker_count, const std::string& name = "test_pool") {
        pool_ = std::make_shared<kcenon::thread::thread_pool>(name, context_);
        job_queue_ = pool_->get_job_queue();

        // Add workers
        for (size_t i = 0; i < worker_count; ++i) {
            auto worker = std::make_unique<kcenon::thread::thread_worker>();
            auto result = pool_->enqueue(std::move(worker));
            ASSERT_TRUE(result) << "Failed to add worker: " << result.get_error().to_string();
        }
    }

    /**
     * @brief Wait for a condition to become true with timeout
     */
    template<typename Predicate>
    bool WaitForCondition(Predicate pred,
                         std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
        auto start = std::chrono::steady_clock::now();
        while (!pred()) {
            if (std::chrono::steady_clock::now() - start > timeout) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return true;
    }

    /**
     * @brief Submit a simple job that increments completed counter
     */
    void SubmitCountingJob() {
        auto job = std::make_unique<kcenon::thread::callback_job>(
            [this]() -> kcenon::thread::result_void {
                completed_jobs_.fetch_add(1);
                return {};
            }
        );
        auto result = pool_->enqueue(std::move(job));
        EXPECT_TRUE(result) << "Failed to enqueue job: " << result.get_error().to_string();
    }

    /**
     * @brief Submit a job with custom work function
     */
    void SubmitJob(std::function<void()> work) {
        auto job = std::make_unique<kcenon::thread::callback_job>(
            [this, work = std::move(work)]() -> kcenon::thread::result_void {
                try {
                    work();
                    completed_jobs_.fetch_add(1);
                    return {};
                } catch (...) {
                    failed_jobs_.fetch_add(1);
                    return kcenon::thread::error{
                        kcenon::thread::error_code::job_execution_failed,
                        "Job execution threw exception"
                    };
                }
            }
        );
        auto result = pool_->enqueue(std::move(job));
        EXPECT_TRUE(result) << "Failed to enqueue job: " << result.get_error().to_string();
    }

    /**
     * @brief Wait for all submitted jobs to complete
     */
    bool WaitForJobCompletion(size_t expected_count,
                             std::chrono::milliseconds timeout = std::chrono::seconds(10)) {
        return WaitForCondition([this, expected_count]() {
            return completed_jobs_.load() >= expected_count;
        }, timeout);
    }

    // Protected member variables
    std::shared_ptr<kcenon::thread::thread_pool> pool_;
    std::shared_ptr<kcenon::thread::job_queue> job_queue_;
    kcenon::thread::thread_context context_;

    std::atomic<size_t> completed_jobs_{0};
    std::atomic<size_t> failed_jobs_{0};
};

/**
 * @class MultiSystemFixture
 * @brief Fixture for tests requiring multiple thread pools or systems
 */
class MultiSystemFixture : public ::testing::Test {
protected:
    void SetUp() override {
        context_ = kcenon::thread::thread_context();
        completed_jobs_.store(0);
    }

    void TearDown() override {
        // Stop all pools
        for (auto& pool : pools_) {
            if (pool && pool->is_running()) {
                pool->stop(true);
            }
        }
        pools_.clear();
    }

    /**
     * @brief Create multiple thread pools
     */
    void CreateMultiplePools(size_t pool_count, size_t workers_per_pool) {
        for (size_t i = 0; i < pool_count; ++i) {
            auto pool = std::make_shared<kcenon::thread::thread_pool>(
                "pool_" + std::to_string(i), context_);

            for (size_t j = 0; j < workers_per_pool; ++j) {
                auto worker = std::make_unique<kcenon::thread::thread_worker>();
                auto result = pool->enqueue(std::move(worker));
                ASSERT_TRUE(result) << "Failed to add worker to pool " << i;
            }

            pools_.push_back(std::move(pool));
        }
    }

    /**
     * @brief Start all pools
     */
    void StartAllPools() {
        for (auto& pool : pools_) {
            auto result = pool->start();
            ASSERT_TRUE(result) << "Failed to start pool";
        }
    }

    /**
     * @brief Wait for condition with timeout
     */
    template<typename Predicate>
    bool WaitForCondition(Predicate pred,
                         std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
        auto start = std::chrono::steady_clock::now();
        while (!pred()) {
            if (std::chrono::steady_clock::now() - start > timeout) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return true;
    }

    std::vector<std::shared_ptr<kcenon::thread::thread_pool>> pools_;
    kcenon::thread::thread_context context_;
    std::atomic<size_t> completed_jobs_{0};
};

} // namespace integration_tests
