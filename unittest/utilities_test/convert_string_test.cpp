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

#ifdef _WIN32_BUT_NOT_TESTED
TEST_F(ConvertStringTest, ToStringFromWstring)
{
	std::wstring wide = L"Hello, 世界";
	auto [result, error] = convert_string::to_string(wide);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_EQ(result.value(), "Hello, 世界");
}
#endif

TEST_F(ConvertStringTest, ToStringFromU16string)
{
	std::u16string u16 = u"Hello, 世界";
	auto [result, error] = convert_string::to_string(u16);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_EQ(result.value(), "Hello, 世界");
}

TEST_F(ConvertStringTest, ToStringFromU32string)
{
	std::u32string u32 = U"Hello, 世界";
	auto [result, error] = convert_string::to_string(u32);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_EQ(result.value(), "Hello, 世界");
}

#ifdef _WIN32_BUT_NOT_TESTED
TEST_F(ConvertStringTest, ToWstringFromString)
{
	std::string utf8 = "Hello, 世界";
	auto [result, error] = convert_string::to_wstring(utf8);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_EQ(result.value(), L"Hello, 世界");
}

TEST_F(ConvertStringTest, ToWstringFromU16String)
{
	std::u16string u16 = u"Hello, 世界";
	auto [result, error] = convert_string::to_wstring(u16);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_EQ(result.value(), L"Hello, 世界");
}

TEST_F(ConvertStringTest, ToWstringFromU32String)
{
	std::u32string u32 = U"Hello, 世界";
	auto [result, error] = convert_string::to_wstring(u32);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_EQ(result.value(), L"Hello, 世界");
}
#endif

TEST_F(ConvertStringTest, ToU16stringFromString)
{
	std::string utf8 = "Hello, 世界";
	auto [result, error] = convert_string::to_u16string(utf8);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_EQ(result.value(), u"Hello, 世界");
}

#ifdef _WIN32_BUT_NOT_TESTED
TEST_F(ConvertStringTest, ToU16stringFromWString)
{
	std::wstring wide = L"Hello, 世界";
	auto [result, error] = convert_string::to_u16string(wide);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_EQ(result.value(), u"Hello, 世界");
}
#endif

TEST_F(ConvertStringTest, ToU16stringFromU32String)
{
	std::u32string u32 = U"Hello, 世界";
	auto [result, error] = convert_string::to_u16string(u32);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_EQ(result.value(), u"Hello, 世界");
}

TEST_F(ConvertStringTest, ToU32stringFromString)
{
	std::string utf8 = "Hello, 世界";
	auto [result, error] = convert_string::to_u32string(utf8);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_EQ(result.value(), U"Hello, 世界");
}

#ifdef _WIN32_BUT_NOT_TESTED
TEST_F(ConvertStringTest, ToU32stringFromWString)
{
	std::wstring wide = L"Hello, 世界";
	auto [result, error] = convert_string::to_u32string(wide);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_EQ(result.value(), U"Hello, 世界");
}
#endif

TEST_F(ConvertStringTest, ToU32stringFromU16String)
{
	std::u16string u16 = u"Hello, 世界";
	auto [result, error] = convert_string::to_u32string(u16);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_EQ(result.value(), U"Hello, 世界");
}

TEST_F(ConvertStringTest, ToArrayFromString)
{
	std::string utf8 = "Hello, 世界";
	auto [result, error] = convert_string::to_array(utf8);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	std::vector<uint8_t> expected
		= { 72, 101, 108, 108, 111, 44, 32, 228, 184, 150, 231, 149, 140 };
	EXPECT_EQ(result.value(), expected);
}

TEST_F(ConvertStringTest, ToStringFromArray)
{
	std::vector<uint8_t> bytes = { 72, 101, 108, 108, 111, 44, 32, 228, 184, 150, 231, 149, 140 };
	auto [result, error] = convert_string::to_string(bytes);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_EQ(result.value(), "Hello, 世界");
}

TEST_F(ConvertStringTest, ConvertEmptyU16String)
{
	std::string empty = "";
	auto [result, error] = convert_string::to_u16string(empty);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_TRUE(result.value().empty());
}

TEST_F(ConvertStringTest, ConvertEmptyU32String)
{
	std::string empty = "";
	auto [result, error] = convert_string::to_u32string(empty);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_TRUE(result.value().empty());
}

TEST_F(ConvertStringTest, ConvertStringWithEmoji)
{
	std::string utf8 = "Hello 🌍";
	auto [result, error] = convert_string::to_u32string(utf8);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_EQ(result.value(), U"Hello 🌍");
}

TEST_F(ConvertStringTest, ConvertLargeString)
{
	std::string large_string(1000000, 'a'); // 1 million 'a's
	auto [result, error] = convert_string::to_u16string(large_string);
	ASSERT_TRUE(result.has_value());
	ASSERT_FALSE(error.has_value());
	EXPECT_EQ(result.value().size(), 1000000);
}