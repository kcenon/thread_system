/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, kcenon
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

/**
 * @file service_registry_test.cpp
 * @brief Comprehensive unit tests for service_registry
 * @since 4.0.0
 */

#include <gtest/gtest.h>

#include <kcenon/thread/core/service_registry.h>

#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <atomic>

using namespace kcenon::thread;

// =============================================================================
// Test service types
// =============================================================================

struct ILogger {
    virtual ~ILogger() = default;
    virtual std::string name() const = 0;
};

struct ConsoleLogger : ILogger {
    std::string name() const override { return "console"; }
};

struct FileLogger : ILogger {
    std::string name() const override { return "file"; }
};

struct IDatabase {
    virtual ~IDatabase() = default;
    virtual int version() const = 0;
};

struct MockDatabase : IDatabase {
    int version() const override { return 42; }
};

// =============================================================================
// Test fixture (clears registry between tests)
// =============================================================================

class ServiceRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        service_registry::clear_services();
    }

    void TearDown() override {
        service_registry::clear_services();
    }
};

// =============================================================================
// Registration and retrieval
// =============================================================================

TEST_F(ServiceRegistryTest, RegisterAndRetrieve) {
    auto logger = std::make_shared<ConsoleLogger>();
    service_registry::register_service<ILogger>(logger);

    auto retrieved = service_registry::get_service<ILogger>();
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->name(), "console");
}

TEST_F(ServiceRegistryTest, RetrieveUnregisteredReturnsNull) {
    auto result = service_registry::get_service<ILogger>();
    EXPECT_EQ(result, nullptr);
}

TEST_F(ServiceRegistryTest, RegisterMultipleTypes) {
    service_registry::register_service<ILogger>(std::make_shared<ConsoleLogger>());
    service_registry::register_service<IDatabase>(std::make_shared<MockDatabase>());

    auto logger = service_registry::get_service<ILogger>();
    auto db = service_registry::get_service<IDatabase>();

    ASSERT_NE(logger, nullptr);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(logger->name(), "console");
    EXPECT_EQ(db->version(), 42);
}

TEST_F(ServiceRegistryTest, ReplaceService) {
    service_registry::register_service<ILogger>(std::make_shared<ConsoleLogger>());
    EXPECT_EQ(service_registry::get_service<ILogger>()->name(), "console");

    service_registry::register_service<ILogger>(std::make_shared<FileLogger>());
    EXPECT_EQ(service_registry::get_service<ILogger>()->name(), "file");
}

// =============================================================================
// Service count
// =============================================================================

TEST_F(ServiceRegistryTest, InitialCountIsZero) {
    EXPECT_EQ(service_registry::get_service_count(), 0u);
}

TEST_F(ServiceRegistryTest, CountIncreasesOnRegistration) {
    service_registry::register_service<ILogger>(std::make_shared<ConsoleLogger>());
    EXPECT_EQ(service_registry::get_service_count(), 1u);

    service_registry::register_service<IDatabase>(std::make_shared<MockDatabase>());
    EXPECT_EQ(service_registry::get_service_count(), 2u);
}

TEST_F(ServiceRegistryTest, ReplaceDoesNotIncreaseCount) {
    service_registry::register_service<ILogger>(std::make_shared<ConsoleLogger>());
    EXPECT_EQ(service_registry::get_service_count(), 1u);

    service_registry::register_service<ILogger>(std::make_shared<FileLogger>());
    EXPECT_EQ(service_registry::get_service_count(), 1u);
}

// =============================================================================
// Clear services
// =============================================================================

TEST_F(ServiceRegistryTest, ClearRemovesAll) {
    service_registry::register_service<ILogger>(std::make_shared<ConsoleLogger>());
    service_registry::register_service<IDatabase>(std::make_shared<MockDatabase>());
    EXPECT_EQ(service_registry::get_service_count(), 2u);

    service_registry::clear_services();
    EXPECT_EQ(service_registry::get_service_count(), 0u);
    EXPECT_EQ(service_registry::get_service<ILogger>(), nullptr);
    EXPECT_EQ(service_registry::get_service<IDatabase>(), nullptr);
}

TEST_F(ServiceRegistryTest, ClearThenReregister) {
    service_registry::register_service<ILogger>(std::make_shared<ConsoleLogger>());
    service_registry::clear_services();

    service_registry::register_service<ILogger>(std::make_shared<FileLogger>());
    auto logger = service_registry::get_service<ILogger>();
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->name(), "file");
}

// =============================================================================
// Type isolation
// =============================================================================

TEST_F(ServiceRegistryTest, DifferentTypesAreIsolated) {
    service_registry::register_service<ILogger>(std::make_shared<ConsoleLogger>());

    // IDatabase should not be found even though ILogger is registered
    EXPECT_EQ(service_registry::get_service<IDatabase>(), nullptr);
    EXPECT_EQ(service_registry::get_service_count(), 1u);
}

// =============================================================================
// Thread safety
// =============================================================================

TEST_F(ServiceRegistryTest, ConcurrentRegistrationAndRetrieval) {
    std::atomic<int> found_count{0};
    std::atomic<int> errors{0};

    std::vector<std::thread> threads;

    // Writers
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 50; ++j) {
                service_registry::register_service<ILogger>(
                    std::make_shared<ConsoleLogger>());
            }
        });
    }

    // Readers
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 100; ++j) {
                auto logger = service_registry::get_service<ILogger>();
                if (logger) {
                    found_count++;
                    if (logger->name() != "console") {
                        errors++;
                    }
                }
            }
        });
    }

    for (auto& th : threads) th.join();

    EXPECT_EQ(errors.load(), 0);
    EXPECT_GT(found_count.load(), 0);
}

TEST_F(ServiceRegistryTest, ConcurrentCountReads) {
    service_registry::register_service<ILogger>(std::make_shared<ConsoleLogger>());
    service_registry::register_service<IDatabase>(std::make_shared<MockDatabase>());

    std::atomic<int> errors{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 100; ++j) {
                auto count = service_registry::get_service_count();
                if (count != 2u) {
                    errors++;
                }
            }
        });
    }

    for (auto& th : threads) th.join();
    EXPECT_EQ(errors.load(), 0);
}

// =============================================================================
// Shared pointer semantics
// =============================================================================

TEST_F(ServiceRegistryTest, RetrievedServiceSharesOwnership) {
    auto logger = std::make_shared<ConsoleLogger>();
    EXPECT_EQ(logger.use_count(), 1);

    service_registry::register_service<ILogger>(logger);
    EXPECT_GE(logger.use_count(), 2);  // held by both local and registry

    auto retrieved = service_registry::get_service<ILogger>();
    EXPECT_GE(logger.use_count(), 3);
    EXPECT_EQ(logger.get(), retrieved.get());
}
