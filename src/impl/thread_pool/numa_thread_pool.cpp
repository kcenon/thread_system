// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#include <kcenon/thread/core/numa_thread_pool.h>
#include <kcenon/thread/stealing/numa_work_stealer.h>

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
    enhanced_ws_config_ = config;
    worker_policy_.enable_work_stealing = config.enabled;

    if (config.enabled) {
        if (cached_topology_.node_count() == 0) {
            ensure_topology_detected();
        }
        // Note: Full work stealer setup is done in start() or when workers are added
    }
}

const enhanced_work_stealing_config& numa_thread_pool::numa_work_stealing_config() const {
    return enhanced_ws_config_;
}

work_stealing_stats_snapshot numa_thread_pool::numa_work_stealing_stats() const {
    if (numa_work_stealer_) {
        return numa_work_stealer_->get_stats_snapshot();
    }
    return work_stealing_stats_snapshot{};
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
