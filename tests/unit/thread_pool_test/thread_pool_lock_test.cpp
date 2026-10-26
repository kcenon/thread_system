// BSD 3-Clause License
// See the LICENSE file in the project root for full license information.

#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/core/thread_pool.h>

#include "../lifecycle_lock_test_helpers.h"

namespace kcenon::thread::test {

TEST(ThreadPoolLifecycleLockTest, StopAfterJobAcquiresCallerMutex) {
    lifecycle_lock_scenario scenario;
    thread_pool pool;
    ASSERT_TRUE(pool.enqueue(std::make_unique<thread_worker>()).is_ok());
    ASSERT_TRUE(pool.enqueue(std::make_unique<callback_job>(
                                 [&scenario]() -> common::VoidResult {
                                     scenario.run_work();
                                     return common::ok();
                                 },
                                 "application_mutex_job"))
                    .is_ok());

    verify_stop_with_application_lock(pool, scenario);
}

}  // namespace kcenon::thread::test
