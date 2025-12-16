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
 * @brief Performance benchmarks for latency measurement
 *
 * Goal: Measure job submission and execution latency
 * Expected time: < 2 minutes
 */
class LatencyBenchmark : public SystemFixture {
protected:
    size_t ScaleForCI(size_t value) const {
        return std::getenv("CI") ? value / 10 : value;
    }
};

TEST_F(LatencyBenchmark, JobSubmissionLatency) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result.is_ok());

    const size_t job_count = ScaleForCI(5'000);  // Reduced from 10k
    PerformanceMetrics metrics;

    for (size_t i = 0; i < job_count; ++i) {
        auto start = std::chrono::high_resolution_clock::now();

        auto job = std::make_unique<kcenon::thread::callback_job>(
            [this]() -> kcenon::common::VoidResult {
                completed_jobs_.fetch_add(1);
                return kcenon::common::ok();
            }
        );
        auto submit_result = pool_->enqueue(std::move(job));
        ASSERT_TRUE(submit_result.is_ok());

        auto end = std::chrono::high_resolution_clock::now();
        metrics.add_sample(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start));
    }

    EXPECT_TRUE(WaitForJobCompletion(job_count, std::chrono::seconds(30)));

    std::cout << "\nJob Submission Latency Benchmark:\n"
              << "  Jobs submitted: " << job_count << "\n"
              << "  Mean latency: " << metrics.mean() << " ns\n"
              << "  P50 latency: " << metrics.p50() << " ns\n"
              << "  P95 latency: " << metrics.p95() << " ns\n"
              << "  P99 latency: " << metrics.p99() << " ns\n"
              << "  Min latency: " << metrics.min() << " ns\n"
              << "  Max latency: " << metrics.max() << " ns\n";

    // Baseline expectation: P50 < 5000ns (5µs) - relaxed for varying system loads
    EXPECT_LT(metrics.p50(), 5'000);
}

TEST_F(LatencyBenchmark, EndToEndLatency) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result.is_ok());

    const size_t job_count = ScaleForCI(1'000);  // Small count for accurate e2e measurement
    std::vector<std::chrono::nanoseconds> latencies(job_count);
    std::atomic<size_t> completed{0};

    for (size_t i = 0; i < job_count; ++i) {
        auto submit_time = std::make_shared<std::chrono::high_resolution_clock::time_point>(
            std::chrono::high_resolution_clock::now());

        auto job = std::make_unique<kcenon::thread::callback_job>(
            [this, &latencies, &completed, i, submit_time]() -> kcenon::common::VoidResult {
                auto complete_time = std::chrono::high_resolution_clock::now();
                latencies[i] = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    complete_time - *submit_time);
                completed.fetch_add(1);
                completed_jobs_.fetch_add(1);
                return kcenon::common::ok();
            }
        );
        pool_->enqueue(std::move(job));
    }

    EXPECT_TRUE(WaitForJobCompletion(job_count, std::chrono::seconds(10)));

    PerformanceMetrics metrics;
    for (const auto& latency : latencies) {
        metrics.add_sample(latency);
    }

    std::cout << "\nEnd-to-End Latency Benchmark:\n"
              << "  Jobs completed: " << completed.load() << "\n"
              << "  Mean latency: " << metrics.mean() << " ns\n"
              << "  P50 latency: " << metrics.p50() << " ns\n"
              << "  P95 latency: " << metrics.p95() << " ns\n"
              << "  P99 latency: " << metrics.p99() << " ns\n";

    EXPECT_EQ(completed.load(), job_count);
}
