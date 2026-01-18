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

#include <kcenon/thread/pool_policies/autoscaling_pool_policy.h>
#include <kcenon/thread/core/job.h>

namespace kcenon::thread
{

autoscaling_pool_policy::autoscaling_pool_policy(thread_pool& pool, const autoscaling_policy& config)
    : autoscaler_(std::make_shared<autoscaler>(pool, config))
{
}

autoscaling_pool_policy::autoscaling_pool_policy(std::shared_ptr<autoscaler> scaler)
    : autoscaler_(std::move(scaler))
{
}

autoscaling_pool_policy::~autoscaling_pool_policy()
{
    if (autoscaler_ && autoscaler_->is_active()) {
        autoscaler_->stop();
    }
}

auto autoscaling_pool_policy::on_enqueue(job& j) -> common::VoidResult
{
    (void)j;  // Autoscaling does not reject jobs
    return common::ok();
}

void autoscaling_pool_policy::on_job_start(job& j)
{
    (void)j;  // Metrics are collected by the autoscaler's monitor thread
}

void autoscaling_pool_policy::on_job_complete(job& j, bool success, const std::exception* error)
{
    (void)j;
    (void)success;
    (void)error;
    // Metrics are collected by the autoscaler's monitor thread
}

auto autoscaling_pool_policy::get_name() const -> std::string
{
    return "autoscaling_pool_policy";
}

auto autoscaling_pool_policy::is_enabled() const -> bool
{
    return enabled_.load(std::memory_order_acquire);
}

void autoscaling_pool_policy::set_enabled(bool enabled)
{
    bool was_enabled = enabled_.exchange(enabled, std::memory_order_acq_rel);

    if (autoscaler_) {
        if (enabled && !was_enabled) {
            // Transitioning to enabled - start the autoscaler
            if (!autoscaler_->is_active()) {
                autoscaler_->start();
            }
        } else if (!enabled && was_enabled) {
            // Transitioning to disabled - stop the autoscaler
            if (autoscaler_->is_active()) {
                autoscaler_->stop();
            }
        }
    }
}

void autoscaling_pool_policy::start()
{
    if (autoscaler_ && enabled_.load(std::memory_order_acquire)) {
        autoscaler_->start();
    }
}

void autoscaling_pool_policy::stop()
{
    if (autoscaler_) {
        autoscaler_->stop();
    }
}

auto autoscaling_pool_policy::is_active() const -> bool
{
    return autoscaler_ && autoscaler_->is_active();
}

auto autoscaling_pool_policy::get_autoscaler() const -> std::shared_ptr<autoscaler>
{
    return autoscaler_;
}

auto autoscaling_pool_policy::get_stats() const -> autoscaling_stats
{
    if (autoscaler_) {
        return autoscaler_->get_stats();
    }
    return autoscaling_stats{};
}

void autoscaling_pool_policy::set_policy(const autoscaling_policy& config)
{
    if (autoscaler_) {
        autoscaler_->set_policy(config);
    }
}

auto autoscaling_pool_policy::get_policy() const -> const autoscaling_policy&
{
    static autoscaling_policy default_policy;
    if (autoscaler_) {
        return autoscaler_->get_policy();
    }
    return default_policy;
}

auto autoscaling_pool_policy::evaluate_now() -> scaling_decision
{
    if (autoscaler_) {
        return autoscaler_->evaluate_now();
    }
    return scaling_decision{};
}

auto autoscaling_pool_policy::scale_to(std::size_t target_workers) -> common::VoidResult
{
    if (autoscaler_) {
        return autoscaler_->scale_to(target_workers);
    }
    return common::ok();
}

} // namespace kcenon::thread
