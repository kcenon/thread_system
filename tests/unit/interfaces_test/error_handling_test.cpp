/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, kcenon
All rights reserved.
*****************************************************************************/

#include <gtest/gtest.h>
#include <kcenon/thread/core/error_handling.h>
#include <system_error>

namespace error_handling_test {

using namespace kcenon::thread;
namespace common = kcenon::common;

class ErrorHandlingTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// =============================================================================
// error_code enum tests
// =============================================================================

TEST_F(ErrorHandlingTest, ErrorCodeToStringConversion) {
    EXPECT_EQ(error_code_to_string(error_code::success), "Success");
    EXPECT_EQ(error_code_to_string(error_code::queue_full), "Queue is full");
    EXPECT_EQ(error_code_to_string(error_code::queue_empty), "Queue is empty");
    EXPECT_EQ(error_code_to_string(error_code::thread_already_running), "Thread is already running");
    EXPECT_EQ(error_code_to_string(error_code::invalid_argument), "Invalid argument");
}

TEST_F(ErrorHandlingTest, ErrorCodeUnknownValue) {
    // Test with an invalid/unknown error code value
    auto unknown = static_cast<error_code>(9999);
    EXPECT_EQ(error_code_to_string(unknown), "Unknown error code");
}

// =============================================================================
// to_error_info tests
// =============================================================================

TEST_F(ErrorHandlingTest, ToErrorInfoConstruction) {
    auto info = to_error_info(error_code::queue_full, "Queue capacity exceeded");

    EXPECT_EQ(get_error_code(info), error_code::queue_full);
    EXPECT_EQ(info.message, "Queue capacity exceeded");
    EXPECT_EQ(info.module, "thread_system");
}

TEST_F(ErrorHandlingTest, ToErrorInfoDefaultMessage) {
    auto info = to_error_info(error_code::queue_full);
    EXPECT_EQ(info.message, "Queue is full");

    auto info2 = to_error_info(error_code::queue_full, "Max capacity: 100");
    EXPECT_EQ(info2.message, "Max capacity: 100");
}

// =============================================================================
// common::Result<T> tests
// =============================================================================

TEST_F(ErrorHandlingTest, ResultWithValue) {
    common::Result<int> res = common::Result<int>::ok(42);

    EXPECT_TRUE(res.is_ok());
    EXPECT_FALSE(res.is_err());
    EXPECT_EQ(res.value(), 42);
}

TEST_F(ErrorHandlingTest, ResultWithError) {
    common::Result<int> res = make_error_result<int>(error_code::queue_empty);

    EXPECT_FALSE(res.is_ok());
    EXPECT_TRUE(res.is_err());
    EXPECT_EQ(get_error_code(res.error()), error_code::queue_empty);
}

TEST_F(ErrorHandlingTest, ResultValueOr) {
    common::Result<int> success_res = common::Result<int>::ok(42);
    common::Result<int> error_res = make_error_result<int>(error_code::queue_empty);

    EXPECT_EQ(success_res.value_or(0), 42);
    EXPECT_EQ(error_res.value_or(0), 0);
}

// =============================================================================
// common::VoidResult tests
// =============================================================================

TEST_F(ErrorHandlingTest, VoidResultSuccess) {
    common::VoidResult res = common::ok();

    EXPECT_TRUE(res.is_ok());
    EXPECT_FALSE(res.is_err());
}

TEST_F(ErrorHandlingTest, VoidResultError) {
    common::VoidResult res = make_error_result(error_code::mutex_error);

    EXPECT_TRUE(res.is_err());
    EXPECT_FALSE(res.is_ok());
    EXPECT_EQ(get_error_code(res.error()), error_code::mutex_error);
}

// =============================================================================
// std::error_code integration tests
// =============================================================================

TEST_F(ErrorHandlingTest, StdErrorCodeCreation) {
    std::error_code ec = make_error_code(error_code::queue_full);

    EXPECT_EQ(ec.value(), static_cast<int>(error_code::queue_full));
    EXPECT_EQ(ec.category().name(), std::string("thread_system"));
    EXPECT_EQ(ec.message(), "Queue is full");
}

TEST_F(ErrorHandlingTest, StdErrorCodeImplicitConversion) {
    // This tests that is_error_code_enum specialization works
    std::error_code ec = error_code::queue_full;

    EXPECT_EQ(ec.value(), static_cast<int>(error_code::queue_full));
    EXPECT_EQ(ec.category().name(), std::string("thread_system"));
}

TEST_F(ErrorHandlingTest, StdErrorCodeComparison) {
    std::error_code ec1 = error_code::queue_full;
    std::error_code ec2 = error_code::queue_full;
    std::error_code ec3 = error_code::queue_empty;

    EXPECT_EQ(ec1, ec2);
    EXPECT_NE(ec1, ec3);
}

TEST_F(ErrorHandlingTest, StdErrorCodeEquivalence) {
    std::error_code ec = error_code::invalid_argument;

    // Should be equivalent to std::errc::invalid_argument
    EXPECT_TRUE(ec == std::errc::invalid_argument);
}

TEST_F(ErrorHandlingTest, StdErrorCodeSuccessCheck) {
    std::error_code success = error_code::success;
    std::error_code failure = error_code::queue_full;

    // success error_code should evaluate to false (no error)
    EXPECT_FALSE(static_cast<bool>(success));
    EXPECT_TRUE(static_cast<bool>(failure));
}

TEST_F(ErrorHandlingTest, ThreadCategorySingleton) {
    const std::error_category& cat1 = thread_category();
    const std::error_category& cat2 = thread_category();

    // Should be the same instance
    EXPECT_EQ(&cat1, &cat2);
}

// =============================================================================
// GetErrorCode helper tests
// =============================================================================

TEST_F(ErrorHandlingTest, GetErrorCodeFromInfo) {
    common::error_info info{
        static_cast<int>(error_code::io_error),
        "I/O failure",
        "thread_system"
    };

    EXPECT_EQ(get_error_code(info), error_code::io_error);
}

TEST_F(ErrorHandlingTest, MakeErrorResultVoid) {
    common::VoidResult res = make_error_result(error_code::io_error, "Custom message");

    EXPECT_TRUE(res.is_err());
    EXPECT_EQ(get_error_code(res.error()), error_code::io_error);
    EXPECT_EQ(res.error().message, "Custom message");
}

TEST_F(ErrorHandlingTest, MakeErrorResultWithType) {
    common::Result<int> res = make_error_result<int>(error_code::invalid_argument, "Bad value");

    EXPECT_TRUE(res.is_err());
    EXPECT_EQ(get_error_code(res.error()), error_code::invalid_argument);
    EXPECT_EQ(res.error().message, "Bad value");
}

} // namespace error_handling_test
