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

#include <gtest/gtest.h>
#include <memory>
#include <type_traits>
#include <functional>
#include <thread>
#include <atomic>
#include <vector>

#include "../../common_interfaces/threading_interface.h"
#include "../../common_interfaces/service_container_interface.h"
#include "../../common_interfaces/thread_context_interface.h"
#include "../../implementations/thread_pool/include/thread_pool.h"
#include "../../core/base/include/service_registry.h"
#include "../../interfaces/thread_context.h"

namespace {

// Mock implementations for testing polymorphic behavior
class mock_thread_pool : public common_interfaces::interface_thread_pool {
private:
    std::atomic<std::size_t> thread_count_{4};
    std::atomic<bool> running_{false};
    std::atomic<std::size_t> pending_tasks_{0};

public:
    auto submit_task(std::function<void()> task) -> bool override {
        if (running_ && task) {
            pending_tasks_.fetch_add(1, std::memory_order_relaxed);
            // Simulate task execution
            std::thread([this, task]() {
                task();
                pending_tasks_.fetch_sub(1, std::memory_order_relaxed);
            }).detach();
            return true;
        }
        return false;
    }

    auto get_thread_count() const -> std::size_t override {
        return thread_count_.load(std::memory_order_relaxed);
    }

    auto shutdown_pool(bool immediate = false) -> bool override {
        if (running_) {
            running_.store(false, std::memory_order_relaxed);
            return true;
        }
        return false;
    }

    auto is_running() const -> bool override {
        return running_.load(std::memory_order_relaxed);
    }

    auto get_pending_task_count() const -> std::size_t override {
        return pending_tasks_.load(std::memory_order_relaxed);
    }

    void start() {
        running_.store(true, std::memory_order_relaxed);
    }
};

class mock_thread : public common_interfaces::interface_thread {
private:
    std::atomic<bool> running_{false};
    std::thread::id thread_id_{};
    std::unique_ptr<std::thread> thread_;

public:
    auto start_thread() -> bool override {
        if (!running_) {
            running_.store(true, std::memory_order_relaxed);
            thread_ = std::make_unique<std::thread>([this]() {
                thread_id_ = std::this_thread::get_id();
                while (running_.load(std::memory_order_relaxed)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
            return true;
        }
        return false;
    }

    auto stop_thread(bool immediate = false) -> bool override {
        if (running_) {
            running_.store(false, std::memory_order_relaxed);
            return true;
        }
        return false;
    }

    auto is_thread_running() const -> bool override {
        return running_.load(std::memory_order_relaxed);
    }

    auto get_thread_id() const -> std::thread::id override {
        return thread_id_;
    }

    auto join_thread() -> bool override {
        if (thread_ && thread_->joinable()) {
            thread_->join();
            return true;
        }
        return false;
    }

    ~mock_thread() {
        if (running_) {
            stop_thread();
        }
        if (thread_ && thread_->joinable()) {
            thread_->join();
        }
    }
};

// Test service for service container compliance
class test_service {
public:
    virtual ~test_service() = default;
    virtual auto process() -> int = 0;
};

class test_service_impl : public test_service {
private:
    int counter_{0};

public:
    auto process() -> int override {
        return ++counter_;
    }

    auto get_counter() const -> int {
        return counter_;
    }
};

} // anonymous namespace

class InterfaceComplianceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test environment
    }

