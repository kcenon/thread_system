// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#include <kcenon/thread/queue/queue_factory.h>
#include <kcenon/thread/policies/policy_queue.h>

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

    // If lock-free is preferred and no accuracy needs
    // Use adaptive_job_queue with performance_first policy instead of direct lockfree_job_queue
    // (lockfree_job_queue is now an internal implementation detail)
    if (reqs.prefer_lock_free) {
        return std::make_unique<adaptive_job_queue>(adaptive_job_queue::policy::performance_first);
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
    // ARM and other architectures have weaker memory models
    constexpr bool strong_memory_model = false;
#endif

    // ARM and other weak memory model architectures: prefer mutex for safety
    if (!strong_memory_model) {
        return std::make_unique<job_queue>();
    }

    // Low core count: mutex is efficient enough
    const auto core_count = std::thread::hardware_concurrency();
    if (core_count <= 2) {
        return std::make_unique<job_queue>();
    }

    // Default: adaptive for best of both worlds
    return std::make_unique<adaptive_job_queue>();
}

auto queue_factory::create_policy_queue() -> std::unique_ptr<standard_queue>
{
    return std::make_unique<standard_queue>();
}

auto queue_factory::create_lockfree_policy_queue() -> std::unique_ptr<policy_lockfree_queue>
{
    return std::make_unique<policy_lockfree_queue>();
}

auto queue_factory::create_bounded_policy_queue(std::size_t max_size)
    -> std::unique_ptr<policy_queue<
        policies::mutex_sync_policy,
        policies::bounded_policy,
        policies::overflow_reject_policy>>
{
    return std::make_unique<policy_queue<
        policies::mutex_sync_policy,
        policies::bounded_policy,
        policies::overflow_reject_policy>>(policies::bounded_policy(max_size));
}

} // namespace kcenon::thread
