/**
 * @file memory_benchmark.cpp
 * @brief Memory usage benchmarks for Thread System
 */

#include <iomanip>
#include <thread>
#include <vector>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <unistd.h>
#include <sys/resource.h>
#ifdef __APPLE__
#include <mach/mach.h>
#endif
#endif

#include "thread_pool.h"
#include "priority_thread_pool.h"
#include "logger.h"
#include "format_string.h"

using namespace thread_pool_module;
using namespace priority_thread_pool_module;
using namespace log_module;

class MemoryMonitor {
public:
    struct MemoryStats {
        size_t virtual_size;    // Virtual memory size in bytes
        size_t resident_size;   // Resident set size in bytes
        size_t peak_size;       // Peak memory usage
    };
    
    static MemoryStats get_current_memory() {
        MemoryStats stats = {0, 0, 0};
        
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            stats.virtual_size = pmc.PrivateUsage;
            stats.resident_size = pmc.WorkingSetSize;
            stats.peak_size = pmc.PeakWorkingSetSize;
        }
#elif defined(__APPLE__)
        struct mach_task_basic_info info;
        mach_msg_type_number_t size = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &size) == KERN_SUCCESS) {
            stats.virtual_size = info.virtual_size;
            stats.resident_size = info.resident_size;
            stats.peak_size = info.resident_size_max;
        }
#else
        // Linux
        FILE* file = fopen("/proc/self/status", "r");
        if (file) {
            char line[128];
            while (fgets(line, 128, file)) {
                if (strncmp(line, "VmSize:", 7) == 0) {
                    stats.virtual_size = parse_kb(line) * 1024;
                } else if (strncmp(line, "VmRSS:", 6) == 0) {
                    stats.resident_size = parse_kb(line) * 1024;
                } else if (strncmp(line, "VmPeak:", 7) == 0) {
                    stats.peak_size = parse_kb(line) * 1024;
                }
            }
            fclose(file);
        }
#endif
        return stats;
    }
    
private:
    static size_t parse_kb(const char* line) {
        size_t value = 0;
        const char* p = line;
        while (*p && !isdigit(*p)) p++;
        if (*p) {
            value = atol(p);
        }
        return value;
    }
};

class MemoryBenchmark {
public:
    MemoryBenchmark() {
        log_module::start();
        log_module::console_target(log_module::log_types::Information);
    }
    
    ~MemoryBenchmark() {
        log_module::stop();
    }
    
    void run_all_benchmarks() {
        information(L"\n=== Thread System Memory Benchmarks ===\n");
        
        benchmark_base_memory();
        benchmark_thread_pool_memory();
        benchmark_priority_pool_memory();
        benchmark_job_queue_memory();
        benchmark_logger_memory();
        
        information(L"\n=== Memory Benchmark Complete ===\n");
    }
    
private:
    void benchmark_base_memory() {
        information(L"\n1. Base Memory Usage");
        information(L"-------------------");
        
        auto initial = MemoryMonitor::get_current_memory();
        print_memory_stats("Initial state", initial);
    }
    
    void benchmark_thread_pool_memory() {
        information(L"\n2. Thread Pool Memory Usage");
        information(L"---------------------------");
        
        std::vector<size_t> worker_counts = {1, 4, 8, 16, 32};
        
        for (size_t count : worker_counts) {
            auto before = MemoryMonitor::get_current_memory();
            
            auto [pool, error] = create_default(count);
            if (error) {
                log_module::error(format_string(L"Error creating pool: %s", error->c_str()));
                continue;
            }
            
            pool->start();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            auto after = MemoryMonitor::get_current_memory();
            
            size_t memory_increase = after.resident_size - before.resident_size;
            double per_worker = static_cast<double>(memory_increase) / count / 1024.0;
            
            information(format_string(L"%3zu workers: Total: %.2f MB, Per worker: %.2f KB",
                                    count, 
                                    (memory_increase / 1024.0 / 1024.0),
                                    per_worker));
            
            pool->stop();
        }
    }
    
    void benchmark_priority_pool_memory() {
        information(L"\n3. Priority Thread Pool Memory Usage");
        information(L"------------------------------------");
        
        enum class Priority { High = 1, Medium = 5, Low = 10 };
        
        auto before = MemoryMonitor::get_current_memory();
        
        auto [pool, error] = create_priority_default<Priority>(8);
        if (!error) {
            pool->start();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            auto after = MemoryMonitor::get_current_memory();
            
            size_t memory_increase = after.resident_size - before.resident_size;
            information(format_string(L"Priority pool (8 workers): %.2f MB",
                                    (memory_increase / 1024.0 / 1024.0)));
            
            pool->stop();
        }
    }
    
    void benchmark_job_queue_memory() {
        information(L"\n4. Job Queue Memory Usage");
        information(L"-------------------------");
        
        auto [pool, error] = create_default(4);
        if (error) {
            log_module::error(format_string(L"Error creating pool: %s", error->c_str()));
            return;
        }
        
        pool->start();
        
        std::vector<size_t> job_counts = {1000, 10000, 50000, 100000};
        
        for (size_t count : job_counts) {
            auto before = MemoryMonitor::get_current_memory();
            
            // Submit jobs that will queue up
            for (size_t i = 0; i < count; ++i) {
                pool->add_job([] {
                    std::this_thread::sleep_for(std::chrono::seconds(10));
                });
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            auto after = MemoryMonitor::get_current_memory();
            
            size_t memory_increase = after.resident_size - before.resident_size;
            double per_job = static_cast<double>(memory_increase) / count;
            
            information(format_string(L"%6zu jobs: Total: %.2f MB, Per job: %.2f bytes",
                                    count,
                                    (memory_increase / 1024.0 / 1024.0),
                                    per_job));
            
            // Clear the queue
            pool->stop();
            pool->start();
        }
        
        pool->stop();
    }
    
    void benchmark_logger_memory() {
        information(L"\n5. Logger Memory Usage");
        information(L"----------------------");
        
        // Restart logger with different configurations
        log_module::stop();
        
        auto before = MemoryMonitor::get_current_memory();
        
        log_module::set_title("memory_benchmark");
        log_module::file_target(log_module::log_types::All);
        log_module::console_target(log_module::log_types::None);
        log_module::start();
        
        // Generate log entries
        for (int i = 0; i < 10000; ++i) {
            log_module::info(L"Test log entry {}: {}", i, 
                           L"This is a test message to measure memory usage");
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto after = MemoryMonitor::get_current_memory();
        
        size_t memory_increase = after.resident_size - before.resident_size;
        information(format_string(L"Logger with 10k entries: %.2f MB",
                                (memory_increase / 1024.0 / 1024.0)));
    }
    
    void print_memory_stats(const std::string& label, const MemoryMonitor::MemoryStats& stats) {
        information(format_string(L"%s: Virtual=%.2f MB, Resident=%.2f MB, Peak=%.2f MB",
                                label.c_str(),
                                (stats.virtual_size / 1024.0 / 1024.0),
                                (stats.resident_size / 1024.0 / 1024.0),
                                (stats.peak_size / 1024.0 / 1024.0)));
    }
};

int main() {
    MemoryBenchmark benchmark;
    benchmark.run_all_benchmarks();
    
    return 0;
}