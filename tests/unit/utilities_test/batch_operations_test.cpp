/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, DongCheol Shin
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

#include <future>
#include <string>
#include <vector>

#include <kcenon/thread/utils/batch_operations.h>

using namespace kcenon::thread;

class BatchOperationsTest : public ::testing::Test
{
protected:
	BatchOperationsTest() {}
	virtual ~BatchOperationsTest() {}
};

TEST_F(BatchOperationsTest, BatchApplyWithIntegers)
{
	std::vector<int> numbers = {1, 2, 3, 4, 5};
	auto doubled = detail::batch_apply(std::move(numbers), [](int n) { return n * 2; });

	ASSERT_EQ(doubled.size(), 5);
	EXPECT_EQ(doubled[0], 2);
	EXPECT_EQ(doubled[1], 4);
	EXPECT_EQ(doubled[2], 6);
	EXPECT_EQ(doubled[3], 8);
	EXPECT_EQ(doubled[4], 10);
}

TEST_F(BatchOperationsTest, BatchApplyWithStrings)
{
	std::vector<std::string> words = {"hello", "world"};
	auto lengths = detail::batch_apply(std::move(words), [](std::string s) {
		return s.length();
	});

	ASSERT_EQ(lengths.size(), 2);
	EXPECT_EQ(lengths[0], 5);
	EXPECT_EQ(lengths[1], 5);
}

TEST_F(BatchOperationsTest, BatchApplyWithEmptyVector)
{
	std::vector<int> empty;
	auto result = detail::batch_apply(std::move(empty), [](int n) { return n * 2; });

	EXPECT_TRUE(result.empty());
}

TEST_F(BatchOperationsTest, BatchApplyWithTypeConversion)
{
	std::vector<int> numbers = {1, 2, 3};
	auto strings = detail::batch_apply(std::move(numbers), [](int n) {
		return std::to_string(n);
	});

	ASSERT_EQ(strings.size(), 3);
	EXPECT_EQ(strings[0], "1");
	EXPECT_EQ(strings[1], "2");
	EXPECT_EQ(strings[2], "3");
}

TEST_F(BatchOperationsTest, CollectAllWithIntegers)
{
	std::vector<std::future<int>> futures;
	futures.push_back(std::async(std::launch::deferred, []() { return 1; }));
	futures.push_back(std::async(std::launch::deferred, []() { return 2; }));
	futures.push_back(std::async(std::launch::deferred, []() { return 3; }));

	auto results = detail::collect_all(futures);

	ASSERT_EQ(results.size(), 3);
	EXPECT_EQ(results[0], 1);
	EXPECT_EQ(results[1], 2);
	EXPECT_EQ(results[2], 3);
}

TEST_F(BatchOperationsTest, CollectAllWithStrings)
{
	std::vector<std::future<std::string>> futures;
	futures.push_back(std::async(std::launch::deferred, []() { return std::string("a"); }));
	futures.push_back(std::async(std::launch::deferred, []() { return std::string("b"); }));

	auto results = detail::collect_all(futures);

	ASSERT_EQ(results.size(), 2);
	EXPECT_EQ(results[0], "a");
	EXPECT_EQ(results[1], "b");
}

TEST_F(BatchOperationsTest, CollectAllWithEmptyVector)
{
	std::vector<std::future<int>> futures;
	auto results = detail::collect_all(futures);

	EXPECT_TRUE(results.empty());
}

TEST_F(BatchOperationsTest, CollectAllVoid)
{
	int counter = 0;
	std::vector<std::future<void>> futures;
	futures.push_back(std::async(std::launch::deferred, [&counter]() { ++counter; }));
	futures.push_back(std::async(std::launch::deferred, [&counter]() { ++counter; }));

	detail::collect_all(futures);

	EXPECT_EQ(counter, 2);
}

TEST_F(BatchOperationsTest, BatchApplyPreservesOrder)
{
	std::vector<int> numbers;
	for (int i = 0; i < 100; ++i) {
		numbers.push_back(i);
	}

	auto results = detail::batch_apply(std::move(numbers), [](int n) { return n; });

	ASSERT_EQ(results.size(), 100);
	for (int i = 0; i < 100; ++i) {
		EXPECT_EQ(results[i], i);
	}
}

TEST_F(BatchOperationsTest, CollectAllPreservesOrder)
{
	std::vector<std::future<int>> futures;
	for (int i = 0; i < 100; ++i) {
		futures.push_back(std::async(std::launch::deferred, [i]() { return i; }));
	}

	auto results = detail::collect_all(futures);

	ASSERT_EQ(results.size(), 100);
	for (int i = 0; i < 100; ++i) {
		EXPECT_EQ(results[i], i);
	}
}
