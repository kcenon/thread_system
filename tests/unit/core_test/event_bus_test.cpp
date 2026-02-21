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
 * @file event_bus_test.cpp
 * @brief Unit tests for event_bus
 * @since 4.0.0
 */

#include <gtest/gtest.h>

#include <kcenon/thread/core/event_bus.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace kcenon::thread;
using namespace std::chrono_literals;

// =============================================================================
// Test event types
// =============================================================================

struct test_event {
	int value{0};
	explicit test_event(int v = 0) : value(v) {}
};

struct another_event {
	std::string message;
	explicit another_event(std::string msg = "") : message(std::move(msg)) {}
};

// =============================================================================
// Helper: wait for atomic flag with timeout
// =============================================================================

static bool wait_for(const std::atomic<bool>& flag, std::chrono::milliseconds timeout = 2000ms) {
	auto deadline = std::chrono::steady_clock::now() + timeout;
	while (!flag.load() && std::chrono::steady_clock::now() < deadline) {
		std::this_thread::sleep_for(5ms);
	}
	return flag.load();
}

static bool wait_for_count(const std::atomic<int>& counter, int expected,
                           std::chrono::milliseconds timeout = 2000ms) {
	auto deadline = std::chrono::steady_clock::now() + timeout;
	while (counter.load() < expected && std::chrono::steady_clock::now() < deadline) {
		std::this_thread::sleep_for(5ms);
	}
	return counter.load() >= expected;
}

// =============================================================================
// Test fixture
// =============================================================================

class EventBusTest : public ::testing::Test {
protected:
	void SetUp() override {
		bus_ = std::make_unique<event_bus>();
	}

	void TearDown() override {
		bus_.reset();
	}

	std::unique_ptr<event_bus> bus_;
};

// =============================================================================
// publish (async dispatch)
// =============================================================================

TEST_F(EventBusTest, PublishDispatchesToHandler) {
	std::atomic<bool> handled{false};
	std::atomic<int> received_value{0};

	auto sub = bus_->subscribe<test_event>(
	    [&](const test_event& evt) {
		    received_value.store(evt.value);
		    handled.store(true);
	    });

	bus_->publish(test_event(42));

	ASSERT_TRUE(wait_for(handled));
	EXPECT_EQ(received_value.load(), 42);
}

TEST_F(EventBusTest, PublishWithNoSubscribersDoesNotCrash) {
	EXPECT_NO_THROW(bus_->publish(test_event(1)));
}

// =============================================================================
// publish_sync (synchronous dispatch)
// =============================================================================

TEST_F(EventBusTest, PublishSyncCallsHandlerBeforeReturn) {
	bool handled = false;
	int received_value = 0;

	auto sub = bus_->subscribe<test_event>(
	    [&](const test_event& evt) {
		    received_value = evt.value;
		    handled = true;
	    });

	bus_->publish_sync(test_event(99));

	// Handler must have been called synchronously
	EXPECT_TRUE(handled);
	EXPECT_EQ(received_value, 99);
}

TEST_F(EventBusTest, PublishSyncWithNoSubscribersDoesNotCrash) {
	EXPECT_NO_THROW(bus_->publish_sync(test_event(1)));
}

// =============================================================================
// subscribe (RAII subscription handle)
// =============================================================================

TEST_F(EventBusTest, SubscribeReturnsActiveHandle) {
	auto sub = bus_->subscribe<test_event>(
	    [](const test_event&) {});

	EXPECT_TRUE(sub.is_active());
	EXPECT_EQ(bus_->subscriber_count<test_event>(), 1u);
}

// =============================================================================
// subscription::unsubscribe() — explicit mid-lifetime
// =============================================================================

TEST_F(EventBusTest, ExplicitUnsubscribe) {
	auto sub = bus_->subscribe<test_event>(
	    [](const test_event&) {});

	EXPECT_TRUE(sub.is_active());
	EXPECT_EQ(bus_->subscriber_count<test_event>(), 1u);

	sub.unsubscribe();
	EXPECT_FALSE(sub.is_active());
	EXPECT_EQ(bus_->subscriber_count<test_event>(), 0u);
}

