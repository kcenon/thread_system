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
// error class tests
// =============================================================================

TEST_F(ErrorHandlingTest, ErrorConstruction) {
    error err(error_code::queue_full, "Queue capacity exceeded");

    EXPECT_EQ(err.code(), error_code::queue_full);
    EXPECT_EQ(err.message(), "Queue capacity exceeded");
}

TEST_F(ErrorHandlingTest, ErrorToString) {
    error err1(error_code::queue_full);
    EXPECT_EQ(err1.to_string(), "Queue is full");

    error err2(error_code::queue_full, "Max capacity: 100");
    EXPECT_EQ(err2.to_string(), "Queue is full: Max capacity: 100");
}

// =============================================================================
// result<T> tests
// =============================================================================

TEST_F(ErrorHandlingTest, ResultWithValue) {
    result<int> res(42);

    EXPECT_TRUE(res.has_value());
    EXPECT_TRUE(static_cast<bool>(res));
    EXPECT_EQ(res.value(), 42);
}

TEST_F(ErrorHandlingTest, ResultWithError) {
    auto err = error{error_code::queue_empty};
    result<int> res{err};

    EXPECT_FALSE(res.has_value());
    EXPECT_FALSE(static_cast<bool>(res));
    EXPECT_EQ(res.get_error().code(), error_code::queue_empty);
}

TEST_F(ErrorHandlingTest, ResultValueOr) {
    result<int> success_res{42};
    auto err = error{error_code::queue_empty};
    result<int> error_res{err};

    EXPECT_EQ(success_res.value_or(0), 42);
    EXPECT_EQ(error_res.value_or(0), 0);
}

TEST_F(ErrorHandlingTest, ResultValueOrThrow) {
    result<int> success_res{42};
    auto err = error{error_code::queue_empty};
    result<int> error_res{err};

    EXPECT_EQ(success_res.value_or_throw(), 42);
    EXPECT_THROW(error_res.value_or_throw(), std::runtime_error);
}

TEST_F(ErrorHandlingTest, ResultMap) {
    result<int> res(10);
    auto mapped = res.map([](int v) { return v * 2; });

    EXPECT_TRUE(mapped.has_value());
    EXPECT_EQ(mapped.value(), 20);
}

TEST_F(ErrorHandlingTest, ResultMapError) {
    auto err = error{error_code::queue_empty};
    result<int> res{err};
    auto mapped = res.map([](int v) { return v * 2; });

    EXPECT_FALSE(mapped.has_value());
    EXPECT_EQ(mapped.get_error().code(), error_code::queue_empty);
}

// =============================================================================
// result<void> tests
// =============================================================================

TEST_F(ErrorHandlingTest, VoidResultSuccess) {
    result<void> res;

    EXPECT_TRUE(res.has_value());
    EXPECT_TRUE(static_cast<bool>(res));
}

TEST_F(ErrorHandlingTest, VoidResultError) {
    auto err = error{error_code::mutex_error};
    result<void> res{err};

    EXPECT_FALSE(res.has_value());
    EXPECT_EQ(res.get_error().code(), error_code::mutex_error);
}

TEST_F(ErrorHandlingTest, VoidResultValueOrThrow) {
    result<void> success_res;
    auto err = error{error_code::mutex_error};
    result<void> error_res{err};

    EXPECT_NO_THROW(success_res.value_or_throw());
    EXPECT_THROW(error_res.value_or_throw(), std::runtime_error);
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
// result_void tests
// =============================================================================

TEST_F(ErrorHandlingTest, ResultVoidSuccess) {
    result_void res;

    EXPECT_FALSE(res.has_error());
    EXPECT_TRUE(static_cast<bool>(res));
}

TEST_F(ErrorHandlingTest, ResultVoidError) {
    auto err = error{error_code::io_error};
    result_void res{err};

    EXPECT_TRUE(res.has_error());
    EXPECT_FALSE(static_cast<bool>(res));
    EXPECT_EQ(res.get_error().code(), error_code::io_error);
}

// =============================================================================
// Helper function tests
// =============================================================================

TEST_F(ErrorHandlingTest, OptionalErrorToResult) {
    std::optional<std::string> no_error = std::nullopt;
    std::optional<std::string> has_error = "Something went wrong";

    auto success = optional_error_to_result(no_error, 42);
    auto failure = optional_error_to_result(has_error, 0);

    EXPECT_TRUE(success.has_value());
    EXPECT_EQ(success.value(), 42);

    EXPECT_FALSE(failure.has_value());
}

TEST_F(ErrorHandlingTest, ResultToOptionalError) {
    result<void> success;
    auto err = error{error_code::io_error};
    result<void> failure{err};

    auto opt1 = result_to_optional_error(success);
    auto opt2 = result_to_optional_error(failure);

    EXPECT_FALSE(opt1.has_value());
    EXPECT_TRUE(opt2.has_value());
}

} // namespace error_handling_test
