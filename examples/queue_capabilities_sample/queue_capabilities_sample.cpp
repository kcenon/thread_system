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

/**
 * @file queue_capabilities_sample.cpp
 * @brief Demonstrates queue_capabilities_interface usage for runtime capability introspection
 *
 * This sample shows how to use the queue_capabilities_interface to query
 * and adapt behavior based on queue implementation characteristics.
 */

#include <kcenon/thread/core/job_queue.h>
#include <kcenon/thread/lockfree/lockfree_job_queue.h>
#include <kcenon/thread/queue/adaptive_job_queue.h>
#include <kcenon/thread/interfaces/queue_capabilities_interface.h>
#include <kcenon/thread/interfaces/scheduler_interface.h>
#include <kcenon/thread/core/callback_job.h>

#include <iostream>
#include <memory>
#include <string>

using namespace kcenon::thread;

/**
 * @brief Example 1: Basic capability query
 *
 * Demonstrates how to directly query capabilities from different queue types.
 */
void basic_capability_query()
{
    std::cout << "=== Example 1: Basic Capability Query ===" << std::endl;

    auto mutex_queue = std::make_shared<job_queue>();
    auto lockfree_queue = std::make_unique<lockfree_job_queue>();

    // Query capabilities using get_capabilities()
    auto mutex_caps = mutex_queue->get_capabilities();
    auto lockfree_caps = lockfree_queue->get_capabilities();

    std::cout << "\njob_queue capabilities:" << std::endl;
    std::cout << "  exact_size: " << std::boolalpha << mutex_caps.exact_size << std::endl;
    std::cout << "  atomic_empty_check: " << mutex_caps.atomic_empty_check << std::endl;
    std::cout << "  lock_free: " << mutex_caps.lock_free << std::endl;
    std::cout << "  wait_free: " << mutex_caps.wait_free << std::endl;
    std::cout << "  supports_batch: " << mutex_caps.supports_batch << std::endl;
    std::cout << "  supports_blocking_wait: " << mutex_caps.supports_blocking_wait << std::endl;
    std::cout << "  supports_stop: " << mutex_caps.supports_stop << std::endl;

    std::cout << "\nlockfree_job_queue capabilities:" << std::endl;
    std::cout << "  exact_size: " << lockfree_caps.exact_size << std::endl;
    std::cout << "  atomic_empty_check: " << lockfree_caps.atomic_empty_check << std::endl;
    std::cout << "  lock_free: " << lockfree_caps.lock_free << std::endl;
    std::cout << "  wait_free: " << lockfree_caps.wait_free << std::endl;
    std::cout << "  supports_batch: " << lockfree_caps.supports_batch << std::endl;
    std::cout << "  supports_blocking_wait: " << lockfree_caps.supports_blocking_wait << std::endl;
    std::cout << "  supports_stop: " << lockfree_caps.supports_stop << std::endl;

    std::cout << std::endl;
}

/**
 * @brief Example 2: Convenience methods
 *
 * Shows how to use the convenience methods instead of get_capabilities().
 */
void convenience_methods()
{
    std::cout << "=== Example 2: Convenience Methods ===" << std::endl;

    auto queue = std::make_shared<job_queue>();

    std::cout << "\nUsing convenience methods on job_queue:" << std::endl;

    // Use convenience methods for cleaner code
    if (queue->has_exact_size()) {
        std::cout << "  [OK] Queue size is exact: " << queue->size() << std::endl;
    }

    if (queue->has_atomic_empty()) {
        std::cout << "  [OK] Empty check is atomic: " << std::boolalpha << queue->empty() << std::endl;
    }

    if (!queue->is_lock_free()) {
        std::cout << "  [OK] Queue uses mutex (good for accuracy)" << std::endl;
    }

    if (!queue->is_wait_free()) {
        std::cout << "  [OK] Queue is not wait-free" << std::endl;
    }

    if (queue->supports_batch()) {
        std::cout << "  [OK] Batch operations supported" << std::endl;
    }

    if (queue->supports_blocking_wait()) {
        std::cout << "  [OK] Blocking wait supported" << std::endl;
    }

    if (queue->supports_stop()) {
        std::cout << "  [OK] Stop signaling supported" << std::endl;
    }

    std::cout << std::endl;
}

