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

#include "../framework/system_fixture.h"
#include "../framework/test_helpers.h"
#include <gtest/gtest.h>

using namespace integration_tests;

/**
 * @brief Stress tests for sustained load
 *
 * Goal: Verify system stability under continuous load
 * Expected time: 5-30 minutes (manual execution only)
 * These tests are NOT run in regular CI
 */
class SustainedLoadStress : public SystemFixture {};

TEST_F(SustainedLoadStress, ContinuousLoad5Minutes) {
    CreateThreadPool(8);

    auto result = pool_->start();
    ASSERT_TRUE(result);

    const size_t duration_seconds = std::getenv("CI") ? 10 : 300;  // 5 min in normal, 10s in CI
    const size_t jobs_per_second = 1'000;

    std::atomic<bool> stop_flag{false};
    std::atomic<size_t> total_submitted{0};

    std::thread submitter([this, &stop_flag, &total_submitted, jobs_per_second]() {
        while (!stop_flag.load()) {
            for (size_t i = 0; i < jobs_per_second / 10; ++i) {
                auto job = std::make_unique<kcenon::thread::callback_job>(
                    [this]() -> kcenon::thread::result_void {
                        WorkSimulator::simulate_work(std::chrono::microseconds(10));
                        completed_jobs_.fetch_add(1);
                        return {};
                    }
                );
                pool_->enqueue(std::move(job));
                total_submitted.fetch_add(1);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
    stop_flag.store(true);
    submitter.join();

    // Wait for all jobs to complete
    EXPECT_TRUE(WaitForJobCompletion(total_submitted.load(), std::chrono::seconds(60)));

    std::cout << "\nSustained Load Stress Test:\n"
              << "  Duration: " << duration_seconds << " seconds\n"
              << "  Total jobs: " << total_submitted.load() << "\n"
              << "  Completed jobs: " << completed_jobs_.load() << "\n"
              << "  Average rate: " << (total_submitted.load() / duration_seconds)
              << " jobs/sec\n";

    EXPECT_EQ(completed_jobs_.load(), total_submitted.load());
}

TEST_F(SustainedLoadStress, HighContentionLoad) {
    CreateThreadPool(8);

    auto result = pool_->start();
    ASSERT_TRUE(result);

    const size_t num_producers = std::getenv("CI") ? 4 : 16;
    const size_t jobs_per_producer = std::getenv("CI") ? 1'000 : 10'000;
    const size_t total_jobs = num_producers * jobs_per_producer;

    std::atomic<size_t> submitted{0};

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> producers;
    for (size_t t = 0; t < num_producers; ++t) {
        producers.emplace_back([this, &submitted, jobs_per_producer]() {
            for (size_t i = 0; i < jobs_per_producer; ++i) {
                auto job = std::make_unique<kcenon::thread::callback_job>(
                    [this]() -> kcenon::thread::result_void {
                        completed_jobs_.fetch_add(1);
                        return {};
                    }
                );
                auto result = pool_->enqueue(std::move(job));
                if (result) {
                    submitted.fetch_add(1);
                }
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }

    EXPECT_TRUE(WaitForJobCompletion(total_jobs, std::chrono::seconds(120)));

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    double throughput = CalculateThroughput(total_jobs, duration);

    std::cout << "\nHigh Contention Stress:\n"
              << "  Producers: " << num_producers << "\n"
              << "  Total jobs: " << total_jobs << "\n"
              << "  Duration: " << FormatDuration(duration) << "\n"
              << "  Throughput: " << static_cast<size_t>(throughput) << " jobs/sec\n";

    EXPECT_EQ(submitted.load(), total_jobs);
}

TEST_F(SustainedLoadStress, RepeatedStartStop) {
    CreateThreadPool(4);

    const size_t cycles = std::getenv("CI") ? 5 : 20;
    const size_t jobs_per_cycle = std::getenv("CI") ? 10 : 100;

    for (size_t cycle = 0; cycle < cycles; ++cycle) {
        auto result = pool_->start();
        ASSERT_TRUE(result) << "Failed to start in cycle " << cycle;

        for (size_t i = 0; i < jobs_per_cycle; ++i) {
            SubmitCountingJob();
        }

        size_t expected = (cycle + 1) * jobs_per_cycle;
        EXPECT_TRUE(WaitForJobCompletion(expected, std::chrono::seconds(10)));

        result = pool_->stop();
        ASSERT_TRUE(result) << "Failed to stop in cycle " << cycle;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "\nRepeated Start/Stop Stress:\n"
              << "  Cycles: " << cycles << "\n"
              << "  Total jobs: " << completed_jobs_.load() << "\n";

    EXPECT_EQ(completed_jobs_.load(), cycles * jobs_per_cycle);
}
