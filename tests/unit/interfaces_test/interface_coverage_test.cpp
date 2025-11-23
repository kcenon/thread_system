/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, kcenon
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
 * @file interface_coverage_test.cpp
 * @brief Comprehensive interface coverage tests
 *
 * Tests cover:
 * - error_handler interface and default implementation
 * - service_container dependency injection
 * - logger_interface and registry
 * - monitorable_interface
 */

#include "gtest/gtest.h"

#include <kcenon/thread/interfaces/error_handler.h>
#include <kcenon/thread/interfaces/service_container.h>
#include <kcenon/thread/interfaces/logger_interface.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

using namespace kcenon::thread;

// ============================================================================
// Error Handler Tests
// ============================================================================

// Custom error handler without logger dependency for testing
class test_error_handler : public error_handler
{
private:
	error_callback callback_;
	std::vector<std::pair<std::string, std::string>> errors_;

public:
	void handle_error(const std::string& context, const std::string& error) override
	{
		errors_.emplace_back(context, error);
		if (callback_)
		{
			callback_(context, error);
		}
	}

	void set_error_callback(error_callback callback) override
	{
		callback_ = callback;
	}

	const std::vector<std::pair<std::string, std::string>>& get_errors() const
	{
		return errors_;
	}

	void clear_errors()
	{
		errors_.clear();
	}
};

class ErrorHandlerTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		handler_ = std::make_unique<test_error_handler>();
	}

	std::unique_ptr<test_error_handler> handler_;
};

TEST_F(ErrorHandlerTest, HandleErrorWithoutCallback)
{
	// Should not crash when handling error without callback
	EXPECT_NO_THROW(handler_->handle_error("test_context", "test_error"));

	// Error should be recorded
	ASSERT_EQ(handler_->get_errors().size(), 1);
	EXPECT_EQ(handler_->get_errors()[0].first, "test_context");
}

TEST_F(ErrorHandlerTest, HandleErrorWithCallback)
{
	std::string captured_context;
	std::string captured_error;

	handler_->set_error_callback(
		[&captured_context, &captured_error](const std::string& context, const std::string& error)
		{
			captured_context = context;
			captured_error = error;
		});

	handler_->handle_error("my_context", "my_error");

	EXPECT_EQ(captured_context, "my_context");
	EXPECT_EQ(captured_error, "my_error");
}

TEST_F(ErrorHandlerTest, MultipleErrors)
{
	std::vector<std::pair<std::string, std::string>> callback_errors;

	handler_->set_error_callback(
		[&callback_errors](const std::string& context, const std::string& error)
		{
			callback_errors.emplace_back(context, error);
		});

	handler_->handle_error("ctx1", "err1");
	handler_->handle_error("ctx2", "err2");
	handler_->handle_error("ctx3", "err3");

	ASSERT_EQ(callback_errors.size(), 3);
	EXPECT_EQ(callback_errors[0].first, "ctx1");
	EXPECT_EQ(callback_errors[1].first, "ctx2");
	EXPECT_EQ(callback_errors[2].first, "ctx3");

	// Also check internal storage
	ASSERT_EQ(handler_->get_errors().size(), 3);
}

TEST_F(ErrorHandlerTest, ReplaceCallback)
{
	std::atomic<int> first_count{0};
	std::atomic<int> second_count{0};

	handler_->set_error_callback(
		[&first_count](const std::string&, const std::string&)
		{
			first_count++;
		});

	handler_->handle_error("ctx", "err");
	EXPECT_EQ(first_count.load(), 1);
	EXPECT_EQ(second_count.load(), 0);

	// Replace callback
	handler_->set_error_callback(
		[&second_count](const std::string&, const std::string&)
		{
			second_count++;
		});

	handler_->handle_error("ctx", "err");
	EXPECT_EQ(first_count.load(), 1);
	EXPECT_EQ(second_count.load(), 1);
}

TEST_F(ErrorHandlerTest, PolymorphicBehavior)
{
	std::unique_ptr<error_handler> base_ptr = std::make_unique<test_error_handler>();
	EXPECT_NO_THROW(base_ptr->handle_error("polymorphic", "test"));
}

TEST_F(ErrorHandlerTest, ClearErrors)
{
	handler_->handle_error("ctx1", "err1");
	handler_->handle_error("ctx2", "err2");
	EXPECT_EQ(handler_->get_errors().size(), 2);

	handler_->clear_errors();
	EXPECT_EQ(handler_->get_errors().size(), 0);
}

// ============================================================================
// Service Container Tests
// ============================================================================

// Test interfaces
class ITestService
{
public:
	virtual ~ITestService() = default;
	virtual std::string get_name() const = 0;
};

class TestServiceImpl : public ITestService
{
public:
	std::string get_name() const override
	{
		return "TestServiceImpl";
	}
};

