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

#include "core/thread_pool.h"

using namespace thread_pool_module;

TEST(thread_pool_test, enqueue)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto error = pool->enqueue(std::move(worker));
	EXPECT_EQ(error, std::nullopt);
}

TEST(thread_pool_test, stop)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto error = pool->enqueue(std::move(worker));
	EXPECT_EQ(error, std::nullopt);

	pool->stop(false);
}

TEST(thread_pool_test, stop_immediately)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto error = pool->enqueue(std::move(worker));
	EXPECT_EQ(error, std::nullopt);

	pool->stop(true);
}

TEST(thread_pool_test, stop_no_workers)
{
	auto pool = std::make_shared<thread_pool>();

	pool->stop(false);
}

TEST(thread_pool_test, start_and_stop)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto error = pool->enqueue(std::move(worker));
	EXPECT_EQ(error, std::nullopt);

	auto start_error = pool->start();
	EXPECT_EQ(start_error, std::nullopt);

	pool->stop(false);
}

TEST(thread_pool_test, start_and_stop_no_worker)
{
	auto pool = std::make_shared<thread_pool>();

	auto start_error = pool->start();
	EXPECT_EQ(start_error, "No workers to start");

	pool->stop(false);
}

TEST(thread_pool_test, start_and_stop_immediately)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto error = pool->enqueue(std::move(worker));
	EXPECT_EQ(error, std::nullopt);

	auto start_error = pool->start();
	EXPECT_EQ(start_error, std::nullopt);

	pool->stop(true);
}

TEST(thread_pool_test, start_and_stop_immediately_no_worker)
{
	auto pool = std::make_shared<thread_pool>();

	auto start_error = pool->start();
	EXPECT_EQ(start_error, "No workers to start");

	pool->stop(true);
}

TEST(thread_pool_test, start_and_one_sec_job_and_stop)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto error = pool->enqueue(std::move(worker));
	EXPECT_EQ(error, std::nullopt);

	auto start_error = pool->start();
	EXPECT_EQ(start_error, std::nullopt);

	error = pool->enqueue(std::make_unique<callback_job>(
		[](void) -> result_void
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));

			return result_void();
		},
		"10sec job"));
	EXPECT_EQ(error, std::nullopt);

	pool->stop(false);
}