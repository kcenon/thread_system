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

#include "thread_base/lockfree/memory/node_pool.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <atomic>
#include <memory>
#include <iomanip>

using namespace thread_module;

struct TestData {
    int value{0};
    double data{0.0};
    char padding[48] = {0}; // Make it a decent size for testing
    
    TestData() = default;
    TestData(int v, double d) : value(v), data(d) {}
};

void demonstrate_basic_usage() {
    std::cout << "\n=== Basic Node Pool Usage Demo ===\n";
    
    // Create a node pool with 2 initial chunks, 512 nodes per chunk
    node_pool<TestData> pool(2, 512);
    
    // Show initial statistics
    auto stats = pool.get_statistics();
    std::cout << "Initial pool statistics:\n";
    std::cout << "  Total chunks: " << stats.total_chunks << "\n";
    std::cout << "  Total nodes: " << stats.total_nodes << "\n";
    std::cout << "  Allocated nodes: " << stats.allocated_nodes << "\n";
    std::cout << "  Free list size: " << stats.free_list_size << "\n";
    
    // Allocate some nodes
    std::vector<TestData*> allocated_nodes;
    const int NUM_ALLOCATIONS = 100;
    
    std::cout << "\nAllocating " << NUM_ALLOCATIONS << " nodes...\n";
    for (int i = 0; i < NUM_ALLOCATIONS; ++i) {
        auto* node = pool.allocate();
        node->value = i;
        node->data = i * 3.14;
        allocated_nodes.push_back(node);
    }
    
    // Show statistics after allocation
    stats = pool.get_statistics();
    std::cout << "After allocation:\n";
    std::cout << "  Total chunks: " << stats.total_chunks << "\n";
    std::cout << "  Total nodes: " << stats.total_nodes << "\n";
    std::cout << "  Allocated nodes: " << stats.allocated_nodes << "\n";
    std::cout << "  Free list size: " << stats.free_list_size << "\n";
    
    // Verify data integrity
    std::cout << "\nVerifying data integrity...\n";
    bool integrity_ok = true;
    for (int i = 0; i < NUM_ALLOCATIONS; ++i) {
        if (allocated_nodes[i]->value != i || 
            std::abs(allocated_nodes[i]->data - i * 3.14) > 0.001) {
            integrity_ok = false;
            break;
        }
    }
    std::cout << "Data integrity: " << (integrity_ok ? "OK" : "FAILED") << "\n";
    
    // Deallocate half the nodes
    std::cout << "\nDeallocating half the nodes...\n";
    for (int i = 0; i < NUM_ALLOCATIONS / 2; ++i) {
        pool.deallocate(allocated_nodes[i]);
        allocated_nodes[i] = nullptr;
    }
    
    // Show statistics after partial deallocation
    stats = pool.get_statistics();
    std::cout << "After partial deallocation:\n";
    std::cout << "  Total chunks: " << stats.total_chunks << "\n";
    std::cout << "  Total nodes: " << stats.total_nodes << "\n";
    std::cout << "  Allocated nodes: " << stats.allocated_nodes << "\n";
    std::cout << "  Free list size: " << stats.free_list_size << "\n";
    
    // Deallocate remaining nodes
    for (int i = NUM_ALLOCATIONS / 2; i < NUM_ALLOCATIONS; ++i) {
        pool.deallocate(allocated_nodes[i]);
    }
    
    // Final statistics
    stats = pool.get_statistics();
    std::cout << "After full deallocation:\n";
    std::cout << "  Total chunks: " << stats.total_chunks << "\n";
    std::cout << "  Total nodes: " << stats.total_nodes << "\n";
    std::cout << "  Allocated nodes: " << stats.allocated_nodes << "\n";
    std::cout << "  Free list size: " << stats.free_list_size << "\n";
}