/**
 * @brief Example 3: Dynamic capability check (polymorphic)
 *
 * Demonstrates how to check capabilities through a base interface pointer.
 */
void dynamic_capability_check(scheduler_interface* scheduler)
{
    std::cout << "=== Example 3: Dynamic Capability Check ===" << std::endl;

    std::cout << "\nChecking capabilities through scheduler_interface*:" << std::endl;

    // Check if scheduler supports capabilities interface using dynamic_cast
    if (auto* cap = dynamic_cast<queue_capabilities_interface*>(scheduler)) {
        std::cout << "  [OK] Scheduler supports capability introspection" << std::endl;

        if (cap->has_exact_size()) {
            std::cout << "    -> Safe to use size() for decisions" << std::endl;
        } else {
            std::cout << "    -> size() is approximate, use with caution" << std::endl;
        }

        if (cap->is_lock_free()) {
            std::cout << "    -> Lock-free implementation (high throughput)" << std::endl;
        } else {
            std::cout << "    -> Mutex-based implementation (accurate metrics)" << std::endl;
        }

        if (cap->supports_blocking_wait()) {
            std::cout << "    -> Blocking dequeue available" << std::endl;
        } else {
            std::cout << "    -> Use polling/spin-wait for dequeue" << std::endl;
        }
    } else {
        std::cout << "  [!] Scheduler does not support capability introspection" << std::endl;
    }

    std::cout << std::endl;
}

/**
 * @brief Smart job processor that adapts to queue capabilities
 *
 * Demonstrates capability-driven queue selection and usage.
 */
class SmartJobProcessor {
    std::unique_ptr<scheduler_interface> queue_;
    bool exact_metrics_available_;

public:
    explicit SmartJobProcessor(bool need_exact_metrics)
    {
        if (need_exact_metrics) {
            auto jq = std::make_unique<job_queue>();
            exact_metrics_available_ = true;
            queue_ = std::move(jq);
        } else {
            auto lfq = std::make_unique<lockfree_job_queue>();
            exact_metrics_available_ = false;
            queue_ = std::move(lfq);
        }
    }

    void log_status() const
    {
        if (exact_metrics_available_) {
            // Safe to use for monitoring/alerting
            std::cout << "  Exact queue size: " << get_size() << std::endl;
        } else {
            // Approximate, for logging only
            std::cout << "  Approximate queue size: ~" << get_size() << std::endl;
        }
    }

    [[nodiscard]] bool has_exact_metrics() const { return exact_metrics_available_; }

private:
    [[nodiscard]] size_t get_size() const
    {
        if (auto* cap = dynamic_cast<queue_capabilities_interface*>(queue_.get())) {
            // Use the polymorphic interface for demonstration
            if (auto* jq = dynamic_cast<job_queue*>(queue_.get())) {
                return jq->size();
            }
            if (auto* lfq = dynamic_cast<lockfree_job_queue*>(queue_.get())) {
                return lfq->size();
            }
        }
        return 0;
    }
};

/**
 * @brief Example 4: Capability-driven queue selection
 *
 * Shows how to select queue implementation based on requirements.
 */
void capability_driven_selection()
{
    std::cout << "=== Example 4: Capability-Driven Selection ===" << std::endl;

    std::cout << "\nCreating processors with different requirements:" << std::endl;

    // Monitoring processor needs exact metrics
    SmartJobProcessor monitoring_processor(true);
    std::cout << "\n[Monitoring Processor] (needs exact metrics)" << std::endl;
    std::cout << "  exact_metrics_available: " << std::boolalpha
              << monitoring_processor.has_exact_metrics() << std::endl;
    monitoring_processor.log_status();

    // Logging processor can use approximate metrics
    SmartJobProcessor logging_processor(false);
    std::cout << "\n[Logging Processor] (approximate is fine)" << std::endl;
    std::cout << "  exact_metrics_available: "
              << logging_processor.has_exact_metrics() << std::endl;
    logging_processor.log_status();

    std::cout << std::endl;
}

