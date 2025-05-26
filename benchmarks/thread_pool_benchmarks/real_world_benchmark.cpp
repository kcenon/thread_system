/**
 * @file real_world_benchmark.cpp
 * @brief Real-world scenario benchmarks for Thread System
 * 
 * Tests that simulate actual usage patterns:
 * - Web server request handling
 * - Image processing pipeline
 * - Data analysis workloads
 * - Mixed I/O and CPU tasks
 */

#include <chrono>
#include <vector>
#include <random>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <atomic>
#include <future>
#include <map>
#include <thread>

#include "thread_pool.h"
#include "priority_thread_pool.h"
#include "logger.h"
#include "formatter.h"

using namespace std::chrono;
using namespace thread_pool_module;
using namespace priority_thread_pool_module;

// Simulate different types of workloads
class WorkloadSimulator {
public:
    // Simulate CPU-intensive work (e.g., image processing)
    static void simulate_cpu_work(int complexity) {
        volatile double result = 0;
        for (int i = 0; i < complexity * 1000; ++i) {
            result += std::sin(i) * std::cos(i);
        }
    }
    
    // Simulate I/O operation (e.g., database query)
    static void simulate_io_work(int duration_ms) {
        std::this_thread::sleep_for(milliseconds(duration_ms));
    }
    
    // Simulate memory-intensive work
    static void simulate_memory_work(size_t size_mb) {
        std::vector<char> buffer(size_mb * 1024 * 1024);
        // Touch memory to ensure allocation
        for (size_t i = 0; i < buffer.size(); i += 4096) {
            buffer[i] = static_cast<char>(i & 0xFF);
        }
    }
    
    // Simulate mixed workload
    static void simulate_mixed_work(int cpu_complexity, int io_duration_ms) {
        simulate_cpu_work(cpu_complexity);
        simulate_io_work(io_duration_ms);
    }
};

class RealWorldBenchmark {
public:
    RealWorldBenchmark() {
        log_module::start();
        log_module::console_target(log_module::log_types::Information);
    }
    
    ~RealWorldBenchmark() {
        log_module::stop();
    }
    
    void run_all_benchmarks() {
        log_module::write_information("\n=== Real-World Scenario Benchmarks ===\n");
        
        benchmark_web_server_simulation();
        benchmark_image_processing_pipeline();
        benchmark_data_analysis_workload();
        benchmark_game_engine_simulation();
        benchmark_microservice_communication();
        benchmark_batch_file_processing();
        
        log_module::write_information("\n=== Real-World Benchmarks Complete ===\n");
    }
    
private:
    void benchmark_web_server_simulation() {
        log_module::write_information("\n1. Web Server Request Handling Simulation\n");
        log_module::write_information("-----------------------------------------\n");
        
        // Simulate different request types with varying processing times
        struct RequestType {
            std::string name;
            int cpu_work;      // CPU complexity (1-100)
            int io_duration;   // I/O duration in ms
            double frequency;  // Relative frequency (0.0-1.0)
        };
        
        std::vector<RequestType> request_types = {
            {"Static file", 1, 1, 0.5},
            {"API query", 5, 10, 0.3},
            {"Database write", 10, 50, 0.15},
            {"Complex computation", 50, 5, 0.05}
        };
        
        // Test with different worker counts
        std::vector<size_t> worker_counts = {8, 16, 32, 64};
        
        for (size_t workers : worker_counts) {
            auto [pool, error] = create_default(workers);
            if (error) continue;
            
            pool->start();
            
            const size_t total_requests = 10000;
            std::atomic<size_t> completed_requests{0};
            std::atomic<size_t> total_response_time_ms{0};
            
            auto start = high_resolution_clock::now();
            
            // Random request generator
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(0.0, 1.0);
            
            for (size_t i = 0; i < total_requests; ++i) {
                // Select request type based on frequency
                double rand_val = dis(gen);
                double cumulative = 0.0;
                
                for (const auto& req_type : request_types) {
                    cumulative += req_type.frequency;
                    if (rand_val <= cumulative) {
                        pool->add_job([&req_type, &completed_requests, &total_response_time_ms] {
                            auto req_start = high_resolution_clock::now();
                            
                            // Process request
                            WorkloadSimulator::simulate_mixed_work(
                                req_type.cpu_work, 
                                req_type.io_duration
                            );
                            
                            auto req_end = high_resolution_clock::now();
                            auto response_time = duration_cast<milliseconds>(req_end - req_start).count();
                            
                            total_response_time_ms.fetch_add(response_time);
                            completed_requests.fetch_add(1);
                        });
                        break;
                    }
                }
            }
            
            pool->stop();
            
            auto end = high_resolution_clock::now();
            double total_time_s = duration_cast<milliseconds>(end - start).count() / 1000.0;
            double requests_per_second = total_requests / total_time_s;
            double avg_response_time = static_cast<double>(total_response_time_ms.load()) / total_requests;
            
            log_module::write_information(utility_module::formatter::format(
                "Workers: %3zu | RPS: %.0f | Avg Response: %.1fms",
                workers, requests_per_second, avg_response_time).c_str());
        }
    }
    