TEST_F(EventBusTest, DoubleUnsubscribeIsSafe) {
	auto sub = bus_->subscribe<test_event>(
	    [](const test_event&) {});

	sub.unsubscribe();
	EXPECT_NO_THROW(sub.unsubscribe());
	EXPECT_FALSE(sub.is_active());
}

// =============================================================================
// subscription::is_active() — state tracking
// =============================================================================

TEST_F(EventBusTest, DefaultSubscriptionIsInactive) {
	event_bus::subscription sub;
	EXPECT_FALSE(sub.is_active());
}

TEST_F(EventBusTest, IsActiveReflectsState) {
	auto sub = bus_->subscribe<test_event>(
	    [](const test_event&) {});

	EXPECT_TRUE(sub.is_active());
	sub.unsubscribe();
	EXPECT_FALSE(sub.is_active());
}

// =============================================================================
// Subscription destructor — auto-unsubscribe on scope exit
// =============================================================================

TEST_F(EventBusTest, DestructorAutoUnsubscribes) {
	{
		auto sub = bus_->subscribe<test_event>(
		    [](const test_event&) {});
		EXPECT_EQ(bus_->subscriber_count<test_event>(), 1u);
	}
	// sub destroyed here
	EXPECT_EQ(bus_->subscriber_count<test_event>(), 0u);
}

// =============================================================================
// Subscription move semantics
// =============================================================================

TEST_F(EventBusTest, MoveConstructorTransfersOwnership) {
	auto sub1 = bus_->subscribe<test_event>(
	    [](const test_event&) {});

	EXPECT_TRUE(sub1.is_active());

	event_bus::subscription sub2(std::move(sub1));
	EXPECT_FALSE(sub1.is_active());  // NOLINT(bugprone-use-after-move)
	EXPECT_TRUE(sub2.is_active());
	EXPECT_EQ(bus_->subscriber_count<test_event>(), 1u);
}

TEST_F(EventBusTest, MoveAssignmentTransfersOwnership) {
	auto sub1 = bus_->subscribe<test_event>(
	    [](const test_event&) {});
	auto sub2 = bus_->subscribe<another_event>(
	    [](const another_event&) {});

	EXPECT_EQ(bus_->subscriber_count<test_event>(), 1u);
	EXPECT_EQ(bus_->subscriber_count<another_event>(), 1u);

	// Move-assign sub1 into sub2; sub2's old subscription should be released
	sub2 = std::move(sub1);

	EXPECT_FALSE(sub1.is_active());  // NOLINT(bugprone-use-after-move)
	EXPECT_TRUE(sub2.is_active());
	EXPECT_EQ(bus_->subscriber_count<test_event>(), 1u);
	EXPECT_EQ(bus_->subscriber_count<another_event>(), 0u);
}

// =============================================================================
// clear_subscriptions<Event>() — per-type removal
// =============================================================================

TEST_F(EventBusTest, ClearSubscriptionsRemovesSpecificType) {
	auto sub1 = bus_->subscribe<test_event>(
	    [](const test_event&) {});
	auto sub2 = bus_->subscribe<another_event>(
	    [](const another_event&) {});

	EXPECT_EQ(bus_->subscriber_count<test_event>(), 1u);
	EXPECT_EQ(bus_->subscriber_count<another_event>(), 1u);

	bus_->clear_subscriptions<test_event>();

	EXPECT_EQ(bus_->subscriber_count<test_event>(), 0u);
	EXPECT_EQ(bus_->subscriber_count<another_event>(), 1u);
}

// =============================================================================
// clear_all_subscriptions() — remove all handlers
// =============================================================================

TEST_F(EventBusTest, ClearAllSubscriptionsRemovesEverything) {
	auto sub1 = bus_->subscribe<test_event>(
	    [](const test_event&) {});
	auto sub2 = bus_->subscribe<another_event>(
	    [](const another_event&) {});

	bus_->clear_all_subscriptions();

	EXPECT_EQ(bus_->subscriber_count<test_event>(), 0u);
	EXPECT_EQ(bus_->subscriber_count<another_event>(), 0u);
}

