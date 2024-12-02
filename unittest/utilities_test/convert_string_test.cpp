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

#include <string>
#include <vector>

#include "convert_string.h"

using namespace utility_module;

class ConvertStringTest : public ::testing::Test
{
protected:
	ConvertStringTest() {}
	virtual ~ConvertStringTest() {}
};

TEST_F(ConvertStringTest, ToStringFromWstring)
{
	std::wstring wide = L"Hello, 世界";
	auto [result, error] = convert_string::to_string(wide);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_EQ(result.value(), "Hello, 世界");
}

TEST_F(ConvertStringTest, ToWstringFromString)
{
	std::string utf8 = "Hello, 世界";
	auto [result, error] = convert_string::to_wstring(utf8);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_EQ(result.value(), L"Hello, 世界");
}

TEST_F(ConvertStringTest, ToArrayBasicConversion)
{
	std::string input = "Hello, World!";
	auto [result, error] = convert_string::to_array(input);

	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());

	std::vector<uint8_t> expected(input.begin(), input.end());
	EXPECT_EQ(result.value(), expected);
}

TEST_F(ConvertStringTest, ToArrayWithUTF8BOM)
{
	std::vector<uint8_t> input = { 0xEF, 0xBB, 0xBF, 'H', 'e', 'l', 'l', 'o' };
	std::string input_str(input.begin(), input.end());

	auto [result, error] = convert_string::to_array(input_str);

	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());

	std::vector<uint8_t> expected = { 'H', 'e', 'l', 'l', 'o' };
	EXPECT_EQ(result.value(), expected);
}

TEST_F(ConvertStringTest, ToArrayWithKoreanCharacters)
{
	std::string input = "안녕하세요";
	auto [result, error] = convert_string::to_array(input);

	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());

	std::vector<uint8_t> expected = {
		0xEC, 0x95, 0x88, // 안
		0xEB, 0x85, 0x95, // 녕
		0xED, 0x95, 0x98, // 하
		0xEC, 0x84, 0xB8, // 세
		0xEC, 0x9A, 0x94  // 요
	};
	EXPECT_EQ(result.value(), expected);
}

TEST_F(ConvertStringTest, ToStringBasicConversion)
{
	std::vector<uint8_t> input = { 'H', 'e', 'l', 'l', 'o' };
	auto [result, error] = convert_string::to_string(input);

	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_EQ(result.value(), "Hello");
}

TEST_F(ConvertStringTest, ToStringWithKoreanCharacters)
{
	std::vector<uint8_t> input = {
		0xEC, 0x95, 0x88, // 안
		0xEB, 0x85, 0x95, // 녕
		0xED, 0x95, 0x98, // 하
		0xEC, 0x84, 0xB8, // 세
		0xEC, 0x9A, 0x94  // 요
	};

	auto [result, error] = convert_string::to_string(input);

	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_EQ(result.value(), "안녕하세요");
}

TEST_F(ConvertStringTest, RoundTripConversion)
{
	std::string original = "Hello 안녕하세요 World!";

	auto [array_result, array_error] = convert_string::to_array(original);
	ASSERT_TRUE(array_result.has_value());
	ASSERT_FALSE(array_error.has_value());

	auto [string_result, string_error] = convert_string::to_string(array_result.value());
	ASSERT_TRUE(string_result.has_value());
	ASSERT_FALSE(string_error.has_value());

	EXPECT_EQ(original, string_result.value());
}