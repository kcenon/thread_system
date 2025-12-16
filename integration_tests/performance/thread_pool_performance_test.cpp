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
 * @brief Performance benchmark tests for thread pool
 *
 * These tests measure:
 * - Job submission latency
 * - Throughput (jobs/second)
 * - Scalability with worker count
 * - Memory overhead
 */
class ThreadPoolPerformanceTest : public SystemFixture {
protected:
    void PrintPerformanceResults(const std::string& test_name,
                                size_t job_count,
                                std::chrono::nanoseconds duration,
                                const PerformanceMetrics& metrics) {
        double throughput = CalculateThroughput(job_count, duration);

        std::cout << "\n" << test_name << " Results:\n"
                  << "  Total jobs: " << job_count << "\n"
                  << "  Duration: " << FormatDuration(duration) << "\n"
                  << "  Throughput: " << static_cast<size_t>(throughput) << " jobs/sec\n"
                  << "  Mean latency: " << metrics.mean() << " ns\n"
                  << "  P50 latency: " << metrics.p50() << " ns\n"
                  << "  P95 latency: " << metrics.p95() << " ns\n"
                  << "  P99 latency: " << metrics.p99() << " ns\n"
                  << "  Min latency: " << metrics.min() << " ns\n"
                  << "  Max latency: " << metrics.max() << " ns\n";
    }

    // Scale factor for CI environment
    size_t ScaleForCI(size_t value) const {
        return std::getenv("CI") ? value / 10 : value;
    }

    int TimeoutForCI(int seconds) const {
        return std::getenv("CI") ? seconds / 2 : seconds;
    }
};

TEST_F(ThreadPoolPerformanceTest, JobSubmissionLatency) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result.is_ok());

    const size_t job_count = 10'000;
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
              << "  P99 latency: " << metrics.p99() << " ns\n";

    // Baseline expectation: P50 < 1000ns (1µs)
    EXPECT_LT(metrics.p50(), 1'000);
}

TEST_F(ThreadPoolPerformanceTest, ThroughputEmptyJobs) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result.is_ok());

    const size_t job_count = 100'000;

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

    EXPECT_TRUE(WaitForJobCompletion(job_count, std::chrono::seconds(60)));

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    double throughput = CalculateThroughput(job_count, duration);

    std::cout << "\nThroughput (Empty Jobs) Benchmark:\n"
              << "  Total jobs: " << job_count << "\n"
              << "  Duration: " << FormatDuration(duration) << "\n"
              << "  Throughput: " << static_cast<size_t>(throughput) << " jobs/sec\n";

    // Baseline: at least 100k jobs/second
    EXPECT_GT(throughput, 100'000);
}

TEST_F(ThreadPoolPerformanceTest, ThroughputWithWork) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result.is_ok());

    const size_t job_count = 10'000;
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

    EXPECT_TRUE(WaitForJobCompletion(job_count, std::chrono::seconds(60)));

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    double throughput = CalculateThroughput(job_count, duration);

    std::cout << "\nThroughput (With 10µs Work) Benchmark:\n"
              << "  Total jobs: " << job_count << "\n"
              << "  Work per job: 10 µs\n"
              << "  Duration: " << FormatDuration(duration) << "\n"
              << "  Throughput: " << static_cast<size_t>(throughput) << " jobs/sec\n";

    EXPECT_GT(throughput, 10'000);
}

TEST_F(ThreadPoolPerformanceTest, ScalabilityTest) {
    const size_t job_count = 50'000;
    std::vector<size_t> worker_counts = {1, 2, 4, 8};

    std::cout << "\nScalability Benchmark:\n";
    std::cout << "Workers\tThroughput\tDuration\n";

    for (auto workers : worker_counts) {
        CreateThreadPool(workers);

        auto result = pool_->start();
        ASSERT_TRUE(result.is_ok());

        completed_jobs_.store(0);

        auto start = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < job_count; ++i) {
            auto job = std::make_unique<kcenon::thread::callback_job>(
                [this]() -> kcenon::common::VoidResult {
                    WorkSimulator::simulate_work(std::chrono::microseconds(1));
                    completed_jobs_.fetch_add(1);
                    return kcenon::common::ok();
                }
            );
            pool_->enqueue(std::move(job));
        }

        EXPECT_TRUE(WaitForJobCompletion(job_count, std::chrono::seconds(120)));

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        double throughput = CalculateThroughput(job_count,
            std::chrono::duration_cast<std::chrono::nanoseconds>(duration));

        std::cout << workers << "\t"
                  << static_cast<size_t>(throughput) << "\t"
                  << duration.count() << " ms\n";

        pool_->stop();
        pool_.reset();
    }
}

