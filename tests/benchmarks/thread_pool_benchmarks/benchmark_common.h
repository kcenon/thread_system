// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#pragma once

#include <memory>
#include <string>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/impl/typed_pool/typed_thread_pool.h>
#include <kcenon/thread/impl/typed_pool/typed_thread_worker.h>

// Include common_system Result types for v3.0 API
#include <kcenon/common/patterns/result.h>

// Alias for common::VoidResult to simplify benchmark code
using VoidResult = kcenon::common::VoidResult;

// Helper macro to create callback jobs with proper return type
#define MAKE_JOB(lambda_body) \
    std::make_unique<kcenon::thread::callback_job>([&]() -> VoidResult { \
        lambda_body \
        return kcenon::common::ok(); \
    })

// Helper function to create and setup a thread pool
inline auto setup_thread_pool(size_t worker_count) -> std::shared_ptr<kcenon::thread::thread_pool> {
    auto pool = std::make_shared<kcenon::thread::thread_pool>();
    
    // Add workers
    for (size_t i = 0; i < worker_count; ++i) {
        auto worker = std::make_unique<kcenon::thread::thread_worker>();
        pool->enqueue(std::move(worker));
    }
    
    // Start the pool
    pool->start();
    
    return pool;
}

// Helper function for typed thread pool
template<typename PriorityType>
inline auto setup_typed_thread_pool(size_t worker_count) -> std::shared_ptr<kcenon::thread::typed_thread_pool_t<PriorityType>> {
    auto pool = std::make_shared<kcenon::thread::typed_thread_pool_t<PriorityType>>();

    // Add workers
    for (size_t i = 0; i < worker_count; ++i) {
        auto worker = std::make_unique<kcenon::thread::typed_thread_worker_t<PriorityType>>();
        pool->enqueue(std::move(worker));
    }

    // Start the pool
    pool->start();

    return pool;
}