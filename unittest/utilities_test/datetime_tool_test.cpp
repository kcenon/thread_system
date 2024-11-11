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

#include "datetime_tool.h"

#include <regex>
#include <thread>

using namespace utility_module;

class DateTimeToolTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		// 2024-03-15 14:30:45.123456789
		fixed_time = std::chrono::system_clock::from_time_t(1710507045); // 2024-03-15 14:30:45 UTC

		// Add microseconds
		fixed_time += std::chrono::microseconds(123456);

		// Store nanoseconds separately since system_clock precision is microseconds
		nano_portion = 789;
	}

	std::chrono::system_clock::time_point fixed_time;
	int nano_portion; // Store nanoseconds separately
};

TEST_F(DateTimeToolTest, DateFormatTest)
{
	// Test with different separators
	EXPECT_EQ(datetime_tool::date(fixed_time, "-"), "2024-03-15");
	EXPECT_EQ(datetime_tool::date(fixed_time, "/"), "2024/03/15");
	EXPECT_EQ(datetime_tool::date(fixed_time, ""), "20240315");
}

TEST_F(DateTimeToolTest, TimeFormatTest)
{
	// Note: The exact time might vary depending on timezone
	// Test with different separators
	const auto result1 = datetime_tool::time(fixed_time, ":");
	const auto result2 = datetime_tool::time(fixed_time, "-");
	const auto result3 = datetime_tool::time(fixed_time, "");

	// Verify format pattern (HH:MM:SS)
	EXPECT_TRUE(std::regex_match(result1, std::regex("\\d{2}:\\d{2}:\\d{2}")));
	EXPECT_TRUE(std::regex_match(result2, std::regex("\\d{2}-\\d{2}-\\d{2}")));
	EXPECT_TRUE(std::regex_match(result3, std::regex("\\d{6}")));
}

TEST_F(DateTimeToolTest, MillisecondsTest)
{
	const auto result = datetime_tool::milliseconds(fixed_time);
	EXPECT_EQ(result.length(), 3);
	EXPECT_EQ(result, "123");
}

TEST_F(DateTimeToolTest, MicrosecondsTest)
{
	const auto result = datetime_tool::microseconds(fixed_time);
	EXPECT_EQ(result.length(), 3);
	EXPECT_EQ(result, "456");
}

TEST_F(DateTimeToolTest, NanosecondsTest)
{
	// Test nanoseconds portion separately
	const auto result = datetime_tool::nanoseconds(fixed_time, nano_portion);
	EXPECT_EQ(result.length(), 3);
	EXPECT_EQ(result, "789");
}

TEST_F(DateTimeToolTest, TimeDifferenceTest)
{
	using namespace std::chrono;

	auto start = high_resolution_clock::now();
	auto end = start + milliseconds(100); // 명시적으로 100ms 차이를 만듦

	double diff = datetime_tool::time_difference<milliseconds>(start, end);

	EXPECT_DOUBLE_EQ(diff, 100.0);
}

TEST_F(DateTimeToolTest, TimeDifferenceRealTest)
{
	using namespace std::chrono;

	auto start = high_resolution_clock::now();
	std::this_thread::sleep_for(milliseconds(100));
	auto end = high_resolution_clock::now();

	double diff = datetime_tool::time_difference<milliseconds>(start, end);

	EXPECT_GE(diff, 95.0);	// 허용 오차 범위 확대
	EXPECT_LE(diff, 150.0); // 상한값 증가
}

TEST_F(DateTimeToolTest, EdgeCasesTest)
{
	// Test with epoch time
	auto epoch = std::chrono::system_clock::from_time_t(0);
	EXPECT_EQ(datetime_tool::date(epoch, "-"), "1970-01-01");

	// Test with future date
	auto future = std::chrono::system_clock::from_time_t(2145916800); // 2038-01-01
	EXPECT_EQ(datetime_tool::date(future, "-"), "2038-01-01");
}

TEST_F(DateTimeToolTest, InvalidSeparatorTest)
{
	// Test with multi-character separator
	EXPECT_NO_THROW(datetime_tool::date(fixed_time, "##"));
	EXPECT_NO_THROW(datetime_tool::time(fixed_time, "##"));
}