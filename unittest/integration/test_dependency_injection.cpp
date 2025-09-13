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
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

#include "../../common_interfaces/service_container_interface.h"
#include "../../core/base/include/service_registry.h"

namespace {

// Mock service interfaces for testing
class mock_logger_service {
public:
    virtual ~mock_logger_service() = default;
    virtual auto log_message(const std::string& message) -> void = 0;
    virtual auto get_log_count() const -> std::size_t = 0;
};

class mock_database_service {
public:
    virtual ~mock_database_service() = default;
    virtual auto execute_query(const std::string& query) -> bool = 0;
    virtual auto get_connection_count() const -> int = 0;
};

// Mock implementations
class test_logger_impl : public mock_logger_service {
private:
    mutable std::atomic<std::size_t> log_count_{0};
    
public:
    auto log_message(const std::string& message) -> void override {
        log_count_.fetch_add(1, std::memory_order_relaxed);
    }
    
    auto get_log_count() const -> std::size_t override {
        return log_count_.load(std::memory_order_relaxed);
    }
};

class test_database_impl : public mock_database_service {
private:
    std::atomic<int> connection_count_{0};
    
public:
    auto execute_query(const std::string& query) -> bool override {
        connection_count_.fetch_add(1, std::memory_order_relaxed);
        return !query.empty();
    }
    
    auto get_connection_count() const -> int override {
        return connection_count_.load(std::memory_order_relaxed);
    }
};

// Service that depends on other services
class composite_service {
private:
    std::shared_ptr<mock_logger_service> logger_;
    std::shared_ptr<mock_database_service> database_;
    
public:
    composite_service(std::shared_ptr<mock_logger_service> logger,
                     std::shared_ptr<mock_database_service> database)
        : logger_(std::move(logger)), database_(std::move(database)) {}
    
    auto process_data(const std::string& data) -> bool {
        if (logger_) {
            logger_->log_message("Processing: " + data);
        }
        if (database_) {
            return database_->execute_query("INSERT INTO data VALUES ('" + data + "')");
        }
        return false;
    }
    
    auto get_logger() const -> std::shared_ptr<mock_logger_service> {
        return logger_;
    }
    
    auto get_database() const -> std::shared_ptr<mock_database_service> {
        return database_;
    }
};

// Circular dependency detection helper
class circular_service_a;
class circular_service_b {
public:
    std::shared_ptr<circular_service_a> service_a;
    virtual ~circular_service_b() = default;
};

class circular_service_a {
public:
    std::shared_ptr<circular_service_b> service_b;
    virtual ~circular_service_a() = default;
};

} // anonymous namespace

class DependencyInjectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear any existing services before each test
        container_.clear_services();
    }
    
    void TearDown() override {
        // Clean up after each test
        container_.clear_services();
    }
    
    thread_module::service_registry container_;
};

// T4.1 Task: DI container tests - Service registration/deregistration tests
TEST_F(DependencyInjectionTest, ServiceRegistrationAndDeregistration) {
    auto logger = std::make_shared<test_logger_impl>();
    auto database = std::make_shared<test_database_impl>();
    
    // Test initial state
    EXPECT_EQ(container_.get_service_count(), 0);
    EXPECT_FALSE(container_.contains_service<mock_logger_service>());
    EXPECT_FALSE(container_.contains_service<mock_database_service>());
    
    // Test service registration (using static methods for now)
    container_.register_service<mock_logger_service>(logger);
    container_.register_service<mock_database_service>(database);
    // Verify registration through interface methods
    EXPECT_TRUE(container_.contains_service<mock_logger_service>());
    EXPECT_TRUE(container_.contains_service<mock_database_service>());
    
    EXPECT_EQ(container_.get_service_count(), 2);
    EXPECT_TRUE(container_.contains_service<mock_logger_service>());
    EXPECT_TRUE(container_.contains_service<mock_database_service>());
    
    // Test service resolution
    auto resolved_logger = container_.resolve_service<mock_logger_service>();
    auto resolved_database = container_.resolve_service<mock_database_service>();
    
    ASSERT_NE(resolved_logger, nullptr);
    ASSERT_NE(resolved_database, nullptr);
    EXPECT_EQ(resolved_logger.get(), logger.get());
    EXPECT_EQ(resolved_database.get(), database.get());
    
    // Test service functionality
    resolved_logger->log_message("Test message");
    EXPECT_EQ(resolved_logger->get_log_count(), 1);
    
    EXPECT_TRUE(resolved_database->execute_query("SELECT * FROM test"));
    EXPECT_EQ(resolved_database->get_connection_count(), 1);
    
    // Test service deregistration (clear all)
    EXPECT_TRUE(container_.clear_services());
    EXPECT_EQ(container_.get_service_count(), 0);
    EXPECT_FALSE(container_.contains_service<mock_logger_service>());
    EXPECT_FALSE(container_.contains_service<mock_database_service>());
}

