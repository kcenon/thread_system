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
 * @file ilogger_di_integration_test.cpp
 * @brief Integration tests for ILogger registration with ServiceContainer
 *
 * These tests verify the ILogger DI registration functions work correctly,
 * providing the preferred way to integrate logging without direct
 * logger_system dependency.
 *
 * @see Issue #336 for context on deprecating logger_system_adapter
 */

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

#ifdef BUILD_WITH_COMMON_SYSTEM

#include <kcenon/common/di/service_container.h>
#include <kcenon/common/interfaces/logger_interface.h>
#include <kcenon/common/patterns/result.h>

// Provide global namespace alias for common_executor_adapter.h compatibility
namespace common = kcenon::common;

#include <kcenon/thread/di/service_registration.h>

namespace {

/**
 * @brief Simple in-memory logger for testing
 *
 * This logger stores log messages in memory for verification.
 */
class TestLogger : public kcenon::common::interfaces::ILogger {
public:
    struct LogMessage {
        kcenon::common::interfaces::log_level level;
        std::string message;
        std::string file;
        int line;
        std::string function;
    };

    kcenon::common::VoidResult log(
        kcenon::common::interfaces::log_level level,
        const std::string& message) override {
        std::lock_guard<std::mutex> lock(mutex_);
        messages_.push_back({level, message, "", 0, ""});
        return kcenon::common::ok();
    }

    kcenon::common::VoidResult log(
        kcenon::common::interfaces::log_level level,
        std::string_view message,
        const kcenon::common::source_location& loc = kcenon::common::source_location::current()) override {
        std::lock_guard<std::mutex> lock(mutex_);
        messages_.push_back({
            level,
            std::string(message),
            std::string(loc.file_name()),
            static_cast<int>(loc.line()),
            std::string(loc.function_name())
        });
        return kcenon::common::ok();
    }

    kcenon::common::VoidResult log(
        const kcenon::common::interfaces::log_entry& entry) override {
        std::lock_guard<std::mutex> lock(mutex_);
        messages_.push_back({
            entry.level,
            entry.message,
            entry.file,
            entry.line,
            entry.function
        });
        return kcenon::common::ok();
    }

    bool is_enabled(kcenon::common::interfaces::log_level level) const override {
        return level >= min_level_;
    }

    kcenon::common::VoidResult set_level(kcenon::common::interfaces::log_level level) override {
        min_level_ = level;
        return kcenon::common::ok();
    }

    kcenon::common::interfaces::log_level get_level() const override {
        return min_level_;
    }

    kcenon::common::VoidResult flush() override {
        return kcenon::common::ok();
    }

    // Test helper methods
    std::vector<LogMessage> get_messages() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return messages_;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        messages_.clear();
    }

    size_t message_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return messages_.size();
    }

private:
    mutable std::mutex mutex_;
    std::vector<LogMessage> messages_;
    kcenon::common::interfaces::log_level min_level_ = kcenon::common::interfaces::log_level::info;
};

} // anonymous namespace

/**
 * @brief Test fixture for ILogger DI integration tests
 */
class ILoggerDIIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        container_ = std::make_unique<kcenon::common::di::service_container>();
    }

    void TearDown() override {
        container_.reset();
    }

    std::unique_ptr<kcenon::common::di::service_container> container_;
};

TEST_F(ILoggerDIIntegrationTest, RegisterLoggerInstance) {
    auto logger = std::make_shared<TestLogger>();

    auto result = kcenon::thread::di::register_logger_instance(*container_, logger);
    ASSERT_TRUE(result.is_ok()) << "Failed to register logger: " << result.error().message;

    EXPECT_TRUE(kcenon::thread::di::is_logger_registered(*container_));
}

TEST_F(ILoggerDIIntegrationTest, ResolveRegisteredLogger) {
    auto logger = std::make_shared<TestLogger>();

    auto result = kcenon::thread::di::register_logger_instance(*container_, logger);
    ASSERT_TRUE(result.is_ok());

    auto resolved = container_->resolve<kcenon::common::interfaces::ILogger>();
    ASSERT_TRUE(resolved.is_ok()) << "Failed to resolve logger";
    EXPECT_EQ(resolved.value().get(), logger.get());
}

TEST_F(ILoggerDIIntegrationTest, LogThroughResolvedLogger) {
    auto logger = std::make_shared<TestLogger>();

    auto result = kcenon::thread::di::register_logger_instance(*container_, logger);
    ASSERT_TRUE(result.is_ok());

    auto resolved = container_->resolve<kcenon::common::interfaces::ILogger>();
    ASSERT_TRUE(resolved.is_ok());

    resolved.value()->log(kcenon::common::interfaces::log_level::info, "Test message");

    auto messages = logger->get_messages();
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].level, kcenon::common::interfaces::log_level::info);
    EXPECT_EQ(messages[0].message, "Test message");
}

TEST_F(ILoggerDIIntegrationTest, RegisterNullLoggerFails) {
    auto result = kcenon::thread::di::register_logger_instance(*container_, nullptr);
    EXPECT_TRUE(result.is_err());
    EXPECT_FALSE(kcenon::thread::di::is_logger_registered(*container_));
}

