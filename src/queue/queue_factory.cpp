/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, DongCheol Shin
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#include <kcenon/thread/queue/queue_factory.h>

#include <thread>

namespace kcenon::thread {

auto queue_factory::create_for_requirements(const requirements& reqs)
    -> std::unique_ptr<scheduler_interface>
{
    // If accuracy is required, use mutex-based queue
    if (reqs.need_exact_size || reqs.need_atomic_empty ||
        reqs.need_batch_operations || reqs.need_blocking_wait) {
        return std::make_unique<job_queue>();
    }

    // If lock-free is preferred and no accuracy requirements
    if (reqs.prefer_lock_free) {
        return std::make_unique<lockfree_job_queue>();
    }

    // Default: adaptive queue for flexibility
    return std::make_unique<adaptive_job_queue>();
}

auto queue_factory::create_optimal() -> std::unique_ptr<scheduler_interface>
{
    // Check architecture for memory model strength
    #if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
        constexpr bool strong_memory_model = true;
    #else
        // ARM, RISC-V, and other architectures have weaker memory models
        constexpr bool strong_memory_model = false;
    #endif

    // ARM and other weak memory model architectures: prefer mutex for safety
    // Lock-free code is more prone to subtle bugs on weak memory models
    if (!strong_memory_model) {
        return std::make_unique<job_queue>();
    }

    // Get hardware concurrency (number of logical CPUs)
    unsigned int core_count = std::thread::hardware_concurrency();

    // If hardware_concurrency() returns 0, it means detection failed
    // Default to conservative choice (mutex-based)
    if (core_count == 0) {
        return std::make_unique<job_queue>();
    }

    // Low core count: mutex-based queue is efficient enough
    // Lock-free overhead may not be worth it with few cores
    if (core_count <= 2) {
        return std::make_unique<job_queue>();
    }

    // Default: adaptive queue provides best of both worlds
    // - Automatically adjusts based on runtime conditions
    // - Can switch to accurate mode when needed
    // - Falls back to mutex mode for safety when appropriate
    return std::make_unique<adaptive_job_queue>();
}

} // namespace kcenon::thread
