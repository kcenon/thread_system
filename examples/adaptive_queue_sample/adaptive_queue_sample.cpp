/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, 🍀☀🌕🌥 🌊
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

#include <kcenon/thread/queue/adaptive_job_queue.h>
#include <kcenon/thread/core/callback_job.h>

#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>
#include <iostream>
#include <iomanip>

using namespace kcenon::thread;
using namespace std::chrono_literals;

// Helper to convert mode to string
std::string mode_to_string(adaptive_job_queue::mode m) {
    switch (m) {
        case adaptive_job_queue::mode::mutex: return "mutex";
        case adaptive_job_queue::mode::lock_free: return "lock_free";
    }
    return "unknown";
}

// Example 1: Basic queue policies comparison
void policy_comparison_example()
{
    std::cout << "[Example 1] Queue Policy Comparison" << std::endl;

    const int num_jobs = 10000;
    const int num_producers = 4;
    const int num_consumers = 4;

    // Test each policy
    for (auto policy : {adaptive_job_queue::policy::accuracy_first,
                        adaptive_job_queue::policy::performance_first,
                        adaptive_job_queue::policy::balanced})
    {
        adaptive_job_queue queue(policy);
        std::atomic<int> produced{0};
        std::atomic<int> consumed{0};

        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> producers;
        std::vector<std::thread> consumers;

        // Start producers
        for (int p = 0; p < num_producers; ++p) {
            producers.emplace_back([&queue, &produced, p, num_jobs, num_producers]() {
                for (int i = 0; i < num_jobs / num_producers; ++i) {
                    auto job = std::make_unique<callback_job>(
                        [p, i]() -> result_void {
                            return result_void();
                        });

                    while (true) {
                        auto r = queue.enqueue(std::move(job));
                        if (!r.has_error()) break;
                        std::this_thread::yield();
                        // Recreate moved job for retry
                        job = std::make_unique<callback_job>(
                            [p, i]() -> result_void { return result_void(); });
                    }
                    produced.fetch_add(1);
                }
            });
        }

        // Start consumers
        for (int c = 0; c < num_consumers; ++c) {
            consumers.emplace_back([&queue, &consumed, num_jobs]() {
                while (consumed.load() < num_jobs) {
                    auto result = queue.dequeue();
                    if (result.has_value()) {
                        auto& job = result.value();
                        auto work_result = job->do_work();
                        (void)work_result; // Ignore result for sample
                        consumed.fetch_add(1);
                    } else {
                        std::this_thread::yield();
                    }
                }
            });
        }

        // Wait for completion
        for (auto& t : producers) t.join();
        for (auto& t : consumers) t.join();

        auto duration = std::chrono::high_resolution_clock::now() - start;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        std::string policy_name;
        switch (policy) {
            case adaptive_job_queue::policy::accuracy_first:
                policy_name = "Accuracy (Mutex)";
                break;
            case adaptive_job_queue::policy::performance_first:
                policy_name = "Performance (Lock-free)";
                break;
            case adaptive_job_queue::policy::balanced:
                policy_name = "Balanced (Adaptive)";
                break;
            case adaptive_job_queue::policy::manual:
                policy_name = "Manual";
                break;
        }

        double ops_per_sec = (ms > 0) ? (num_jobs * 1000.0 / ms) : 0;
        std::cout << policy_name << " policy: " << num_jobs << " jobs in "
                  << ms << " ms = " << std::fixed << std::setprecision(0)
                  << ops_per_sec << " ops/sec" << std::endl;
    }
}

