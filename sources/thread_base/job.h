/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions of source code must reproduce the above copyright notice,
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

#pragma once

#include "formatter.h"
#include "convert_string.h"

#include <tuple>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <string_view>

using namespace utility_module;

namespace thread_module
{
	class job_queue;

	/**
	 * @class job
	 * @brief Represents a unit of work (task) to be executed, typically by a job queue.
	 *
	 * The @c job class provides a base interface for scheduling and executing discrete
	 * tasks within a multi-threaded environment. Derived classes can override the
	 * @c do_work() method to implement custom logic for their specific tasks.
	 *
	 * ### Thread-Safety
	 * - @c do_work() will generally be called from a worker thread. If your task accesses
	 *   shared data or interacts with shared resources, you must ensure your implementation
	 *   is thread-safe.
	 * - The job itself is meant to be used via @c std::shared_ptr, making it easier to
	 *   manage lifetimes across multiple threads.
	 *
	 * ### Error Handling
	 * - A job returns a @c std::optional<std::string> from @c do_work().
	 *   - Returning @c std::nullopt indicates success.
	 *   - Returning a non-empty string indicates an error message or reason for failure.
	 * - Error messages can be used for logging, debugging, or to retry a job if desired.
	 *
	 * ### Usage Example
	 * @code
	 * // 1. Create a custom job that overrides do_work().
	 * class my_job : public thread_module::job
	 * {
	 * public:
	 *     my_job() : job("my_custom_job") {}
	 *
	 *     std::optional<std::string> do_work() override
	 *     {
	 *         // Execute custom logic:
	 *         bool success = perform_operation();
	 *         if (!success)
	 *             return std::string{"Operation failed in my_custom_job"};
	 *
	 *         return std::nullopt; // success
	 *     }
	 * };
	 *
	 * // 2. Submit the job to a queue.
	 * auto q = std::make_shared<thread_module::job_queue>();
	 * auto job_ptr = std::make_shared<my_job>();
	 * q->enqueue(job_ptr);
	 * @endcode
	 */
	class job
	{
	public:
		/**
		 * @brief Constructs a new @c job with an optional human-readable name.
		 *
		 * Use this constructor when your derived job class does not need to store
		 * any initial data beyond what you might manually store in derived class members.
		 *
		 * @param name A descriptive name for the job (default is "job").
		 *
		 * #### Example
		 * @code
		 * // Creating a basic job with a custom name
		 * auto my_simple_job = std::make_shared<thread_module::job>("simple_job");
		 * @endcode
		 */
		job(const std::string& name = "job");

		/**
		 * @brief Constructs a new @c job with associated raw byte data and a name.
		 *
		 * This constructor is particularly useful if your job needs an inline payload or
		 * associated data that should be passed directly to @c do_work().
		 *
		 * @param data A vector of bytes serving as the job's payload.
		 * @param name A descriptive name for the job (default is "data_job").
		 *
		 * #### Example
		 * @code
		 * // Creating a job that has binary data.
		 * std::vector<uint8_t> my_data = {0xDE, 0xAD, 0xBE, 0xEF};
		 * auto data_job = std::make_shared<thread_module::job>(my_data, "data_handling_job");
		 * @endcode
		 */
		job(const std::vector<uint8_t>& data, const std::string& name = "data_job");

		/**
		 * @brief Virtual destructor for the @c job class to allow proper cleanup in derived
		 * classes.
		 */
		virtual ~job(void);

		/**
		 * @brief Retrieves the name of this job.
		 * @return A string containing the name assigned to this job.
		 *
		 * The name can be useful for logging and diagnostic messages, especially
		 * when multiple jobs are running concurrently.
		 */
		[[nodiscard]] auto get_name(void) const -> std::string;

		/**
		 * @brief The core task execution method to be overridden by derived classes.
		 *
		 * @return A @c std::optional<std::string> indicating success or error:
		 * - @c std::nullopt on success, meaning no error occurred.
		 * - A non-empty string on failure, providing an error message or explanation.
		 *
		 * #### Default Behavior
		 * The base class implementation simply returns @c std::nullopt (i.e., no error).
		 * Override this method in a derived class to perform meaningful work.
		 *
		 * #### Concurrency
		 * - Typically invoked by worker threads in a @c job_queue.
		 * - Ensure that any shared data or resources accessed here are protected with
		 *   appropriate synchronization mechanisms (mutexes, locks, etc.) if needed.
		 */
		[[nodiscard]] virtual auto do_work(void) -> std::optional<std::string>;

