/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, kcenon
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

#include <kcenon/thread/pool_policies/work_stealing_pool_policy.h>
#include <kcenon/thread/core/job.h>

namespace kcenon::thread
{

work_stealing_pool_policy::work_stealing_pool_policy(const worker_policy& config)
    : policy_(config)
    , enabled_(config.enable_work_stealing)
{
}

auto work_stealing_pool_policy::on_enqueue(job& j) -> common::VoidResult
{
    (void)j;  // Job not used, work-stealing doesn't affect enqueue
    return common::ok();
}

void work_stealing_pool_policy::on_job_start(job& j)
{
    (void)j;  // Currently no action needed on job start
}

void work_stealing_pool_policy::on_job_complete(job& j, bool success, const std::exception* error)
{
    (void)j;      // Job not used for work-stealing statistics
    (void)success;
    (void)error;
    // Statistics are tracked through record_successful_steal/record_failed_steal
}

auto work_stealing_pool_policy::get_name() const -> std::string
{
    return "work_stealing_pool_policy";
}

auto work_stealing_pool_policy::is_enabled() const -> bool
{
    return enabled_.load(std::memory_order_acquire);
}

void work_stealing_pool_policy::set_enabled(bool enabled)
{
    enabled_.store(enabled, std::memory_order_release);
    policy_.enable_work_stealing = enabled;
}

auto work_stealing_pool_policy::get_policy() const -> const worker_policy&
{
    return policy_;
}

void work_stealing_pool_policy::set_policy(const worker_policy& config)
{
    policy_ = config;
    enabled_.store(config.enable_work_stealing, std::memory_order_release);
}

auto work_stealing_pool_policy::get_steal_policy() const -> steal_policy
{
    return policy_.victim_selection;
}

void work_stealing_pool_policy::set_steal_policy(steal_policy policy)
{
    policy_.victim_selection = policy;
}

auto work_stealing_pool_policy::get_max_steal_attempts() const -> std::size_t
{
    return policy_.max_steal_attempts;
}

void work_stealing_pool_policy::set_max_steal_attempts(std::size_t attempts)
{
    policy_.max_steal_attempts = attempts;
}

auto work_stealing_pool_policy::get_steal_backoff() const -> std::chrono::microseconds
{
    return policy_.steal_backoff;
}

void work_stealing_pool_policy::set_steal_backoff(std::chrono::microseconds backoff)
{
    policy_.steal_backoff = backoff;
}

auto work_stealing_pool_policy::get_successful_steals() const -> std::uint64_t
{
    return successful_steals_.load(std::memory_order_acquire);
}

auto work_stealing_pool_policy::get_failed_steals() const -> std::uint64_t
{
    return failed_steals_.load(std::memory_order_acquire);
}

void work_stealing_pool_policy::reset_stats()
{
    successful_steals_.store(0, std::memory_order_release);
    failed_steals_.store(0, std::memory_order_release);
}

void work_stealing_pool_policy::record_successful_steal()
{
    successful_steals_.fetch_add(1, std::memory_order_relaxed);
}

void work_stealing_pool_policy::record_failed_steal()
{
    failed_steals_.fetch_add(1, std::memory_order_relaxed);
}

} // namespace kcenon::thread
