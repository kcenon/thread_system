// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#pragma once

// Deprecated: include <kcenon/thread/core/cancellation_token.h> instead.
// This forwarding header will be removed in the next minor release.

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC warning "core/sync/include/cancellation_token.h is deprecated; include <kcenon/thread/core/cancellation_token.h> instead"
#elif defined(_MSC_VER)
#  pragma message("warning: core/sync/include/cancellation_token.h is deprecated; include <kcenon/thread/core/cancellation_token.h> instead")
#endif

#include <kcenon/thread/core/cancellation_token.h>
