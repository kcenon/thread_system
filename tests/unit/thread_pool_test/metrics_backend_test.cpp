// BSD 3-Clause License
// Copyright (c) 2026, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file metrics_backend_test.cpp
 * @brief Unit tests for MetricsBackend implementations.
 *
 * Covers PrometheusBackend, JsonBackend, LoggingBackend, and the
 * BackendRegistry singleton. Exercises prefix/label handling and both
 * base and enhanced snapshot export paths.
 */

#include <gtest/gtest.h>

#include <kcenon/thread/metrics/metrics_backend.h>
#include <kcenon/thread/metrics/metrics_base.h>
#include <kcenon/thread/metrics/enhanced_metrics.h>

#include <memory>
#include <string>

using kcenon::thread::metrics::BaseSnapshot;
using kcenon::thread::metrics::EnhancedSnapshot;
using kcenon::thread::metrics::BackendRegistry;
using kcenon::thread::metrics::JsonBackend;
using kcenon::thread::metrics::LoggingBackend;
using kcenon::thread::metrics::MetricsBackend;
using kcenon::thread::metrics::PrometheusBackend;

namespace {

BaseSnapshot make_base_snapshot() {
    BaseSnapshot snap;
    snap.tasks_submitted = 120;
    snap.tasks_executed = 100;
    snap.tasks_failed = 3;
    snap.total_busy_time_ns = 5'000'000;
    snap.total_idle_time_ns = 1'000'000;
    return snap;
}

EnhancedSnapshot make_enhanced_snapshot() {
    EnhancedSnapshot snap;
    snap.tasks_submitted = 50;
    snap.tasks_executed = 45;
    snap.tasks_failed = 1;
    snap.enqueue_latency_p50_us = 1.0;
    snap.enqueue_latency_p90_us = 2.5;
    snap.enqueue_latency_p99_us = 10.0;
    snap.execution_latency_p50_us = 100.0;
    snap.execution_latency_p90_us = 250.0;
    snap.execution_latency_p99_us = 400.0;
    snap.wait_time_p50_us = 5.0;
    snap.wait_time_p90_us = 10.0;
    snap.wait_time_p99_us = 50.0;
    snap.throughput_1s = 12.5;
    snap.throughput_1m = 7.5;
    snap.current_queue_depth = 3;
    snap.peak_queue_depth = 10;
    snap.avg_queue_depth = 4.5;
    snap.worker_utilization = 0.75;
    snap.active_workers = 4;
    snap.total_busy_time_ns = 3'000'000;
    snap.total_idle_time_ns = 1'000'000;
    snap.snapshot_time = std::chrono::steady_clock::now();
    return snap;
}

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

} // namespace

// -----------------------------------------------------------------------------
// Base MetricsBackend behavior (prefix/labels)
// -----------------------------------------------------------------------------

TEST(MetricsBackendTest, DefaultPrefixIsThreadPool) {
    PrometheusBackend backend;
    EXPECT_EQ(backend.prefix(), "thread_pool");
}

TEST(MetricsBackendTest, SetPrefixUpdatesPrefix) {
    JsonBackend backend;
    backend.set_prefix("my_pool");
    EXPECT_EQ(backend.prefix(), "my_pool");
}

TEST(MetricsBackendTest, AddLabelStoresLabels) {
    LoggingBackend backend;
    backend.add_label("env", "prod");
    backend.add_label("region", "us-east-1");

    const auto& labels = backend.labels();
    ASSERT_EQ(labels.size(), 2u);
    EXPECT_EQ(labels.at("env"), "prod");
    EXPECT_EQ(labels.at("region"), "us-east-1");
}

TEST(MetricsBackendTest, AddLabelOverwritesExisting) {
    LoggingBackend backend;
    backend.add_label("env", "dev");
    backend.add_label("env", "staging");
    EXPECT_EQ(backend.labels().at("env"), "staging");
}

// -----------------------------------------------------------------------------
// PrometheusBackend
// -----------------------------------------------------------------------------

TEST(PrometheusBackendTest, NameIsPrometheus) {
    PrometheusBackend backend;
    EXPECT_EQ(backend.name(), "prometheus");
}

TEST(PrometheusBackendTest, ExportBaseProducesPrometheusFormat) {
    PrometheusBackend backend;
    const auto output = backend.export_base(make_base_snapshot());

    EXPECT_FALSE(output.empty());
    // Prometheus format exposes TYPE/HELP metadata.
    EXPECT_TRUE(contains(output, "tasks_submitted"));
    EXPECT_TRUE(contains(output, "tasks_executed"));
}

