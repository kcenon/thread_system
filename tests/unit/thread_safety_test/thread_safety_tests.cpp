/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, 🍀☀🌕🌥 🌊
All rights reserved.
*****************************************************************************/

#include <gtest/gtest.h>
#include "kcenon/thread/core/thread_pool.h"
#include "kcenon/thread/core/job_queue.h"
#include "kcenon/thread/utils/cancellation_token.h"

#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <barrier>
#include <future>

using namespace kcenon::thread;
using namespace std::chrono_literals;

class ThreadSystemThreadSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test 1: High contention job submission
TEST_F(ThreadSystemThreadSafetyTest, HighContentionSubmission) {
    auto pool = thread_pool::create(4);

    const int num_submitters = 20;
    const int jobs_per_submitter = 500;

    std::atomic<int> jobs_executed{0};
    std::atomic<int> submission_errors{0};
    std::vector<std::thread> threads;
    std::barrier sync_point(num_submitters);

    for (int i = 0; i < num_submitters; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            sync_point.arrive_and_wait();

            for (int j = 0; j < jobs_per_submitter; ++j) {
                try {
                    auto result = pool->submit([&jobs_executed]() {
                        ++jobs_executed;
                    });

                    if (!result.is_ok()) {
                        ++submission_errors;
                    }
                } catch (...) {
                    ++submission_errors;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    pool->wait_all();
    std::this_thread::sleep_for(100ms);

    EXPECT_EQ(submission_errors.load(), 0);
    EXPECT_EQ(jobs_executed.load(), num_submitters * jobs_per_submitter);

    pool->shutdown();
}

// Test 2: Concurrent shutdown while submitting jobs
TEST_F(ThreadSystemThreadSafetyTest, ConcurrentShutdown) {
    auto pool = thread_pool::create(8);

    const int num_submitters = 15;
    const int jobs_per_submitter = 300;

    std::atomic<bool> should_stop{false};
    std::atomic<int> jobs_attempted{0};
    std::atomic<int> jobs_completed{0};
    std::vector<std::thread> threads;

    // Submitter threads
    for (int i = 0; i < num_submitters; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < jobs_per_submitter && !should_stop.load(); ++j) {
                ++jobs_attempted;
                try {
                    auto result = pool->submit([&jobs_completed]() {
                        ++jobs_completed;
                        std::this_thread::sleep_for(1ms);
                    });
                } catch (...) {
                    // Expected during shutdown
                }
            }
        });
    }

    // Let threads run for a bit
    std::this_thread::sleep_for(200ms);

    // Initiate shutdown while threads are submitting
    should_stop.store(true);
    pool->shutdown();

    // Wait for submitter threads
    for (auto& t : threads) {
        t.join();
    }

    // Pool should be in valid state after shutdown
    EXPECT_NO_THROW(pool->shutdown()); // Should be idempotent
}

// Test 3: Job cancellation race conditions
TEST_F(ThreadSystemThreadSafetyTest, CancellationRace) {
    auto pool = thread_pool::create(6);

    const int num_jobs = 1000;
    std::atomic<int> jobs_completed{0};
    std::atomic<int> jobs_cancelled{0};

    std::vector<std::shared_ptr<cancellation_token>> tokens;
    std::vector<std::thread> canceller_threads;

    // Submit jobs with cancellation tokens
    for (int i = 0; i < num_jobs; ++i) {
        auto token = std::make_shared<cancellation_token>();
        tokens.push_back(token);

        pool->submit([&jobs_completed, &jobs_cancelled, token]() {
            for (int j = 0; j < 100 && !token->is_cancelled(); ++j) {
                std::this_thread::sleep_for(100us);
            }

            if (token->is_cancelled()) {
                ++jobs_cancelled;
            } else {
                ++jobs_completed;
            }
        });
    }

    // Concurrently cancel some jobs
    for (int i = 0; i < 5; ++i) {
        canceller_threads.emplace_back([&tokens]() {
            for (size_t j = 0; j < tokens.size(); j += 10) {
                tokens[j]->cancel();
                std::this_thread::sleep_for(5ms);
            }
        });
    }

    for (auto& t : canceller_threads) {
        t.join();
    }

    pool->wait_all();
    EXPECT_EQ(jobs_completed.load() + jobs_cancelled.load(), num_jobs);

    pool->shutdown();
}

