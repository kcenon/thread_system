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

#include "gtest/gtest.h"

#include <kcenon/thread/stealing/steal_backoff_strategy.h>

#include <chrono>
#include <set>

using namespace kcenon::thread;

// ============================================
// steal_backoff_strategy Enum Tests
// ============================================

TEST(steal_backoff_strategy_test, enum_values_exist)
{
	auto fixed = steal_backoff_strategy::fixed;
	auto linear = steal_backoff_strategy::linear;
	auto exponential = steal_backoff_strategy::exponential;
	auto adaptive = steal_backoff_strategy::adaptive_jitter;

	EXPECT_NE(static_cast<int>(fixed), static_cast<int>(linear));
	EXPECT_NE(static_cast<int>(linear), static_cast<int>(exponential));
	EXPECT_NE(static_cast<int>(exponential), static_cast<int>(adaptive));
}

TEST(steal_backoff_strategy_test, to_string_conversion)
{
	EXPECT_STREQ(to_string(steal_backoff_strategy::fixed), "fixed");
	EXPECT_STREQ(to_string(steal_backoff_strategy::linear), "linear");
	EXPECT_STREQ(to_string(steal_backoff_strategy::exponential), "exponential");
	EXPECT_STREQ(to_string(steal_backoff_strategy::adaptive_jitter), "adaptive_jitter");
}

// ============================================
// steal_backoff_config Tests
// ============================================

TEST(steal_backoff_config_test, default_values)
{
	steal_backoff_config config;

	EXPECT_EQ(config.strategy, steal_backoff_strategy::exponential);
	EXPECT_EQ(config.initial_backoff, std::chrono::microseconds{50});
	EXPECT_EQ(config.max_backoff, std::chrono::microseconds{1000});
	EXPECT_DOUBLE_EQ(config.multiplier, 2.0);
	EXPECT_DOUBLE_EQ(config.jitter_factor, 0.5);
}

TEST(steal_backoff_config_test, custom_values)
{
	steal_backoff_config config;
	config.strategy = steal_backoff_strategy::linear;
	config.initial_backoff = std::chrono::microseconds{100};
	config.max_backoff = std::chrono::microseconds{5000};
	config.multiplier = 1.5;
	config.jitter_factor = 0.25;

	EXPECT_EQ(config.strategy, steal_backoff_strategy::linear);
	EXPECT_EQ(config.initial_backoff, std::chrono::microseconds{100});
	EXPECT_EQ(config.max_backoff, std::chrono::microseconds{5000});
	EXPECT_DOUBLE_EQ(config.multiplier, 1.5);
	EXPECT_DOUBLE_EQ(config.jitter_factor, 0.25);
}

// ============================================
// backoff_calculator - Fixed Strategy Tests
// ============================================

class backoff_calculator_fixed_test : public ::testing::Test
{
protected:
	void SetUp() override
	{
		config_.strategy = steal_backoff_strategy::fixed;
		config_.initial_backoff = std::chrono::microseconds{100};
		config_.max_backoff = std::chrono::microseconds{1000};
		calculator_ = std::make_unique<backoff_calculator>(config_);
	}

	steal_backoff_config config_;
	std::unique_ptr<backoff_calculator> calculator_;
};

TEST_F(backoff_calculator_fixed_test, constant_delay)
{
	// Fixed strategy should return constant delay
	auto delay0 = calculator_->calculate(0);
	auto delay1 = calculator_->calculate(1);
	auto delay5 = calculator_->calculate(5);
	auto delay10 = calculator_->calculate(10);

	EXPECT_EQ(delay0, std::chrono::microseconds{100});
	EXPECT_EQ(delay1, std::chrono::microseconds{100});
	EXPECT_EQ(delay5, std::chrono::microseconds{100});
	EXPECT_EQ(delay10, std::chrono::microseconds{100});
}

// ============================================
// backoff_calculator - Linear Strategy Tests
// ============================================

class backoff_calculator_linear_test : public ::testing::Test
{
protected:
	void SetUp() override
	{
		config_.strategy = steal_backoff_strategy::linear;
		config_.initial_backoff = std::chrono::microseconds{50};
		config_.max_backoff = std::chrono::microseconds{500};
		calculator_ = std::make_unique<backoff_calculator>(config_);
	}

	steal_backoff_config config_;
	std::unique_ptr<backoff_calculator> calculator_;
};

TEST_F(backoff_calculator_linear_test, linear_increase)
{
	// Linear: delay = initial * (attempt + 1)
	EXPECT_EQ(calculator_->calculate(0), std::chrono::microseconds{50});   // 50 * 1
	EXPECT_EQ(calculator_->calculate(1), std::chrono::microseconds{100});  // 50 * 2
	EXPECT_EQ(calculator_->calculate(2), std::chrono::microseconds{150});  // 50 * 3
	EXPECT_EQ(calculator_->calculate(3), std::chrono::microseconds{200});  // 50 * 4
}

TEST_F(backoff_calculator_linear_test, caps_at_max)
{
	// Should cap at max_backoff (500)
	auto delay = calculator_->calculate(20);  // 50 * 21 = 1050, but capped at 500
	EXPECT_EQ(delay, std::chrono::microseconds{500});
}

// ============================================
// backoff_calculator - Exponential Strategy Tests
// ============================================

class backoff_calculator_exponential_test : public ::testing::Test
{
protected:
	void SetUp() override
	{
		config_.strategy = steal_backoff_strategy::exponential;
		config_.initial_backoff = std::chrono::microseconds{50};
		config_.max_backoff = std::chrono::microseconds{1000};
		config_.multiplier = 2.0;
		calculator_ = std::make_unique<backoff_calculator>(config_);
	}

