/*
 * BSD 3-Clause License
 * Copyright (c) 2025, kcenon
 */

#include <gtest/gtest.h>

#if defined(BUILD_WITH_COMMON_SYSTEM) && defined(BUILD_WITH_LOGGER_SYSTEM)

#include <kcenon/thread/adapters/logger_system_adapter.h>
#include <kcenon/logger/writers/console_writer.h>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <atomic>

namespace kcenon::thread::adapters::test {

class LoggerSystemAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        logger_ = std::make_shared<::kcenon::logger::logger>(false, 4096);
        adapter_ = std::make_shared<logger_system_adapter>(logger_);
    }

    void TearDown() override {
        if (logger_->is_running()) {
            logger_->stop();
        }
        adapter_.reset();
        logger_.reset();
    }

    std::shared_ptr<::kcenon::logger::logger> logger_;
    std::shared_ptr<logger_system_adapter> adapter_;
};

TEST_F(LoggerSystemAdapterTest, ConstructionWithValidLogger) {
    ASSERT_NE(adapter_, nullptr);
    EXPECT_NE(adapter_->get_logger(), nullptr);
}

TEST_F(LoggerSystemAdapterTest, ConstructionWithNullLogger) {
    auto null_adapter = std::make_shared<logger_system_adapter>(nullptr);
    ASSERT_NE(null_adapter, nullptr);
    EXPECT_EQ(null_adapter->get_logger(), nullptr);
}

TEST_F(LoggerSystemAdapterTest, LogSimpleMessage) {
    auto result = adapter_->log(
        ::common::interfaces::log_level::info,
        "Test message");
    EXPECT_TRUE(result.has_value());
}

TEST_F(LoggerSystemAdapterTest, LogWithSourceLocation) {
    auto result = adapter_->log(
        ::common::interfaces::log_level::debug,
        "Test with location",
        "test_file.cpp",
        42,
        "test_function");
    EXPECT_TRUE(result.has_value());
}

TEST_F(LoggerSystemAdapterTest, LogWithCpp20SourceLocation) {
    auto result = adapter_->log(
        ::common::interfaces::log_level::warning,
        std::string_view("Test with C++20 source_location"));
    EXPECT_TRUE(result.has_value());
}

TEST_F(LoggerSystemAdapterTest, LogStructuredEntry) {
    ::common::interfaces::log_entry entry;
    entry.level = ::common::interfaces::log_level::error;
    entry.message = "Structured entry test";
    entry.file = "structured_test.cpp";
    entry.line = 100;
    entry.function = "structured_function";

    auto result = adapter_->log(entry);
    EXPECT_TRUE(result.has_value());
}

TEST_F(LoggerSystemAdapterTest, LogWithNullLoggerReturnsError) {
    auto null_adapter = std::make_shared<logger_system_adapter>(nullptr);

    auto result = null_adapter->log(
        ::common::interfaces::log_level::info,
        "Should fail");
    EXPECT_FALSE(result.has_value());
}

TEST_F(LoggerSystemAdapterTest, IsEnabledForAllLevels) {
    adapter_->set_level(::common::interfaces::log_level::trace);

    EXPECT_TRUE(adapter_->is_enabled(::common::interfaces::log_level::trace));
    EXPECT_TRUE(adapter_->is_enabled(::common::interfaces::log_level::debug));
    EXPECT_TRUE(adapter_->is_enabled(::common::interfaces::log_level::info));
    EXPECT_TRUE(adapter_->is_enabled(::common::interfaces::log_level::warning));
    EXPECT_TRUE(adapter_->is_enabled(::common::interfaces::log_level::error));
    EXPECT_TRUE(adapter_->is_enabled(::common::interfaces::log_level::critical));
}

TEST_F(LoggerSystemAdapterTest, IsEnabledFiltersByLevel) {
    adapter_->set_level(::common::interfaces::log_level::warning);

    EXPECT_FALSE(adapter_->is_enabled(::common::interfaces::log_level::trace));
    EXPECT_FALSE(adapter_->is_enabled(::common::interfaces::log_level::debug));
    EXPECT_FALSE(adapter_->is_enabled(::common::interfaces::log_level::info));
    EXPECT_TRUE(adapter_->is_enabled(::common::interfaces::log_level::warning));
    EXPECT_TRUE(adapter_->is_enabled(::common::interfaces::log_level::error));
    EXPECT_TRUE(adapter_->is_enabled(::common::interfaces::log_level::critical));
}