    void benchmark_image_processing_pipeline() {
        log_module::write_information("\n2. Image Processing Pipeline Simulation\n");
        log_module::write_information("---------------------------------------\n");
        
        // Simulate image processing stages
        struct ProcessingStage {
            std::string name;
            int complexity;  // Processing complexity
        };
        
        std::vector<ProcessingStage> stages = {
            {"Load", 10},
            {"Resize", 20},
            {"Filter", 50},
            {"Compress", 30},
            {"Save", 15}
        };
        
        std::vector<size_t> image_counts = {100, 500, 1000, 5000};
        
        auto [pool, error] = create_default(std::thread::hardware_concurrency());
        if (error) return;
        
        pool->start();
        
        for (size_t num_images : image_counts) {
            std::atomic<size_t> images_processed{0};
            
            auto start = high_resolution_clock::now();
            
            // Process each image through all stages
            for (size_t img = 0; img < num_images; ++img) {
                pool->add_job([img, &stages, &images_processed] {
                    // Simulate processing through each stage
                    for (const auto& stage : stages) {
                        WorkloadSimulator::simulate_cpu_work(stage.complexity);
                    }
                    
                    images_processed.fetch_add(1);
                });
            }
            
            pool->stop();
            pool->start();  // Reset for next test
            
            auto end = high_resolution_clock::now();
            double elapsed_s = duration_cast<milliseconds>(end - start).count() / 1000.0;
            double images_per_second = num_images / elapsed_s;
            
            log_module::write_information(utility_module::formatter::format(
                "%5zu images: %.1f img/s, Total time: %.1fs",
                num_images, images_per_second, elapsed_s).c_str());
        }
        
        pool->stop();
    }
    
    void benchmark_data_analysis_workload() {
        log_module::write_information("\n3. Data Analysis Workload Simulation\n");
        log_module::write_information("------------------------------------\n");
        
        // Simulate MapReduce-style data processing
        const size_t data_size_mb = 100;
        const size_t chunk_size_mb = 10;
        const size_t num_chunks = data_size_mb / chunk_size_mb;
        
        std::vector<size_t> worker_counts = {2, 4, 8, 16};
        
        for (size_t workers : worker_counts) {
            auto [pool, error] = create_default(workers);
            if (error) continue;
            
            pool->start();
            
            // Map phase
            std::vector<std::future<double>> map_results;
            std::vector<std::promise<double>> promises(num_chunks);
            
            for (size_t i = 0; i < num_chunks; ++i) {
                map_results.push_back(promises[i].get_future());
            }
            
            auto start = high_resolution_clock::now();
            
            // Submit map tasks
            for (size_t i = 0; i < num_chunks; ++i) {
                pool->add_job([i, chunk_size_mb, p = std::move(promises[i])]() mutable {
                    // Simulate data processing
                    WorkloadSimulator::simulate_memory_work(chunk_size_mb);
                    WorkloadSimulator::simulate_cpu_work(100);
                    
                    // Return partial result
                    double result = static_cast<double>(i) * 3.14159;
                    p.set_value(result);
                });
            }
            
            // Collect map results
            double map_sum = 0;
            for (auto& future : map_results) {
                map_sum += future.get();
            }
            
            // Reduce phase
            std::promise<double> reduce_promise;
            auto reduce_future = reduce_promise.get_future();
            
            pool->add_job([map_sum, p = std::move(reduce_promise)]() mutable {
                // Simulate reduce operation
                WorkloadSimulator::simulate_cpu_work(50);
                p.set_value(map_sum / 2.0);
            });
            
            double final_result = reduce_future.get();
            
            pool->stop();
            
            auto end = high_resolution_clock::now();
            double elapsed_ms = duration_cast<milliseconds>(end - start).count();
            double throughput_mb_s = (data_size_mb * 1000.0) / elapsed_ms;
            
            log_module::write_information(utility_module::formatter::format(
                "%2zu workers: %.2f MB/s, Time: %.0fms",
                workers, throughput_mb_s, elapsed_ms).c_str());
        }
    }
    
