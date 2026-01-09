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

#include <gtest/gtest.h>

#include <kcenon/thread/core/enhanced_cancellation_token.h>

#include <atomic>
#include <chrono>
#include <latch>
#include <thread>
#include <vector>

namespace kcenon::thread
{
namespace test
{

// Reduce iteration count for coverage builds
#ifdef ENABLE_COVERAGE
constexpr int TEST_ITERATIONS = 5;
#else
constexpr int TEST_ITERATIONS = 20;
#endif

class enhanced_cancellation_token_test : public ::testing::Test
{
protected:
	void SetUp() override {}
	void TearDown() override {}

	template <typename Predicate>
	auto wait_for(Predicate pred, std::chrono::milliseconds timeout
									  = std::chrono::milliseconds(1000)) -> bool
	{
		auto start = std::chrono::steady_clock::now();
		while (!pred())
		{
			if (std::chrono::steady_clock::now() - start > timeout)
			{
				return false;
			}
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}
		return true;
	}
};

// ============================================================================
// Basic functionality tests
// ============================================================================

TEST_F(enhanced_cancellation_token_test, CreateToken)
{
	auto token = enhanced_cancellation_token::create();
	EXPECT_FALSE(token.is_cancelled());
	EXPECT_FALSE(token.get_reason().has_value());
}

TEST_F(enhanced_cancellation_token_test, CancelToken)
{
	auto token = enhanced_cancellation_token::create();
	token.cancel();

	EXPECT_TRUE(token.is_cancelled());
	EXPECT_TRUE(token.is_cancellation_requested());

	auto reason = token.get_reason();
	ASSERT_TRUE(reason.has_value());
	EXPECT_EQ(reason->reason_type, cancellation_reason::type::user_requested);
}

TEST_F(enhanced_cancellation_token_test, CancelWithMessage)
{
	auto token = enhanced_cancellation_token::create();
	token.cancel("Custom reason message");

	EXPECT_TRUE(token.is_cancelled());

	auto reason = token.get_reason();
	ASSERT_TRUE(reason.has_value());
	EXPECT_EQ(reason->message, "Custom reason message");
}

TEST_F(enhanced_cancellation_token_test, CancelWithException)
{
	auto token = enhanced_cancellation_token::create();

	try
	{
		throw std::runtime_error("Test error");
	}
	catch (...)
	{
		token.cancel(std::current_exception());
	}

	EXPECT_TRUE(token.is_cancelled());

	auto reason = token.get_reason();
	ASSERT_TRUE(reason.has_value());
	EXPECT_EQ(reason->reason_type, cancellation_reason::type::error);
	EXPECT_TRUE(reason->exception.has_value());
}

TEST_F(enhanced_cancellation_token_test, ThrowIfCancelled)
{
	auto token = enhanced_cancellation_token::create();

	// Should not throw when not cancelled
	EXPECT_NO_THROW(token.throw_if_cancelled());

	token.cancel("Test cancellation");

	// Should throw when cancelled
	EXPECT_THROW(token.throw_if_cancelled(), operation_cancelled_exception);

	try
	{
		token.throw_if_cancelled();
	}
	catch (const operation_cancelled_exception& ex)
	{
		EXPECT_EQ(ex.reason().reason_type, cancellation_reason::type::user_requested);
	}
}

// ============================================================================
// Timeout tests
// ============================================================================

TEST_F(enhanced_cancellation_token_test, CreateWithTimeout)
{
	auto token = enhanced_cancellation_token::create_with_timeout(
		std::chrono::milliseconds{100});

	EXPECT_FALSE(token.is_cancelled());
	EXPECT_TRUE(token.has_timeout());

	// Wait for timeout
	EXPECT_TRUE(wait_for([&]() { return token.is_cancelled(); },
						 std::chrono::milliseconds{500}));

	auto reason = token.get_reason();
	ASSERT_TRUE(reason.has_value());
	EXPECT_EQ(reason->reason_type, cancellation_reason::type::timeout);
}

TEST_F(enhanced_cancellation_token_test, RemainingTime)
{
	auto token = enhanced_cancellation_token::create_with_timeout(
		std::chrono::milliseconds{500});

	auto remaining = token.remaining_time();
	EXPECT_GT(remaining.count(), 0);
	EXPECT_LE(remaining.count(), 500);

	std::this_thread::sleep_for(std::chrono::milliseconds{100});

	auto remaining2 = token.remaining_time();
	EXPECT_LT(remaining2.count(), remaining.count());
}

TEST_F(enhanced_cancellation_token_test, ExtendTimeout)
{
	auto token = enhanced_cancellation_token::create_with_timeout(
		std::chrono::milliseconds{100});

	token.extend_timeout(std::chrono::milliseconds{200});

	// Should not be cancelled yet
	std::this_thread::sleep_for(std::chrono::milliseconds{50});
	EXPECT_FALSE(token.is_cancelled());

	// Wait longer and check
	EXPECT_TRUE(wait_for([&]() { return token.is_cancelled(); },
						 std::chrono::milliseconds{500}));
}

// ============================================================================
// Deadline tests
// ============================================================================

TEST_F(enhanced_cancellation_token_test, CreateWithDeadline)
{
	auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds{100};
	auto token = enhanced_cancellation_token::create_with_deadline(deadline);

	EXPECT_FALSE(token.is_cancelled());
	EXPECT_TRUE(token.has_timeout());

	// Wait for deadline
	EXPECT_TRUE(wait_for([&]() { return token.is_cancelled(); },
						 std::chrono::milliseconds{500}));

	auto reason = token.get_reason();
	ASSERT_TRUE(reason.has_value());
	// Deadline cancellation also uses timeout reason type
	EXPECT_EQ(reason->reason_type, cancellation_reason::type::timeout);
}

// ============================================================================
// Linked token tests
// ============================================================================

TEST_F(enhanced_cancellation_token_test, LinkedTokenCancellation)
{
	auto parent1 = enhanced_cancellation_token::create();
	auto parent2 = enhanced_cancellation_token::create();
	auto linked = enhanced_cancellation_token::create_linked({parent1, parent2});

	EXPECT_FALSE(linked.is_cancelled());

	// Cancel first parent
	parent1.cancel();

	// Linked token should be cancelled
	EXPECT_TRUE(
		wait_for([&]() { return linked.is_cancelled(); },
				 std::chrono::milliseconds{100}));

	auto reason = linked.get_reason();
	ASSERT_TRUE(reason.has_value());
	EXPECT_EQ(reason->reason_type, cancellation_reason::type::parent_cancelled);
}

TEST_F(enhanced_cancellation_token_test, LinkedWithTimeout)
{
	auto parent = enhanced_cancellation_token::create();
	auto linked = enhanced_cancellation_token::create_linked_with_timeout(
		parent, std::chrono::milliseconds{100});

	EXPECT_FALSE(linked.is_cancelled());

	// Wait for timeout
	EXPECT_TRUE(wait_for([&]() { return linked.is_cancelled(); },
						 std::chrono::milliseconds{500}));

	auto reason = linked.get_reason();
	ASSERT_TRUE(reason.has_value());
	EXPECT_EQ(reason->reason_type, cancellation_reason::type::timeout);
}

TEST_F(enhanced_cancellation_token_test, LinkedWithTimeoutParentCancel)
{
	auto parent = enhanced_cancellation_token::create();
	auto linked = enhanced_cancellation_token::create_linked_with_timeout(
		parent, std::chrono::milliseconds{5000}); // Long timeout

	EXPECT_FALSE(linked.is_cancelled());

	// Cancel parent before timeout
	parent.cancel();

	// Linked token should be cancelled due to parent
	EXPECT_TRUE(wait_for([&]() { return linked.is_cancelled(); },
						 std::chrono::milliseconds{100}));

	auto reason = linked.get_reason();
	ASSERT_TRUE(reason.has_value());
	EXPECT_EQ(reason->reason_type, cancellation_reason::type::parent_cancelled);
}

// ============================================================================
// Callback tests
// ============================================================================

TEST_F(enhanced_cancellation_token_test, CallbackInvocation)
{
	auto token = enhanced_cancellation_token::create();
	std::atomic<int> callback_count{0};

	token.register_callback([&]() { callback_count.fetch_add(1); });

	token.cancel();

	EXPECT_EQ(callback_count.load(), 1);
}

TEST_F(enhanced_cancellation_token_test, CallbackWithReason)
{
	auto token = enhanced_cancellation_token::create();
	std::atomic<bool> callback_invoked{false};
	cancellation_reason::type received_type = cancellation_reason::type::none;

	token.register_callback([&](const cancellation_reason& reason)
							{
		received_type = reason.reason_type;
		callback_invoked.store(true); });

	token.cancel("Test message");

	EXPECT_TRUE(callback_invoked.load());
	EXPECT_EQ(received_type, cancellation_reason::type::user_requested);
}

TEST_F(enhanced_cancellation_token_test, CallbackAfterCancellation)
{
	auto token = enhanced_cancellation_token::create();
	token.cancel();

	std::atomic<bool> callback_invoked{false};
	token.register_callback([&]() { callback_invoked.store(true); });

	// Callback should be invoked immediately
	EXPECT_TRUE(callback_invoked.load());
}

TEST_F(enhanced_cancellation_token_test, UnregisterCallback)
{
	auto token = enhanced_cancellation_token::create();
	std::atomic<int> callback_count{0};

	auto handle = token.register_callback([&]() { callback_count.fetch_add(1); });

	token.unregister_callback(handle);
	token.cancel();

	EXPECT_EQ(callback_count.load(), 0);
}

TEST_F(enhanced_cancellation_token_test, MultipleCallbacks)
{
	auto token = enhanced_cancellation_token::create();
	std::atomic<int> callback_count{0};

	for (int i = 0; i < 10; ++i)
	{
		token.register_callback([&]() { callback_count.fetch_add(1); });
	}

	token.cancel();

	EXPECT_EQ(callback_count.load(), 10);
}

// ============================================================================
// Wait method tests
// ============================================================================

TEST_F(enhanced_cancellation_token_test, WaitForCancellation)
{
	auto token = enhanced_cancellation_token::create();

	std::thread cancel_thread(
		[&]()
		{
			std::this_thread::sleep_for(std::chrono::milliseconds{50});
			token.cancel();
		});

	auto result = token.wait_for(std::chrono::milliseconds{500});

	EXPECT_TRUE(result);
	EXPECT_TRUE(token.is_cancelled());

	cancel_thread.join();
}

TEST_F(enhanced_cancellation_token_test, WaitForTimeout)
{
	auto token = enhanced_cancellation_token::create();

	auto result = token.wait_for(std::chrono::milliseconds{50});

	EXPECT_FALSE(result);
	EXPECT_FALSE(token.is_cancelled());
}

TEST_F(enhanced_cancellation_token_test, WaitUntilCancellation)
{
	auto token = enhanced_cancellation_token::create();

	std::thread cancel_thread(
		[&]()
		{
			std::this_thread::sleep_for(std::chrono::milliseconds{50});
			token.cancel();
		});

	auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds{500};
	auto result = token.wait_until(deadline);

	EXPECT_TRUE(result);
	EXPECT_TRUE(token.is_cancelled());

	cancel_thread.join();
}

// ============================================================================
// Helper class tests
// ============================================================================

TEST_F(enhanced_cancellation_token_test, CallbackGuard)
{
	auto token = enhanced_cancellation_token::create();
	std::atomic<int> callback_count{0};

	{
		cancellation_callback_guard guard(token,
										  [&]() { callback_count.fetch_add(1); });
		// Guard is in scope, callback is registered
	}
	// Guard is out of scope, callback should be unregistered

	token.cancel();
	EXPECT_EQ(callback_count.load(), 0);
}

TEST_F(enhanced_cancellation_token_test, CallbackGuardMove)
{
	auto token = enhanced_cancellation_token::create();
	std::atomic<int> callback_count{0};

	cancellation_callback_guard guard1(token,
									   [&]() { callback_count.fetch_add(1); });

	// Move guard
	cancellation_callback_guard guard2(std::move(guard1));

	token.cancel();
	EXPECT_EQ(callback_count.load(), 1);
}

TEST_F(enhanced_cancellation_token_test, CancellationScope)
{
	auto token = enhanced_cancellation_token::create();
	cancellation_scope scope(token);

	EXPECT_FALSE(scope.is_cancelled());
	EXPECT_NO_THROW(scope.check_cancelled());

	token.cancel();

	EXPECT_TRUE(scope.is_cancelled());
	EXPECT_THROW(scope.check_cancelled(), operation_cancelled_exception);
}

TEST_F(enhanced_cancellation_token_test, CancellationContext)
{
	auto token = enhanced_cancellation_token::create();

	{
		cancellation_context::guard guard(token);

		auto current = cancellation_context::current();
		// Should get the pushed token (they share the same state)
		EXPECT_FALSE(current.is_cancelled());

		token.cancel();

		auto current2 = cancellation_context::current();
		EXPECT_TRUE(current2.is_cancelled());
	}

	// After guard is destroyed, context should return a new uncancelled token
	auto current3 = cancellation_context::current();
	EXPECT_FALSE(current3.is_cancelled());
}

// ============================================================================
// Thread safety tests
// ============================================================================

TEST_F(enhanced_cancellation_token_test, ConcurrentCallbackRegistration)
{
	for (int iter = 0; iter < TEST_ITERATIONS; ++iter)
	{
		auto token = enhanced_cancellation_token::create();
		std::atomic<int> callback_count{0};
		const int num_threads = 4;
		const int callbacks_per_thread = 25;

		std::vector<std::thread> threads;
		std::latch start_latch(num_threads);

		for (int t = 0; t < num_threads; ++t)
		{
			threads.emplace_back(
				[&, token]() mutable
				{
					start_latch.arrive_and_wait();
					for (int i = 0; i < callbacks_per_thread; ++i)
					{
						token.register_callback(
							[&]()
							{ callback_count.fetch_add(1, std::memory_order_relaxed); });
					}
				});
		}

		for (auto& t : threads)
		{
			t.join();
		}

		token.cancel();
		EXPECT_EQ(callback_count.load(), num_threads * callbacks_per_thread);
	}
}

TEST_F(enhanced_cancellation_token_test, ConcurrentCancellation)
{
	for (int iter = 0; iter < TEST_ITERATIONS; ++iter)
	{
		auto token = enhanced_cancellation_token::create();
		std::atomic<int> callback_count{0};

		token.register_callback([&]() { callback_count.fetch_add(1); });

		const int num_threads = 4;
		std::vector<std::thread> threads;
		std::latch start_latch(num_threads);

		for (int t = 0; t < num_threads; ++t)
		{
			threads.emplace_back(
				[&, token]() mutable
				{
					start_latch.arrive_and_wait();
					token.cancel();
				});
		}

		for (auto& t : threads)
		{
			t.join();
		}

		// Callback should be invoked exactly once
		EXPECT_EQ(callback_count.load(), 1);
	}
}

// ============================================================================
// Cancellation reason tests
// ============================================================================

TEST_F(enhanced_cancellation_token_test, CancellationReasonToString)
{
	cancellation_reason reason;
	reason.reason_type = cancellation_reason::type::timeout;
	reason.message = "Timeout expired";
	reason.cancel_time = std::chrono::steady_clock::now();

	auto str = reason.to_string();
	EXPECT_NE(str.find("timeout"), std::string::npos);
	EXPECT_NE(str.find("Timeout expired"), std::string::npos);
}

TEST_F(enhanced_cancellation_token_test, CancellationReasonTypeToString)
{
	EXPECT_EQ(cancellation_reason::type_to_string(cancellation_reason::type::none),
			  "none");
	EXPECT_EQ(cancellation_reason::type_to_string(
				  cancellation_reason::type::user_requested),
			  "user_requested");
	EXPECT_EQ(
		cancellation_reason::type_to_string(cancellation_reason::type::timeout),
		"timeout");
	EXPECT_EQ(
		cancellation_reason::type_to_string(cancellation_reason::type::deadline),
		"deadline");
	EXPECT_EQ(cancellation_reason::type_to_string(
				  cancellation_reason::type::parent_cancelled),
			  "parent_cancelled");
	EXPECT_EQ(cancellation_reason::type_to_string(
				  cancellation_reason::type::pool_shutdown),
			  "pool_shutdown");
	EXPECT_EQ(
		cancellation_reason::type_to_string(cancellation_reason::type::error),
		"error");
}

// ============================================================================
// Exception tests
// ============================================================================

TEST_F(enhanced_cancellation_token_test, OperationCancelledException)
{
	cancellation_reason reason;
	reason.reason_type = cancellation_reason::type::timeout;
	reason.message = "Test timeout";

	operation_cancelled_exception ex(reason);

	EXPECT_NE(std::string(ex.what()).find("cancelled"), std::string::npos);
	EXPECT_EQ(ex.reason().reason_type, cancellation_reason::type::timeout);
	EXPECT_EQ(ex.reason().message, "Test timeout");
}

} // namespace test
} // namespace kcenon::thread
