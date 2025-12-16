/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.
*****************************************************************************/

/**
 * @file job_cancellation_example.cpp
 * @brief Demonstrates the job cancellation system in thread_system.
 *
 * This example showcases:
 * 1. Jobs that cooperatively check for cancellation
 * 2. Worker-level cancellation when stop() is called
 * 3. Pool-level hierarchical cancellation
 * 4. Different cancellation scenarios (immediate vs. graceful)
 *
 * Compilation:
 * g++ -std=c++20 -I../include job_cancellation_example.cpp -L../build/lib -lThreadSystem -o cancellation_demo
 *
 * Usage:
 * ./cancellation_demo
 */

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/job.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/utils/formatter.h>

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

using namespace kcenon::thread;
using utility_module::formatter;

/**
 * @class cancellable_long_job
 * @brief A job that performs a long-running task with periodic cancellation checks.
 *
 * This demonstrates the recommended pattern for implementing cancellable jobs:
 * - Check cancellation token periodically during execution
 * - Return operation_canceled error when cancelled
 * - Perform cleanup before returning
 */
class cancellable_long_job : public job
{
public:
	cancellable_long_job(const std::string& name, int iterations = 100)
		: job(name), iterations_(iterations)
	{
	}

	kcenon::common::VoidResult do_work() override
	{
		std::cout << formatter::format("[{}] Starting job with {} iterations\n",
		                               get_name(), iterations_);

		for (int i = 0; i < iterations_; ++i)
		{
			// ✅ BEST PRACTICE: Check cancellation periodically
			if (cancellation_token_.is_cancelled())
			{
				std::cout << formatter::format("[{}] Job cancelled at iteration {}/{}\n",
				                               get_name(), i, iterations_);

				return kcenon::common::error_info{
				    error_code::operation_canceled,
				    formatter::format("Cancelled at iteration {}", i),
				    "job_cancellation_example"};
			}

			// Simulate work (100ms per iteration)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			// Log progress every 10 iterations
			if (i % 10 == 0)
			{
				std::cout << formatter::format("[{}] Progress: {}/{}\n",
				                               get_name(), i, iterations_);
			}
		}

		std::cout << formatter::format("[{}] Job completed successfully\n", get_name());
		return kcenon::common::ok();
	}

private:
	int iterations_;
};

/**
 * @class non_cancellable_job
 * @brief A job that DOES NOT check for cancellation (anti-pattern).
 *
 * This demonstrates what happens when a job doesn't cooperate with cancellation:
 * - The job will run to completion even after stop() is called
 * - Worker thread will block on join() until job finishes
 * - This defeats the purpose of graceful shutdown
 *
 * ⚠️ NOT RECOMMENDED - shown for educational purposes only
 */
class non_cancellable_job : public job
{
public:
	non_cancellable_job(const std::string& name, int iterations = 50)
		: job(name), iterations_(iterations)
	{
	}

	kcenon::common::VoidResult do_work() override
	{
		std::cout << formatter::format("[{}] Starting non-cancellable job\n", get_name());

		for (int i = 0; i < iterations_; ++i)
		{
			// ❌ BAD PRACTICE: Never checking cancellation
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			if (i % 10 == 0)
			{
				std::cout << formatter::format("[{}] Progress: {}/{} (ignoring cancellation)\n",
				                               get_name(), i, iterations_);
			}
		}

		std::cout << formatter::format("[{}] Job completed (never checked cancellation)\n",
		                               get_name());
		return kcenon::common::ok();
	}

private:
	int iterations_;
};

/**
 * @brief Demonstrates basic job cancellation via worker stop.
 *
 * Shows how stopping a worker cancels the currently running job.
 */
void demo_basic_cancellation()
{
	std::cout << "\n========================================\n";
	std::cout << "Demo 1: Basic Job Cancellation\n";
	std::cout << "========================================\n\n";

	// Create pool with single worker
	auto pool = std::make_shared<thread_pool>("cancellation_demo_pool");

	auto worker = std::make_unique<thread_worker>();
	pool->enqueue(std::move(worker));
	pool->start();

	// Submit a long-running cancellable job (10 seconds total)
	auto long_job = std::make_unique<cancellable_long_job>("long_task", 100);
	pool->enqueue(std::move(long_job));

	// Let job run for 2 seconds
	std::cout << "Letting job run for 2 seconds...\n";
	std::this_thread::sleep_for(std::chrono::seconds(2));

	// Stop the pool (triggers cancellation)
	std::cout << "\n>>> Calling pool->stop() <<<\n\n";
	auto stop_start = std::chrono::steady_clock::now();
	pool->stop();
	auto stop_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - stop_start);

	std::cout << formatter::format("\nPool stopped in {}ms (job cooperated with cancellation)\n",
	                               stop_duration.count());
}

/**
 * @brief Demonstrates what happens with non-cooperating jobs.
 *
 * Shows the limitation of cooperative cancellation - jobs must check the token.
 */
