// BSD 3-Clause License
//
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
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

/*****************************************************************************
BSD 3-Clause License
*****************************************************************************/

#include <iostream>
#include <memory>
#include <thread>
#include <atomic>

#include <kcenon/thread/core/service_registry.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/core/callback_job.h>

using namespace kcenon::thread;
using namespace kcenon::thread;

struct demo_service {
    std::string name;
};

int main() {
    // Register and get a simple service
    auto svc = std::make_shared<demo_service>();
    svc->name = "demo";
    service_registry::register_service<demo_service>(svc);
    auto got = service_registry::get_service<demo_service>();
    std::cout << "service name = " << (got ? got->name : "<null>") << "\n";

    // Use executor_interface via thread_pool
    auto pool = std::make_shared<thread_pool>("svc_pool");
    std::vector<std::unique_ptr<thread_worker>> workers;
    workers.push_back(std::make_unique<thread_worker>(false));
    if (auto r = pool->enqueue_batch(std::move(workers)); r.has_error()) {
        std::cerr << r.get_error().to_string() << "\n";
        return 1;
    }
    if (auto r = pool->start(); r.has_error()) {
        std::cerr << r.get_error().to_string() << "\n";
        return 1;
    }

    std::atomic<int> count{0};
    if (auto r = pool->execute(std::make_unique<callback_job>([&count]() -> result_void {
            count.fetch_add(1);
            return result_void();
        })); r.has_error()) {
        std::cerr << r.get_error().to_string() << "\n";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::cout << "executed jobs = " << count.load() << "\n";
    pool->shutdown();
    return 0;
}
