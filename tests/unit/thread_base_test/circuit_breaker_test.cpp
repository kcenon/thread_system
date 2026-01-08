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

#include <kcenon/thread/resilience/circuit_breaker.h>
#include <kcenon/thread/resilience/failure_window.h>
#include <kcenon/thread/resilience/protected_job.h>
#include <kcenon/thread/core/callback_job.h>

#include <thread>
#include <atomic>
#include <chrono>
#include <vector>

using namespace kcenon::thread;

// ============================================================================
// failure_window tests
// ============================================================================

class FailureWindowTest : public ::testing::Test {
protected:
    void SetUp() override {
        window_ = std::make_unique<failure_window>(std::chrono::seconds{60});
    }

    std::unique_ptr<failure_window> window_;
};

TEST_F(FailureWindowTest, InitialStateIsEmpty) {
    EXPECT_EQ(window_->total_requests(), 0);
    EXPECT_EQ(window_->failure_count(), 0);
    EXPECT_EQ(window_->success_count(), 0);
    EXPECT_DOUBLE_EQ(window_->failure_rate(), 0.0);
}

TEST_F(FailureWindowTest, RecordsSuccesses) {
    window_->record_success();
    window_->record_success();
    window_->record_success();

    EXPECT_EQ(window_->total_requests(), 3);
    EXPECT_EQ(window_->success_count(), 3);
    EXPECT_EQ(window_->failure_count(), 0);
    EXPECT_DOUBLE_EQ(window_->failure_rate(), 0.0);
}

TEST_F(FailureWindowTest, RecordsFailures) {
    window_->record_failure();
    window_->record_failure();

    EXPECT_EQ(window_->total_requests(), 2);
    EXPECT_EQ(window_->failure_count(), 2);
    EXPECT_EQ(window_->success_count(), 0);
    EXPECT_DOUBLE_EQ(window_->failure_rate(), 1.0);
}

TEST_F(FailureWindowTest, CalculatesFailureRate) {
    window_->record_success();
    window_->record_success();
    window_->record_failure();
    window_->record_failure();

    EXPECT_EQ(window_->total_requests(), 4);
    EXPECT_DOUBLE_EQ(window_->failure_rate(), 0.5);
}

TEST_F(FailureWindowTest, ResetClearsAllCounters) {
    window_->record_success();
    window_->record_failure();
    window_->reset();

    EXPECT_EQ(window_->total_requests(), 0);
    EXPECT_EQ(window_->failure_count(), 0);
    EXPECT_EQ(window_->success_count(), 0);
}

// ============================================================================
// circuit_breaker tests
// ============================================================================

class CircuitBreakerTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.failure_threshold = 3;
        config_.failure_rate_threshold = 0.5;
        config_.minimum_requests = 5;
        config_.open_duration = std::chrono::seconds{1};
        config_.half_open_max_requests = 2;
        config_.half_open_success_threshold = 2;
        config_.window_size = std::chrono::seconds{60};

        cb_ = std::make_unique<circuit_breaker>(config_);
    }

    circuit_breaker_config config_;
    std::unique_ptr<circuit_breaker> cb_;
};

TEST_F(CircuitBreakerTest, StartsInClosedState) {
    EXPECT_EQ(cb_->get_state(), circuit_state::closed);
}

TEST_F(CircuitBreakerTest, AllowsRequestsInClosedState) {
    EXPECT_TRUE(cb_->allow_request());
    EXPECT_TRUE(cb_->allow_request());
    EXPECT_TRUE(cb_->allow_request());
}

TEST_F(CircuitBreakerTest, TransitionsToOpenOnConsecutiveFailures) {
    // Record failures up to threshold
    for (std::size_t i = 0; i < config_.failure_threshold; ++i) {
        EXPECT_TRUE(cb_->allow_request());
        cb_->record_failure();
    }

    EXPECT_EQ(cb_->get_state(), circuit_state::open);
}

TEST_F(CircuitBreakerTest, RejectsRequestsInOpenState) {
    // Trip the circuit
    cb_->trip();
    EXPECT_EQ(cb_->get_state(), circuit_state::open);

    EXPECT_FALSE(cb_->allow_request());
    EXPECT_FALSE(cb_->allow_request());
}

TEST_F(CircuitBreakerTest, TransitionsToHalfOpenAfterTimeout) {
    cb_->trip();
    EXPECT_EQ(cb_->get_state(), circuit_state::open);

    // Wait for open duration
    std::this_thread::sleep_for(config_.open_duration + std::chrono::milliseconds{100});

    // Next request should transition to half-open
    EXPECT_TRUE(cb_->allow_request());
    EXPECT_EQ(cb_->get_state(), circuit_state::half_open);
}

TEST_F(CircuitBreakerTest, TransitionsToClosedOnSuccessInHalfOpen) {
    cb_->trip();
    std::this_thread::sleep_for(config_.open_duration + std::chrono::milliseconds{100});

    // Transition to half-open
    EXPECT_TRUE(cb_->allow_request());
    cb_->record_success();

    EXPECT_TRUE(cb_->allow_request());
    cb_->record_success();

    EXPECT_EQ(cb_->get_state(), circuit_state::closed);
}

TEST_F(CircuitBreakerTest, TransitionsBackToOpenOnFailureInHalfOpen) {
    cb_->trip();
    std::this_thread::sleep_for(config_.open_duration + std::chrono::milliseconds{100});

    // Transition to half-open
    EXPECT_TRUE(cb_->allow_request());
    cb_->record_failure();

    EXPECT_EQ(cb_->get_state(), circuit_state::open);
}

