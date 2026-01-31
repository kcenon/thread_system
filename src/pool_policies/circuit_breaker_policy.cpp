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

#include <kcenon/thread/pool_policies/circuit_breaker_policy.h>
#include <kcenon/thread/core/error_handling.h>
#include <kcenon/thread/core/job.h>

namespace kcenon::thread
{

circuit_breaker_policy::circuit_breaker_policy(const circuit_breaker_config& config)
    : circuit_breaker_(std::make_shared<circuit_breaker>(config))
{
}

circuit_breaker_policy::circuit_breaker_policy(std::shared_ptr<circuit_breaker> cb)
    : circuit_breaker_(std::move(cb))
{
    if (!circuit_breaker_) {
        // Create default circuit breaker if nullptr passed
        circuit_breaker_ = std::make_shared<circuit_breaker>();
    }
}

auto circuit_breaker_policy::on_enqueue(job& j) -> common::VoidResult
{
    (void)j;  // Job not used, policy applies to all jobs equally

    if (!enabled_.load(std::memory_order_acquire)) {
        return common::ok();
    }

    // Check if circuit breaker allows the request
    if (!circuit_breaker_->allow_request()) {
        auto state = circuit_breaker_->get_state();
        if (state == circuit_state::OPEN) {
            return make_error_result(error_code::circuit_open,
                                     "Circuit breaker is open, job rejected");
        }
        // half_open state but at capacity
        return make_error_result(error_code::circuit_half_open,
                                 "Circuit breaker is half-open and at capacity");
    }

    return common::ok();
}

void circuit_breaker_policy::on_job_start(job& j)
{
    (void)j;  // Currently no action needed on job start
    // Could be extended to track timing in the future
}

void circuit_breaker_policy::on_job_complete(job& j, bool success, const std::exception* error)
{
    (void)j;  // Job not used, policy applies to all jobs equally

    if (!enabled_.load(std::memory_order_acquire)) {
        return;
    }

    if (success) {
        circuit_breaker_->record_success();
    } else {
        circuit_breaker_->record_failure(error);
    }
}

auto circuit_breaker_policy::get_name() const -> std::string
{
    return "circuit_breaker_policy";
}

auto circuit_breaker_policy::is_enabled() const -> bool
{
    return enabled_.load(std::memory_order_acquire);
}

void circuit_breaker_policy::set_enabled(bool enabled)
{
    enabled_.store(enabled, std::memory_order_release);
}

auto circuit_breaker_policy::is_accepting_work() const -> bool
{
    if (!enabled_.load(std::memory_order_acquire)) {
        return true;  // If disabled, always accept
    }

    auto state = circuit_breaker_->get_state();
    return state != circuit_state::OPEN;
}

auto circuit_breaker_policy::get_state() const -> circuit_state
{
    return circuit_breaker_->get_state();
}

auto circuit_breaker_policy::get_stats() const -> circuit_breaker_stats
{
    // Convert common_system stats map to thread_system stats struct
    // Note: common_system doesn't track all the same metrics, so some fields will be zero
    circuit_breaker_stats stats{};
    stats.current_state = circuit_breaker_->get_state();
    stats.state_since = std::chrono::steady_clock::now(); // Approximation - not tracked by common_system

    // Common_system doesn't expose these metrics in the same way
    // These fields remain at their default (zero) values
    stats.total_requests = 0;
    stats.successful_requests = 0;
    stats.failed_requests = 0;
    stats.rejected_requests = 0;
    stats.failure_rate = 0.0;
    stats.consecutive_failures = 0;
    stats.state_transitions = 0;

    return stats;
}

auto circuit_breaker_policy::get_circuit_breaker() const -> std::shared_ptr<circuit_breaker>
{
    return circuit_breaker_;
}

void circuit_breaker_policy::trip()
{
    // No-op: common_system circuit_breaker doesn't support manual trip
    // State transitions are automatic based on failure thresholds
}

void circuit_breaker_policy::reset()
{
    // No-op: common_system circuit_breaker doesn't support manual reset
    // State transitions are automatic based on success thresholds
}

} // namespace kcenon::thread