TEST_F(ILoggerDIIntegrationTest, RegisterLoggerFactory) {
    int factory_call_count = 0;

    auto result = kcenon::thread::di::register_logger_factory(
        *container_,
        [&factory_call_count]() {
            ++factory_call_count;
            return std::make_shared<TestLogger>();
        },
        kcenon::common::di::service_lifetime::singleton
    );
    ASSERT_TRUE(result.is_ok());

    EXPECT_TRUE(kcenon::thread::di::is_logger_registered(*container_));
    EXPECT_EQ(factory_call_count, 0); // Factory not called yet

    auto resolved = container_->resolve<kcenon::common::interfaces::ILogger>();
    ASSERT_TRUE(resolved.is_ok());
    EXPECT_EQ(factory_call_count, 1); // Factory called on first resolve
}

TEST_F(ILoggerDIIntegrationTest, SingletonLoggerFactory) {
    auto result = kcenon::thread::di::register_logger_factory(
        *container_,
        []() { return std::make_shared<TestLogger>(); },
        kcenon::common::di::service_lifetime::singleton
    );
    ASSERT_TRUE(result.is_ok());

    auto first = container_->resolve<kcenon::common::interfaces::ILogger>();
    auto second = container_->resolve<kcenon::common::interfaces::ILogger>();

    ASSERT_TRUE(first.is_ok());
    ASSERT_TRUE(second.is_ok());
    EXPECT_EQ(first.value().get(), second.value().get());
}

TEST_F(ILoggerDIIntegrationTest, TransientLoggerFactory) {
    auto result = kcenon::thread::di::register_logger_factory(
        *container_,
        []() { return std::make_shared<TestLogger>(); },
        kcenon::common::di::service_lifetime::transient
    );
    ASSERT_TRUE(result.is_ok());

    auto first = container_->resolve<kcenon::common::interfaces::ILogger>();
    auto second = container_->resolve<kcenon::common::interfaces::ILogger>();

    ASSERT_TRUE(first.is_ok());
    ASSERT_TRUE(second.is_ok());
    EXPECT_NE(first.value().get(), second.value().get());
}

TEST_F(ILoggerDIIntegrationTest, UnregisterLogger) {
    auto logger = std::make_shared<TestLogger>();

    auto result = kcenon::thread::di::register_logger_instance(*container_, logger);
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(kcenon::thread::di::is_logger_registered(*container_));

    result = kcenon::thread::di::unregister_logger(*container_);
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(kcenon::thread::di::is_logger_registered(*container_));
}

TEST_F(ILoggerDIIntegrationTest, LogWithSourceLocation) {
    auto logger = std::make_shared<TestLogger>();

    auto result = kcenon::thread::di::register_logger_instance(*container_, logger);
    ASSERT_TRUE(result.is_ok());

    auto resolved = container_->resolve<kcenon::common::interfaces::ILogger>();
    ASSERT_TRUE(resolved.is_ok());

    // Log with source location
    resolved.value()->log(
        kcenon::common::interfaces::log_level::warning,
        "Warning message"
    );

    auto messages = logger->get_messages();
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].level, kcenon::common::interfaces::log_level::warning);
    EXPECT_EQ(messages[0].message, "Warning message");
    // Source location should be captured
    EXPECT_FALSE(messages[0].file.empty());
}

TEST_F(ILoggerDIIntegrationTest, LogEntry) {
    auto logger = std::make_shared<TestLogger>();

    auto result = kcenon::thread::di::register_logger_instance(*container_, logger);
    ASSERT_TRUE(result.is_ok());

    auto resolved = container_->resolve<kcenon::common::interfaces::ILogger>();
    ASSERT_TRUE(resolved.is_ok());

    auto entry = kcenon::common::interfaces::log_entry::create(
        kcenon::common::interfaces::log_level::error,
        "Error message"
    );
    resolved.value()->log(entry);

    auto messages = logger->get_messages();
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].level, kcenon::common::interfaces::log_level::error);
    EXPECT_EQ(messages[0].message, "Error message");
}

TEST_F(ILoggerDIIntegrationTest, LogLevelFiltering) {
    auto logger = std::make_shared<TestLogger>();
    logger->set_level(kcenon::common::interfaces::log_level::warning);

    auto result = kcenon::thread::di::register_logger_instance(*container_, logger);
    ASSERT_TRUE(result.is_ok());

    auto resolved = container_->resolve<kcenon::common::interfaces::ILogger>();
    ASSERT_TRUE(resolved.is_ok());

    EXPECT_FALSE(resolved.value()->is_enabled(kcenon::common::interfaces::log_level::debug));
    EXPECT_FALSE(resolved.value()->is_enabled(kcenon::common::interfaces::log_level::info));
    EXPECT_TRUE(resolved.value()->is_enabled(kcenon::common::interfaces::log_level::warning));
    EXPECT_TRUE(resolved.value()->is_enabled(kcenon::common::interfaces::log_level::error));
}

#else // BUILD_WITH_COMMON_SYSTEM

TEST(ILoggerDIIntegrationTest, RequiresCommonSystem) {
    GTEST_SKIP() << "ILogger DI tests require BUILD_WITH_COMMON_SYSTEM";
}

#endif // BUILD_WITH_COMMON_SYSTEM