TEST_F(CircuitBreakerTest, ManualTripOpensCircuit) {
    EXPECT_EQ(cb_->get_state(), circuit_state::closed);
    cb_->trip();
    EXPECT_EQ(cb_->get_state(), circuit_state::open);
}

TEST_F(CircuitBreakerTest, ManualResetClosesCircuit) {
    cb_->trip();
    EXPECT_EQ(cb_->get_state(), circuit_state::open);

    cb_->reset();
    EXPECT_EQ(cb_->get_state(), circuit_state::closed);
}

TEST_F(CircuitBreakerTest, StatsAreAccurate) {
    cb_->allow_request();
    cb_->record_success();

    cb_->allow_request();
    cb_->record_failure();

    auto stats = cb_->get_stats();
    EXPECT_EQ(stats.current_state, circuit_state::closed);
    EXPECT_EQ(stats.total_requests, 2);
    EXPECT_EQ(stats.successful_requests, 1);
    EXPECT_EQ(stats.failed_requests, 1);
}

TEST_F(CircuitBreakerTest, GuardMarksSuccessOnExplicitCall) {
    {
        auto guard = cb_->make_guard();
        EXPECT_TRUE(guard.is_allowed());
        guard.mark_success();
    }

    auto stats = cb_->get_stats();
    EXPECT_EQ(stats.successful_requests, 1);
    EXPECT_EQ(stats.failed_requests, 0);
}

TEST_F(CircuitBreakerTest, GuardMarksFailureOnDestruction) {
    {
        auto guard = cb_->make_guard();
        EXPECT_TRUE(guard.is_allowed());
        // Don't mark success - should record failure on destruction
    }

    auto stats = cb_->get_stats();
    EXPECT_EQ(stats.successful_requests, 0);
    EXPECT_EQ(stats.failed_requests, 1);
}

TEST_F(CircuitBreakerTest, StateChangeCallbackIsCalled) {
    std::atomic<int> callback_count{0};
    circuit_state last_old_state = circuit_state::closed;
    circuit_state last_new_state = circuit_state::closed;

    config_.state_change_callback = [&](circuit_state old_state, circuit_state new_state) {
        ++callback_count;
        last_old_state = old_state;
        last_new_state = new_state;
    };

    auto cb = circuit_breaker(config_);
    cb.trip();

    EXPECT_EQ(callback_count, 1);
    EXPECT_EQ(last_old_state, circuit_state::closed);
    EXPECT_EQ(last_new_state, circuit_state::open);
}

// ============================================================================
// protected_job tests
// ============================================================================

class ProtectedJobTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.failure_threshold = 3;
        config_.open_duration = std::chrono::seconds{1};
        cb_ = std::make_shared<circuit_breaker>(config_);
    }

    circuit_breaker_config config_;
    std::shared_ptr<circuit_breaker> cb_;
};

TEST_F(ProtectedJobTest, ExecutesInnerJobOnSuccess) {
    std::atomic<bool> executed{false};

    auto inner = std::make_unique<callback_job>([&]() -> kcenon::common::VoidResult {
        executed = true;
        return kcenon::common::ok();
    });

    auto protected_j = protected_job(std::move(inner), cb_);
    auto result = protected_j.do_work();

    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(executed);

    auto stats = cb_->get_stats();
    EXPECT_EQ(stats.successful_requests, 1);
}

TEST_F(ProtectedJobTest, RecordsFailureOnInnerJobFailure) {
    auto inner = std::make_unique<callback_job>([]() -> kcenon::common::VoidResult {
        return kcenon::common::error_info{-1, "test error", "test"};
    });

    auto protected_j = protected_job(std::move(inner), cb_);
    auto result = protected_j.do_work();

    EXPECT_TRUE(result.is_err());

    auto stats = cb_->get_stats();
    EXPECT_EQ(stats.failed_requests, 1);
}

TEST_F(ProtectedJobTest, RejectsWhenCircuitOpen) {
    cb_->trip();

    auto inner = std::make_unique<callback_job>([]() -> kcenon::common::VoidResult {
        return kcenon::common::ok();
    });

    auto protected_j = protected_job(std::move(inner), cb_);
    auto result = protected_j.do_work();

    EXPECT_TRUE(result.is_err());
}

// ============================================================================
// Thread safety tests
// ============================================================================

TEST(CircuitBreakerConcurrencyTest, HandlesMultipleThreads) {
    circuit_breaker_config config;
    config.failure_threshold = 100;  // High threshold for this test
    config.window_size = std::chrono::seconds{60};

    auto cb = std::make_shared<circuit_breaker>(config);

    constexpr int num_threads = 4;
    constexpr int requests_per_thread = 1000;

    std::vector<std::thread> threads;
    std::atomic<int> total_allowed{0};

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&cb, &total_allowed]() {
            for (int j = 0; j < requests_per_thread; ++j) {
                if (cb->allow_request()) {
                    ++total_allowed;
                    if (j % 10 == 0) {
                        cb->record_failure();
                    } else {
                        cb->record_success();
                    }
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto stats = cb->get_stats();
    EXPECT_GT(stats.total_requests, 0);
    EXPECT_EQ(stats.total_requests, stats.successful_requests + stats.failed_requests);
}

TEST(FailureWindowConcurrencyTest, HandlesMultipleThreads) {
    failure_window window(std::chrono::seconds{60});

    constexpr int num_threads = 4;
    constexpr int ops_per_thread = 1000;

    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&window, i]() {
            for (int j = 0; j < ops_per_thread; ++j) {
                if ((i + j) % 3 == 0) {
                    window.record_failure();
                } else {
                    window.record_success();
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(window.total_requests(), num_threads * ops_per_thread);
}
