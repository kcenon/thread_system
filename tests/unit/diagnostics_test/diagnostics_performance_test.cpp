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

#include "gtest/gtest.h"

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/diagnostics.h>

#include <chrono>
#include <thread>

using namespace kcenon::thread;
using namespace kcenon::thread::diagnostics;

// ============================================================================
// Performance tests for Diagnostics API
// Target: < 1μs overhead per operation when enabled
// ============================================================================

class DiagnosticsPerformanceTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		pool_ = std::make_shared<thread_pool>("PerfTestPool");

		for (int i = 0; i < 4; ++i)
		{
			auto result = pool_->enqueue(std::make_unique<thread_worker>());
			ASSERT_FALSE(result.is_err());
		}
		auto start_result = pool_->start();
		ASSERT_FALSE(start_result.is_err());

		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	void TearDown() override
	{
		if (pool_)
		{
			pool_->stop(true);
		}
	}

	std::shared_ptr<thread_pool> pool_;
};

// ============================================================================
// Measurement Helper
// ============================================================================

template<typename Func>
double measure_operation_ns(Func&& func, int iterations = 1000)
{
	auto start = std::chrono::steady_clock::now();

	for (int i = 0; i < iterations; ++i)
	{
		func();
	}

	auto end = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

	return static_cast<double>(duration.count()) / static_cast<double>(iterations);
}

// ============================================================================
// Thread Dump Performance Tests
// ============================================================================

TEST_F(DiagnosticsPerformanceTest, ThreadDumpOverhead)
{
	constexpr int iterations = 100;

	double avg_ns = measure_operation_ns(
		[this]()
		{
			auto threads = pool_->diagnostics().dump_thread_states();
			(void)threads;
		},
		iterations);

	// Thread dump should complete in reasonable time (< 1ms per call)
	// This is O(n) where n is worker count
	EXPECT_LT(avg_ns, 1'000'000.0);  // < 1ms

	// Log for informational purposes
	std::cout << "Thread dump avg time: " << avg_ns / 1000.0 << " μs" << std::endl;
}

TEST_F(DiagnosticsPerformanceTest, FormatThreadDumpOverhead)
{
	constexpr int iterations = 100;

	double avg_ns = measure_operation_ns(
		[this]()
		{
			auto dump = pool_->diagnostics().format_thread_dump();
			(void)dump;
		},
		iterations);

	// Format thread dump includes string formatting, allow more time
	EXPECT_LT(avg_ns, 2'000'000.0);  // < 2ms

	std::cout << "Format thread dump avg time: " << avg_ns / 1000.0 << " μs" << std::endl;
}

// ============================================================================
// Health Check Performance Tests
// ============================================================================

