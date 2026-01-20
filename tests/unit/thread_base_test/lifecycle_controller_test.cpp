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
#include <kcenon/thread/core/lifecycle_controller.h>

#include <thread>
#include <chrono>
#include <atomic>
#include <latch>

using namespace kcenon::thread;

class LifecycleControllerTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		controller_ = std::make_unique<lifecycle_controller>();
	}

	void TearDown() override
	{
		controller_.reset();
	}

	std::unique_ptr<lifecycle_controller> controller_;
};

TEST_F(LifecycleControllerTest, InitialStateIsCreated)
{
	EXPECT_EQ(controller_->get_state(), thread_conditions::Created);
	EXPECT_FALSE(controller_->is_running());
	// Note: In legacy mode (no jthread), has_active_source() returns !stop_requested_
	// which is true initially. In jthread mode, it returns stop_source_.has_value()
	// which is false initially. We test the initialized state instead.
	EXPECT_FALSE(controller_->is_stop_requested());
}

TEST_F(LifecycleControllerTest, StateTransitions)
{
	controller_->set_state(thread_conditions::Waiting);
	EXPECT_EQ(controller_->get_state(), thread_conditions::Waiting);
	EXPECT_TRUE(controller_->is_running());

	controller_->set_state(thread_conditions::Working);
	EXPECT_EQ(controller_->get_state(), thread_conditions::Working);
	EXPECT_TRUE(controller_->is_running());

	controller_->set_state(thread_conditions::Stopping);
	EXPECT_EQ(controller_->get_state(), thread_conditions::Stopping);
	EXPECT_FALSE(controller_->is_running());

	controller_->set_stopped();
	EXPECT_EQ(controller_->get_state(), thread_conditions::Stopped);
	EXPECT_FALSE(controller_->is_running());
}

TEST_F(LifecycleControllerTest, InitializeForStart)
{
	controller_->initialize_for_start();
	// After initialize_for_start, stop is not requested and state is Created
	EXPECT_FALSE(controller_->is_stop_requested());
	EXPECT_EQ(controller_->get_state(), thread_conditions::Created);
	// has_active_source() returns true only when in Working/Waiting state (legacy mode)
	// or when stop_source_ is set (jthread mode)
}

TEST_F(LifecycleControllerTest, StopRequestBehavior)
{
	controller_->initialize_for_start();
	EXPECT_FALSE(controller_->is_stop_requested());

	controller_->request_stop();
	EXPECT_TRUE(controller_->is_stop_requested());
}

TEST_F(LifecycleControllerTest, ResetStopSource)
{
	controller_->initialize_for_start();

	// Simulate an active thread by setting state to Working
	controller_->set_state(thread_conditions::Working);
	EXPECT_TRUE(controller_->has_active_source());

	controller_->request_stop();
	controller_->reset_stop_source();
	controller_->set_stopped();

	EXPECT_FALSE(controller_->has_active_source());
}