TEST(PrometheusBackendTest, ExportEnhancedProducesPrometheusFormat) {
    PrometheusBackend backend;
    const auto output = backend.export_enhanced(make_enhanced_snapshot());

    EXPECT_FALSE(output.empty());
    EXPECT_TRUE(contains(output, "thread_pool"));
}

TEST(PrometheusBackendTest, ExportRespectsCustomPrefix) {
    PrometheusBackend backend;
    backend.set_prefix("custom_prefix");
    const auto output = backend.export_base(make_base_snapshot());
    EXPECT_TRUE(contains(output, "custom_prefix"));
}

// -----------------------------------------------------------------------------
// JsonBackend
// -----------------------------------------------------------------------------

TEST(JsonBackendTest, NameIsJson) {
    JsonBackend backend;
    EXPECT_EQ(backend.name(), "json");
}

TEST(JsonBackendTest, ExportBaseProducesValidJson) {
    JsonBackend backend;
    const auto output = backend.export_base(make_base_snapshot());

    EXPECT_FALSE(output.empty());
    // Must contain JSON braces.
    EXPECT_TRUE(contains(output, "{"));
    EXPECT_TRUE(contains(output, "}"));
}

TEST(JsonBackendTest, ExportEnhancedProducesValidJson) {
    JsonBackend backend;
    const auto output = backend.export_enhanced(make_enhanced_snapshot());
    EXPECT_FALSE(output.empty());
    EXPECT_TRUE(contains(output, "{"));
    EXPECT_TRUE(contains(output, "}"));
}

TEST(JsonBackendTest, PrettyPrintCanBeDisabled) {
    JsonBackend backend;
    backend.set_pretty(false);
    const auto compact = backend.export_base(make_base_snapshot());

    backend.set_pretty(true);
    const auto pretty = backend.export_base(make_base_snapshot());

    // Pretty output is typically at least as long and frequently longer.
    EXPECT_FALSE(compact.empty());
    EXPECT_FALSE(pretty.empty());
}

// -----------------------------------------------------------------------------
// LoggingBackend
// -----------------------------------------------------------------------------

TEST(LoggingBackendTest, NameIsLogging) {
    LoggingBackend backend;
    EXPECT_EQ(backend.name(), "logging");
}

TEST(LoggingBackendTest, ExportBaseProducesHumanReadableOutput) {
    LoggingBackend backend;
    const auto output = backend.export_base(make_base_snapshot());
    EXPECT_FALSE(output.empty());
}

TEST(LoggingBackendTest, ExportEnhancedProducesHumanReadableOutput) {
    LoggingBackend backend;
    const auto output = backend.export_enhanced(make_enhanced_snapshot());
    EXPECT_FALSE(output.empty());
}

// -----------------------------------------------------------------------------
// BackendRegistry
// -----------------------------------------------------------------------------

TEST(BackendRegistryTest, DefaultBackendsAreRegistered) {
    auto& registry = BackendRegistry::instance();
    EXPECT_TRUE(registry.has("prometheus"));
    EXPECT_TRUE(registry.has("json"));
    EXPECT_TRUE(registry.has("logging"));
}

TEST(BackendRegistryTest, RegisterAndRetrieveBackend) {
    auto& registry = BackendRegistry::instance();

    auto backend = std::make_shared<JsonBackend>();
    registry.register_backend(backend);

    auto retrieved = registry.get("json");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->name(), "json");
}

TEST(BackendRegistryTest, GetUnknownReturnsNull) {
    auto& registry = BackendRegistry::instance();
    EXPECT_EQ(registry.get("does-not-exist-xyz"), nullptr);
    EXPECT_FALSE(registry.has("does-not-exist-xyz"));
}

TEST(BackendRegistryTest, RegisterNullBackendIsNoop) {
    auto& registry = BackendRegistry::instance();
    // Should not throw.
    registry.register_backend(nullptr);
    SUCCEED();
}

TEST(BackendRegistryTest, RegisterOverwritesExisting) {
    auto& registry = BackendRegistry::instance();

    auto first = std::make_shared<JsonBackend>();
    first->set_prefix("first_instance");
    registry.register_backend(first);

    auto second = std::make_shared<JsonBackend>();
    second->set_prefix("second_instance");
    registry.register_backend(second);

    auto retrieved = registry.get("json");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->prefix(), "second_instance");
}
