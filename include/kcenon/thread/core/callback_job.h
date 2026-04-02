// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "job.h"

namespace kcenon::thread
{
	/**
	 * @class callback_job
	 * @brief A specialized job class that encapsulates user-defined callbacks.
	 *
	 * The @c callback_job class provides two main mechanisms for defining job behavior:
	 * - A callback that takes no parameters (@c callback_), for general-purpose tasks.
	 * - A callback that takes a @c std::vector<uint8_t> as a parameter (@c data_callback_),
	 *   allowing you to pass raw data to the job when it is constructed.
	 *
	 * Callbacks return @c common::VoidResult for success/error indication.
	 * Legacy callbacks returning @c std::optional<std::string> are also supported
	 * and automatically converted.
	 *
	 * @note For more advanced job composition (cancellation, retry, etc.),
	 * consider using @c job_builder instead.
	 *
	 * @see job_builder
	 */
	class callback_job : public job
	{
	public:
		/**
		 * @brief Constructs a new @c callback_job instance with a parameterless callback.
		 *
		 * Use this constructor if your job logic does not require any input data.
		 *
		 * @param callback A function object that performs the job's work.
		 *                 - Returns @c std::nullopt on success.
		 *                 - Returns a @c std::string on failure.
		 * @param name     An optional name for this job (default is "callback_job").
		 *
		 * Example:
		 * @code
		 * callback_job(
		 *     []() {
		 *         // Job logic...
		 *         return std::nullopt; // or std::string("Error message");
		 *     },
		 *     "my_named_job"
		 * );
		 * @endcode
		 */
		callback_job(const std::function<std::optional<std::string>(void)>& callback,
					 const std::string& name = "callback_job");

		/**
		 * @brief Constructs a new @c callback_job instance with a parameterless callback using modern error handling.
		 *
		 * @param callback A function object that performs the job's work.
		 *                 - Returns @c common::VoidResult on success or failure.
		 * @param name     An optional name for this job (default is "callback_job").
		 */
		callback_job(const std::function<common::VoidResult(void)>& callback,
					 const std::string& name = "callback_job");

		/**
		 * @brief Constructs a new @c callback_job instance with a data-based callback.
		 *
		 * Use this constructor if your job logic requires raw byte data to process.
		 *
		 * @param data_callback A function object that performs the job's work, taking a
		 *                      @c std::vector<uint8_t> as its input.
		 *                      - Returns @c std::nullopt on success.
		 *                      - Returns a @c std::string on failure.
		 * @param data          A vector of bytes that will be passed to @p data_callback
		 *                      when the job is executed.
		 * @param name          An optional name for this job (default is "data_callback_job").
		 *
		 * Example:
		 * @code
		 * std::vector<uint8_t> payload = {0x01, 0x02, 0x03};
		 *
		 * // A job that processes the 'payload'
		 * callback_job(
		 *     [](const std::vector<uint8_t>& data) -> std::optional<std::string> {
		 *         if (data.empty()) {
		 *             return std::string{"No data provided"};
		 *         }
		 *         // Process 'data' here...
		 *         return std::nullopt; // success
		 *     },
		 *     payload,
		 *     "my_data_job"
		 * );
		 * @endcode
		 */
		callback_job(const std::function<std::optional<std::string>(const std::vector<uint8_t>&)>&
						 data_callback,
					 const std::vector<uint8_t>& data,
					 const std::string& name = "data_callback_job");

		/**
		 * @brief Constructs a new @c callback_job instance with a data-based callback using modern error handling.
		 *
		 * @param data_callback A function object that performs the job's work, taking a
		 *                      @c std::vector<uint8_t> as its input.
		 *                      - Returns @c common::VoidResult on success or failure.
		 * @param data          A vector of bytes that will be passed to @p data_callback
		 *                      when the job is executed.
		 * @param name          An optional name for this job (default is "data_callback_job").
		 */
		callback_job(const std::function<common::VoidResult(const std::vector<uint8_t>&)>&
						 data_callback,
					 const std::vector<uint8_t>& data,
					 const std::string& name = "data_callback_job");

		/**
		 * @brief Virtual destructor for proper cleanup in derived classes.
		 */
		~callback_job(void) override;

		/**
		 * @brief Executes the appropriate callback function to perform the job's work.
		 *
		 * @return @c std::optional<std::string>
		 *         - @c std::nullopt if the job completes successfully.
		 *         - A non-empty @c std::string indicating an error message if the job fails.
		 *
		 * This method is typically called by a job-processing mechanism (e.g., a thread pool)
		 * rather than directly by user code. However, manual invocation is possible if desired.
		 *
		 * The logic internally checks which callback has been provided:
		 * - If @c data_callback_ is set, @c do_work() invokes it with the associated data.
		 * - Otherwise, it invokes the parameterless @c callback_.
		 */
		[[nodiscard]] auto do_work(void) -> common::VoidResult override;

	protected:
		/**
		 * @brief Stores the user-defined callback that does not take any parameters.
		 *
		 * This callback is only valid if the @c callback_job was constructed via the
		 * parameterless constructor. If this member is used, @c data_callback_ should be null.
		 */
		std::function<common::VoidResult(void)> callback_;

		/**
		 * @brief Stores the user-defined callback that takes a @c std::vector<uint8_t>.
		 *
		 * This callback is only valid if the @c callback_job was constructed via the
		 * data-based constructor. If this member is used, @c callback_ should be null.
		 */
		std::function<common::VoidResult(const std::vector<uint8_t>&)> data_callback_;
    
    /**
     * @brief Compatibility layer for the old style callbacks.
     */
    std::function<std::optional<std::string>(void)> old_callback_;
    std::function<std::optional<std::string>(const std::vector<uint8_t>&)> old_data_callback_;
	};
} // namespace kcenon::thread
