/*
 * BSD 3-Clause License
 * Copyright (c) 2025, DongCheol Shin
 */

#pragma once

/**
 * @file executor_interface.h
 * @brief DEPRECATED: This interface is deprecated and will be removed in v2.0
 *
 * Phase 2: Executor Interface Unification
 *
 * This interface has been superseded by common::interfaces::IExecutor.
 * Please migrate to the unified executor interface in common_system:
 *
 * Migration:
 *   OLD: #include <kcenon/thread/interfaces/executor_interface.h>
 *        kcenon::thread::executor_interface* executor;
 *        executor->execute(std::move(job));
 *
 *   NEW: #include <kcenon/common/interfaces/executor_interface.h>
 *        common::interfaces::IExecutor* executor;
 *        executor->execute(std::move(job));  // Returns Result<std::future<void>>
 *
 * The unified interface provides:
 * - Result<T> based error handling
 * - Both function-based and job-based execution
 * - Unified IJob interface
 * - Better integration across all systems
 * - Delayed execution support
 *
 * Deprecation Timeline:
 * - v1.x: Deprecated but functional (current)
 * - v2.0: Removed entirely
 *
 * @deprecated Use common::interfaces::IExecutor instead
 */

#include <memory>

// Use project result type and job forward declarations
#include <kcenon/thread/core/error_handling.h>
#include <kcenon/thread/core/job.h>

namespace kcenon::thread {

/**
 * @brief Executor interface for submitting work and coordinating shutdown.
 * @deprecated Use common::interfaces::IExecutor instead
 *
 * MIGRATION: The unified interface provides job-based execution with Result<T>
 * and function-based execution for compatibility.
 */
class [[deprecated("Use common::interfaces::IExecutor instead")]] executor_interface {
public:
    virtual ~executor_interface() = default;

    /**
     * @brief Submit a unit of work for asynchronous execution.
     */
    virtual auto execute(std::unique_ptr<job>&& work) -> result_void = 0;

    /**
     * @brief Initiate a cooperative shutdown.
     */
    virtual auto shutdown() -> result_void = 0;
};

} // namespace kcenon::thread

