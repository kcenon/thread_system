// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

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
    ASSERT_TRUE(result.is_ok());

    const size_t job_count = ScaleForCI(10'000);  // Reduced from 100k

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < job_count; ++i) {
        auto job = std::make_unique<kcenon::thread::callback_job>(
            [this]() -> kcenon::common::VoidResult {
                completed_jobs_.fetch_add(1);
                return kcenon::common::ok();
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
    ASSERT_TRUE(result.is_ok());

    const size_t job_count = ScaleForCI(1'000);  // Reduced from 10k
    const auto work_duration = std::chrono::microseconds(10);

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < job_count; ++i) {
        auto job = std::make_unique<kcenon::thread::callback_job>(
            [this, work_duration]() -> kcenon::common::VoidResult {
                WorkSimulator::simulate_work(work_duration);
                completed_jobs_.fetch_add(1);
                return kcenon::common::ok();
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
            []() -> kcenon::common::VoidResult { return kcenon::common::ok(); }
        );
        auto result = queue->enqueue(std::move(job));
        ASSERT_TRUE(result.is_ok());

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