		/**
		 * @brief Associates this job with a specific @c job_queue.
		 *
		 * Once assigned, the job can be aware of the queue that manages it,
		 * enabling scenarios like re-enqueuing itself upon partial failure,
		 * or querying the queue for state (depending on the queue's interface).
		 *
		 * @param job_queue A shared pointer to the @c job_queue instance.
		 *
		 * #### Implementation Detail
		 * - Stored internally as a @c std::weak_ptr. If the queue is destroyed,
		 *   the pointer becomes invalid.
		 */
		virtual auto set_job_queue(const std::shared_ptr<job_queue>& job_queue) -> void;

		/**
		 * @brief Retrieves the @c job_queue associated with this job, if any.
		 *
		 * @return A @c std::shared_ptr to the associated @c job_queue.
		 *         Will be empty if no queue was set or if the queue has already been destroyed.
		 *
		 * #### Usage Example
		 * @code
		 * auto jq = get_job_queue();
		 * if (jq)
		 * {
		 *     // Safe to use jq here
		 * }
		 * else
		 * {
		 *     // The queue is no longer valid
		 * }
		 * @endcode
		 */
		[[nodiscard]] virtual auto get_job_queue(void) const -> std::shared_ptr<job_queue>;

		/**
		 * @brief Provides a string representation of the job for logging or debugging.
		 *
		 * By default, this returns the job's name. Derived classes can override
		 * to include extra diagnostic details (e.g., job status, data contents, etc.).
		 *
		 * @return A string describing the job (e.g., @c name_).
		 *
		 * #### Example
		 * @code
		 * std::shared_ptr<my_job> job_ptr = std::make_shared<my_job>();
		 * // ...
		 * std::string desc = job_ptr->to_string(); // "my_custom_job", for instance
		 * @endcode
		 */
		[[nodiscard]] virtual auto to_string(void) const -> std::string;

	protected:
		/**
		 * @brief The descriptive name of the job, used primarily for identification and logging.
		 */
		std::string name_;

		/**
		 * @brief An optional container of raw byte data that may be used by the job.
		 *
		 * If the constructor without the data parameter is used, this vector will remain empty
		 * unless manually populated by derived classes or other means.
		 */
		std::vector<uint8_t> data_;

		/**
		 * @brief A weak reference to the @c job_queue that currently manages this job.
		 *
		 * This reference can expire if the queue is destroyed, so always lock it into a
		 * @c std::shared_ptr before use to avoid invalid access.
		 */
		std::weak_ptr<job_queue> job_queue_;
	};
} // namespace thread_module

// ----------------------------------------------------------------------------
// Formatter specializations for job
// ----------------------------------------------------------------------------

#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for @c thread_module::job (narrow string).
 *
 * Enables direct usage of std::format or std::format-like functionality with
 * @c job objects, converting them to a string via @c job::to_string().
 *
 * #### Example
 * @code
 * std::shared_ptr<thread_module::job> my_job_ptr = std::make_shared<thread_module::job>();
 * std::string output = std::format("Job info: {}", *my_job_ptr); // calls to_string()
 * @endcode
 */
template <> struct std::formatter<thread_module::job> : std::formatter<std::string_view>
{
	/**
	 * @brief Formats a @c job object into a string representation using @c job::to_string().
	 *
	 * @tparam FormatContext The type of the format context.
	 * @param item The @c job object to format.
	 * @param ctx  The format context for the output.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_module::job& item, FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for @c thread_module::job (wide string).
 *
 * Allows wide-character formatting with std::format-like functionality, converting
 * the @c job's string representation to wide characters before formatting.
 *
 * #### Example
 * @code
 * std::shared_ptr<thread_module::job> my_job_ptr = std::make_shared<thread_module::job>();
 * std::wstring output = std::format(L"Job info: {}", *my_job_ptr);
 * @endcode
 */
template <>
struct std::formatter<thread_module::job, wchar_t> : std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a @c job object into a wide string representation using @c job::to_string().
	 *
	 * @tparam FormatContext The type of the format context.
	 * @param item The @c job object to format.
	 * @param ctx  The wide-character format context.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_module::job& item, FormatContext& ctx) const
	{
		auto str = item.to_string();
		auto wstr = convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};

#else  // USE_STD_FORMAT

/**
 * @brief Specialization of fmt::formatter for @c thread_module::job.
 *
 * Allows objects of type @c job to be formatted using the {fmt} library,
 * converting them to a string via @c job::to_string() internally.
 *
 * #### Example
 * @code
 * auto my_job_ptr = std::make_shared<thread_module::job>("example_job");
 * std::string output = fmt::format("Job info: {}", *my_job_ptr); // calls to_string()
 * @endcode
 */
template <> struct fmt::formatter<thread_module::job> : fmt::formatter<std::string_view>
{
	/**
	 * @brief Formats a @c job object into a string using @c job::to_string().
	 *
	 * @tparam FormatContext The type of the format context.
	 * @param item The @c job object to format.
	 * @param ctx  The format context for the output.
	 * @return An iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const thread_module::job& item, FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};
#endif // USE_STD_FORMAT