// Test 4: Dynamic worker addition during execution
TEST_F(ThreadSystemThreadSafetyTest, DynamicWorkerAddition) {
    auto pool = thread_pool::create(2);

    const int num_jobs = 1000;
    std::atomic<int> jobs_executed{0};
    std::atomic<bool> running{true};

    // Submit jobs continuously
    std::thread submitter([&]() {
        for (int i = 0; i < num_jobs && running.load(); ++i) {
            pool->submit([&jobs_executed]() {
                ++jobs_executed;
                std::this_thread::sleep_for(1ms);
            });

            if (i % 50 == 0) {
                std::this_thread::sleep_for(5ms);
            }
        }
    });

    // Dynamically add workers
    std::this_thread::sleep_for(100ms);
    pool->resize(6);

    std::this_thread::sleep_for(100ms);
    pool->resize(10);

    std::this_thread::sleep_for(100ms);
    pool->resize(4);

    submitter.join();
    running.store(false);

    pool->wait_all();
    EXPECT_EQ(jobs_executed.load(), num_jobs);

    pool->shutdown();
}

// Test 5: Job queue concurrent access
TEST_F(ThreadSystemThreadSafetyTest, JobQueueConcurrentAccess) {
    job_queue queue;

    const int num_producers = 10;
    const int num_consumers = 5;
    const int jobs_per_producer = 500;

    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};
    std::atomic<bool> running{true};
    std::vector<std::thread> threads;

    // Producers
    for (int i = 0; i < num_producers; ++i) {
        threads.emplace_back([&, prod_id = i]() {
            for (int j = 0; j < jobs_per_producer; ++j) {
                queue.push([prod_id, j]() {
                    // Job execution
                });
                ++produced;

                if (j % 50 == 0) {
                    std::this_thread::sleep_for(1ms);
                }
            }
        });
    }

    // Consumers
    for (int i = 0; i < num_consumers; ++i) {
        threads.emplace_back([&]() {
            while (running.load()) {
                auto job = queue.try_pop();
                if (job) {
                    ++consumed;
                    (*job)();
                } else {
                    std::this_thread::sleep_for(1ms);
                }
            }
        });
    }

    // Wait for producers
    for (int i = 0; i < num_producers; ++i) {
        threads[i].join();
    }

    // Allow consumers to drain queue
    std::this_thread::sleep_for(200ms);
    running.store(false);

    // Join consumers
    for (int i = num_producers; i < threads.size(); ++i) {
        threads[i].join();
    }

    EXPECT_EQ(produced.load(), num_producers * jobs_per_producer);
    EXPECT_LE(consumed.load(), produced.load());
}

// Test 6: Priority queue concurrent operations
TEST_F(ThreadSystemThreadSafetyTest, PriorityQueueConcurrent) {
    auto pool = thread_pool::create_with_priority(4);

    const int num_high_priority = 300;
    const int num_low_priority = 700;

    std::atomic<int> high_executed{0};
    std::atomic<int> low_executed{0};

    // Submit mixed priority jobs concurrently
    std::vector<std::thread> threads;

    threads.emplace_back([&]() {
        for (int i = 0; i < num_high_priority; ++i) {
            pool->submit_with_priority([&high_executed]() {
                ++high_executed;
            }, priority::high);
        }
    });

    threads.emplace_back([&]() {
        for (int i = 0; i < num_low_priority; ++i) {
            pool->submit_with_priority([&low_executed]() {
                ++low_executed;
            }, priority::low);
        }
    });

    for (auto& t : threads) {
        t.join();
    }

    pool->wait_all();

    EXPECT_EQ(high_executed.load(), num_high_priority);
    EXPECT_EQ(low_executed.load(), num_low_priority);

    pool->shutdown();
}

