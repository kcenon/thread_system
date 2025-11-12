// BSD 3-Clause License
//
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/*****************************************************************************
BSD 3-Clause License
*****************************************************************************/

#include "gtest/gtest.h"

#include "thread_pool.h"
#include "callback_job.h"
#include "error_handling.h"

using namespace kcenon::thread;
using namespace kcenon::thread;

TEST(thread_pool_error, start_without_workers)
{
    auto pool = std::make_shared<thread_pool>();
    auto r = pool->start();
    ASSERT_TRUE(r.has_error());
    EXPECT_EQ(r.get_error().code(), error_code::invalid_argument);
}

TEST(thread_pool_error, enqueue_null_job)
{
    auto pool = std::make_shared<thread_pool>();
    std::unique_ptr<job> j{};
    auto r = pool->enqueue(std::move(j));
    ASSERT_TRUE(r.has_error());
    EXPECT_EQ(r.get_error().code(), error_code::invalid_argument);
}

TEST(thread_pool_error, stop_when_not_started)
{
    auto pool = std::make_shared<thread_pool>();
    auto r = pool->stop(false);
    // stop is idempotent; consider success when not running
    EXPECT_FALSE(r.has_error());
}

