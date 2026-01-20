/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, kcenon
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

#include <kcenon/thread/metrics/metrics_service.h>

#include <chrono>
#include <thread>

using namespace kcenon::thread::metrics;

class MetricsServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        service_ = std::make_shared<metrics_service>();
    }

    std::shared_ptr<metrics_service> service_;
};

TEST_F(MetricsServiceTest, ConstructorInitializesBasicMetrics) {
    auto basic = service_->get_basic_metrics();
    ASSERT_NE(basic, nullptr);
    EXPECT_EQ(basic->tasks_submitted(), 0);
    EXPECT_EQ(basic->tasks_executed(), 0);
}

TEST_F(MetricsServiceTest, RecordSubmission) {
    service_->record_submission();
    EXPECT_EQ(service_->basic_metrics().tasks_submitted(), 1);

    service_->record_submission(5);
    EXPECT_EQ(service_->basic_metrics().tasks_submitted(), 6);
}

TEST_F(MetricsServiceTest, RecordEnqueue) {
    service_->record_enqueue();
    EXPECT_EQ(service_->basic_metrics().tasks_enqueued(), 1);

    service_->record_enqueue(3);
    EXPECT_EQ(service_->basic_metrics().tasks_enqueued(), 4);
}

TEST_F(MetricsServiceTest, RecordEnqueueWithLatency) {
    auto latency = std::chrono::nanoseconds{1000};
    service_->record_enqueue_with_latency(latency);
    EXPECT_EQ(service_->basic_metrics().tasks_enqueued(), 1);

    service_->record_enqueue_with_latency(latency, 3);
    EXPECT_EQ(service_->basic_metrics().tasks_enqueued(), 4);
}

TEST_F(MetricsServiceTest, RecordExecutionSuccess) {
    service_->record_execution(1000, true);
    EXPECT_EQ(service_->basic_metrics().tasks_executed(), 1);
    EXPECT_EQ(service_->basic_metrics().tasks_failed(), 0);
    EXPECT_EQ(service_->basic_metrics().total_busy_time_ns(), 1000);
}

TEST_F(MetricsServiceTest, RecordExecutionFailure) {
    service_->record_execution(2000, false);
    EXPECT_EQ(service_->basic_metrics().tasks_executed(), 0);
    EXPECT_EQ(service_->basic_metrics().tasks_failed(), 1);
    EXPECT_EQ(service_->basic_metrics().total_busy_time_ns(), 2000);
}

TEST_F(MetricsServiceTest, RecordIdleTime) {
    service_->record_idle_time(5000);
    EXPECT_EQ(service_->basic_metrics().total_idle_time_ns(), 5000);
}

TEST_F(MetricsServiceTest, EnhancedMetricsDisabledByDefault) {
    EXPECT_FALSE(service_->is_enhanced_metrics_enabled());
}

TEST_F(MetricsServiceTest, EnableEnhancedMetrics) {
    service_->set_enhanced_metrics_enabled(true, 4);
    EXPECT_TRUE(service_->is_enhanced_metrics_enabled());

    // Should not throw
    const auto& enhanced = service_->enhanced_metrics();
    EXPECT_NO_THROW(enhanced.snapshot());
}

TEST_F(MetricsServiceTest, EnhancedMetricsThrowsWhenDisabled) {
    EXPECT_THROW(service_->enhanced_metrics(), std::runtime_error);
}

TEST_F(MetricsServiceTest, EnhancedSnapshotEmptyWhenDisabled) {
    auto snapshot = service_->enhanced_snapshot();
    EXPECT_EQ(snapshot.tasks_submitted, 0);
    EXPECT_EQ(snapshot.tasks_executed, 0);
}

TEST_F(MetricsServiceTest, EnhancedMetricsRecording) {
    service_->set_enhanced_metrics_enabled(true, 2);

    service_->record_submission();
    auto latency = std::chrono::nanoseconds{500};
    service_->record_enqueue_with_latency(latency);
    service_->record_execution_with_wait_time(
        std::chrono::nanoseconds{1000},
        std::chrono::nanoseconds{200},
        true);

    auto snapshot = service_->enhanced_snapshot();
    EXPECT_EQ(snapshot.tasks_submitted, 1);
    EXPECT_EQ(snapshot.tasks_executed, 1);
}

TEST_F(MetricsServiceTest, RecordQueueDepth) {
    service_->set_enhanced_metrics_enabled(true, 4);
    service_->record_queue_depth(10);

    auto snapshot = service_->enhanced_snapshot();
    EXPECT_EQ(snapshot.current_queue_depth, 10);
}

TEST_F(MetricsServiceTest, RecordWorkerState) {
    service_->set_enhanced_metrics_enabled(true, 2);
    service_->record_worker_state(0, true, 1000);

    // Worker state recording should not throw
    EXPECT_NO_THROW(service_->enhanced_snapshot());
}

TEST_F(MetricsServiceTest, UpdateWorkerCount) {
    service_->set_enhanced_metrics_enabled(true, 2);
    service_->update_worker_count(4);

    // Should not throw after update
    EXPECT_NO_THROW(service_->enhanced_snapshot());
}

TEST_F(MetricsServiceTest, SetActiveWorkers) {
    service_->set_enhanced_metrics_enabled(true, 4);
    service_->set_active_workers(3);

    auto snapshot = service_->enhanced_snapshot();
    EXPECT_EQ(snapshot.active_workers, 3);
}

TEST_F(MetricsServiceTest, Reset) {
    service_->record_submission(5);
    service_->record_enqueue(3);
    service_->record_execution(1000, true);

    EXPECT_EQ(service_->basic_metrics().tasks_submitted(), 5);

    service_->reset();

    EXPECT_EQ(service_->basic_metrics().tasks_submitted(), 0);
    EXPECT_EQ(service_->basic_metrics().tasks_enqueued(), 0);
    EXPECT_EQ(service_->basic_metrics().tasks_executed(), 0);
}

TEST_F(MetricsServiceTest, ResetWithEnhancedMetrics) {
    service_->set_enhanced_metrics_enabled(true, 2);
    service_->record_submission(5);

    service_->reset();

    auto snapshot = service_->enhanced_snapshot();
    EXPECT_EQ(snapshot.tasks_submitted, 0);
}

TEST_F(MetricsServiceTest, GetBasicMetricsSharedPtr) {
    auto metrics1 = service_->get_basic_metrics();
    auto metrics2 = service_->get_basic_metrics();

    EXPECT_EQ(metrics1.get(), metrics2.get());
}

TEST_F(MetricsServiceTest, ThreadSafetyStressTest) {
    constexpr int kNumThreads = 4;
    constexpr int kOpsPerThread = 1000;

    std::vector<std::thread> threads;

    service_->set_enhanced_metrics_enabled(true, kNumThreads);

    for (int i = 0; i < kNumThreads; ++i) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < kOpsPerThread; ++j) {
                service_->record_submission();
                service_->record_enqueue();
                service_->record_execution(100, j % 2 == 0);
                service_->record_queue_depth(j);
                if (j % 100 == 0) {
                    service_->enhanced_snapshot();
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(service_->basic_metrics().tasks_submitted(),
              static_cast<std::uint64_t>(kNumThreads * kOpsPerThread));
    EXPECT_EQ(service_->basic_metrics().tasks_enqueued(),
              static_cast<std::uint64_t>(kNumThreads * kOpsPerThread));
}
