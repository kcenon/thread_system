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

#include "metrics_collector.h"
#include "../../utilities/core/formatter.h"

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
#elif defined(__linux__)
    #include <sys/sysinfo.h>
    #include <sys/resource.h>
    #include <fstream>
#elif defined(__APPLE__)
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <mach/mach.h>
#endif

namespace monitoring_module {

    metrics_collector::metrics_collector(monitoring_config config)
        : config_(std::move(config))
        , snapshot_buffer_(std::make_unique<thread_safe_ring_buffer<metrics_snapshot>>(config_.buffer_size)) {
    }

    metrics_collector::~metrics_collector() {
        stop();
    }

    auto metrics_collector::start() -> thread_module::result_void {
        if (running_.load(std::memory_order_acquire)) {
            return thread_module::result_void{thread_module::error{
                thread_module::error_code::thread_already_running,
                "Metrics collector is already running"
            }};
        }

        stop_requested_.store(false, std::memory_order_release);
        running_.store(true, std::memory_order_release);

        try {
            collection_thread_ = std::make_unique<std::thread>(&metrics_collector::collection_loop, this);
        } catch (const std::exception& e) {
            running_.store(false, std::memory_order_release);
            return thread_module::result_void{thread_module::error{
                thread_module::error_code::thread_start_failure,
                utility_module::formatter::format("Failed to start collection thread: {}", e.what())
            }};
        }

        return {};
    }

    auto metrics_collector::stop() -> void {
        if (!running_.load(std::memory_order_acquire)) {
            return;
        }

        stop_requested_.store(true, std::memory_order_release);

        if (collection_thread_ && collection_thread_->joinable()) {
            collection_thread_->join();
        }

        running_.store(false, std::memory_order_release);
        collection_thread_.reset();
    }

    auto metrics_collector::register_system_metrics(std::shared_ptr<system_metrics> metrics) -> void {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        system_metrics_ = std::move(metrics);
    }

    auto metrics_collector::register_thread_pool_metrics(std::shared_ptr<thread_pool_metrics> metrics) -> void {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        thread_pool_metrics_ = std::move(metrics);
    }

    auto metrics_collector::register_worker_metrics(std::shared_ptr<worker_metrics> metrics) -> void {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        worker_metrics_ = std::move(metrics);
    }

    auto metrics_collector::get_current_snapshot() const -> metrics_snapshot {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        
        metrics_snapshot snapshot;
        snapshot.capture_time = std::chrono::steady_clock::now();

        if (system_metrics_) {
            snapshot.system = *system_metrics_;
        }
        if (thread_pool_metrics_) {
            snapshot.thread_pool = *thread_pool_metrics_;
        }
        if (worker_metrics_) {
            snapshot.worker = *worker_metrics_;
        }

        return snapshot;
    }

    auto metrics_collector::get_recent_snapshots(std::size_t count) const -> std::vector<metrics_snapshot> {
        return snapshot_buffer_->get_all_items();
    }

    auto metrics_collector::get_collection_stats() const -> collection_statistics {
        return stats_;
    }

    auto metrics_collector::update_config(const monitoring_config& config) -> void {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        config_ = config;
    }

    auto metrics_collector::collection_loop() -> void {
        while (!stop_requested_.load(std::memory_order_acquire)) {
            auto start_time = std::chrono::steady_clock::now();

            try {
                collect_metrics();
                stats_.total_collections.fetch_add(1, std::memory_order_relaxed);
            } catch (const std::exception&) {
                stats_.collection_errors.fetch_add(1, std::memory_order_relaxed);
            }

            auto end_time = std::chrono::steady_clock::now();
            auto collection_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
                end_time - start_time).count();
            
            stats_.collection_time_ns.store(static_cast<std::uint64_t>(collection_duration), 
                                           std::memory_order_relaxed);

            // Wait for collection interval
            std::this_thread::sleep_for(config_.collection_interval);
        }
    }

    auto metrics_collector::collect_metrics() -> void {
        if (config_.enable_system_metrics) {
            collect_system_metrics();
        }
        if (config_.enable_thread_pool_metrics) {
            collect_thread_pool_metrics();
        }
        if (config_.enable_worker_metrics) {
            collect_worker_metrics();
        }

        // Create and save snapshot
        auto snapshot = get_current_snapshot();
        if (!snapshot_buffer_->push(snapshot)) {
            stats_.buffer_overflows.fetch_add(1, std::memory_order_relaxed);
        }
    }

    auto metrics_collector::collect_system_metrics() -> void {
        if (!system_metrics_) {
            return;
        }

        // Platform-specific system metrics collection
#ifdef _WIN32
        // Windows implementation
        MEMORYSTATUSEX mem_status;
        mem_status.dwLength = sizeof(mem_status);
        if (GlobalMemoryStatusEx(&mem_status)) {
            system_metrics_->memory_usage_bytes.store(
                mem_status.ullTotalPhys - mem_status.ullAvailPhys, 
                std::memory_order_relaxed
            );
        }

        // CPU usage replaced with simple implementation (more complex logic needed in practice)
        system_metrics_->cpu_usage_percent.store(0, std::memory_order_relaxed);

#elif defined(__linux__)
        // Linux implementation
        std::ifstream meminfo("/proc/meminfo");
        std::string line;
        std::uint64_t total_mem = 0, free_mem = 0;

        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal:") == 0) {
                total_mem = std::stoull(line.substr(line.find_last_of(' ') + 1)) * 1024;
            } else if (line.find("MemFree:") == 0) {
                free_mem = std::stoull(line.substr(line.find_last_of(' ') + 1)) * 1024;
                break;
            }
        }

        if (total_mem > 0 && free_mem > 0) {
            system_metrics_->memory_usage_bytes.store(total_mem - free_mem, std::memory_order_relaxed);
        }

