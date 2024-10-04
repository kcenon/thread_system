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

#include "argument_parser.h"

using namespace utility_module;

class ArgumentManagerTest : public ::testing::Test
{
protected:
	argument_manager manager;
};

TEST_F(ArgumentManagerTest, ParseStringArguments)
{
	auto [success, error] = manager.try_parse("--key1 value1 --key2 value2");
	ASSERT_TRUE(success);
	ASSERT_FALSE(error.has_value());

	EXPECT_EQ(manager.to_string("--key1"), "value1");
	EXPECT_EQ(manager.to_string("--key2"), "value2");
}

TEST_F(ArgumentManagerTest, ParseWStringArguments)
{
	auto [success, error] = manager.try_parse(L"--key1 value1 --key2 value2");
	ASSERT_TRUE(success);
	ASSERT_FALSE(error.has_value());

	EXPECT_EQ(manager.to_string("--key1"), "value1");
	EXPECT_EQ(manager.to_string("--key2"), "value2");
}

TEST_F(ArgumentManagerTest, ParseCharArgv)
{
	const char* argv[] = { "program", "--key1", "value1", "--key2", "value2" };
	int argc = 5;
	auto [success, error] = manager.try_parse(argc, const_cast<char**>(argv));
	ASSERT_TRUE(success);
	ASSERT_FALSE(error.has_value());

	EXPECT_EQ(manager.to_string("--key1"), "value1");
	EXPECT_EQ(manager.to_string("--key2"), "value2");
}

TEST_F(ArgumentManagerTest, ParseWCharArgv)
{
	const wchar_t* argv[] = { L"program", L"--key1", L"value1", L"--key2", L"value2" };
	int argc = 5;
	auto [success, error] = manager.try_parse(argc, const_cast<wchar_t**>(argv));
	ASSERT_TRUE(success);
	ASSERT_FALSE(error.has_value());

	EXPECT_EQ(manager.to_string("--key1"), "value1");
	EXPECT_EQ(manager.to_string("--key2"), "value2");
}

TEST_F(ArgumentManagerTest, ToBool)
{
	manager.try_parse("--flag1 true --flag2 false");

	EXPECT_EQ(manager.to_bool("--flag1"), true);
	EXPECT_EQ(manager.to_bool("--flag2"), false);
	EXPECT_EQ(manager.to_bool("--non-existent"), std::nullopt);
}

TEST_F(ArgumentManagerTest, ToNumericTypes)
{
	manager.try_parse("--int 42 --uint 100 --short -30 --ushort 50 --long 1000000");

	EXPECT_EQ(manager.to_int("--int"), 42);
	EXPECT_EQ(manager.to_uint("--uint"), 100u);
	EXPECT_EQ(manager.to_short("--short"), -30);
	EXPECT_EQ(manager.to_ushort("--ushort"), 50u);
#ifdef _WIN32
	EXPECT_EQ(manager.to_llong("--long"), 1000000ll);
#else
	EXPECT_EQ(manager.to_long("--long"), 1000000l);
#endif
}

TEST_F(ArgumentManagerTest, InvalidArguments)
{
	auto [success, error] = manager.try_parse("invalid arguments");
	ASSERT_FALSE(success);
	ASSERT_TRUE(error.has_value());
	EXPECT_EQ(error.value(), "invalid argument: invalid");
}

TEST_F(ArgumentManagerTest, EmptyArguments)
{
	auto [success, error] = manager.try_parse("");
	ASSERT_FALSE(success);
	ASSERT_TRUE(error.has_value());
}

TEST_F(ArgumentManagerTest, HelpArgument)
{
	auto [success, error] = manager.try_parse("--help");
	ASSERT_TRUE(success);
	ASSERT_FALSE(error.has_value());

	EXPECT_EQ(manager.to_string("--help"), "display help");
}