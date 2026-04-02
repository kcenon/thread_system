// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/**
 * @file thread_logger_sdof_test.cpp
 * @brief Tests for Static Destruction Order Fiasco (SDOF) prevention
 *
 * Tests cover:
 * - Early atexit handler registration
 * - Shutdown flag state before and during static destruction
 * - Thread pool destructor behavior during shutdown
 *
 * Related Issues:
 * - GitHub issue #297: Improve atexit handler registration timing for SDOF prevention
 *
 * @see thread_logger_init.cpp for the early registration implementation
 */

#include "gtest/gtest.h"

#include <kcenon/thread/core/thread_logger.h>
#include <kcenon/thread/core/thread_pool.h>

#include <atomic>
#include <thread>

using namespace kcenon::thread;

class ThreadLoggerSdofTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		// Ensure logger instance is created before each test
		// This also triggers any atexit registration that happens in instance()
		auto& logger = thread_logger::instance();
		(void)logger;
	}
};

/**
 * @test Verify shutdown flag is initially false during normal operation
 */
TEST_F(ThreadLoggerSdofTest, ShutdownFlagInitiallyFalse)
{
	// During normal operation (not in atexit handler), is_shutting_down should be false
	EXPECT_FALSE(thread_logger::is_shutting_down());
}

/**
 * @test Verify prepare_shutdown sets the shutdown flag
 */
TEST_F(ThreadLoggerSdofTest, PrepareShutdownSetsFlag)
{
	// Note: This test temporarily sets the shutdown flag
	// We need to be careful not to affect other tests

	// Get initial state
	bool initial_state = thread_logger::is_shutting_down();

	// Call prepare_shutdown
	thread_logger::prepare_shutdown();

	// Verify flag is now true
	EXPECT_TRUE(thread_logger::is_shutting_down());

	// Note: We cannot reset the flag as it's designed to be one-way
	// This is intentional - once shutdown starts, it should stay in shutdown mode
	// Subsequent tests should still pass because:
	// 1. The thread_pool destructor checks is_shutting_down() and uses stop_unsafe()
	// 2. Logging operations early-return when shutting down

	// Restore for other tests by noting this is expected behavior
	if (!initial_state)
	{
		// Log that we've triggered shutdown mode for this test
		// This is expected and other tests should handle it gracefully
	}
}

/**
 * @test Verify prepare_shutdown is idempotent
 */
TEST_F(ThreadLoggerSdofTest, PrepareShutdownIdempotent)
{
	// Call prepare_shutdown multiple times
	thread_logger::prepare_shutdown();
	EXPECT_TRUE(thread_logger::is_shutting_down());

	thread_logger::prepare_shutdown();
	EXPECT_TRUE(thread_logger::is_shutting_down());

	thread_logger::prepare_shutdown();
	EXPECT_TRUE(thread_logger::is_shutting_down());

	// No crash or undefined behavior should occur
}

/**
 * @test Verify thread_logger instance survives after prepare_shutdown
 *
 * The logger uses intentional leak pattern, so it should remain
 * accessible even after prepare_shutdown is called.
 */
TEST_F(ThreadLoggerSdofTest, LoggerSurvivesAfterPrepareShutdown)
{
	thread_logger::prepare_shutdown();

	// Logger instance should still be accessible
	auto& logger = thread_logger::instance();

	// Operations should be no-ops but not crash
	logger.log(log_level::info, "test", "message");

	// Set operations should still work (though logging won't output)
	logger.set_enabled(true);
	logger.set_level(log_level::debug);
}

/**
 * @test Verify thread_pool can be created and destroyed during shutdown
 *
 * During static destruction (is_shutting_down() == true),
 * thread_pool destructor should use stop_unsafe() without logging.
 */
TEST_F(ThreadLoggerSdofTest, ThreadPoolDestructorDuringShutdown)
{
	thread_logger::prepare_shutdown();
	ASSERT_TRUE(thread_logger::is_shutting_down());

	// Create and destroy a thread pool
	// This simulates what happens during static destruction
	{
		auto pool = std::make_shared<thread_pool>("test_pool");

		auto worker = std::make_unique<thread_worker>();
		auto result = pool->enqueue(std::move(worker));
		// Enqueue may or may not succeed during shutdown, that's ok

		// Pool destruction should not crash even during shutdown
	}

	// If we reach here without crash, the test passes
	SUCCEED();
}

/**
 * @test Verify logging is suppressed during shutdown
 */
TEST_F(ThreadLoggerSdofTest, LoggingSuppressedDuringShutdown)
{
	auto& logger = thread_logger::instance();

	// Enable logging
	logger.set_enabled(true);
	logger.set_level(log_level::trace);

	// Trigger shutdown
	thread_logger::prepare_shutdown();

	// These should be no-ops and not crash
	logger.log(log_level::trace, "thread", "message1");
	logger.log(log_level::debug, "thread", "message2");
	logger.log(log_level::info, "thread", "message3");
	logger.log(log_level::warning, "thread", "message4");
	logger.log(log_level::error, "thread", "message5");
	logger.log(log_level::critical, "thread", "message6");

	// No crash means success
	SUCCEED();
}

/**
 * @test Verify thread safety of shutdown flag
 */
TEST_F(ThreadLoggerSdofTest, ShutdownFlagThreadSafety)
{
	std::atomic<int> true_count{0};
	std::atomic<int> false_count{0};
	std::atomic<bool> start{false};

	// Launch multiple threads that check the shutdown flag
	std::vector<std::thread> threads;
	for (int i = 0; i < 10; ++i)
	{
		threads.emplace_back([&]()
		{
			// Wait for start signal
			while (!start.load())
			{
				std::this_thread::yield();
			}

			// Check flag multiple times
			for (int j = 0; j < 1000; ++j)
			{
				if (thread_logger::is_shutting_down())
				{
					true_count.fetch_add(1);
				}
				else
				{
					false_count.fetch_add(1);
				}
			}
		});
	}

	// Start all threads
	start.store(true);

	// Trigger shutdown from main thread while others are checking
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	thread_logger::prepare_shutdown();

	// Wait for all threads
	for (auto& t : threads)
	{
		t.join();
	}

	// After prepare_shutdown, all subsequent checks should return true
	// We should have seen some true values after the flag was set
	EXPECT_GT(true_count.load(), 0);

	// No crash or data race detected
	SUCCEED();
}
