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
 * @brief Performance benchmarks for scalability measurement
 *
 * Goal: Measure how performance scales with worker count
 * Expected time: < 3 minutes
 */
class ScalabilityBenchmark : public SystemFixture {
protected:
    size_t ScaleForCI(size_t value) const {
        return std::getenv("CI") ? value / 10 : value;
    }
};

TEST_F(ScalabilityBenchmark, WorkerScalability) {
    const size_t job_count = ScaleForCI(5'000);  // Reduced from 50k
    std::vector<size_t> worker_counts = {1, 2, 4};  // Reduced from {1,2,4,8}

    std::cout << "\nWorker Scalability Benchmark:\n";
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

        EXPECT_TRUE(WaitForJobCompletion(job_count, std::chrono::seconds(60)));

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

TEST_F(ScalabilityBenchmark, ConcurrentProducerScalability) {
    CreateThreadPool(4);

    auto result = pool_->start();
    ASSERT_TRUE(result.is_ok());

    const size_t jobs_per_producer = ScaleForCI(500);  // Reduced
    std::vector<size_t> producer_counts = {1, 2, 4};  // Reduced from {1,2,4,8}

    std::cout << "\nConcurrent Producer Scalability:\n";
    std::cout << "Producers\tTotal Jobs\tDuration\n";

    for (auto num_producers : producer_counts) {
        completed_jobs_.store(0);
        const size_t total_jobs = num_producers * jobs_per_producer;

        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> producers;
        for (size_t t = 0; t < num_producers; ++t) {
            producers.emplace_back([this, jobs_per_producer]() {
                for (size_t i = 0; i < jobs_per_producer; ++i) {
                    auto job = std::make_unique<kcenon::thread::callback_job>(
                        [this]() -> kcenon::common::VoidResult {
                            completed_jobs_.fetch_add(1);
                            return kcenon::common::ok();
                        }
                    );
                    pool_->enqueue(std::move(job));
                }
            });
        }

        for (auto& t : producers) {
            t.join();
        }

        EXPECT_TRUE(WaitForJobCompletion(total_jobs, std::chrono::seconds(30)));

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << num_producers << "\t\t"
                  << total_jobs << "\t\t"
                  << duration.count() << " ms\n";
    }
}
