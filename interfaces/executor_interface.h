/*
 * BSD 3-Clause License
 * Copyright (c) 2025, DongCheol Shin
 */

#pragma once

#include <memory>

// Use project result type and job forward declarations
#include "../core/sync/include/error_handling.h"
#include "../core/jobs/include/job.h"

namespace thread_module {

/**
 * @brief Executor interface for submitting work and coordinating shutdown.
 */
class executor_interface {
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

} // namespace thread_module

