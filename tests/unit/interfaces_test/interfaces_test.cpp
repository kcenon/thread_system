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

#include <gtest/gtest.h>

#include <kcenon/thread/interfaces/scheduler_interface.h>
#include <kcenon/common/interfaces/monitoring_interface.h>

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/job_queue.h>
#include <kcenon/thread/core/callback_job.h>

#include <kcenon/thread/core/service_registry.h>

using namespace kcenon::thread;

// Support both old (namespace common) and new (namespace kcenon::common) versions
// At global scope, need to use kcenon::common directly
namespace common_test = kcenon::common;

TEST(interfaces_test, scheduler_interface_job_queue)
{
    job_queue queue;

    std::atomic<int> count{0};
    auto r1 = queue.schedule(std::make_unique<callback_job>([&count]() -> kcenon::common::VoidResult {
        count.fetch_add(1);
        return kcenon::common::ok();
    }));
    EXPECT_TRUE(r1.is_ok());

    auto j = queue.get_next_job();
    ASSERT_TRUE(j.is_ok());
    auto res = j.value()->do_work();
    (void)res;
    EXPECT_EQ(count.load(), 1);
}

TEST(interfaces_test, thread_pool_execute)
{
    auto pool = std::make_shared<thread_pool>("ifx_pool");
    // add one worker
    std::vector<std::unique_ptr<thread_worker>> workers;
    {
        auto w = std::make_unique<thread_worker>(false);
        w->set_wake_interval(std::chrono::milliseconds(10));
        workers.push_back(std::move(w));
    }
    ASSERT_TRUE(pool->enqueue_batch(std::move(workers)).is_ok());

    std::atomic<int> count{0};
    // Use thread_pool's enqueue method directly instead of deprecated executor_interface
    auto r = pool->enqueue(std::make_unique<callback_job>([&count]() -> kcenon::common::VoidResult {
        count.fetch_add(1);
        return kcenon::common::ok();
    }));
    ASSERT_TRUE(r.is_ok());

    // Start after enqueue so worker picks up existing job
    ASSERT_TRUE(pool->start().is_ok());

    auto start = std::chrono::steady_clock::now();
    while (count.load() < 1 && std::chrono::steady_clock::now() - start < std::chrono::seconds(2)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_EQ(count.load(), 1);
    EXPECT_TRUE(pool->stop().is_ok());
}

namespace {
// Test implementation of common::interfaces::IMonitorable
class dummy_monitorable : public common_test::interfaces::IMonitorable {
public:
    common_test::Result<common_test::interfaces::metrics_snapshot> get_monitoring_data() override {
        return common_test::ok(snapshot_);
    }

    common_test::Result<common_test::interfaces::health_check_result> health_check() override {
        common_test::interfaces::health_check_result result;
        result.status = common_test::interfaces::health_status::healthy;
        result.message = "OK";
        return common_test::ok(result);
    }

    std::string get_component_name() const override {
        return "dummy_monitorable";
    }

private:
    common_test::interfaces::metrics_snapshot snapshot_{};
};
}

TEST(interfaces_test, monitorable_interface_mock)
{
    dummy_monitorable m;
    auto data_result = m.get_monitoring_data();
    ASSERT_TRUE(data_result.is_ok());

    auto health_result = m.health_check();
    ASSERT_TRUE(health_result.is_ok());
    EXPECT_EQ(health_result.value().status, common_test::interfaces::health_status::healthy);

    EXPECT_EQ(m.get_component_name(), "dummy_monitorable");
}

TEST(interfaces_test, service_registry_basic)
{
    struct foo { int v{0}; };
    auto f = std::make_shared<foo>();
    f->v = 42;
    service_registry::register_service<foo>(f);
    auto r = service_registry::get_service<foo>();
    ASSERT_TRUE(r != nullptr);
    EXPECT_EQ(r->v, 42);
}
