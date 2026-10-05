#pragma once

#include <cstddef>
#include <kcenon/thread/core/lifecycle_controller.h>
#include <kcenon/thread/core/thread_base.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/utils/atomic_wait.h>
#include <kcenon/thread/utils/span.h>
#include <kcenon/thread/utils/synchronization.h>

// Internal linkage: never let the linker fold the library's implementation into
// the consumer's and conceal a disagreement in preprocessor configuration.
static unsigned feature_snapshot()
{
    unsigned result = 0;
#ifdef USE_STD_JTHREAD
    result |= 1u;
#endif
#ifdef USE_STD_CONCEPTS
    result |= 2u;
#endif
#ifdef HAS_STD_ATOMIC_WAIT
    result |= 4u;
#endif
#ifdef HAS_STD_LATCH
    result |= 8u;
#endif
#ifdef USE_STD_SPAN
    result |= 16u;
#endif
#ifdef BUILD_WITH_COMMON_SYSTEM
    result |= 32u;
#endif
#if KCENON_HAS_COMMON_EXECUTOR
    result |= 64u;
#endif
#ifdef THREAD_WORK_STEALING_ENABLED
    result |= 128u;
#endif
    return result;
}

extern "C" unsigned thread_library_features();
extern "C" std::size_t thread_library_base_size();
extern "C" std::size_t thread_library_lifecycle_size();
extern "C" std::size_t thread_library_worker_size();