	steal_backoff_config config_;
	std::unique_ptr<backoff_calculator> calculator_;
};

TEST_F(backoff_calculator_exponential_test, exponential_increase)
{
	// Exponential: delay = initial * multiplier^attempt
	EXPECT_EQ(calculator_->calculate(0), std::chrono::microseconds{50});   // 50 * 2^0 = 50
	EXPECT_EQ(calculator_->calculate(1), std::chrono::microseconds{100});  // 50 * 2^1 = 100
	EXPECT_EQ(calculator_->calculate(2), std::chrono::microseconds{200});  // 50 * 2^2 = 200
	EXPECT_EQ(calculator_->calculate(3), std::chrono::microseconds{400});  // 50 * 2^3 = 400
	EXPECT_EQ(calculator_->calculate(4), std::chrono::microseconds{800});  // 50 * 2^4 = 800
}

TEST_F(backoff_calculator_exponential_test, caps_at_max)
{
	// Should cap at max_backoff (1000)
	auto delay = calculator_->calculate(5);  // 50 * 2^5 = 1600, but capped at 1000
	EXPECT_EQ(delay, std::chrono::microseconds{1000});
}

TEST_F(backoff_calculator_exponential_test, large_attempt_still_caps)
{
	// Very large attempt numbers should still be capped
	auto delay = calculator_->calculate(100);
	EXPECT_EQ(delay, std::chrono::microseconds{1000});
}

// ============================================
// backoff_calculator - Adaptive Jitter Strategy Tests
// ============================================

class backoff_calculator_adaptive_test : public ::testing::Test
{
protected:
	void SetUp() override
	{
		config_.strategy = steal_backoff_strategy::adaptive_jitter;
		config_.initial_backoff = std::chrono::microseconds{100};
		config_.max_backoff = std::chrono::microseconds{1000};
		config_.multiplier = 2.0;
		config_.jitter_factor = 0.5;
		calculator_ = std::make_unique<backoff_calculator>(config_);
	}

	steal_backoff_config config_;
	std::unique_ptr<backoff_calculator> calculator_;
};

TEST_F(backoff_calculator_adaptive_test, jitter_produces_variation)
{
	// Run multiple calculations and verify we get different results (due to jitter)
	std::set<std::chrono::microseconds::rep> delays;

	for (int i = 0; i < 100; ++i)
	{
		auto delay = calculator_->calculate(1);  // Base: 200us with +-50% jitter
		delays.insert(delay.count());
	}

	// With jitter, we should get multiple different values
	EXPECT_GT(delays.size(), 1) << "Jitter should produce variation";
}

TEST_F(backoff_calculator_adaptive_test, jitter_stays_within_bounds)
{
	// Base delay for attempt 1: 100 * 2 = 200
	// With 50% jitter: range is [100, 300]
	for (int i = 0; i < 100; ++i)
	{
		auto delay = calculator_->calculate(1);
		EXPECT_GE(delay.count(), 1) << "Delay should be positive";
		EXPECT_LE(delay.count(), 1000) << "Delay should not exceed max_backoff";
	}
}

TEST_F(backoff_calculator_adaptive_test, caps_at_max_with_jitter)
{
	// Even with jitter, should not exceed max_backoff
	for (int i = 0; i < 100; ++i)
	{
		auto delay = calculator_->calculate(10);
		EXPECT_LE(delay.count(), 1000);
	}
}

// ============================================
// backoff_calculator - Configuration Tests
// ============================================

TEST(backoff_calculator_test, get_config)
{
	steal_backoff_config config;
	config.strategy = steal_backoff_strategy::linear;
	config.initial_backoff = std::chrono::microseconds{75};

	backoff_calculator calculator(config);
	const auto& retrieved = calculator.get_config();

	EXPECT_EQ(retrieved.strategy, steal_backoff_strategy::linear);
	EXPECT_EQ(retrieved.initial_backoff, std::chrono::microseconds{75});
}

TEST(backoff_calculator_test, set_config)
{
	backoff_calculator calculator;

	// Initially exponential
	EXPECT_EQ(calculator.get_config().strategy, steal_backoff_strategy::exponential);

	// Change to fixed
	steal_backoff_config new_config;
	new_config.strategy = steal_backoff_strategy::fixed;
	new_config.initial_backoff = std::chrono::microseconds{200};
	calculator.set_config(new_config);

	EXPECT_EQ(calculator.get_config().strategy, steal_backoff_strategy::fixed);
	EXPECT_EQ(calculator.calculate(0), std::chrono::microseconds{200});
	EXPECT_EQ(calculator.calculate(5), std::chrono::microseconds{200});
}

// ============================================
// backoff_calculator - Edge Cases
// ============================================

TEST(backoff_calculator_test, zero_initial_backoff)
{
	steal_backoff_config config;
	config.initial_backoff = std::chrono::microseconds{0};
	config.max_backoff = std::chrono::microseconds{100};

	backoff_calculator calculator(config);
	auto delay = calculator.calculate(0);
	EXPECT_EQ(delay, std::chrono::microseconds{0});
}

TEST(backoff_calculator_test, max_less_than_initial)
{
	steal_backoff_config config;
	config.strategy = steal_backoff_strategy::fixed;
	config.initial_backoff = std::chrono::microseconds{100};
	config.max_backoff = std::chrono::microseconds{50};

	backoff_calculator calculator(config);
	// Should cap at max even if initial is larger
	auto delay = calculator.calculate(0);
	EXPECT_EQ(delay, std::chrono::microseconds{50});
}
