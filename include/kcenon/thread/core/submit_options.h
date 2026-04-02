// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#pragma once

/**
 * @file submit_options.h
 * @brief Options struct for unified submit() API
 * @date 2026-01-18
 *
 * This file defines the submit_options struct used by the unified submit() methods
 * in thread_pool. It provides a flexible way to configure job submission behavior.
 */

#include <string>
#include <optional>
#include <chrono>

namespace kcenon::thread {

/**
 * @brief Options for submitting jobs to the thread pool.
 *
 * This struct provides a unified way to configure job submission behavior,
 * replacing the need for multiple submit method variants.
 *
 * ### Basic Usage
 * @code
 * // Default options (equivalent to submit_async)
 * auto future = pool->submit([]{ return 42; });
 *
 * // With job name
 * auto future = pool->submit([]{ return 42; }, {.name = "compute_task"});
 * @endcode
 *
 * ### Batch Usage
 * @code
 * std::vector<std::function<int()>> tasks = {...};
 *
 * // Get futures for each task
 * auto futures = pool->submit(std::move(tasks));
 *
 * // Wait for all and get results
 * auto results = pool->submit(std::move(tasks), {.wait_all = true});
 *
 * // Get first completed result
 * auto result = pool->submit(std::move(tasks), {.wait_any = true});
 * @endcode
 */
struct submit_options {
    /**
     * @brief Optional name for the job (useful for debugging/tracing).
     *
     * When empty, a default name like "async_job" is used.
     */
    std::string name;

    /**
     * @brief If true, wait for all tasks and return results directly.
     *
     * Only applicable for batch submissions. When set:
     * - submit() blocks until all tasks complete
     * - Returns std::vector<R> instead of std::vector<std::future<R>>
     *
     * @note Mutually exclusive with wait_any
     */
    bool wait_all = false;

    /**
     * @brief If true, return the first completed result.
     *
     * Only applicable for batch submissions. When set:
     * - submit() blocks until any task completes
     * - Returns R instead of std::vector<std::future<R>>
     *
     * @note Mutually exclusive with wait_all
     */
    bool wait_any = false;

    /**
     * @brief Default constructor with all defaults.
     */
    submit_options() = default;

    /**
     * @brief Construct with job name only.
     * @param job_name Name for the job.
     */
    explicit submit_options(std::string job_name) : name(std::move(job_name)) {}

    /**
     * @brief Create options for a named job.
     * @param job_name Name for the job.
     * @return submit_options with name set.
     */
    static submit_options named(std::string job_name) {
        submit_options opts;
        opts.name = std::move(job_name);
        return opts;
    }

    /**
     * @brief Create options for wait_all batch operation.
     * @return submit_options with wait_all = true.
     */
    static submit_options all() {
        submit_options opts;
        opts.wait_all = true;
        return opts;
    }

    /**
     * @brief Create options for wait_any batch operation.
     * @return submit_options with wait_any = true.
     */
    static submit_options any() {
        submit_options opts;
        opts.wait_any = true;
        return opts;
    }
};

}  // namespace kcenon::thread
