// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/*****************************************************************************
BSD 3-Clause License
*****************************************************************************/

#include "gtest/gtest.h"

#include <kcenon/thread/core/typed_thread_pool.h>
#include <kcenon/thread/core/typed_thread_worker.h>
#include <kcenon/thread/core/error_handling.h>

using namespace kcenon::thread;

TEST(typed_thread_pool_error, start_without_workers)
{
    auto pool = std::make_shared<typed_thread_pool>();
    auto r = pool->start();
    ASSERT_TRUE(r.is_err());
    // Starting a pool without workers returns invalid_argument, not thread_start_failure
    // because the validation happens before attempting to start any threads
    EXPECT_EQ(r.error().code, static_cast<int>(error_code::invalid_argument));
}

TEST(typed_thread_pool_error, enqueue_null_worker)
{
    auto pool = std::make_shared<typed_thread_pool>();
    std::unique_ptr<typed_thread_worker> w{};
    auto r = pool->enqueue(std::move(w));
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error().code, static_cast<int>(error_code::invalid_argument));
}