TEST_F(DiagnosticsPerformanceTest, HealthCheckOverhead)
{
	constexpr int iterations = 100;

	double avg_ns = measure_operation_ns(
		[this]()
		{
			auto health = pool_->diagnostics().health_check();
			(void)health;
		},
		iterations);

	// Health check should be fast (< 100μs)
	EXPECT_LT(avg_ns, 100'000.0);

	std::cout << "Health check avg time: " << avg_ns / 1000.0 << " μs" << std::endl;
}

TEST_F(DiagnosticsPerformanceTest, IsHealthyOverhead)
{
	constexpr int iterations = 1000;

	double avg_ns = measure_operation_ns(
		[this]()
		{
			bool healthy = pool_->diagnostics().is_healthy();
			(void)healthy;
		},
		iterations);

	// Quick health check should be very fast (< 10μs)
	EXPECT_LT(avg_ns, 10'000.0);

	std::cout << "is_healthy avg time: " << avg_ns / 1000.0 << " μs" << std::endl;
}

// ============================================================================
// Bottleneck Detection Performance Tests
// ============================================================================

TEST_F(DiagnosticsPerformanceTest, BottleneckDetectionOverhead)
{
	constexpr int iterations = 100;

	double avg_ns = measure_operation_ns(
		[this]()
		{
			auto report = pool_->diagnostics().detect_bottlenecks();
			(void)report;
		},
		iterations);

	// Bottleneck detection should be reasonably fast (< 500μs)
	EXPECT_LT(avg_ns, 500'000.0);

	std::cout << "Bottleneck detection avg time: " << avg_ns / 1000.0 << " μs" << std::endl;
}

// ============================================================================
// Event Tracing Performance Tests
// ============================================================================

TEST_F(DiagnosticsPerformanceTest, EventRecordingOverheadWhenDisabled)
{
	pool_->diagnostics().enable_tracing(false);
	constexpr int iterations = 10000;

	job_execution_event event;
	event.event_id = 1;
	event.job_id = 100;
	event.type = event_type::completed;

	double avg_ns = measure_operation_ns(
		[this, &event]()
		{
			pool_->diagnostics().record_event(event);
		},
		iterations);

	// When disabled, should be essentially free (< 1μs)
	EXPECT_LT(avg_ns, 1'000.0);

	std::cout << "Event recording (disabled) avg time: " << avg_ns << " ns" << std::endl;
}

TEST_F(DiagnosticsPerformanceTest, EventRecordingOverheadWhenEnabled)
{
	pool_->diagnostics().enable_tracing(true, 1000);
	constexpr int iterations = 1000;

	job_execution_event event;
	event.event_id = 1;
	event.job_id = 100;
	event.type = event_type::completed;

	double avg_ns = measure_operation_ns(
		[this, &event]()
		{
			pool_->diagnostics().record_event(event);
		},
		iterations);

	// When enabled, target < 1μs overhead
	// Allow up to 10μs for realistic conditions
	EXPECT_LT(avg_ns, 10'000.0);

	std::cout << "Event recording (enabled) avg time: " << avg_ns << " ns" << std::endl;
}

TEST_F(DiagnosticsPerformanceTest, GetRecentEventsOverhead)
{
	pool_->diagnostics().enable_tracing(true, 1000);

	// Pre-populate event history
	for (int i = 0; i < 500; ++i)
	{
		job_execution_event event;
		event.event_id = static_cast<uint64_t>(i);
		event.job_id = static_cast<uint64_t>(i * 10);
		pool_->diagnostics().record_event(event);
	}

	constexpr int iterations = 100;

	double avg_ns = measure_operation_ns(
		[this]()
		{
			auto events = pool_->diagnostics().get_recent_events(100);
			(void)events;
		},
		iterations);

	// Getting recent events should be fast (< 100μs)
	EXPECT_LT(avg_ns, 100'000.0);

	std::cout << "Get recent events avg time: " << avg_ns / 1000.0 << " μs" << std::endl;
}

// ============================================================================
// Export Performance Tests
// ============================================================================

TEST_F(DiagnosticsPerformanceTest, ToJsonOverhead)
{
	constexpr int iterations = 100;

	double avg_ns = measure_operation_ns(
		[this]()
		{
			auto json = pool_->diagnostics().to_json();
			(void)json;
		},
		iterations);

	// JSON export should be reasonably fast (< 1ms)
	EXPECT_LT(avg_ns, 1'000'000.0);

	std::cout << "to_json avg time: " << avg_ns / 1000.0 << " μs" << std::endl;
}

TEST_F(DiagnosticsPerformanceTest, ToPrometheusOverhead)
{
	constexpr int iterations = 100;

	double avg_ns = measure_operation_ns(
		[this]()
		{
			auto prom = pool_->diagnostics().to_prometheus();
			(void)prom;
		},
		iterations);

	// Prometheus export should be reasonably fast (< 1ms)
	EXPECT_LT(avg_ns, 1'000'000.0);

	std::cout << "to_prometheus avg time: " << avg_ns / 1000.0 << " μs" << std::endl;
}

// ============================================================================
// Concurrent Access Performance Tests
// ============================================================================

TEST_F(DiagnosticsPerformanceTest, ConcurrentHealthCheckAccess)
{
	constexpr int thread_count = 4;
	constexpr int iterations_per_thread = 100;

	std::vector<std::thread> threads;
	std::atomic<int> completed{0};

	auto start = std::chrono::steady_clock::now();

	for (int t = 0; t < thread_count; ++t)
	{
		threads.emplace_back(
			[this, &completed]()
			{
				for (int i = 0; i < iterations_per_thread; ++i)
				{
					auto health = pool_->diagnostics().health_check();
					(void)health;
				}
				completed++;
			});
	}

	for (auto& t : threads)
	{
		t.join();
	}

	auto end = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

	EXPECT_EQ(completed.load(), thread_count);

	double total_ops = static_cast<double>(thread_count * iterations_per_thread);
	double ops_per_sec = total_ops / (static_cast<double>(duration.count()) / 1000.0);

	std::cout << "Concurrent health check: " << ops_per_sec << " ops/sec" << std::endl;
	std::cout << "Total time: " << duration.count() << " ms" << std::endl;
}

TEST_F(DiagnosticsPerformanceTest, ConcurrentEventRecording)
{
	pool_->diagnostics().enable_tracing(true, 10000);
	constexpr int thread_count = 4;
	constexpr int iterations_per_thread = 1000;

	std::vector<std::thread> threads;
	std::atomic<int> completed{0};

	auto start = std::chrono::steady_clock::now();

	for (int t = 0; t < thread_count; ++t)
	{
		threads.emplace_back(
			[this, t, &completed]()
			{
				for (int i = 0; i < iterations_per_thread; ++i)
				{
					job_execution_event event;
					event.event_id = static_cast<uint64_t>(t * iterations_per_thread + i);
					event.job_id = static_cast<uint64_t>(i);
					event.type = event_type::completed;
					pool_->diagnostics().record_event(event);
				}
				completed++;
			});
	}

	for (auto& t : threads)
	{
		t.join();
	}

	auto end = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

	EXPECT_EQ(completed.load(), thread_count);

	double total_ops = static_cast<double>(thread_count * iterations_per_thread);
	double ops_per_sec = total_ops / (static_cast<double>(duration.count()) / 1000.0);

	std::cout << "Concurrent event recording: " << ops_per_sec << " ops/sec" << std::endl;
	std::cout << "Total time: " << duration.count() << " ms" << std::endl;
}
