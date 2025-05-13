#include "sources/thread_pool/thread_pool_builder.h"
#include "sources/thread_pool/task.h"
#include "sources/thread_base/cancellation_token.h"

#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <random>
#include <iomanip>
#include <string>
#include <functional>

using namespace thread_pool_module;
using namespace thread_module;

// Helper function to calculate prime numbers (CPU-intensive task)
bool is_prime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) {
            return false;
        }
    }
    
    return true;
}

// Function to count primes in a range
int count_primes(int start, int end) {
    int count = 0;
    for (int i = start; i < end; ++i) {
        if (is_prime(i)) {
            count++;
        }
    }
    return count;
}

// Example 1: Basic thread pool usage
void basic_thread_pool_example() {
    std::cout << "\n=== Basic Thread Pool Example ===\n";
    
    // Create a thread pool with 4 workers using the new builder pattern
    auto pool_result = thread_pool_builder()
        .with_thread_count(4)
        .with_thread_name_prefix("worker")
        .with_work_stealing(true)
        .build_and_start();
    
    if (!pool_result) {
        std::cerr << "Failed to create thread pool: " 
                  << pool_result.get_error().message() << std::endl;
        return;
    }
    
    auto pool = pool_result.value();
    
    // Submit a simple lambda job using the job class
    auto job = std::make_unique<thread_module::job>();
    // TODO: We would need to update the job class to use lambdas directly
    
    // Create futures for prime calculations
    std::vector<std::future<int>> futures;
    
    for (int i = 0; i < 10; ++i) {
        auto range_start = i * 10000 + 1;
        auto range_end = (i + 1) * 10000;
        
        // Submit jobs directly
        auto future = std::async(std::launch::async, count_primes, range_start, range_end);
        futures.push_back(std::move(future));
    }
    
    // Collect and display results
    int total_primes = 0;
    for (size_t i = 0; i < futures.size(); ++i) {
        int primes = futures[i].get();
        total_primes += primes;
        std::cout << "Primes in range " << (i * 10000 + 1) << "-" << ((i + 1) * 10000)
                  << ": " << primes << std::endl;
    }
    
    std::cout << "Total primes found: " << total_primes << std::endl;
    
    // Pool will be stopped in destructor
}

// Example 2: Using coroutines and tasks
task<void> coroutine_example() {
    std::cout << "\n=== Coroutine Task Example ===\n";
    
    // Demonstrate co_await with a delay
    std::cout << "Starting a 500ms delay..." << std::endl;
    std::cout.flush(); // Ensure output is displayed immediately
    
    // Get the current time
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Create a delay task and await it
    co_await delay(std::chrono::milliseconds(500));
    
    // Calculate elapsed time
    auto elapsed = std::chrono::high_resolution_clock::now() - start_time;
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    
    std::cout << "Delay completed after " << duration << "ms" << std::endl;
    std::cout << "Coroutine example completed." << std::endl;
    std::cout.flush();
    
    // Create a simple non-awaitable task
    auto simple_task = make_task([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 42;
    });
    
    // Await the simple task
    auto result = co_await simple_task;
    std::cout << "Simple task returned: " << result << std::endl;
    std::cout.flush();
    
    co_return;
}

// Example 3: Error handling and cancellation
task<void> error_handling_example() {
    std::cout << "\n=== Error Handling and Cancellation Example ===\n";
    std::cout.flush();
    
    // Create a task that fails
    auto failing_task = make_task([]() -> result_t<int> {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return error{error_code::job_execution_failed, "This task was designed to fail"};
    });
    
    // Await the failing task directly
    try {
        auto result_value = co_await failing_task;
        if (result_value.has_value()) {
            std::cout << "Task result: " << result_value.value() << std::endl;
        } else {
            std::cout << "Task failed with error: " << result_value.get_error().message() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Task threw exception: " << e.what() << std::endl;
    }
    
    // Create a cancellation token
    auto token = cancellation_token::create();
    
    // Create a task that respects cancellation
    auto cancellable_task = make_task([token]() -> result_t<void> {
        for (int i = 0; i < 10; ++i) {
            // Check for cancellation
            if (token.is_cancelled()) {
                return error{error_code::operation_canceled, "Operation was cancelled"};
            }
            
            std::cout << "Working: " << i * 10 << "% complete" << std::endl;
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        
        return {};
    });
    
    // Start the task (we don't await here to allow cancellation)
    std::thread t([cancellable_task = std::move(cancellable_task)]() mutable {
        auto result = cancellable_task.wait();
        if (!result) {
            std::cout << "Task failed: " << result.get_error().message() << std::endl;
        }
    });
    t.detach();
    
    // Wait a bit, then cancel
    std::cout << "Waiting 700ms before cancelling..." << std::endl;
    std::cout.flush();
    co_await delay(std::chrono::milliseconds(700));
    
    std::cout << "Cancelling the task..." << std::endl;
    std::cout.flush();
    token.cancel();
    
    // Wait for cancellation to take effect
    std::cout << "Waiting 500ms for cancellation to take effect..." << std::endl;
    std::cout.flush();
    co_await delay(std::chrono::milliseconds(500));
    
    std::cout << "Error handling example completed" << std::endl;
    std::cout.flush();
}

// Main function to run examples
int main() {
    std::cout << "Modern Thread System Examples" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout.flush();
    
    // Run the basic example
    basic_thread_pool_example();
    
    // Run the coroutine example
    std::cout << "Starting coroutine example..." << std::endl;
    std::cout.flush();
    
    auto coroutine_task = coroutine_example();
    
    std::cout << "Coroutine task created, waiting for completion..." << std::endl;
    std::cout.flush();
    
    // Wait for the task to complete
    auto coroutine_result = coroutine_task.wait();
    if (!coroutine_result) {
        std::cerr << "Coroutine example failed: " << coroutine_result.get_error().message() << std::endl;
    }
    
    // Run the error handling example
    std::cout << "Starting error handling example..." << std::endl;
    std::cout.flush();
    
    auto error_task = error_handling_example();
    
    std::cout << "Error handling task created, waiting for completion..." << std::endl;
    std::cout.flush();
    
    // Wait for the task to complete
    auto error_result = error_task.wait();
    if (!error_result) {
        std::cerr << "Error handling example failed: " << error_result.get_error().message() << std::endl;
    }
    
    std::cout << "\nAll examples completed successfully!" << std::endl;
    std::cout.flush();
    
    // Allow any detached threads to complete
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    return 0;
}