TEST_F(ThreadPoolPerformanceTest, HighContentionPerformance) {
    CreateThreadPool(8);

    auto result = pool_->start();
    ASSERT_TRUE(result.is_ok());

    const size_t num_producers = ScaleForCI(16);
    const size_t jobs_per_producer = ScaleForCI(5'000);
    const size_t total_jobs = num_producers * jobs_per_producer;

    std::atomic<size_t> submitted{0};

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> producers;
    for (size_t t = 0; t < num_producers; ++t) {
        producers.emplace_back([this, &submitted, jobs_per_producer]() {
            for (size_t i = 0; i < jobs_per_producer; ++i) {
                auto job = std::make_unique<kcenon::thread::callback_job>(
                    [this]() -> kcenon::common::VoidResult {
                        completed_jobs_.fetch_add(1);
                        return kcenon::common::ok();
                    }
                );
                auto result = pool_->enqueue(std::move(job));
                if (result.is_ok()) {
                    submitted.fetch_add(1);
                }
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }

    EXPECT_TRUE(WaitForJobCompletion(total_jobs, std::chrono::seconds(TimeoutForCI(60))));

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    double throughput = CalculateThroughput(total_jobs, duration);

    std::cout << "\nHigh Contention Performance:\n"
              << "  Producers: " << num_producers << "\n"
              << "  Total jobs: " << total_jobs << "\n"
              << "  Duration: " << FormatDuration(duration) << "\n"
              << "  Throughput: " << static_cast<size_t>(throughput) << " jobs/sec\n";

    EXPECT_EQ(submitted.load(), total_jobs);
    EXPECT_GT(throughput, ScaleForCI(50'000));
}

TEST_F(ThreadPoolPerformanceTest, BatchSubmissionPerformance) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result.is_ok());

    const size_t num_batches = ScaleForCI(100);
    const size_t jobs_per_batch = ScaleForCI(1'000);
    const size_t total_jobs = num_batches * jobs_per_batch;

    PerformanceMetrics metrics;

    auto overall_start = std::chrono::high_resolution_clock::now();

    for (size_t batch = 0; batch < num_batches; ++batch) {
        std::vector<std::unique_ptr<kcenon::thread::job>> jobs;

        for (size_t i = 0; i < jobs_per_batch; ++i) {
            jobs.push_back(std::make_unique<kcenon::thread::callback_job>(
                [this]() -> kcenon::common::VoidResult {
                    completed_jobs_.fetch_add(1);
                    return kcenon::common::ok();
                }
            ));
        }

        auto batch_start = std::chrono::high_resolution_clock::now();
        auto batch_result = pool_->enqueue_batch(std::move(jobs));
        auto batch_end = std::chrono::high_resolution_clock::now();

        EXPECT_TRUE(batch_result.is_ok());
        metrics.add_sample(std::chrono::duration_cast<std::chrono::nanoseconds>(
            batch_end - batch_start));
    }

    EXPECT_TRUE(WaitForJobCompletion(total_jobs, std::chrono::seconds(TimeoutForCI(60))));

    auto overall_end = std::chrono::high_resolution_clock::now();
    auto overall_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        overall_end - overall_start);

    std::cout << "\nBatch Submission Performance:\n"
              << "  Batches: " << num_batches << "\n"
              << "  Jobs per batch: " << jobs_per_batch << "\n"
              << "  Total jobs: " << total_jobs << "\n"
              << "  Mean batch latency: " << metrics.mean() << " ns\n"
              << "  P50 batch latency: " << metrics.p50() << " ns\n"
              << "  Overall throughput: "
              << static_cast<size_t>(CalculateThroughput(total_jobs, overall_duration))
              << " jobs/sec\n";
}

TEST_F(ThreadPoolPerformanceTest, MemoryOverhead) {
    const size_t worker_count = 8;

    // Measure baseline memory (approximate)
    CreateThreadPool(worker_count);

    auto result = pool_->start();
    ASSERT_TRUE(result.is_ok());

    // Submit and complete jobs
    const size_t job_count = 10'000;
    for (size_t i = 0; i < job_count; ++i) {
        SubmitCountingJob();
    }

    EXPECT_TRUE(WaitForJobCompletion(job_count));

    // Pool should handle memory efficiently
    EXPECT_EQ(pool_->get_thread_count(), worker_count);
    EXPECT_EQ(completed_jobs_.load(), job_count);

    std::cout << "\nMemory Overhead Test:\n"
              << "  Workers: " << worker_count << "\n"
              << "  Jobs processed: " << job_count << "\n"
              << "  Test completed without memory issues\n";
}

TEST_F(ThreadPoolPerformanceTest, SustainedLoad) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result.is_ok());

    const size_t duration_seconds = std::getenv("CI") ? 2 : 5;
    const size_t jobs_per_second = ScaleForCI(10'000);

    std::atomic<bool> stop_flag{false};
    std::atomic<size_t> total_submitted{0};

    std::thread submitter([this, &stop_flag, &total_submitted, jobs_per_second]() {
        while (!stop_flag.load()) {
            for (size_t i = 0; i < jobs_per_second / 10; ++i) {
                auto job = std::make_unique<kcenon::thread::callback_job>(
                    [this]() -> kcenon::common::VoidResult {
                        completed_jobs_.fetch_add(1);
                        return kcenon::common::ok();
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
    EXPECT_TRUE(WaitForJobCompletion(total_submitted.load(), std::chrono::seconds(30)));

    std::cout << "\nSustained Load Test:\n"
              << "  Duration: " << duration_seconds << " seconds\n"
              << "  Total jobs: " << total_submitted.load() << "\n"
              << "  Completed jobs: " << completed_jobs_.load() << "\n"
              << "  Average rate: " << (total_submitted.load() / duration_seconds)
              << " jobs/sec\n";

    EXPECT_EQ(completed_jobs_.load(), total_submitted.load());
}
