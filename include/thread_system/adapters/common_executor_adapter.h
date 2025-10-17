/*
 * BSD 3-Clause License
 * Copyright (c) 2025, thread_system contributors
 */

#pragma once

#ifdef BUILD_WITH_COMMON_SYSTEM
#include <kcenon/thread/adapters/common_executor_adapter.h>

namespace thread_system {
namespace adapters {

using thread_pool_executor_adapter = kcenon::thread::adapters::thread_pool_executor_adapter;
using common_executor_factory = kcenon::thread::adapters::common_executor_factory;

} // namespace adapters
} // namespace thread_system

#endif // BUILD_WITH_COMMON_SYSTEM

