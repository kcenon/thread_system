// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#pragma once

// Deprecated: include <kcenon/thread/utils/formatter_macros.h> instead.
// This forwarding header will be removed in the next minor release.

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC warning "utilities/include/formatter_macros.h is deprecated; include <kcenon/thread/utils/formatter_macros.h> instead"
#elif defined(_MSC_VER)
#  pragma message("warning: utilities/include/formatter_macros.h is deprecated; include <kcenon/thread/utils/formatter_macros.h> instead")
#endif

#include <kcenon/thread/utils/formatter_macros.h>
