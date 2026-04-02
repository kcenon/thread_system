// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/*****************************************************************************
BSD 3-Clause License
*****************************************************************************/

#include "gtest/gtest.h"

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/core/error_handling.h>

using namespace kcenon::thread;

TEST(thread_pool_error, start_without_workers)
{
    auto pool = std::make_shared<thread_pool>();
    auto r = pool->start();
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error().code, static_cast<int>(error_code::invalid_argument));
}

TEST(thread_pool_error, enqueue_null_job)
{
    auto pool = std::make_shared<thread_pool>();
    std::unique_ptr<job> j{};
    auto r = pool->enqueue(std::move(j));
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error().code, static_cast<int>(error_code::invalid_argument));
}

TEST(thread_pool_error, stop_when_not_started)
{
    auto pool = std::make_shared<thread_pool>();
    auto r = pool->stop(false);
    // stop is idempotent; consider success when not running
    EXPECT_FALSE(r.is_err());
}