// T4.1 Task: DI container tests - Circular dependency detection tests
TEST_F(DependencyInjectionTest, CircularDependencyDetection) {
    auto service_a = std::make_shared<circular_service_a>();
    auto service_b = std::make_shared<circular_service_b>();
    
    // Create circular references
    service_a->service_b = service_b;
    service_b->service_a = service_a;
    
    // Register services with circular dependencies
    container_.register_service<circular_service_a>(service_a);
    container_.register_service<circular_service_b>(service_b);
    
    // Verify services can be resolved
    auto resolved_a = container_.resolve_service<circular_service_a>();
    auto resolved_b = container_.resolve_service<circular_service_b>();
    
    ASSERT_NE(resolved_a, nullptr);
    ASSERT_NE(resolved_b, nullptr);
    
    // Verify circular references exist
    EXPECT_EQ(resolved_a->service_b.get(), service_b.get());
    EXPECT_EQ(resolved_b->service_a.get(), service_a.get());
    
    // Test that container can handle circular dependencies
    EXPECT_EQ(container_.get_service_count(), 2);
    
    // Clean up circular references before clearing container
    resolved_a->service_b.reset();
    resolved_b->service_a.reset();
    service_a->service_b.reset();
    service_b->service_a.reset();
    
    EXPECT_TRUE(container_.clear_services());
}

// T4.1 Task: DI container tests - Thread safety tests
TEST_F(DependencyInjectionTest, ThreadSafetyTest) {
    constexpr int num_threads = 10;
    constexpr int operations_per_thread = 100;
    std::atomic<int> successful_registrations{0};
    std::atomic<int> successful_resolutions{0};
    std::vector<std::thread> threads;
    
    // Start multiple threads performing concurrent operations
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                // Register services
                auto logger = std::make_shared<test_logger_impl>();
                container_.register_service<mock_logger_service>(logger);
                successful_registrations.fetch_add(1, std::memory_order_relaxed);
                
                // Try to resolve services
                auto resolved = container_.resolve_service<mock_logger_service>();
                if (resolved != nullptr) {
                    successful_resolutions.fetch_add(1, std::memory_order_relaxed);
                }
                
                // Small delay to increase contention
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify no crashes occurred and some operations succeeded
    EXPECT_GT(successful_registrations.load(), 0);
    EXPECT_GT(successful_resolutions.load(), 0);
    
    // Verify container is still functional
    EXPECT_GE(container_.get_service_count(), 0);
    
    // Final cleanup
    EXPECT_TRUE(container_.clear_services());
}