TEST_F(LifecycleControllerTest, WaitWithPredicate)
{
	controller_->initialize_for_start();

	std::atomic<bool> predicate_met{false};
	std::atomic<bool> wait_completed{false};

	std::thread waiter([this, &predicate_met, &wait_completed]() {
		auto lock = controller_->acquire_lock();
		controller_->wait(lock, [&predicate_met]() {
			return predicate_met.load();
		});
		wait_completed.store(true);
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	EXPECT_FALSE(wait_completed.load());

	predicate_met.store(true);
	controller_->notify_all();

	waiter.join();
	EXPECT_TRUE(wait_completed.load());
}

TEST_F(LifecycleControllerTest, WaitExitsOnStopRequest)
{
	controller_->initialize_for_start();

	std::atomic<bool> wait_completed{false};

	std::thread waiter([this, &wait_completed]() {
		auto lock = controller_->acquire_lock();
		controller_->wait(lock, []() {
			return false;
		});
		wait_completed.store(true);
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	EXPECT_FALSE(wait_completed.load());

	controller_->request_stop();
	controller_->notify_all();

	waiter.join();
	EXPECT_TRUE(wait_completed.load());
}

TEST_F(LifecycleControllerTest, WaitForWithTimeout)
{
	controller_->initialize_for_start();

	auto lock = controller_->acquire_lock();

	auto start = std::chrono::steady_clock::now();
	bool result = controller_->wait_for(
		lock,
		std::chrono::milliseconds(100),
		[]() { return false; }
	);
	auto elapsed = std::chrono::steady_clock::now() - start;

	EXPECT_FALSE(result);
	EXPECT_GE(elapsed, std::chrono::milliseconds(90));
}

TEST_F(LifecycleControllerTest, WaitForReturnsImmediatelyWhenPredicateTrue)
{
	controller_->initialize_for_start();

	auto lock = controller_->acquire_lock();

	auto start = std::chrono::steady_clock::now();
	bool result = controller_->wait_for(
		lock,
		std::chrono::milliseconds(1000),
		[]() { return true; }
	);
	auto elapsed = std::chrono::steady_clock::now() - start;

	EXPECT_TRUE(result);
	EXPECT_LT(elapsed, std::chrono::milliseconds(100));
}

TEST_F(LifecycleControllerTest, NotifyOneBehavior)
{
	controller_->initialize_for_start();

	std::atomic<int> woken_count{0};
	std::latch start_latch{2};

	auto waiter_func = [this, &woken_count, &start_latch]() {
		auto lock = controller_->acquire_lock();
		start_latch.count_down();
		controller_->wait(lock, [&woken_count]() {
			return woken_count.load() > 0;
		});
		woken_count.fetch_add(1);
	};

	std::thread waiter1(waiter_func);
	std::thread waiter2(waiter_func);

	start_latch.wait();
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	woken_count.store(1);
	controller_->notify_one();

	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	controller_->request_stop();
	controller_->notify_all();

	waiter1.join();
	waiter2.join();

	EXPECT_GE(woken_count.load(), 2);
}

TEST_F(LifecycleControllerTest, NotifyAllBehavior)
{
	controller_->initialize_for_start();

	std::atomic<int> woken_count{0};
	std::latch start_latch{3};
	std::atomic<bool> should_wake{false};

	auto waiter_func = [this, &woken_count, &start_latch, &should_wake]() {
		auto lock = controller_->acquire_lock();
		start_latch.count_down();
		controller_->wait(lock, [&should_wake]() {
			return should_wake.load();
		});
		woken_count.fetch_add(1);
	};

	std::thread waiter1(waiter_func);
	std::thread waiter2(waiter_func);
	std::thread waiter3(waiter_func);

	start_latch.wait();
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	EXPECT_EQ(woken_count.load(), 0);

	should_wake.store(true);
	controller_->notify_all();

	waiter1.join();
	waiter2.join();
	waiter3.join();

	EXPECT_EQ(woken_count.load(), 3);
}

TEST_F(LifecycleControllerTest, ConcurrentStateUpdates)
{
	controller_->initialize_for_start();

	const int thread_count = 10;
	const int iterations = 100;
	std::atomic<int> completed{0};

	auto worker = [this, iterations, &completed]() {
		for (int i = 0; i < iterations; ++i)
		{
			controller_->set_state(thread_conditions::Working);
			controller_->set_state(thread_conditions::Waiting);
		}
		completed.fetch_add(1);
	};

	std::vector<std::thread> threads;
	threads.reserve(thread_count);
	for (int i = 0; i < thread_count; ++i)
	{
		threads.emplace_back(worker);
	}

	for (auto& t : threads)
	{
		t.join();
	}

	EXPECT_EQ(completed.load(), thread_count);

	auto final_state = controller_->get_state();
	EXPECT_TRUE(final_state == thread_conditions::Working ||
	            final_state == thread_conditions::Waiting);
}

TEST_F(LifecycleControllerTest, MultipleInitializeCycles)
{
	for (int cycle = 0; cycle < 5; ++cycle)
	{
		controller_->initialize_for_start();
		EXPECT_FALSE(controller_->is_stop_requested());

		controller_->set_state(thread_conditions::Working);
		EXPECT_TRUE(controller_->is_running());
		EXPECT_TRUE(controller_->has_active_source());

		controller_->request_stop();
		EXPECT_TRUE(controller_->is_stop_requested());

		controller_->reset_stop_source();
		controller_->set_stopped();
		EXPECT_FALSE(controller_->is_running());
		EXPECT_FALSE(controller_->has_active_source());
	}
}