void demonstrate_concurrent_usage() {
    std::cout << "\n=== Concurrent Usage Demo ===\n";
    
    constexpr int NUM_THREADS = 4;
    constexpr int OPERATIONS_PER_THREAD = 1000;
    constexpr int INITIAL_CHUNKS = 2;
    constexpr int CHUNK_SIZE = 256;
    
    node_pool<TestData> pool(INITIAL_CHUNKS, CHUNK_SIZE);
    
    std::atomic<int> total_allocations{0};
    std::atomic<int> total_deallocations{0};
    std::atomic<int> allocation_failures{0};
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);
    
    // Create worker threads
    for (int thread_id = 0; thread_id < NUM_THREADS; ++thread_id) {
        threads.emplace_back([&, thread_id]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 100);
            
            std::vector<TestData*> local_nodes;
            local_nodes.reserve(OPERATIONS_PER_THREAD / 2);
            
            for (int op = 0; op < OPERATIONS_PER_THREAD; ++op) {
                if (dis(gen) < 70 || local_nodes.empty()) { // 70% chance to allocate
                    try {
                        auto* node = pool.allocate();
                        node->value = thread_id * 10000 + op;
                        node->data = thread_id + op * 0.001;
                        local_nodes.push_back(node);
                        total_allocations.fetch_add(1, std::memory_order_relaxed);
                    } catch (const std::bad_alloc&) {
                        allocation_failures.fetch_add(1, std::memory_order_relaxed);
                    }
                } else { // Deallocate
                    if (!local_nodes.empty()) {
                        auto idx = std::uniform_int_distribution<size_t>(0, local_nodes.size() - 1)(gen);
                        pool.deallocate(local_nodes[idx]);
                        local_nodes.erase(local_nodes.begin() + idx);
                        total_deallocations.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            }
            
            // Clean up remaining nodes
            for (auto* node : local_nodes) {
                if (node) {
                    pool.deallocate(node);
                    total_deallocations.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Concurrent operations completed in " << duration.count() << " ms\n";
    std::cout << "Total allocations: " << total_allocations.load() << "\n";
    std::cout << "Total deallocations: " << total_deallocations.load() << "\n";
    std::cout << "Allocation failures: " << allocation_failures.load() << "\n";
    
    // Final pool statistics
    auto stats = pool.get_statistics();
    std::cout << "Final pool statistics:\n";
    std::cout << "  Total chunks: " << stats.total_chunks << "\n";
    std::cout << "  Total nodes: " << stats.total_nodes << "\n";
    std::cout << "  Allocated nodes: " << stats.allocated_nodes << "\n";
    std::cout << "  Free list size: " << stats.free_list_size << "\n";
    
    // Calculate performance
    int total_ops = total_allocations.load() + total_deallocations.load();
    double ops_per_second = (double)total_ops / (duration.count() / 1000.0);
    std::cout << "Performance: " << static_cast<int>(ops_per_second) << " ops/second\n";
}

void demonstrate_performance_comparison() {
    std::cout << "\n=== Performance Comparison Demo ===\n";
    
    constexpr int NUM_OPERATIONS = 100000;
    constexpr int WARMUP_OPERATIONS = 10000;
    
    // Test with node pool
    std::cout << "Testing node pool performance...\n";
    node_pool<TestData> pool(4, 1024);
    
    // Warmup
    std::vector<TestData*> warmup_nodes;
    for (int i = 0; i < WARMUP_OPERATIONS; ++i) {
        warmup_nodes.push_back(pool.allocate());
    }
    for (auto* node : warmup_nodes) {
        pool.deallocate(node);
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<TestData*> pool_nodes;
    pool_nodes.reserve(NUM_OPERATIONS);
    
    // Allocate
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        pool_nodes.push_back(pool.allocate());
    }
    
    // Deallocate
    for (auto* node : pool_nodes) {
        pool.deallocate(node);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto pool_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Test with standard allocation
    std::cout << "Testing standard allocation performance...\n";
    
    start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::unique_ptr<TestData>> std_nodes;
    std_nodes.reserve(NUM_OPERATIONS);
    
    // Allocate
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        std_nodes.push_back(std::make_unique<TestData>());
    }
    
    // Deallocate (automatic with unique_ptr)
    std_nodes.clear();
    
    end_time = std::chrono::high_resolution_clock::now();
    auto std_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::cout << "Results:\n";
    std::cout << "  Node pool: " << pool_duration.count() << " μs\n";
    std::cout << "  Standard allocation: " << std_duration.count() << " μs\n";
    
    if (std_duration.count() > 0) {
        double speedup = (double)std_duration.count() / pool_duration.count();
        std::cout << "  Speedup: " << std::fixed << std::setprecision(2) << speedup << "x\n";
    }
    
    // Calculate operations per second
    double pool_ops_per_sec = (2.0 * NUM_OPERATIONS) / (pool_duration.count() / 1000000.0);
    double std_ops_per_sec = (2.0 * NUM_OPERATIONS) / (std_duration.count() / 1000000.0);
    
    std::cout << "  Node pool ops/sec: " << static_cast<int>(pool_ops_per_sec) << "\n";
    std::cout << "  Standard ops/sec: " << static_cast<int>(std_ops_per_sec) << "\n";
}

void demonstrate_memory_efficiency() {
    std::cout << "\n=== Memory Efficiency Demo ===\n";
    
    // Small pool for testing
    node_pool<TestData> small_pool(1, 256);
    node_pool<TestData> medium_pool(2, 512);
    node_pool<TestData> large_pool(4, 1024);
    
    auto show_pool_info = [](const auto& pool, const char* name) {
        auto stats = pool.get_statistics();
        size_t memory_usage = stats.total_nodes * sizeof(TestData);
        std::cout << name << ":\n";
        std::cout << "  Total chunks: " << stats.total_chunks << "\n";
        std::cout << "  Total nodes: " << stats.total_nodes << "\n";
        std::cout << "  Memory usage: " << memory_usage << " bytes (" 
                  << (memory_usage / 1024.0) << " KB)\n";
        std::cout << "  Node size: " << sizeof(TestData) << " bytes\n\n";
    };
    
    show_pool_info(small_pool, "Small pool (1x256)");
    show_pool_info(medium_pool, "Medium pool (2x512)");
    show_pool_info(large_pool, "Large pool (4x1024)");
    
    // Test fragmentation
    std::cout << "Testing fragmentation scenario...\n";
    std::vector<TestData*> nodes;
    
    // Allocate many nodes
    for (int i = 0; i < 100; ++i) {
        nodes.push_back(medium_pool.allocate());
    }
    
    // Deallocate every other node (create fragmentation)
    for (size_t i = 0; i < nodes.size(); i += 2) {
        medium_pool.deallocate(nodes[i]);
        nodes[i] = nullptr;
    }
    
    auto stats = medium_pool.get_statistics();
    std::cout << "After fragmentation:\n";
    std::cout << "  Allocated nodes: " << stats.allocated_nodes << "\n";
    std::cout << "  Free list size: " << stats.free_list_size << "\n";
    
    // Allocate new nodes (should reuse freed nodes)
    int reused_count = 0;
    for (size_t i = 0; i < nodes.size() && reused_count < 25; i += 2) {
        if (!nodes[i]) {
            nodes[i] = medium_pool.allocate();
            reused_count++;
        }
    }
    
    stats = medium_pool.get_statistics();
    std::cout << "After reuse (" << reused_count << " nodes):\n";
    std::cout << "  Allocated nodes: " << stats.allocated_nodes << "\n";
    std::cout << "  Free list size: " << stats.free_list_size << "\n";
    
    // Clean up
    for (auto* node : nodes) {
        if (node) {
            medium_pool.deallocate(node);
        }
    }
}

int main() {
    std::cout << "Node Pool Sample\n";
    std::cout << "================\n";
    
    try {
        demonstrate_basic_usage();
        demonstrate_concurrent_usage();
        demonstrate_performance_comparison();
        demonstrate_memory_efficiency();
        
        std::cout << "\n=== All demos completed successfully! ===\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}