    void benchmark_game_engine_simulation() {
        log_module::write_information("\n4. Game Engine Update Loop Simulation\n");
        log_module::write_information("-------------------------------------\n");
        
        // Simulate game engine subsystems
        enum class Priority { 
            Physics = 1,      // Highest priority
            AI = 2,
            Rendering = 3,
            Audio = 4,
            Network = 5       // Lowest priority
        };
        
        struct Subsystem {
            std::string name;
            Priority priority;
            int update_time_us;  // Microseconds per update
            int frequency;       // Updates per frame
        };
        
        std::vector<Subsystem> subsystems = {
            {"Physics", Priority::Physics, 1000, 2},
            {"AI", Priority::AI, 500, 1},
            {"Rendering", Priority::Rendering, 2000, 1},
            {"Audio", Priority::Audio, 200, 4},
            {"Network", Priority::Network, 300, 2}
        };
        
        auto [pool, error] = create_priority_default<Priority>(8);
        if (error) return;
        
        pool->start();
        
        const int target_fps = 60;
        const int frame_time_ms = 1000 / target_fps;
        const int num_frames = 300;  // 5 seconds at 60 FPS
        
        std::atomic<int> completed_frames{0};
        std::atomic<int> missed_frames{0};
        
        auto start = high_resolution_clock::now();
        
        for (int frame = 0; frame < num_frames; ++frame) {
            auto frame_start = high_resolution_clock::now();
            std::atomic<int> subsystems_completed{0};
            int total_subsystems = 0;
            
            // Submit all subsystem updates for this frame
            for (const auto& subsystem : subsystems) {
                for (int i = 0; i < subsystem.frequency; ++i) {
                    total_subsystems++;
                    
                    pool->add_job([&subsystem, &subsystems_completed] {
                        // Simulate subsystem update
                        auto end_time = high_resolution_clock::now() + 
                                       microseconds(subsystem.update_time_us);
                        while (high_resolution_clock::now() < end_time) {
                            // Busy wait to simulate work
                        }
                        
                        subsystems_completed.fetch_add(1);
                    }, subsystem.priority);
                }
            }
            
            // Wait for frame completion or timeout
            auto frame_deadline = frame_start + milliseconds(frame_time_ms);
            while (subsystems_completed.load() < total_subsystems && 
                   high_resolution_clock::now() < frame_deadline) {
                std::this_thread::sleep_for(microseconds(100));
            }
            
            auto frame_end = high_resolution_clock::now();
            auto frame_duration = duration_cast<milliseconds>(frame_end - frame_start).count();
            
            if (frame_duration > frame_time_ms) {
                missed_frames.fetch_add(1);
            }
            
            completed_frames.fetch_add(1);
            
            // Sleep if frame completed early
            if (frame_duration < frame_time_ms) {
                std::this_thread::sleep_for(milliseconds(frame_time_ms - frame_duration));
            }
        }
        
        pool->stop();
        
        auto end = high_resolution_clock::now();
        double total_time_s = duration_cast<milliseconds>(end - start).count() / 1000.0;
        double actual_fps = completed_frames.load() / total_time_s;
        double frame_miss_rate = (missed_frames.load() * 100.0) / completed_frames.load();
        
        log_module::write_information(utility_module::formatter::format(
            "Target FPS: %d\nActual FPS: %.1f\nMissed frames: %d (%.1f%%)",
            target_fps, actual_fps, missed_frames.load(), frame_miss_rate).c_str());
    }
    
