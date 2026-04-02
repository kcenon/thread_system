// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

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

auto circuit_breaker_policy::get_circuit_breaker() const -> std::shared_ptr<circuit_breaker>
{
    return circuit_breaker_;
}

} // namespace kcenon::thread