// Test 7: Future-based job execution concurrent
TEST_F(ThreadSystemThreadSafetyTest, FutureBasedJobsConcurrent) {
    auto pool = thread_pool::create(8);

    const int num_jobs = 500;
    std::vector<std::future<int>> futures;

    // Submit jobs returning futures
    for (int i = 0; i < num_jobs; ++i) {
        auto future = pool->submit_with_result([i]() {
            std::this_thread::sleep_for(1ms);
            return i * 2;
        });
        futures.push_back(std::move(future));
    }

    // Collect results concurrently
    std::atomic<int> collection_errors{0};
    std::vector<std::thread> collectors;

    const int num_collectors = 10;
    std::atomic<int> next_index{0};

    for (int i = 0; i < num_collectors; ++i) {
        collectors.emplace_back([&]() {
            while (true) {
                int index = next_index.fetch_add(1);
                if (index >= num_jobs) break;

                try {
                    int result = futures[index].get();
                    if (result != index * 2) {
                        ++collection_errors;
                    }
                } catch (...) {
                    ++collection_errors;
                }
            }
        });
    }

    for (auto& t : collectors) {
        t.join();
    }

    EXPECT_EQ(collection_errors.load(), 0);
    pool->shutdown();
}

// Test 8: Exception handling concurrent
TEST_F(ThreadSystemThreadSafetyTest, ExceptionHandlingConcurrent) {
    auto pool = thread_pool::create(6);

    const int num_jobs = 1000;
    std::atomic<int> exceptions_handled{0};

    for (int i = 0; i < num_jobs; ++i) {
        pool->submit([i, &exceptions_handled]() {
            try {
                if (i % 3 == 0) {
                    throw std::runtime_error("Test exception");
                }
            } catch (const std::exception&) {
                ++exceptions_handled;
            }
        });
    }

    pool->wait_all();

    int expected_exceptions = num_jobs / 3;
    EXPECT_NEAR(exceptions_handled.load(), expected_exceptions, 2);

    pool->shutdown();
}