    void benchmark_microservice_communication() {
        log_module::write_information("\n5. Microservice Communication Pattern\n");
        log_module::write_information("-------------------------------------\n");
        
        // Simulate service-to-service communication
        struct Service {
            std::string name;
            int processing_time_ms;
            std::vector<std::string> dependencies;
        };
        
        std::vector<Service> services = {
            {"Gateway", 5, {}},
            {"Auth", 10, {"Gateway"}},
            {"UserService", 15, {"Auth"}},
            {"OrderService", 20, {"Auth", "UserService"}},
            {"PaymentService", 25, {"OrderService"}},
            {"NotificationService", 10, {"OrderService", "PaymentService"}}
        };
        
        auto [pool, error] = create_default(16);
        if (error) return;
        
        pool->start();
        
        const size_t num_requests = 1000;
        std::atomic<size_t> completed_requests{0};
        std::atomic<size_t> total_latency_ms{0};
        
        auto start = high_resolution_clock::now();
        
        for (size_t req = 0; req < num_requests; ++req) {
            pool->add_job([&services, &completed_requests, &total_latency_ms, &pool] {
                auto req_start = high_resolution_clock::now();
                
                // Process through service chain
                std::map<std::string, std::future<void>> service_futures;
                
                for (const auto& service : services) {
                    // Wait for dependencies
                    for (const auto& dep : service.dependencies) {
                        if (service_futures.count(dep)) {
                            service_futures[dep].get();
                        }
                    }
                    
                    // Process service
                    std::promise<void> promise;
                    service_futures[service.name] = promise.get_future();
                    
                    pool->add_job([&service, p = std::move(promise)]() mutable {
                        WorkloadSimulator::simulate_io_work(service.processing_time_ms);
                        p.set_value();
                    });
                }
                
                // Wait for final service
                if (service_futures.count("NotificationService")) {
                    service_futures["NotificationService"].get();
                }
                
                auto req_end = high_resolution_clock::now();
                auto latency = duration_cast<milliseconds>(req_end - req_start).count();
                
                total_latency_ms.fetch_add(latency);
                completed_requests.fetch_add(1);
            });
        }
        
        // Wait for all requests
        while (completed_requests.load() < num_requests) {
            std::this_thread::sleep_for(milliseconds(10));
        }
        
        pool->stop();
        
        auto end = high_resolution_clock::now();
        double total_time_s = duration_cast<milliseconds>(end - start).count() / 1000.0;
        double requests_per_second = num_requests / total_time_s;
        double avg_latency = static_cast<double>(total_latency_ms.load()) / num_requests;
        
        log_module::write_information(utility_module::formatter::format(
            "Requests/second: %.0f\nAverage latency: %.1fms",
            requests_per_second, avg_latency).c_str());
    }
    
    void benchmark_batch_file_processing() {
        log_module::write_information("\n6. Batch File Processing Simulation\n");
        log_module::write_information("-----------------------------------\n");
        
        // Simulate processing different file types
        struct FileType {
            std::string extension;
            int processing_complexity;
            size_t avg_size_kb;
        };
        
        std::vector<FileType> file_types = {
            {".txt", 10, 50},
            {".csv", 20, 500},
            {".json", 30, 200},
            {".xml", 40, 300},
            {".log", 15, 1000}
        };
        
        const size_t total_files = 10000;
        std::vector<size_t> batch_sizes = {10, 50, 100, 500};
        
        auto [pool, error] = create_default(std::thread::hardware_concurrency() * 2);
        if (error) return;
        
        pool->start();
        
        for (size_t batch_size : batch_sizes) {
            std::atomic<size_t> files_processed{0};
            std::atomic<size_t> total_bytes_processed{0};
            
            auto start = high_resolution_clock::now();
            
            // Process files in batches
            for (size_t i = 0; i < total_files; i += batch_size) {
                size_t current_batch_size = std::min(batch_size, total_files - i);
                
                pool->add_job([current_batch_size, &file_types, &files_processed, &total_bytes_processed] {
                    size_t batch_bytes = 0;
                    
                    for (size_t j = 0; j < current_batch_size; ++j) {
                        // Randomly select file type
                        const auto& file_type = file_types[j % file_types.size()];
                        
                        // Simulate file processing
                        WorkloadSimulator::simulate_cpu_work(file_type.processing_complexity);
                        WorkloadSimulator::simulate_io_work(1);  // File I/O
                        
                        batch_bytes += file_type.avg_size_kb * 1024;
                    }
                    
                    files_processed.fetch_add(current_batch_size);
                    total_bytes_processed.fetch_add(batch_bytes);
                });
            }
            
            pool->stop();
            pool->start();  // Reset for next test
            
            auto end = high_resolution_clock::now();
            double elapsed_s = duration_cast<milliseconds>(end - start).count() / 1000.0;
            double files_per_second = total_files / elapsed_s;
            double mb_per_second = (total_bytes_processed.load() / 1024.0 / 1024.0) / elapsed_s;
            
            log_module::write_information(utility_module::formatter::format(
                "Batch size %3zu: %.0f files/s, %.1f MB/s",
                batch_size, files_per_second, mb_per_second).c_str());
        }
        
        pool->stop();
    }
};

int main() {
    RealWorldBenchmark benchmark;
    benchmark.run_all_benchmarks();
    
    return 0;
}