// Example 2: Adaptive strategy behavior under varying contention
void adaptive_behavior_example()
{
    std::cout << "\n[Example 2] Balanced Policy Behavior" << std::endl;

    adaptive_job_queue queue(adaptive_job_queue::policy::balanced);

    // Low contention phase (1 producer, 1 consumer)
    std::cout << "Phase 1: Low contention (1P-1C)" << std::endl;
    {
        std::atomic<bool> running{true};
        std::atomic<int> jobs_processed{0};

        std::thread producer([&queue, &running]() {
            while (running) {
                auto job = std::make_unique<callback_job>(
                    []() -> result_void { return result_void(); });
                auto enqueue_result = queue.enqueue(std::move(job));
                if (enqueue_result.has_error()) {
                    std::cerr << "enqueue failed: " << enqueue_result.get_error().message() << std::endl;
                }
                std::this_thread::sleep_for(1ms);
            }
        });

        std::thread consumer([&queue, &running, &jobs_processed]() {
            while (running) {
                auto result = queue.dequeue();
                if (result.has_value()) {
                    auto work_result = result.value()->do_work();
                    (void)work_result; // Ignore result for sample
                    jobs_processed.fetch_add(1);
                }
                std::this_thread::sleep_for(1ms);
            }
        });

        std::this_thread::sleep_for(2s);
        running = false;
        producer.join();
        consumer.join();

        auto current_mode = queue.current_mode();
        std::cout << "  Current mode: " << mode_to_string(current_mode)
                  << ", Jobs processed: " << jobs_processed.load() << std::endl;
    }

    // High contention phase (8 producers, 8 consumers)
    std::cout << "Phase 2: High contention (8P-8C)" << std::endl;
    {
        std::atomic<bool> running{true};
        std::atomic<int> jobs_processed{0};
        std::vector<std::thread> threads;

        // Start producers
        for (int i = 0; i < 8; ++i) {
            threads.emplace_back([&queue, &running]() {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dist(0, 100);

                while (running) {
                    auto job = std::make_unique<callback_job>(
                        []() -> result_void { return result_void(); });
                    auto enqueue_result = queue.enqueue(std::move(job));
                    if (enqueue_result.has_error()) {
                        // Best-effort: ignore for demo
                    }
                    if (dist(gen) < 10) {  // 10% chance of sleep
                        std::this_thread::sleep_for(std::chrono::microseconds(dist(gen)));
                    }
                }
            });
        }

        // Start consumers
        for (int i = 0; i < 8; ++i) {
            threads.emplace_back([&queue, &running, &jobs_processed]() {
                while (running) {
                    auto result = queue.dequeue();
                    if (result.has_value()) {
                        auto work_result = result.value()->do_work();
                        (void)work_result; // Ignore result for sample
                        jobs_processed.fetch_add(1);
                    }
                }
            });
        }

        std::this_thread::sleep_for(2s);
        running = false;
        for (auto& t : threads) t.join();

        auto current_mode = queue.current_mode();
        std::cout << "  Current mode: " << mode_to_string(current_mode)
                  << ", Jobs processed: " << jobs_processed.load() << std::endl;
    }
}

// Example 3: Different queue policies
void different_policies_example()
{
    std::cout << "\n[Example 3] Different Queue Policies" << std::endl;

    // Create queue with accuracy-first policy (mutex mode)
    adaptive_job_queue mutex_queue(adaptive_job_queue::policy::accuracy_first);
    std::cout << "Accuracy-first queue mode: " << mode_to_string(mutex_queue.current_mode()) << std::endl;

    // Perform some operations
    std::vector<std::unique_ptr<job>> jobs;
    for (int i = 0; i < 100; ++i) {
        jobs.push_back(std::make_unique<callback_job>(
            [i]() -> result_void {
                // Job executed silently for batch demo
                return result_void(); // Success
            }));
    }

    // Enqueue jobs one by one
    int enqueue_count = 0;
    for (auto& job : jobs) {
        auto result = mutex_queue.enqueue(std::move(job));
        if (!result.has_error()) {
            enqueue_count++;
        }
    }
    std::cout << "Enqueued " << enqueue_count << " jobs" << std::endl;

    // Create queue with performance-first policy (lock-free mode)
    adaptive_job_queue lockfree_queue(adaptive_job_queue::policy::performance_first);
    std::cout << "Performance-first queue mode: " << mode_to_string(lockfree_queue.current_mode()) << std::endl;

    // Dequeue and process jobs from mutex queue
    int success_count = 0;
    int fail_count = 0;
    while (!mutex_queue.empty()) {
        auto result = mutex_queue.dequeue();
        if (result.has_value()) {
            auto work_result = result.value()->do_work();
            if (!work_result) {
                success_count++;
            } else {
                fail_count++;
                std::cerr << "Job failed: " << work_result.get_error().message() << std::endl;
            }
        }
    }
    std::cout << "Processed " << success_count << " jobs successfully, "
              << fail_count << " failed" << std::endl;
}

