// BSD 3-Clause License
// Copyright (c) 2026, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file enhanced_metrics_test.cpp
 * @brief Unit tests for EnhancedThreadPoolMetrics.
 *
 * Exercises recording paths (submission, enqueue, execution, wait time,
 * queue depth, worker state), snapshot consistency, per-worker metrics,
 * reset/update_worker_count, and the JSON/Prometheus export helpers.
 */

#include <gtest/gtest.h>

#include <kcenon/thread/metrics/enhanced_metrics.h>

#include <chrono>
#include <string>
#include <vector>

using kcenon::thread::metrics::EnhancedThreadPoolMetrics;
using kcenon::thread::metrics::EnhancedSnapshot;
using kcenon::thread::metrics::WorkerMetrics;
using namespace std::chrono_literals;

class EnhancedThreadPoolMetricsTest : public ::testing::Test {
protected:
    EnhancedThreadPoolMetrics metrics_{2};
};

// -----------------------------------------------------------------------------
// Construction
// -----------------------------------------------------------------------------

TEST_F(EnhancedThreadPoolMetricsTest, FreshInstanceIsEmpty) {
    const auto snap = metrics_.snapshot();
    EXPECT_EQ(snap.tasks_submitted, 0u);
    EXPECT_EQ(snap.tasks_executed, 0u);
    EXPECT_EQ(snap.tasks_failed, 0u);
    EXPECT_EQ(snap.current_queue_depth, 0u);
    EXPECT_EQ(snap.peak_queue_depth, 0u);
    EXPECT_EQ(snap.active_workers, 0u);
    EXPECT_TRUE(metrics_.enqueue_latency().empty());
    EXPECT_TRUE(metrics_.execution_latency().empty());
    EXPECT_TRUE(metrics_.wait_time().empty());
}

TEST_F(EnhancedThreadPoolMetricsTest, DefaultConstructedNoWorkers) {
    EnhancedThreadPoolMetrics m;
    const auto snap = m.snapshot();
    EXPECT_EQ(snap.active_workers, 0u);
}

// -----------------------------------------------------------------------------
// Recording paths
// -----------------------------------------------------------------------------

TEST_F(EnhancedThreadPoolMetricsTest, RecordSubmissionIncrementsCounter) {
    metrics_.record_submission();
    metrics_.record_submission();
    EXPECT_EQ(metrics_.tasks_submitted(), 2u);
}

TEST_F(EnhancedThreadPoolMetricsTest, RecordEnqueueAddsHistogramSample) {
    metrics_.record_enqueue(1000ns);
    metrics_.record_enqueue(2000ns);

    const auto& hist = metrics_.enqueue_latency();
    EXPECT_FALSE(hist.empty());
    EXPECT_GE(hist.count(), 2u);
}

TEST_F(EnhancedThreadPoolMetricsTest, RecordExecutionSuccessUpdatesBoth) {
    metrics_.record_execution(5000ns, true);
    EXPECT_EQ(metrics_.tasks_executed(), 1u);
    EXPECT_EQ(metrics_.tasks_failed(), 0u);
    EXPECT_FALSE(metrics_.execution_latency().empty());
}

TEST_F(EnhancedThreadPoolMetricsTest, RecordExecutionFailureIncrementsFailed) {
    metrics_.record_execution(1000ns, false);
    EXPECT_EQ(metrics_.tasks_failed(), 1u);
}

TEST_F(EnhancedThreadPoolMetricsTest, RecordWaitTimePopulatesHistogram) {
    metrics_.record_wait_time(500ns);
    metrics_.record_wait_time(1000ns);
    EXPECT_GE(metrics_.wait_time().count(), 2u);
}

TEST_F(EnhancedThreadPoolMetricsTest, RecordQueueDepthTracksCurrentAndPeak) {
    metrics_.record_queue_depth(5);
    metrics_.record_queue_depth(10);
    metrics_.record_queue_depth(3);

    const auto snap = metrics_.snapshot();
    EXPECT_EQ(snap.current_queue_depth, 3u);
    EXPECT_EQ(snap.peak_queue_depth, 10u);
    EXPECT_GT(snap.avg_queue_depth, 0.0);
}

TEST_F(EnhancedThreadPoolMetricsTest, SetActiveWorkers) {
    metrics_.set_active_workers(3);
    EXPECT_EQ(metrics_.snapshot().active_workers, 3u);
}

// -----------------------------------------------------------------------------
// Worker state
// -----------------------------------------------------------------------------

TEST_F(EnhancedThreadPoolMetricsTest, RecordWorkerStateBusyAccumulatesBusyTime) {
    metrics_.record_worker_state(0, true, 1000);
    metrics_.record_worker_state(0, true, 2000);
    const auto workers = metrics_.worker_metrics();
    ASSERT_FALSE(workers.empty());
    EXPECT_GT(workers[0].busy_time_ns, 0u);
}