// Test 9: Thread pool stress test
TEST_F(ThreadSystemThreadSafetyTest, ThreadPoolStressTest) {
    auto pool = thread_pool::create(12);

    const int duration_seconds = 2;
    std::atomic<int> jobs_completed{0};
    std::atomic<bool> running{true};

    std::vector<std::thread> submitters;
    const int num_submitters = 20;

    for (int i = 0; i < num_submitters; ++i) {
        submitters.emplace_back([&]() {
            while (running.load()) {
                pool->submit([&jobs_completed]() {
                    ++jobs_completed;
                    std::this_thread::sleep_for(100us);
                });
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
    running.store(false);

    for (auto& t : submitters) {
        t.join();
    }

    pool->wait_all();

    EXPECT_GT(jobs_completed.load(), 0);
    pool->shutdown();
}

// Test 10: Memory safety - no leaks during concurrent operations
TEST_F(ThreadSystemThreadSafetyTest, MemorySafetyTest) {
    const int num_iterations = 50;
    const int jobs_per_iteration = 100;

    std::atomic<int> total_errors{0};

    for (int iteration = 0; iteration < num_iterations; ++iteration) {
        auto pool = thread_pool::create(4);

        std::vector<std::thread> submitters;
        for (int i = 0; i < 5; ++i) {
            submitters.emplace_back([&]() {
                for (int j = 0; j < jobs_per_iteration; ++j) {
                    try {
                        pool->submit([]() {
                            std::this_thread::sleep_for(100us);
                        });
                    } catch (...) {
                        ++total_errors;
                    }
                }
            });
        }

        for (auto& t : submitters) {
            t.join();
        }

        pool->wait_all();
        pool->shutdown();

        // Pool destructor called here
    }

    EXPECT_EQ(total_errors.load(), 0);
}

// Test 11: Cancellation token hierarchical propagation
TEST_F(ThreadSystemThreadSafetyTest, CancellationTokenHierarchy) {
    const int depth = 10;
    const int operations_per_level = 100;
    std::atomic<int> cancelled_operations{0};
    std::atomic<int> errors{0};

    // Create a deep chain of linked cancellation tokens
    std::vector<std::shared_ptr<cancellation_token>> tokens;
    tokens.push_back(std::make_shared<cancellation_token>());

    for (int i = 1; i < depth; ++i) {
        tokens.push_back(std::make_shared<cancellation_token>(*tokens[i-1]));
    }

    // Register callbacks at each level
    std::vector<cancellation_registration> registrations;
    for (int i = 0; i < depth; ++i) {
        auto reg = tokens[i]->register_callback([&, level = i]() {
            ++cancelled_operations;
        });
        registrations.push_back(std::move(reg));
    }

    // Spawn threads that check cancellation at different levels
    std::vector<std::thread> threads;
    for (int i = 0; i < depth; ++i) {
        threads.emplace_back([&, level = i]() {
            for (int j = 0; j < operations_per_level; ++j) {
                try {
                    if (tokens[level]->is_cancelled()) {
                        ++cancelled_operations;
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                } catch (...) {
                    ++errors;
                }
            }
        });
    }

    // Cancel the root token after some time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    tokens[0]->cancel();

    for (auto& t : threads) {
        t.join();
    }

    // All tokens should be cancelled
    for (const auto& token : tokens) {
        EXPECT_TRUE(token->is_cancelled());
    }

    EXPECT_EQ(errors.load(), 0);
    EXPECT_GT(cancelled_operations.load(), 0);
}

// Test 12: Concurrent cancellation token registration and cancellation
TEST_F(ThreadSystemThreadSafetyTest, ConcurrentCancellationOperations) {
    const int num_threads = 20;
    const int operations_per_thread = 200;
    std::atomic<int> callback_invocations{0};
    std::atomic<int> errors{0};

    cancellation_token token;
    std::vector<std::thread> threads;

    // Half the threads register callbacks
    for (int i = 0; i < num_threads / 2; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                try {
                    auto reg = token.register_callback([&]() {
                        ++callback_invocations;
                    });
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                } catch (...) {
                    ++errors;
                }
            }
        });
    }

    // Other half check cancellation status
    for (int i = 0; i < num_threads / 2; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                try {
                    volatile bool cancelled = token.is_cancelled();
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                } catch (...) {
                    ++errors;
                }
            }
        });
    }

    // Cancel after some registrations
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    token.cancel();

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_TRUE(token.is_cancelled());
    EXPECT_EQ(errors.load(), 0);
    EXPECT_GT(callback_invocations.load(), 0);
}

// Test 13: Cancellation during job execution
TEST_F(ThreadSystemThreadSafetyTest, CancellationDuringExecution) {
    auto pool = thread_pool::create(8);
    cancellation_token token;

    const int num_jobs = 1000;
    std::atomic<int> jobs_started{0};
    std::atomic<int> jobs_completed{0};
    std::atomic<int> jobs_cancelled{0};

    std::vector<std::future<void>> futures;

    // Submit jobs that respect cancellation
    for (int i = 0; i < num_jobs; ++i) {
        auto result = pool->submit([&, job_id = i]() {
            ++jobs_started;

            for (int step = 0; step < 10; ++step) {
                if (token.is_cancelled()) {
                    ++jobs_cancelled;
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            ++jobs_completed;
        });

        if (result.is_ok()) {
            futures.push_back(std::move(result.value()));
        }
    }

    // Cancel after jobs have started
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    token.cancel();

    // Wait for all jobs
    for (auto& f : futures) {
        f.wait();
    }

    pool->shutdown();

    EXPECT_TRUE(token.is_cancelled());
    EXPECT_EQ(jobs_started.load(), jobs_completed.load() + jobs_cancelled.load());
    EXPECT_GT(jobs_cancelled.load(), 0);
}