// Example 4: Performance monitoring
void performance_monitoring_example()
{
    std::cout << "\n[Example 4] Performance Monitoring" << std::endl;

    adaptive_job_queue queue(adaptive_job_queue::policy::balanced);

    const int num_operations = 50000;
    std::atomic<bool> running{true};
    std::atomic<int> enqueued{0};
    std::atomic<int> dequeued{0};

    // Producer thread
    std::thread producer([&queue, &enqueued, num_operations]() {
        for (int i = 0; i < num_operations; ++i) {
            auto job = std::make_unique<callback_job>(
                []() -> result_void { return result_void(); });

            while (queue.enqueue(std::move(job)).has_error()) {
                std::this_thread::yield();
                job = std::make_unique<callback_job>(
                    []() -> result_void { return result_void(); });
            }
            enqueued.fetch_add(1);
        }
    });

    // Consumer thread
    std::thread consumer([&queue, &dequeued, num_operations]() {
        while (dequeued.load() < num_operations) {
            auto result = queue.dequeue();
            if (result.has_value()) {
                auto work_result = result.value()->do_work();
                (void)work_result; // Ignore result for sample
                dequeued.fetch_add(1);
            }
        }
    });

    // Monitor thread
    std::thread monitor([&queue, &running, &enqueued, &dequeued, num_operations]() {
        auto start = std::chrono::steady_clock::now();

        while (dequeued.load() < num_operations) {
            std::this_thread::sleep_for(500ms);

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration<double>(now - start).count();

            auto current_mode = queue.current_mode();
            double rate = (elapsed > 0) ? (dequeued.load() / elapsed) : 0;

            std::cout << "Status: " << mode_to_string(current_mode) << " mode, Enqueued: "
                      << enqueued.load() << ", Dequeued: " << dequeued.load()
                      << ", Rate: " << std::fixed << std::setprecision(0)
                      << rate << " ops/sec" << std::endl;
        }
    });

    producer.join();
    consumer.join();
    running = false;
    monitor.join();

    // Print statistics
    auto stats = queue.get_stats();
    std::cout << "Completed " << num_operations << " operations" << std::endl;
    std::cout << "Statistics: mode_switches=" << stats.mode_switches
              << ", enqueues=" << stats.enqueue_count
              << ", dequeues=" << stats.dequeue_count << std::endl;
}

// Example 5: Real-world scenario - Web server simulation
void web_server_simulation()
{
    std::cout << "\n[Example 5] Web Server Simulation" << std::endl;

    adaptive_job_queue request_queue(adaptive_job_queue::policy::balanced);
    std::atomic<bool> server_running{true};
    std::atomic<int> requests_handled{0};
    std::atomic<int> requests_failed{0};

    // Request types
    enum class request_type { GET, POST, PUT, DELETE };

    // Simulate incoming requests
    std::vector<std::thread> clients;
    for (int client_id = 0; client_id < 5; ++client_id) {
        clients.emplace_back([&request_queue, &server_running, &requests_failed, client_id]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> type_dist(0, 3);
            std::uniform_int_distribution<> delay_dist(10, 100);

            while (server_running) {
                auto type = static_cast<request_type>(type_dist(gen));

                auto request = std::make_unique<callback_job>(
                    [type]() -> result_void {
                        // Simulate request processing
                        std::this_thread::sleep_for(std::chrono::microseconds(
                            type == request_type::GET ? 10 : 50));
                        return result_void(); // Success
                    });

                auto r = request_queue.enqueue(std::move(request));
                if (r.has_error()) requests_failed.fetch_add(1);

                std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
            }
        });
    }

    // Worker threads (server handlers)
    std::vector<std::thread> workers;
    for (int worker_id = 0; worker_id < 3; ++worker_id) {
        workers.emplace_back([&request_queue, &server_running, &requests_handled, worker_id]() {
            while (server_running) {
                auto request = request_queue.dequeue();
                if (request.has_value()) {
                    auto result = request.value()->do_work();
                    if (!result) {
                        // Request processed successfully
                        requests_handled.fetch_add(1);
                    } else {
                        std::cerr << "Worker " << worker_id << " request failed: "
                                  << result.get_error().message() << std::endl;
                    }
                } else {
                    std::this_thread::sleep_for(1ms);
                }
            }
        });
    }

    // Run simulation for 5 seconds
    std::this_thread::sleep_for(5s);
    server_running = false;

    // Cleanup
    for (auto& t : clients) t.join();
    for (auto& t : workers) t.join();

    std::cout << "Server simulation complete: " << requests_handled.load()
              << " requests handled, " << requests_failed.load() << " failed" << std::endl;

    // Print final statistics
    auto stats = request_queue.get_stats();
    std::cout << "Final stats: mode_switches=" << stats.mode_switches
              << ", time_in_mutex=" << stats.time_in_mutex_ms << "ms"
              << ", time_in_lockfree=" << stats.time_in_lockfree_ms << "ms" << std::endl;
}

int main()
{
    std::cout << "Adaptive Job Queue Sample" << std::endl;
    std::cout << "=========================" << std::endl;

    try {
        policy_comparison_example();
        adaptive_behavior_example();
        different_policies_example();
        performance_monitoring_example();
        web_server_simulation();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\nAll examples completed!" << std::endl;

    return 0;
}
