// BSD 3-Clause License
// See the LICENSE file in the project root for full license information.

#include <kcenon/thread/core/thread_base.h>

#include <optional>

#include "../lifecycle_lock_test_helpers.h"

namespace kcenon::thread::test {
namespace {

class lock_test_worker : public thread_base {
public:
    explicit lock_test_worker(lifecycle_lock_scenario& scenario)
        : thread_base("lifecycle_lock_test"), scenario_(scenario) {}

    lock_test_worker(const lock_test_worker&) = delete;
    auto operator=(const lock_test_worker&) -> lock_test_worker& = delete;
    lock_test_worker(lock_test_worker&&) = delete;
    auto operator=(lock_test_worker&&) -> lock_test_worker& = delete;

    ~lock_test_worker() override {
        scenario_.allow_completion.store(true, std::memory_order_release);
        stop();
    }

protected:
    auto should_continue_work() const -> bool override {
        return pending_.load();
    }

    auto do_work() -> common::VoidResult override {
        if (pending_.exchange(false)) {
            scenario_.run_work();
        }
        return common::ok();
    }

private:
    lifecycle_lock_scenario& scenario_;
    std::atomic<bool> pending_{true};
};

class thread_base_lifecycle_lock_test : public ::testing::TestWithParam<bool> {};

TEST_P(thread_base_lifecycle_lock_test, StopAfterWorkAcquiresCallerMutex) {
    lifecycle_lock_scenario scenario;
    lock_test_worker worker(scenario);
    worker.set_wake_interval(GetParam() ? std::make_optional(std::chrono::milliseconds(1))
                                        : std::nullopt);
    verify_stop_with_application_lock(worker, scenario);
}

INSTANTIATE_TEST_SUITE_P(WaitStrategies, thread_base_lifecycle_lock_test, ::testing::Bool(),
                         [](const ::testing::TestParamInfo<bool>& info) {
                             return info.param ? "TimedWait" : "IndefiniteWait";
                         });

}  // namespace
}  // namespace kcenon::thread::test
