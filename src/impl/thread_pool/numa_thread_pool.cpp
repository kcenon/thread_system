/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
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

#include <kcenon/thread/core/numa_thread_pool.h>

namespace kcenon::thread {

numa_thread_pool::numa_thread_pool(const std::string& thread_title,
                                   const thread_context& context)
    : thread_pool(thread_title, context) {
    // Eagerly detect topology on construction
    ensure_topology_detected();
}

numa_thread_pool::numa_thread_pool(const std::string& thread_title,
                                   std::shared_ptr<job_queue> custom_queue,
                                   const thread_context& context)
    : thread_pool(thread_title, std::move(custom_queue), context) {
    ensure_topology_detected();
}

numa_thread_pool::numa_thread_pool(const std::string& thread_title,
                                   std::unique_ptr<pool_queue_adapter_interface> queue_adapter,
                                   const thread_context& context)
    : thread_pool(thread_title, std::move(queue_adapter), context) {
    ensure_topology_detected();
}

void numa_thread_pool::configure_numa_work_stealing(const enhanced_work_stealing_config& config) {
    // Delegate to base class method
    set_work_stealing_config(config);
}

const enhanced_work_stealing_config& numa_thread_pool::numa_work_stealing_config() const {
    return get_work_stealing_config();
}

work_stealing_stats_snapshot numa_thread_pool::numa_work_stealing_stats() const {
    return get_work_stealing_stats();
}

const numa_topology& numa_thread_pool::numa_topology_info() const {
    ensure_topology_detected();
    return cached_topology_;
}

bool numa_thread_pool::is_numa_system() const {
    ensure_topology_detected();
    return cached_topology_.is_numa_available();
}

void numa_thread_pool::enable_numa_work_stealing() {
    configure_numa_work_stealing(enhanced_work_stealing_config::numa_optimized());
}

void numa_thread_pool::disable_numa_work_stealing() {
    enhanced_work_stealing_config config;
    config.enabled = false;
    configure_numa_work_stealing(config);
}

bool numa_thread_pool::is_numa_work_stealing_enabled() const {
    return numa_work_stealing_config().enabled && numa_work_stealing_config().numa_aware;
}

void numa_thread_pool::ensure_topology_detected() const {
    if (!topology_detected_) {
        cached_topology_ = numa_topology::detect();
        topology_detected_ = true;
    }
}

} // namespace kcenon::thread
