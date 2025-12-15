// BSD 3-Clause License
//
// Copyright (c) 2021-2025, kcenon
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

/**
 * @file thread_logger_init.cpp
 * @brief Early initialization of thread_logger shutdown handler for SDOF prevention
 *
 * This file ensures that the thread_logger's atexit handler is registered as early
 * as possible during program initialization, before any user-defined static objects
 * are constructed. This prevents Static Destruction Order Fiasco (SDOF) by ensuring
 * that is_shutting_down() returns true before any static object destructors run.
 *
 * The Problem:
 * - atexit handlers are called in LIFO order (reverse registration order)
 * - If atexit(prepare_shutdown) is registered after static object A is created,
 *   then A's destructor runs AFTER prepare_shutdown(), not before
 * - This means A's destructor may call thread_pool methods while is_shutting_down_
 *   is still false, leading to SDOF issues
 *
 * The Solution:
 * - Register the atexit handler during early program initialization
 * - On GCC/Clang: Use __attribute__((constructor)) with high priority
 * - On MSVC: Use CRT initialization section pragma
 * - This ensures the handler is registered before any user static objects,
 *   so prepare_shutdown() runs AFTER all user destructors complete
 *
 * Related Issues:
 * - GitHub issue #297: Improve atexit handler registration timing for SDOF prevention
 * - Related: #295, #296 (initial SDOF prevention)
 *
 * @see thread_logger::prepare_shutdown()
 * @see thread_logger::is_shutting_down()
 */

#include <kcenon/thread/core/thread_logger.h>

namespace {

/**
 * @brief Register the shutdown handler during early program initialization
 *
 * This function is called automatically before main() and before most
 * user-defined static object constructors. It registers the prepare_shutdown
 * handler with atexit, ensuring proper SDOF protection.
 */
void register_shutdown_handler() {
    // Register the shutdown handler
    // Since this runs early, it will be called late during shutdown (LIFO order)
    // This ensures is_shutting_down() returns true during static destruction
    std::atexit(kcenon::thread::thread_logger::prepare_shutdown);
}

#if defined(__GNUC__) || defined(__clang__)
// GCC/Clang: Use constructor attribute with priority 101
// Priority 101 runs after CRT initialization (65535-100) but before
// user constructors (default priority is 65535)
// Lower numbers = earlier execution, so 101 is very early
__attribute__((constructor(101)))
static void early_register_shutdown_handler() {
    register_shutdown_handler();
}

#elif defined(_MSC_VER)
// MSVC: Use CRT initialization section
// .CRT$XCU runs during C++ static initialization, early in the process

#pragma section(".CRT$XCU", read)

// Function pointer that will be called during CRT initialization
static void __cdecl msvc_register_shutdown_handler() {
    register_shutdown_handler();
}

// Place the function pointer in the .CRT$XCU section
// This causes it to be called during CRT initialization
__declspec(allocate(".CRT$XCU"))
static void (__cdecl *p_msvc_register_shutdown_handler)() = msvc_register_shutdown_handler;

#else
// Fallback for other compilers: Use static initialization
// This is less reliable for initialization order but better than nothing
namespace {
struct ShutdownHandlerRegistrar {
    ShutdownHandlerRegistrar() {
        register_shutdown_handler();
    }
};
static ShutdownHandlerRegistrar shutdown_handler_registrar;
}
#endif

} // anonymous namespace