/**
 * @brief Example 5: Capability comparison table
 *
 * Displays a comparison table of all queue implementations.
 */
void capability_comparison()
{
    std::cout << "=== Example 5: Capability Comparison Table ===" << std::endl;

    auto mutex_q = std::make_shared<job_queue>();
    auto lockfree_q = std::make_unique<lockfree_job_queue>();
    auto adaptive_q = std::make_unique<adaptive_job_queue>();

    auto print_caps = [](const char* name, const queue_capabilities& caps) {
        std::cout << "\n" << name << ":" << std::endl;
        std::cout << "  exact_size         = " << std::boolalpha << caps.exact_size << std::endl;
        std::cout << "  atomic_empty_check = " << caps.atomic_empty_check << std::endl;
        std::cout << "  lock_free          = " << caps.lock_free << std::endl;
        std::cout << "  wait_free          = " << caps.wait_free << std::endl;
        std::cout << "  supports_batch     = " << caps.supports_batch << std::endl;
        std::cout << "  supports_blocking  = " << caps.supports_blocking_wait << std::endl;
        std::cout << "  supports_stop      = " << caps.supports_stop << std::endl;
    };

    print_caps("job_queue (mutex-based)", mutex_q->get_capabilities());
    print_caps("lockfree_job_queue", lockfree_q->get_capabilities());
    print_caps("adaptive_job_queue (default mode)", adaptive_q->get_capabilities());

    // Summary table
    std::cout << "\n--- Summary Table ---" << std::endl;
    std::cout << "Queue Type            | exact | atomic | lock-free | batch | blocking" << std::endl;
    std::cout << "----------------------|-------|--------|-----------|-------|----------" << std::endl;

    auto caps = mutex_q->get_capabilities();
    std::cout << "job_queue             |   "
              << (caps.exact_size ? "Y" : "N") << "   |   "
              << (caps.atomic_empty_check ? "Y" : "N") << "    |     "
              << (caps.lock_free ? "Y" : "N") << "     |   "
              << (caps.supports_batch ? "Y" : "N") << "   |    "
              << (caps.supports_blocking_wait ? "Y" : "N") << std::endl;

    caps = lockfree_q->get_capabilities();
    std::cout << "lockfree_job_queue    |   "
              << (caps.exact_size ? "Y" : "N") << "   |   "
              << (caps.atomic_empty_check ? "Y" : "N") << "    |     "
              << (caps.lock_free ? "Y" : "N") << "     |   "
              << (caps.supports_batch ? "Y" : "N") << "   |    "
              << (caps.supports_blocking_wait ? "Y" : "N") << std::endl;

    caps = adaptive_q->get_capabilities();
    std::cout << "adaptive_job_queue    |   "
              << (caps.exact_size ? "Y" : "N") << "   |   "
              << (caps.atomic_empty_check ? "Y" : "N") << "    |     "
              << (caps.lock_free ? "Y" : "N") << "     |   "
              << (caps.supports_batch ? "Y" : "N") << "   |    "
              << (caps.supports_blocking_wait ? "Y" : "N") << std::endl;

    std::cout << std::endl;
}

int main()
{
    std::cout << "Queue Capabilities Sample" << std::endl;
    std::cout << "=========================" << std::endl;
    std::cout << std::endl;
    std::cout << "This sample demonstrates queue_capabilities_interface usage" << std::endl;
    std::cout << "for runtime capability introspection." << std::endl;
    std::cout << std::endl;

    try {
        // Example 1: Basic capability query
        basic_capability_query();

        // Example 2: Convenience methods
        convenience_methods();

        // Example 3: Dynamic capability check
        auto queue = std::make_unique<job_queue>();
        dynamic_capability_check(queue.get());

        // Example 4: Capability-driven selection
        capability_driven_selection();

        // Example 5: Capability comparison
        capability_comparison();

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "All examples completed successfully!" << std::endl;

    return 0;
}
