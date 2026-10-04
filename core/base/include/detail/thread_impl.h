// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#pragma once

// Deprecated: include <kcenon/thread/core/thread_impl.h> instead.
// This forwarding header will be removed in the next minor release.

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC warning "core/base/include/detail/thread_impl.h is deprecated; include <kcenon/thread/core/thread_impl.h> instead"
#elif defined(_MSC_VER)
#  pragma message("warning: core/base/include/detail/thread_impl.h is deprecated; include <kcenon/thread/core/thread_impl.h> instead")
#endif

#include <kcenon/thread/core/thread_impl.h>