#elif defined(__APPLE__)
        // macOS implementation
        vm_size_t page_size;
        vm_statistics64_data_t vm_stats;
        mach_msg_type_number_t info_count = HOST_VM_INFO64_COUNT;
        
        host_page_size(mach_host_self(), &page_size);
        
        if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                            (host_info64_t)&vm_stats, &info_count) == KERN_SUCCESS) {
            
            std::uint64_t used_memory = (vm_stats.active_count + 
                                        vm_stats.inactive_count + 
                                        vm_stats.wire_count) * page_size;
            
            system_metrics_->memory_usage_bytes.store(used_memory, std::memory_order_relaxed);
        }
#endif

        // Active thread count (simple implementation)
        system_metrics_->active_threads.store(
            std::thread::hardware_concurrency(), 
            std::memory_order_relaxed
        );

        system_metrics_->timestamp = std::chrono::steady_clock::now();
    }

    auto metrics_collector::collect_thread_pool_metrics() -> void {
        if (!thread_pool_metrics_) {
            return;
        }

        // Thread pool metrics are updated externally, so only update timestamp
        thread_pool_metrics_->timestamp = std::chrono::steady_clock::now();
    }

    auto metrics_collector::collect_worker_metrics() -> void {
        if (!worker_metrics_) {
            return;
        }

        // Worker metrics are also updated externally, so only update timestamp
        worker_metrics_->timestamp = std::chrono::steady_clock::now();
    }

    // Global metrics collector implementation
    auto global_metrics_collector::instance() -> global_metrics_collector& {
        static global_metrics_collector instance;
        return instance;
    }

    auto global_metrics_collector::initialize(monitoring_config config) -> thread_module::result_void {
        std::lock_guard<std::mutex> lock(init_mutex_);
        
        if (collector_) {
            return thread_module::result_void{thread_module::error{
                thread_module::error_code::thread_already_running,
                "Global metrics collector already initialized"
            }};
        }

        collector_ = std::make_shared<metrics_collector>(std::move(config));
        return collector_->start();
    }

    auto global_metrics_collector::shutdown() -> void {
        std::lock_guard<std::mutex> lock(init_mutex_);
        
        if (collector_) {
            collector_->stop();
            collector_.reset();
        }
    }

    auto global_metrics_collector::get_collector() -> std::shared_ptr<metrics_collector> {
        std::lock_guard<std::mutex> lock(init_mutex_);
        return collector_;
    }

    // Convenience function implementation
    namespace metrics {
        auto start_global_monitoring(monitoring_config config) -> thread_module::result_void {
            return global_metrics_collector::instance().initialize(std::move(config));
        }

        auto stop_global_monitoring() -> void {
            global_metrics_collector::instance().shutdown();
        }

        auto get_current_metrics() -> metrics_snapshot {
            auto collector = global_metrics_collector::instance().get_collector();
            if (collector) {
                return collector->get_current_snapshot();
            }
            return {};
        }

        auto get_recent_metrics(std::size_t count) -> std::vector<metrics_snapshot> {
            auto collector = global_metrics_collector::instance().get_collector();
            if (collector) {
                return collector->get_recent_snapshots(count);
            }
            return {};
        }

        auto is_monitoring_active() -> bool {
            auto collector = global_metrics_collector::instance().get_collector();
            return collector && collector->is_running();
        }
    }

} // namespace monitoring_module