    void TearDown() override {
        // Cleanup test environment
    }
};

// T4.1 Task: Verify interface compliance of all implementation classes
TEST_F(InterfaceComplianceTest, ThreadPoolInterfaceCompliance) {
    // Test that thread_pool properly implements interface_thread_pool
    static_assert(std::is_base_of_v<common_interfaces::interface_thread_pool, 
                                    thread_pool_module::thread_pool>,
                  "thread_pool must inherit from interface_thread_pool");

    // Test polymorphic behavior
    std::unique_ptr<common_interfaces::interface_thread_pool> pool = 
        std::make_unique<thread_pool_module::thread_pool>("test_pool");

    // Verify interface methods are accessible
    EXPECT_EQ(pool->get_thread_count(), 0);  // Not started yet
    EXPECT_FALSE(pool->is_running());
    EXPECT_EQ(pool->get_pending_task_count(), 0);

    // Start the pool using internal method (cast needed for specific implementation)
    auto concrete_pool = static_cast<thread_pool_module::thread_pool*>(pool.get());
    auto start_result = concrete_pool->start();
    EXPECT_TRUE(start_result.has_value());

    if (start_result.has_value()) {
        EXPECT_TRUE(pool->is_running());
        EXPECT_GT(pool->get_thread_count(), 0);

        // Test task submission
        std::atomic<bool> task_executed{false};
        auto task = [&task_executed]() {
            task_executed.store(true, std::memory_order_relaxed);
        };

        EXPECT_TRUE(pool->submit_task(task));
        
        // Wait for task execution
        auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(1);
        while (!task_executed.load(std::memory_order_relaxed) && 
               std::chrono::steady_clock::now() < timeout) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        EXPECT_TRUE(task_executed.load(std::memory_order_relaxed));

        // Test shutdown
        EXPECT_TRUE(pool->shutdown_pool(false));
        EXPECT_FALSE(pool->is_running());
    }
}

// T4.1 Task: Virtual function table verification
TEST_F(InterfaceComplianceTest, VirtualFunctionTableVerification) {
    // Test that virtual function calls work correctly through base interface
    auto mock_pool = std::make_unique<mock_thread_pool>();
    mock_pool->start();

    std::unique_ptr<common_interfaces::interface_thread_pool> interface_ptr = 
        std::move(mock_pool);

    // Verify virtual function calls work through interface
    EXPECT_TRUE(interface_ptr->is_running());
    EXPECT_EQ(interface_ptr->get_thread_count(), 4);

    std::atomic<int> execution_count{0};
    auto task = [&execution_count]() {
        execution_count.fetch_add(1, std::memory_order_relaxed);
    };

    EXPECT_TRUE(interface_ptr->submit_task(task));
    
    // Wait for task execution
    auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (execution_count.load(std::memory_order_relaxed) == 0 && 
           std::chrono::steady_clock::now() < timeout) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    EXPECT_EQ(execution_count.load(std::memory_order_relaxed), 1);
    EXPECT_TRUE(interface_ptr->shutdown_pool());
    EXPECT_FALSE(interface_ptr->is_running());
}

// T4.1 Task: Polymorphic behavior verification  
TEST_F(InterfaceComplianceTest, PolymorphicBehaviorVerification) {
    // Create vector of different implementations through common interface
    std::vector<std::unique_ptr<common_interfaces::interface_thread_pool>> pools;
    
    // Add mock implementation
    auto mock_pool = std::make_unique<mock_thread_pool>();
    mock_pool->start();
    pools.push_back(std::move(mock_pool));

    // Add real implementation
    auto real_pool = std::make_unique<thread_pool_module::thread_pool>("real_pool");
    auto concrete_real = static_cast<thread_pool_module::thread_pool*>(real_pool.get());
    auto start_result = concrete_real->start();
    if (start_result.has_value()) {
        pools.push_back(std::move(real_pool));
    }

    // Test polymorphic behavior
    std::atomic<int> total_executions{0};
    for (auto& pool : pools) {
        EXPECT_TRUE(pool->is_running());
        
        auto task = [&total_executions]() {
            total_executions.fetch_add(1, std::memory_order_relaxed);
        };

        EXPECT_TRUE(pool->submit_task(task));
    }

    // Wait for all tasks to complete
    auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (total_executions.load(std::memory_order_relaxed) < static_cast<int>(pools.size()) && 
           std::chrono::steady_clock::now() < timeout) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(total_executions.load(std::memory_order_relaxed), static_cast<int>(pools.size()));

    // Shutdown all pools
    for (auto& pool : pools) {
        EXPECT_TRUE(pool->shutdown_pool());
        EXPECT_FALSE(pool->is_running());
    }
}

// T4.1 Task: Service container interface compliance
TEST_F(InterfaceComplianceTest, ServiceContainerInterfaceCompliance) {
    // Test that service_registry properly implements interface_service_container
    static_assert(std::is_base_of_v<common_interfaces::interface_service_container, 
                                    thread_module::service_registry>,
                  "service_registry must inherit from interface_service_container");

    std::unique_ptr<common_interfaces::interface_service_container> container = 
        std::make_unique<thread_module::service_registry>();

    // Test initial state
    EXPECT_EQ(container->get_service_count(), 0);
    EXPECT_FALSE(container->contains_service<test_service>());

    // Test service registration
    auto service = std::make_shared<test_service_impl>();
    EXPECT_TRUE(container->register_service<test_service>(service));
    EXPECT_EQ(container->get_service_count(), 1);
    EXPECT_TRUE(container->contains_service<test_service>());

    // Test service resolution
    auto resolved = container->resolve_service<test_service>();
    ASSERT_NE(resolved, nullptr);

    // Test polymorphic behavior
    EXPECT_EQ(resolved->process(), 1);
    EXPECT_EQ(resolved->process(), 2);

    // Verify it's the same instance
    auto concrete_service = std::dynamic_pointer_cast<test_service_impl>(resolved);
    ASSERT_NE(concrete_service, nullptr);
    EXPECT_EQ(concrete_service->get_counter(), 2);

    // Test cleanup
    EXPECT_TRUE(container->clear_services());
    EXPECT_EQ(container->get_service_count(), 0);
    EXPECT_FALSE(container->contains_service<test_service>());
}

// T4.1 Task: Thread interface compliance
TEST_F(InterfaceComplianceTest, ThreadInterfaceCompliance) {
    // Test mock thread implementation
    std::unique_ptr<common_interfaces::interface_thread> thread = 
        std::make_unique<mock_thread>();

    // Test initial state
    EXPECT_FALSE(thread->is_thread_running());
    EXPECT_EQ(thread->get_thread_id(), std::thread::id{});

    // Test thread start
    EXPECT_TRUE(thread->start_thread());
    EXPECT_TRUE(thread->is_thread_running());

    // Wait a moment for thread to start properly
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_NE(thread->get_thread_id(), std::thread::id{});

    // Test thread stop
    EXPECT_TRUE(thread->stop_thread());
    EXPECT_FALSE(thread->is_thread_running());

    // Test join
    EXPECT_TRUE(thread->join_thread());
}

// T4.1 Task: Multiple interface inheritance verification
TEST_F(InterfaceComplianceTest, MultipleInterfaceInheritanceVerification) {
    // Test that thread_pool correctly implements multiple interfaces
    auto pool = std::make_unique<thread_pool_module::thread_pool>("multi_interface_test");

    // Test interface_thread_pool interface
    common_interfaces::interface_thread_pool* thread_pool_interface = pool.get();
    EXPECT_NE(thread_pool_interface, nullptr);
    EXPECT_FALSE(thread_pool_interface->is_running());

    // Test executor_interface (if accessible)
    thread_module::executor_interface* executor_interface = pool.get();
    EXPECT_NE(executor_interface, nullptr);

    // Start pool and test functionality through different interfaces
    auto start_result = pool->start();
    if (start_result.has_value()) {
        EXPECT_TRUE(thread_pool_interface->is_running());
        EXPECT_GT(thread_pool_interface->get_thread_count(), 0);

        // Test task submission through interface
        std::atomic<bool> task_completed{false};
        auto task = [&task_completed]() {
            task_completed.store(true, std::memory_order_relaxed);
        };

        EXPECT_TRUE(thread_pool_interface->submit_task(task));

        // Wait for completion
        auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(1);
        while (!task_completed.load(std::memory_order_relaxed) && 
               std::chrono::steady_clock::now() < timeout) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        EXPECT_TRUE(task_completed.load(std::memory_order_relaxed));
        EXPECT_TRUE(thread_pool_interface->shutdown_pool());
    }
}

// T4.1 Task: Interface method override verification
TEST_F(InterfaceComplianceTest, InterfaceMethodOverrideVerification) {
    // Verify that all interface methods are properly overridden in implementations
    
    // Create instances and test that methods are overridden (not pure virtual)
    thread_module::service_registry registry;
    
    // These calls should not fail with pure virtual function calls
    EXPECT_NO_THROW({
        auto count = registry.get_service_count();
        auto cleared = registry.clear_services();
        auto contains = registry.contains_service<test_service>();
        EXPECT_GE(count, 0);
        EXPECT_TRUE(cleared);
        EXPECT_FALSE(contains);
    });

    // Test thread pool overrides
    thread_pool_module::thread_pool pool("override_test");
    
    EXPECT_NO_THROW({
        auto count = pool.get_thread_count();
        auto running = pool.is_running();
        auto pending = pool.get_pending_task_count();
        EXPECT_GE(count, 0);
        EXPECT_FALSE(running);  // Not started
        EXPECT_EQ(pending, 0);
    });
}

// T4.1 Task: Interface const-correctness verification
TEST_F(InterfaceComplianceTest, InterfaceConstCorrectnessVerification) {
    const thread_module::service_registry const_registry;
    
    // Test const methods don't modify state
    EXPECT_NO_THROW({
        auto count1 = const_registry.get_service_count();
        auto contains1 = const_registry.contains_service<test_service>();
        auto count2 = const_registry.get_service_count();
        auto contains2 = const_registry.contains_service<test_service>();
        
        EXPECT_EQ(count1, count2);
        EXPECT_EQ(contains1, contains2);
    });

    const thread_pool_module::thread_pool const_pool("const_test");
    
    EXPECT_NO_THROW({
        auto count1 = const_pool.get_thread_count();
        auto running1 = const_pool.is_running();
        auto pending1 = const_pool.get_pending_task_count();
        
        auto count2 = const_pool.get_thread_count();
        auto running2 = const_pool.is_running();
        auto pending2 = const_pool.get_pending_task_count();
        
        EXPECT_EQ(count1, count2);
        EXPECT_EQ(running1, running2);
        EXPECT_EQ(pending1, pending2);
    });
}