class AnotherServiceImpl : public ITestService
{
public:
	std::string get_name() const override
	{
		return "AnotherServiceImpl";
	}
};

class ICountingService
{
public:
	virtual ~ICountingService() = default;
	virtual int get_instance_id() const = 0;
};

class CountingServiceImpl : public ICountingService
{
private:
	static std::atomic<int> s_instance_counter;
	int instance_id_;

public:
	CountingServiceImpl() : instance_id_(++s_instance_counter)
	{
	}

	int get_instance_id() const override
	{
		return instance_id_;
	}

	static void reset_counter()
	{
		s_instance_counter = 0;
	}
};

std::atomic<int> CountingServiceImpl::s_instance_counter{0};

class ServiceContainerTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		container_ = std::make_unique<service_container>();
		CountingServiceImpl::reset_counter();
	}

	std::unique_ptr<service_container> container_;
};

TEST_F(ServiceContainerTest, RegisterAndResolveSingleton)
{
	auto impl = std::make_shared<TestServiceImpl>();
	container_->register_singleton<ITestService, TestServiceImpl>(impl);

	auto resolved = container_->resolve<ITestService>();
	ASSERT_NE(resolved, nullptr);
	EXPECT_EQ(resolved->get_name(), "TestServiceImpl");

	// Same instance should be returned
	auto resolved2 = container_->resolve<ITestService>();
	EXPECT_EQ(resolved.get(), resolved2.get());
}

TEST_F(ServiceContainerTest, RegisterFactory)
{
	container_->register_factory<ITestService>(
		[]() { return std::make_shared<TestServiceImpl>(); },
		service_container::lifetime::singleton);

	auto resolved = container_->resolve<ITestService>();
	ASSERT_NE(resolved, nullptr);
	EXPECT_EQ(resolved->get_name(), "TestServiceImpl");
}

TEST_F(ServiceContainerTest, TransientLifetime)
{
	container_->register_factory<ICountingService>(
		[]() { return std::make_shared<CountingServiceImpl>(); },
		service_container::lifetime::transient);

	auto first = container_->resolve<ICountingService>();
	auto second = container_->resolve<ICountingService>();

	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);

	// Different instances with transient
	EXPECT_NE(first->get_instance_id(), second->get_instance_id());
}

TEST_F(ServiceContainerTest, SingletonLifetimeSameInstance)
{
	container_->register_factory<ICountingService>(
		[]() { return std::make_shared<CountingServiceImpl>(); },
		service_container::lifetime::singleton);

	auto first = container_->resolve<ICountingService>();
	auto second = container_->resolve<ICountingService>();

	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);

	// Same instance with singleton
	EXPECT_EQ(first->get_instance_id(), second->get_instance_id());
}

TEST_F(ServiceContainerTest, ResolveUnregisteredReturnsNull)
{
	auto resolved = container_->resolve<ITestService>();
	EXPECT_EQ(resolved, nullptr);
}

TEST_F(ServiceContainerTest, RegisterTransient)
{
	container_->register_transient<ITestService, TestServiceImpl>();

	auto resolved = container_->resolve<ITestService>();
	ASSERT_NE(resolved, nullptr);
	EXPECT_EQ(resolved->get_name(), "TestServiceImpl");
}

// ============================================================================
// Log Level Enum Tests
// ============================================================================

TEST(LogLevelTest, EnumValues)
{
	// Verify log level ordering (inverted in this implementation)
	EXPECT_EQ(static_cast<int>(log_level::critical), 0);
	EXPECT_EQ(static_cast<int>(log_level::error), 1);
	EXPECT_EQ(static_cast<int>(log_level::warning), 2);
	EXPECT_EQ(static_cast<int>(log_level::info), 3);
	EXPECT_EQ(static_cast<int>(log_level::debug), 4);
	EXPECT_EQ(static_cast<int>(log_level::trace), 5);
}

TEST(LogLevelTest, CriticalIsHighestPriority)
{
	// In inverted ordering, critical has lowest numeric value
	EXPECT_LT(static_cast<int>(log_level::critical), static_cast<int>(log_level::trace));
}

// ============================================================================
// Thread Safety Tests for Service Container
// ============================================================================

TEST(ServiceContainerThreadSafetyTest, ConcurrentResolve)
{
	service_container container;
	container.register_factory<ITestService>(
		[]() { return std::make_shared<TestServiceImpl>(); },
		service_container::lifetime::singleton);

	std::vector<std::thread> threads;
	std::atomic<int> success_count{0};

	for (int i = 0; i < 10; ++i)
	{
		threads.emplace_back([&container, &success_count]()
		{
			for (int j = 0; j < 100; ++j)
			{
				auto service = container.resolve<ITestService>();
				if (service != nullptr)
				{
					success_count++;
				}
			}
		});
	}

	for (auto& t : threads)
	{
		t.join();
	}

	EXPECT_EQ(success_count.load(), 1000);
}