void demo_non_cooperative_job()
{
	std::cout << "\n========================================\n";
	std::cout << "Demo 2: Non-Cooperative Job (Anti-Pattern)\n";
	std::cout << "========================================\n\n";

	auto pool = std::make_shared<thread_pool>("non_coop_pool");

	auto worker = std::make_unique<thread_worker>();
	pool->enqueue(std::move(worker));
	pool->start();

	// Submit a job that doesn't check cancellation (5 seconds total)
	auto stubborn_job = std::make_unique<non_cancellable_job>("stubborn_task", 50);
	pool->enqueue(std::move(stubborn_job));

	// Let job run for 1 second
	std::cout << "Letting job run for 1 second...\n";
	std::this_thread::sleep_for(std::chrono::seconds(1));

	// Stop the pool
	std::cout << "\n>>> Calling pool->stop() <<<\n";
	std::cout << "⚠️  Job is NOT checking cancellation token!\n";
	std::cout << "Worker must wait for job to complete...\n\n";

	auto stop_start = std::chrono::steady_clock::now();
	pool->stop();
	auto stop_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - stop_start);

	std::cout << formatter::format("\nPool stopped in {}ms (job did NOT cooperate)\n",
	                               stop_duration.count());
	std::cout << "Notice how much longer this took!\n";
}

/**
 * @brief Demonstrates pool-level cancellation with multiple workers.
 *
 * Shows hierarchical cancellation propagating to all workers.
 */
void demo_pool_level_cancellation()
{
	std::cout << "\n========================================\n";
	std::cout << "Demo 3: Pool-Level Multi-Worker Cancellation\n";
	std::cout << "========================================\n\n";

	auto pool = std::make_shared<thread_pool>("multi_worker_pool");

	// Add 3 workers
	for (int i = 0; i < 3; ++i)
	{
		auto worker = std::make_unique<thread_worker>();
		pool->enqueue(std::move(worker));
	}
	pool->start();

	// Submit multiple jobs to different workers
	for (int i = 0; i < 3; ++i)
	{
		auto job = std::make_unique<cancellable_long_job>(
			formatter::format("worker_{}_task", i), 100);
		pool->enqueue(std::move(job));
	}

	// Let jobs run for 2 seconds
	std::cout << "All workers running jobs...\n";
	std::this_thread::sleep_for(std::chrono::seconds(2));

	// Stop the entire pool
	std::cout << "\n>>> Calling pool->stop() - cancelling ALL workers <<<\n\n";
	auto stop_start = std::chrono::steady_clock::now();
	pool->stop();
	auto stop_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - stop_start);

	std::cout << formatter::format("\nAll workers stopped in {}ms\n", stop_duration.count());
	std::cout << "All jobs received cancellation signal simultaneously!\n";
}

/**
 * @brief Demonstrates immediate vs. graceful shutdown.
 */
void demo_immediate_vs_graceful()
{
	std::cout << "\n========================================\n";
	std::cout << "Demo 4: Immediate vs. Graceful Shutdown\n";
	std::cout << "========================================\n\n";

	// Test 1: Graceful shutdown
	{
		std::cout << "--- Graceful Shutdown (immediately_stop = false) ---\n";
		auto pool = std::make_shared<thread_pool>("graceful_pool");
		auto worker = std::make_unique<thread_worker>();
		pool->enqueue(std::move(worker));
		pool->start();

		// Enqueue multiple jobs
		for (int i = 0; i < 5; ++i)
		{
			auto job = std::make_unique<cancellable_long_job>(
				formatter::format("graceful_job_{}", i), 20);
			pool->enqueue(std::move(job));
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		std::cout << "Stopping gracefully (pending jobs remain in queue)...\n";
		pool->stop(false);  // Graceful shutdown
		std::cout << "Done\n\n";
	}

	// Test 2: Immediate shutdown
	{
		std::cout << "--- Immediate Shutdown (immediately_stop = true) ---\n";
		auto pool = std::make_shared<thread_pool>("immediate_pool");
		auto worker = std::make_unique<thread_worker>();
		pool->enqueue(std::move(worker));
		pool->start();

		// Enqueue multiple jobs
		for (int i = 0; i < 5; ++i)
		{
			auto job = std::make_unique<cancellable_long_job>(
				formatter::format("immediate_job_{}", i), 20);
			pool->enqueue(std::move(job));
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		std::cout << "Stopping immediately (clearing pending jobs)...\n";
		pool->stop(true);  // Immediate shutdown
		std::cout << "Done (pending jobs were cleared)\n\n";
	}
}

int main()
{
	std::cout << "╔═══════════════════════════════════════════════════════╗\n";
	std::cout << "║   Thread System - Job Cancellation System Demo       ║\n";
	std::cout << "╚═══════════════════════════════════════════════════════╝\n";

	try
	{
		demo_basic_cancellation();
		std::this_thread::sleep_for(std::chrono::seconds(1));

		demo_non_cooperative_job();
		std::this_thread::sleep_for(std::chrono::seconds(1));

		demo_pool_level_cancellation();
		std::this_thread::sleep_for(std::chrono::seconds(1));

		demo_immediate_vs_graceful();

		std::cout << "\n========================================\n";
		std::cout << "All demonstrations completed!\n";
		std::cout << "========================================\n\n";

		std::cout << "Key Takeaways:\n";
		std::cout << "1. ✅ Jobs MUST check cancellation_token periodically\n";
		std::cout << "2. ✅ Worker stop() propagates cancellation to running job\n";
		std::cout << "3. ✅ Pool stop() cancels all workers simultaneously\n";
		std::cout << "4. ⚠️  Non-cooperative jobs delay shutdown\n";
		std::cout << "5. ✅ Immediate stop clears pending jobs from queue\n\n";
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
