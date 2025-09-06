/*
 * BSD 3-Clause License
 * Copyright (c) 2025, DongCheol Shin
 */

#pragma once

#include <memory>

#include "../core/sync/include/error_handling.h"
#include "../core/jobs/include/job.h"

namespace thread_module {

/**
 * @brief Scheduler interface for queuing and retrieving jobs.
 */
class scheduler_interface {
public:
    virtual ~scheduler_interface() = default;

    /**
     * @brief Enqueue a job for processing.
     */
    virtual auto schedule(std::unique_ptr<job>&& work) -> result_void = 0;

    /**
     * @brief Dequeue the next available job.
     */
    virtual auto get_next_job() -> result<std::unique_ptr<job>> = 0;
};

} // namespace thread_module