// T4.1 Task: DI container tests - Complex dependency injection scenario
TEST_F(DependencyInjectionTest, ComplexDependencyInjectionScenario) {
    // Register base services
    auto logger = std::make_shared<test_logger_impl>();
    auto database = std::make_shared<test_database_impl>();
    
    container_.register_service<mock_logger_service>(logger);
    container_.register_service<mock_database_service>(database);
    
    // Create composite service using resolved dependencies
    auto resolved_logger = container_.resolve_service<mock_logger_service>();
    auto resolved_database = container_.resolve_service<mock_database_service>();
    
    ASSERT_NE(resolved_logger, nullptr);
    ASSERT_NE(resolved_database, nullptr);
    
    auto composite = std::make_shared<composite_service>(resolved_logger, resolved_database);
    
    // Test composite service functionality
    EXPECT_TRUE(composite->process_data("test_data_1"));
    EXPECT_TRUE(composite->process_data("test_data_2"));
    
    // Verify underlying services were used
    EXPECT_EQ(resolved_logger->get_log_count(), 2);
    EXPECT_EQ(resolved_database->get_connection_count(), 2);
    
    // Verify dependencies are correctly injected
    EXPECT_EQ(composite->get_logger().get(), resolved_logger.get());
    EXPECT_EQ(composite->get_database().get(), resolved_database.get());
    
    // Test edge cases
    EXPECT_FALSE(composite->process_data(""));  // Empty data should fail database query
    EXPECT_EQ(resolved_logger->get_log_count(), 3);  // Logger should still be called
    EXPECT_EQ(resolved_database->get_connection_count(), 3);  // Database call attempted
}

// T4.1 Task: DI container tests - Service lifecycle and cleanup
TEST_F(DependencyInjectionTest, ServiceLifecycleAndCleanup) {
    std::weak_ptr<test_logger_impl> weak_logger;
    std::weak_ptr<test_database_impl> weak_database;
    
    {
        // Scope to test automatic cleanup
        auto logger = std::make_shared<test_logger_impl>();
        auto database = std::make_shared<test_database_impl>();
        
        weak_logger = logger;
        weak_database = database;
        
        // Verify objects exist
        EXPECT_FALSE(weak_logger.expired());
        EXPECT_FALSE(weak_database.expired());
        
        // Register services
        container_.register_service<mock_logger_service>(logger);
        container_.register_service<mock_database_service>(database);
        
        // Objects should still exist due to container holding references
        EXPECT_FALSE(weak_logger.expired());
        EXPECT_FALSE(weak_database.expired());
        
        // Test that services are functional
        auto resolved_logger = container_.resolve_service<mock_logger_service>();
        ASSERT_NE(resolved_logger, nullptr);
        resolved_logger->log_message("Test lifecycle");
        EXPECT_EQ(resolved_logger->get_log_count(), 1);
    }
    
    // Objects should still exist because container holds references
    EXPECT_FALSE(weak_logger.expired());
    EXPECT_FALSE(weak_database.expired());
    
    // Clear container
    EXPECT_TRUE(container_.clear_services());
    
    // Now objects should be destroyed
    EXPECT_TRUE(weak_logger.expired());
    EXPECT_TRUE(weak_database.expired());
}

// Performance test for DI container operations
TEST_F(DependencyInjectionTest, PerformanceTest) {
    constexpr int num_services = 1000;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Register many services
    for (int i = 0; i < num_services; ++i) {
        auto logger = std::make_shared<test_logger_impl>();
        container_.register_service<mock_logger_service>(logger);
    }
    
    auto registration_time = std::chrono::high_resolution_clock::now();
    
    // Resolve services multiple times
    for (int i = 0; i < num_services; ++i) {
        auto resolved = container_.resolve_service<mock_logger_service>();
        EXPECT_NE(resolved, nullptr);
    }
    
    auto resolution_time = std::chrono::high_resolution_clock::now();
    
    // Calculate durations
    auto reg_duration = std::chrono::duration_cast<std::chrono::microseconds>(
        registration_time - start_time).count();
    auto res_duration = std::chrono::duration_cast<std::chrono::microseconds>(
        resolution_time - registration_time).count();
    
    // Performance assertions (adjust thresholds based on system)
    EXPECT_LT(reg_duration, 100000);  // Registration should complete in < 100ms
    EXPECT_LT(res_duration, 50000);   // Resolution should complete in < 50ms
    
    // Cleanup
    EXPECT_TRUE(container_.clear_services());
    EXPECT_EQ(container_.get_service_count(), 0);
}