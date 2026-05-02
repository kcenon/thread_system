// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#pragma once

// Deprecated: include <kcenon/thread/core/sync_primitives.h> instead.
// This forwarding header will be removed in the next minor release.

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC warning "core/sync/include/sync_primitives.h is deprecated; include <kcenon/thread/core/sync_primitives.h> instead"
#elif defined(_MSC_VER)
#  pragma message("warning: core/sync/include/sync_primitives.h is deprecated; include <kcenon/thread/core/sync_primitives.h> instead")
#endif

#include <kcenon/thread/core/sync_primitives.h>