TEST_F(EnhancedThreadPoolMetricsTest, RecordWorkerStateIdleAccumulatesIdleTime) {
    metrics_.record_worker_state(1, false, 500);
    metrics_.record_worker_state(1, false, 1500);
    const auto workers = metrics_.worker_metrics();
    ASSERT_GE(workers.size(), 2u);
    EXPECT_GT(workers[1].idle_time_ns, 0u);
}

TEST_F(EnhancedThreadPoolMetricsTest, RecordWorkerStateOutOfRangeIsIgnored) {
    // Worker index beyond the registered count should not crash.
    metrics_.record_worker_state(99, true, 100);
    SUCCEED();
}

// -----------------------------------------------------------------------------
// Management: reset / update_worker_count
// -----------------------------------------------------------------------------

TEST_F(EnhancedThreadPoolMetricsTest, ResetClearsAllState) {
    metrics_.record_submission();
    metrics_.record_execution(1000ns, true);
    metrics_.record_enqueue(100ns);
    metrics_.record_wait_time(200ns);
    metrics_.record_queue_depth(7);
    metrics_.set_active_workers(4);

    metrics_.reset();

    const auto snap = metrics_.snapshot();
    EXPECT_EQ(snap.tasks_submitted, 0u);
    EXPECT_EQ(snap.tasks_executed, 0u);
    EXPECT_EQ(snap.tasks_failed, 0u);
    EXPECT_EQ(snap.current_queue_depth, 0u);
    EXPECT_EQ(snap.peak_queue_depth, 0u);
    EXPECT_TRUE(metrics_.enqueue_latency().empty());
    EXPECT_TRUE(metrics_.execution_latency().empty());
    EXPECT_TRUE(metrics_.wait_time().empty());
}

TEST_F(EnhancedThreadPoolMetricsTest, UpdateWorkerCountGrowsPerWorkerSlots) {
    metrics_.update_worker_count(5);
    metrics_.record_worker_state(4, true, 100);
    const auto workers = metrics_.worker_metrics();
    EXPECT_GE(workers.size(), 5u);
}

// -----------------------------------------------------------------------------
// Snapshot content
// -----------------------------------------------------------------------------

TEST_F(EnhancedThreadPoolMetricsTest, SnapshotIncludesRecordedLatencies) {
    for (int i = 1; i <= 50; ++i) {
        metrics_.record_enqueue(std::chrono::nanoseconds{i * 100});
        metrics_.record_execution(std::chrono::nanoseconds{i * 1000}, true);
        metrics_.record_wait_time(std::chrono::nanoseconds{i * 10});
    }

    const auto snap = metrics_.snapshot();
    EXPECT_GT(snap.enqueue_latency_p50_us, 0.0);
    EXPECT_GT(snap.execution_latency_p50_us, 0.0);
    EXPECT_GT(snap.wait_time_p50_us, 0.0);
    EXPECT_LE(snap.enqueue_latency_p50_us, snap.enqueue_latency_p99_us);
    EXPECT_LE(snap.execution_latency_p50_us, snap.execution_latency_p99_us);
    EXPECT_LE(snap.wait_time_p50_us, snap.wait_time_p99_us);
}

TEST_F(EnhancedThreadPoolMetricsTest, ThroughputCountersAccessible) {
    metrics_.record_execution(1000ns, true);
    metrics_.record_execution(1000ns, true);
    const auto& one_sec = metrics_.throughput_1s();
    const auto& one_min = metrics_.throughput_1m();
    EXPECT_GE(one_sec.all_time_total(), 2u);
    EXPECT_GE(one_min.all_time_total(), 2u);
}

// -----------------------------------------------------------------------------
// Export
// -----------------------------------------------------------------------------

TEST_F(EnhancedThreadPoolMetricsTest, ToJsonIsNotEmpty) {
    metrics_.record_submission();
    const auto json = metrics_.to_json();
    EXPECT_FALSE(json.empty());
}

TEST_F(EnhancedThreadPoolMetricsTest, ToPrometheusIsNotEmpty) {
    metrics_.record_submission();
    const auto prom = metrics_.to_prometheus();
    EXPECT_FALSE(prom.empty());
    EXPECT_NE(prom.find("thread_pool"), std::string::npos);
}

TEST_F(EnhancedThreadPoolMetricsTest, ToPrometheusWithCustomPrefix) {
    const auto prom = metrics_.to_prometheus("custom_prefix");
    EXPECT_FALSE(prom.empty());
    EXPECT_NE(prom.find("custom_prefix"), std::string::npos);
}