TEST_F(LoggerSystemAdapterTest, IsEnabledWithNullLoggerReturnsFalse) {
    auto null_adapter = std::make_shared<logger_system_adapter>(nullptr);
    EXPECT_FALSE(null_adapter->is_enabled(::common::interfaces::log_level::info));
}

TEST_F(LoggerSystemAdapterTest, SetAndGetLevel) {
    auto set_result = adapter_->set_level(::common::interfaces::log_level::debug);
    EXPECT_TRUE(set_result.has_value());

    auto level = adapter_->get_level();
    EXPECT_EQ(level, ::common::interfaces::log_level::debug);
}

TEST_F(LoggerSystemAdapterTest, SetLevelWithNullLoggerReturnsError) {
    auto null_adapter = std::make_shared<logger_system_adapter>(nullptr);
    auto result = null_adapter->set_level(::common::interfaces::log_level::info);
    EXPECT_FALSE(result.has_value());
}

TEST_F(LoggerSystemAdapterTest, FlushSucceeds) {
    adapter_->log(::common::interfaces::log_level::info, "Message before flush");
    auto result = adapter_->flush();
    EXPECT_TRUE(result.has_value());
}

TEST_F(LoggerSystemAdapterTest, FlushWithNullLoggerReturnsError) {
    auto null_adapter = std::make_shared<logger_system_adapter>(nullptr);
    auto result = null_adapter->flush();
    EXPECT_FALSE(result.has_value());
}

TEST_F(LoggerSystemAdapterTest, GetLoggerReturnsUnderlyingLogger) {
    auto underlying = adapter_->get_logger();
    EXPECT_EQ(underlying.get(), logger_.get());
}

TEST_F(LoggerSystemAdapterTest, UnwrapReturnsUnderlyingLogger) {
    auto underlying = adapter_->unwrap();
    EXPECT_EQ(underlying.get(), logger_.get());
}

TEST_F(LoggerSystemAdapterTest, LogLevelConversionAllLevels) {
    const std::vector<::common::interfaces::log_level> levels = {
        ::common::interfaces::log_level::trace,
        ::common::interfaces::log_level::debug,
        ::common::interfaces::log_level::info,
        ::common::interfaces::log_level::warning,
        ::common::interfaces::log_level::error,
        ::common::interfaces::log_level::critical
    };

    adapter_->set_level(::common::interfaces::log_level::trace);

    for (auto level : levels) {
        auto result = adapter_->log(level, "Level test message");
        EXPECT_TRUE(result.has_value()) << "Failed for level: " << static_cast<int>(level);
    }
}

TEST_F(LoggerSystemAdapterTest, ThreadSafetyMultipleWriters) {
    constexpr int num_threads = 4;
    constexpr int messages_per_thread = 100;
    std::atomic<int> success_count{0};
    std::vector<std::thread> threads;

    adapter_->set_level(::common::interfaces::log_level::trace);

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, &success_count]() {
            for (int j = 0; j < messages_per_thread; ++j) {
                auto result = adapter_->log(
                    ::common::interfaces::log_level::info,
                    "Thread " + std::to_string(i) + " message " + std::to_string(j));
                if (result.has_value()) {
                    ++success_count;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count.load(), num_threads * messages_per_thread);
}

TEST_F(LoggerSystemAdapterTest, AdapterDepthTracking) {
    EXPECT_EQ(adapter_->get_wrapper_depth(), 0);
    EXPECT_FALSE(adapter_->is_wrapped_adapter());
}

TEST_F(LoggerSystemAdapterTest, MoveConstruction) {
    auto moved_adapter = std::move(adapter_);
    ASSERT_NE(moved_adapter, nullptr);
    EXPECT_NE(moved_adapter->get_logger(), nullptr);
}

} // namespace kcenon::thread::adapters::test

#else

TEST(LoggerSystemAdapterSkipped, BuildFlagsNotSet) {
    GTEST_SKIP() << "LoggerSystemAdapter tests require BUILD_WITH_COMMON_SYSTEM and BUILD_WITH_LOGGER_SYSTEM";
}

#endif // BUILD_WITH_COMMON_SYSTEM && BUILD_WITH_LOGGER_SYSTEM
