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

	void SetUp() override { manager = argument_manager(); }

	void VerifyBasicParsing()
	{
		EXPECT_EQ(manager.to_string("--key1"), "value1");
		EXPECT_EQ(manager.to_string("--key2"), "value2");
		EXPECT_EQ(manager.to_string("--non-existent"), std::nullopt);
	}
};

TEST_F(ArgumentManagerTest, ParseStringArguments)
{
	const std::vector<std::string> test_cases
		= { "--key1 value1 --key2 value2", "program --key1 value1 --key2 value2" };

	for (const auto& test_case : test_cases)
	{
		SCOPED_TRACE("Testing with input: " + test_case);

		auto [success, error] = manager.try_parse(test_case);
		ASSERT_TRUE(success) << "Failed with error: "
							 << (error.has_value() ? error.value() : "none");
		ASSERT_FALSE(error.has_value());

		VerifyBasicParsing();
	}
}

TEST_F(ArgumentManagerTest, ParseWStringArguments)
{
	const std::vector<std::wstring> test_cases
		= { L"--key1 value1 --key2 value2", L"program --key1 value1 --key2 value2" };

	for (const auto& test_case : test_cases)
	{
		SCOPED_TRACE("Testing with wide string input");

		auto [success, error] = manager.try_parse(test_case);
		ASSERT_TRUE(success) << "Failed with error: "
							 << (error.has_value() ? error.value() : "none");
		ASSERT_FALSE(error.has_value());

		VerifyBasicParsing();
	}
}

TEST_F(ArgumentManagerTest, ParseCharArgv)
{
	{
		const char* argv[] = { "program", "--key1", "value1", "--key2", "value2" };
		int argc = 5;
		auto [success, error] = manager.try_parse(argc, const_cast<char**>(argv));
		ASSERT_TRUE(success) << "Failed with error: "
							 << (error.has_value() ? error.value() : "none");
		ASSERT_FALSE(error.has_value());
		VerifyBasicParsing();
	}

	{
		const char* argv[] = { "--key1", "value1", "--key2", "value2" };
		int argc = 4;
		auto [success, error] = manager.try_parse(argc, const_cast<char**>(argv));
		ASSERT_TRUE(success) << "Failed with error: "
							 << (error.has_value() ? error.value() : "none");
		ASSERT_FALSE(error.has_value());
		VerifyBasicParsing();
	}
}

TEST_F(ArgumentManagerTest, ParseWCharArgv)
{
	{
		const wchar_t* argv[] = { L"program", L"--key1", L"value1", L"--key2", L"value2" };
		int argc = 5;
		auto [success, error] = manager.try_parse(argc, const_cast<wchar_t**>(argv));
		ASSERT_TRUE(success) << "Failed with error: "
							 << (error.has_value() ? error.value() : "none");
		ASSERT_FALSE(error.has_value());
		VerifyBasicParsing();
	}

	{
		const wchar_t* argv[] = { L"--key1", L"value1", L"--key2", L"value2" };
		int argc = 4;
		auto [success, error] = manager.try_parse(argc, const_cast<wchar_t**>(argv));
		ASSERT_TRUE(success) << "Failed with error: "
							 << (error.has_value() ? error.value() : "none");
		ASSERT_FALSE(error.has_value());
		VerifyBasicParsing();
	}
}

TEST_F(ArgumentManagerTest, ToBool)
{
	const std::vector<std::string> test_cases
		= { "--flag1 true --flag2 false", "--flag1 1 --flag2 0",
			"program --flag1 true --flag2 false" };

	for (const auto& test_case : test_cases)
	{
		SCOPED_TRACE("Testing bool parsing with: " + test_case);

		auto [success, error] = manager.try_parse(test_case);
		ASSERT_TRUE(success) << "Failed with error: "
							 << (error.has_value() ? error.value() : "none");

		EXPECT_EQ(manager.to_bool("--flag1"), true);
		EXPECT_EQ(manager.to_bool("--flag2"), false);
		EXPECT_EQ(manager.to_bool("--non-existent"), std::nullopt);
	}
}

TEST_F(ArgumentManagerTest, ToNumericTypes)
{
	const std::vector<std::string> test_cases
		= { "--int 42 --uint 100 --short -30 --ushort 50 --long 1000000",
			"program --int 42 --uint 100 --short -30 --ushort 50 --long 1000000" };

	for (const auto& test_case : test_cases)
	{
		SCOPED_TRACE("Testing numeric parsing with: " + test_case);

		auto [success, error] = manager.try_parse(test_case);
		ASSERT_TRUE(success) << "Failed with error: "
							 << (error.has_value() ? error.value() : "none");

		EXPECT_EQ(manager.to_int("--int"), 42);
		EXPECT_EQ(manager.to_uint("--uint"), 100u);
		EXPECT_EQ(manager.to_short("--short"), static_cast<short>(-30));
		EXPECT_EQ(manager.to_ushort("--ushort"), static_cast<unsigned short>(50));
#ifdef _WIN32
		EXPECT_EQ(manager.to_llong("--long"), 1000000ll);
#else
		EXPECT_EQ(manager.to_long("--long"), 1000000l);
#endif
	}
}

TEST_F(ArgumentManagerTest, InvalidArguments)
{
	const std::vector<std::pair<std::string, std::string>> test_cases
		= { { "program invalid", "invalid argument: invalid" },
			{ "invalid", "invalid argument: invalid" },
			{ "program --key", "argument '--key' expects a value." },
			{ "--key", "argument '--key' expects a value." } };

	for (const auto& [input, expected_error] : test_cases)
	{
		SCOPED_TRACE("Testing with input: \"" + input + "\"");

		auto [success, error] = manager.try_parse(input);
		ASSERT_FALSE(success) << "Expected to fail with input: " << input;
		ASSERT_TRUE(error.has_value()) << "Expected error message for input: " << input;
		EXPECT_EQ(error.value(), expected_error) << "Input: \"" << input << "\"\n"
												 << "Expected: \"" << expected_error << "\"\n"
												 << "Actual: \"" << error.value() << "\"";
	}
}

TEST_F(ArgumentManagerTest, EmptyArguments)
{
	const std::vector<std::string> test_cases
		= { "", " ", "  ", "\t", "\n", "\r", " \t\n\r", std::string(1, 0) };

	for (const auto& test_case : test_cases)
	{
		SCOPED_TRACE("Testing empty input: '" + test_case + "'");

		auto [success, error] = manager.try_parse(test_case);
		ASSERT_FALSE(success) << "Should fail for empty input";
		ASSERT_TRUE(error.has_value()) << "Should have error message";
		EXPECT_EQ(error.value(), "no valid arguments found.")
			<< "Failed for input: '" << test_case << "'";
	}
}

TEST_F(ArgumentManagerTest, HelpArgument)
{
	const std::vector<std::string> test_cases
		= { "--help", "program --help", "--help --key value", "program --help --key value" };

	for (const auto& test_case : test_cases)
	{
		SCOPED_TRACE("Testing help with: " + test_case);

		auto [success, error] = manager.try_parse(test_case);
		ASSERT_TRUE(success) << "Failed with error: "
							 << (error.has_value() ? error.value() : "none");
		ASSERT_FALSE(error.has_value());

		EXPECT_EQ(manager.to_string("--help"), "display help");
	}
}