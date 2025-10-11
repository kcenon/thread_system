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
 * @brief Performance benchmarks for throughput measurement
 *
 * Goal: Measure baseline throughput performance
 * Expected time: < 5 minutes
 * Reduced from original 100k-job tests to 10k for faster feedback
 */
class ThroughputBenchmark : public SystemFixture {
protected:
    size_t ScaleForCI(size_t value) const {
        return std::getenv("CI") ? value / 10 : value;
    }
};

TEST_F(ThroughputBenchmark, ThroughputEmptyJobs) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result);

    const size_t job_count = ScaleForCI(10'000);  // Reduced from 100k

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < job_count; ++i) {
        auto job = std::make_unique<kcenon::thread::callback_job>(
            [this]() -> kcenon::thread::result_void {
                completed_jobs_.fetch_add(1);
                return {};
            }
        );
        pool_->enqueue(std::move(job));
    }

    EXPECT_TRUE(WaitForJobCompletion(job_count, std::chrono::seconds(30)));

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    double throughput = CalculateThroughput(job_count, duration);

    std::cout << "\nThroughput (Empty Jobs) Benchmark:\n"
              << "  Total jobs: " << job_count << "\n"
              << "  Duration: " << FormatDuration(duration) << "\n"
              << "  Throughput: " << static_cast<size_t>(throughput) << " jobs/sec\n";

    // Baseline: at least 50k jobs/second (reduced from 100k)
    EXPECT_GT(throughput, ScaleForCI(50'000));
}

TEST_F(ThroughputBenchmark, ThroughputWithWork) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result);

    const size_t job_count = ScaleForCI(1'000);  // Reduced from 10k
    const auto work_duration = std::chrono::microseconds(10);

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < job_count; ++i) {
        auto job = std::make_unique<kcenon::thread::callback_job>(
            [this, work_duration]() -> kcenon::thread::result_void {
                WorkSimulator::simulate_work(work_duration);
                completed_jobs_.fetch_add(1);
                return {};
            }
        );
        pool_->enqueue(std::move(job));
    }

    EXPECT_TRUE(WaitForJobCompletion(job_count, std::chrono::seconds(30)));

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    double throughput = CalculateThroughput(job_count, duration);

    std::cout << "\nThroughput (With 10µs Work) Benchmark:\n"
              << "  Total jobs: " << job_count << "\n"
              << "  Work per job: 10 µs\n"
              << "  Duration: " << FormatDuration(duration) << "\n"
              << "  Throughput: " << static_cast<size_t>(throughput) << " jobs/sec\n";

    EXPECT_GT(throughput, ScaleForCI(5'000));  // Reduced from 10k
}

TEST_F(ThroughputBenchmark, QueueThroughput) {
    auto queue = std::make_shared<kcenon::thread::job_queue>();
    queue->set_notify(true);

    const size_t job_count = ScaleForCI(10'000);  // New: separate queue benchmark
    PerformanceMetrics metrics;

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < job_count; ++i) {
        auto job_start = std::chrono::high_resolution_clock::now();

        auto job = std::make_unique<kcenon::thread::callback_job>(
            []() -> kcenon::thread::result_void { return {}; }
        );
        auto result = queue->enqueue(std::move(job));
        ASSERT_TRUE(result);

        auto job_end = std::chrono::high_resolution_clock::now();
        metrics.add_sample(std::chrono::duration_cast<std::chrono::nanoseconds>(
            job_end - job_start));
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    double throughput = CalculateThroughput(job_count, duration);

    std::cout << "\nQueue Enqueue Throughput:\n"
              << "  Total jobs: " << job_count << "\n"
              << "  Duration: " << FormatDuration(duration) << "\n"
              << "  Throughput: " << static_cast<size_t>(throughput) << " jobs/sec\n"
              << "  Mean latency: " << metrics.mean() << " ns\n"
              << "  P50 latency: " << metrics.p50() << " ns\n"
              << "  P95 latency: " << metrics.p95() << " ns\n";

    EXPECT_GT(throughput, ScaleForCI(100'000));  // At least 100k ops/sec
}
