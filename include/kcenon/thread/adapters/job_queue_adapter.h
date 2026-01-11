/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, DongCheol Shin
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

#pragma once

#include <kcenon/thread/interfaces/pool_queue_adapter.h>
#include <kcenon/thread/core/job_queue.h>

namespace kcenon::thread {

/**
 * @class job_queue_adapter
 * @brief Adapter for job_queue to pool_queue_adapter_interface
 *
 * This adapter wraps a job_queue and provides the unified interface
 * expected by thread_pool. Since job_queue already has all required
 * methods, this adapter simply delegates to the underlying queue.
 *
 * ### Usage
 * @code
 * auto queue = std::make_shared<job_queue>();
 * auto adapter = std::make_unique<job_queue_adapter>(queue);
 * @endcode
 */
class job_queue_adapter : public pool_queue_adapter_interface {
public:
    /**
     * @brief Construct adapter with existing job_queue
     * @param queue Shared pointer to job_queue
     */
    explicit job_queue_adapter(std::shared_ptr<job_queue> queue)
        : queue_(std::move(queue)) {}

    /**
     * @brief Construct adapter with new job_queue
     */
    job_queue_adapter()
        : queue_(std::make_shared<job_queue>()) {}

    ~job_queue_adapter() override = default;

    // Non-copyable
    job_queue_adapter(const job_queue_adapter&) = delete;
    job_queue_adapter& operator=(const job_queue_adapter&) = delete;

    // Movable
    job_queue_adapter(job_queue_adapter&&) = default;
    job_queue_adapter& operator=(job_queue_adapter&&) = default;

    [[nodiscard]] auto enqueue(std::unique_ptr<job>&& j) -> common::VoidResult override {
        return queue_->enqueue(std::move(j));
    }

    [[nodiscard]] auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> common::VoidResult override {
        return queue_->enqueue_batch(std::move(jobs));
    }

    [[nodiscard]] auto dequeue() -> common::Result<std::unique_ptr<job>> override {
        return queue_->dequeue();
    }

    [[nodiscard]] auto try_dequeue() -> common::Result<std::unique_ptr<job>> override {
        return queue_->try_dequeue();
    }

    [[nodiscard]] auto empty() const -> bool override {
        return queue_->empty();
    }

    [[nodiscard]] auto size() const -> std::size_t override {
        return queue_->size();
    }

    auto clear() -> void override {
        queue_->clear();
    }

    auto stop() -> void override {
        queue_->stop();
    }

    [[nodiscard]] auto is_stopped() const -> bool override {
        return queue_->is_stopped();
    }

    [[nodiscard]] auto get_capabilities() const -> queue_capabilities override {
        return queue_->get_capabilities();
    }

    [[nodiscard]] auto to_string() const -> std::string override {
        return queue_->to_string();
    }

    [[nodiscard]] auto get_job_queue() const -> std::shared_ptr<job_queue> override {
        return queue_;
    }

    [[nodiscard]] auto get_scheduler() -> scheduler_interface& override {
        return *queue_;
    }

    [[nodiscard]] auto get_scheduler() const -> const scheduler_interface& override {
        return *queue_;
    }

private:
    std::shared_ptr<job_queue> queue_;
};

} // namespace kcenon::thread