// =============================================================================
// subscriber_count<Event>() — count verification
// =============================================================================

TEST_F(EventBusTest, SubscriberCountReflectsRegistrations) {
	EXPECT_EQ(bus_->subscriber_count<test_event>(), 0u);

	auto sub1 = bus_->subscribe<test_event>(
	    [](const test_event&) {});
	EXPECT_EQ(bus_->subscriber_count<test_event>(), 1u);

	auto sub2 = bus_->subscribe<test_event>(
	    [](const test_event&) {});
	EXPECT_EQ(bus_->subscriber_count<test_event>(), 2u);

	sub1.unsubscribe();
	EXPECT_EQ(bus_->subscriber_count<test_event>(), 1u);
}

// =============================================================================
// Multi-subscriber fan-out
// =============================================================================

TEST_F(EventBusTest, AllSubscribersReceiveEvent) {
	std::atomic<int> total{0};
	constexpr int num_subscribers = 5;

	std::vector<event_bus::subscription> subs;
	for (int i = 0; i < num_subscribers; ++i) {
		subs.push_back(bus_->subscribe<test_event>(
		    [&](const test_event& evt) {
			    total.fetch_add(evt.value);
		    }));
	}

	bus_->publish(test_event(10));

	ASSERT_TRUE(wait_for_count(total, num_subscribers * 10));
	EXPECT_EQ(total.load(), num_subscribers * 10);
}

TEST_F(EventBusTest, SyncFanOutToAllSubscribers) {
	int total = 0;
	constexpr int num_subscribers = 3;

	std::vector<event_bus::subscription> subs;
	for (int i = 0; i < num_subscribers; ++i) {
		subs.push_back(bus_->subscribe<test_event>(
		    [&](const test_event& evt) {
			    total += evt.value;
		    }));
	}

	bus_->publish_sync(test_event(7));
	EXPECT_EQ(total, num_subscribers * 7);
}

// =============================================================================
// Handler exception isolation
// =============================================================================

TEST_F(EventBusTest, SyncExceptionDoesNotStopOtherHandlers) {
	int handler1_count = 0;
	int handler2_count = 0;
	int handler3_count = 0;

	auto sub1 = bus_->subscribe<test_event>(
	    [&](const test_event&) { ++handler1_count; });

	auto sub2 = bus_->subscribe<test_event>(
	    [&](const test_event&) {
		    throw std::runtime_error("handler error");
	    });

	auto sub3 = bus_->subscribe<test_event>(
	    [&](const test_event&) { ++handler3_count; });

	EXPECT_NO_THROW(bus_->publish_sync(test_event(1)));

	EXPECT_EQ(handler1_count, 1);
	EXPECT_EQ(handler3_count, 1);
}

TEST_F(EventBusTest, AsyncExceptionDoesNotStopOtherHandlers) {
	std::atomic<int> success_count{0};

	auto sub1 = bus_->subscribe<test_event>(
	    [&](const test_event&) { success_count.fetch_add(1); });

	auto sub2 = bus_->subscribe<test_event>(
	    [&](const test_event&) {
		    throw std::runtime_error("async handler error");
	    });

	auto sub3 = bus_->subscribe<test_event>(
	    [&](const test_event&) { success_count.fetch_add(1); });

	bus_->publish(test_event(1));

	ASSERT_TRUE(wait_for_count(success_count, 2));
	EXPECT_EQ(success_count.load(), 2);
}

// =============================================================================
// Type isolation: different event types are independent
// =============================================================================

TEST_F(EventBusTest, DifferentTypesAreIsolated) {
	std::atomic<bool> test_handled{false};
	std::atomic<bool> another_handled{false};

	auto sub1 = bus_->subscribe<test_event>(
	    [&](const test_event&) { test_handled.store(true); });
	auto sub2 = bus_->subscribe<another_event>(
	    [&](const another_event&) { another_handled.store(true); });

	bus_->publish(test_event(1));

	ASSERT_TRUE(wait_for(test_handled));
	// Brief wait to verify another_event was NOT triggered
	std::this_thread::sleep_for(50ms);
	EXPECT_FALSE(another_handled.load());
}
