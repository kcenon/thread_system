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

#pragma once

#include "formatter.h"
#include "convert_string.h"

#include <tuple>
#include <memory>
#include <string>
#include <optional>
#include <string_view>

using namespace utility_module;

namespace thread_module
{
	class job_queue;

	/**
	 * @class job
	 * @brief A base class representing work items to be executed by a job queue.
	 *
	 * The @c job class provides a foundation for all work items that can be submitted to
	 * a @c job_queue. Derived classes can override @c do_work() to implement custom logic.
	 * Once submitted to a queue, the queue is responsible for calling @c do_work() at an
	 * appropriate time, typically in a worker thread or similar context.
	 *
	 * ### Usage Example
	 * @code
	 * // A custom job derived from thread_module::job
	 * class my_job : public thread_module::job
	 * {
	 * public:
	 *     // Initialize the base job with a descriptive name
	 *     my_job() : job("my_custom_job") {}
	 *
	 *     // Override the do_work method to perform actual work
	 *     std::optional<std::string> do_work() override
	 *     {
	 *         // Some hypothetical operation
	 *         bool success = perform_operation();
	 *         if (!success)
	 *             return std::string{"Operation failed"};
	 *
	 *         return std::nullopt; // return empty on success
	 *     }
	 * };
	 *
	 * // Submitting the job to a queue
	 * auto q = std::make_shared<thread_module::job_queue>();
	 * auto job_ptr = std::make_shared<my_job>();
	 * q->enqueue(job_ptr);
	 * @endcode
	 */
	class job
	{
	public:
		/**
		 * @brief Constructs a new @c job with an optional name.
		 * @param name A human-readable name for the job (default is "job").
		 *
		 * The @p name can be used for logging, debugging, or diagnostic purposes.
		 */
		job(const std::string& name = "job");

		/**
		 * @brief Virtual destructor for the @c job class.
		 *
		 * Ensures proper cleanup in derived classes.
		 */
		virtual ~job(void);

		/**
		 * @brief Retrieves the name of this job.
		 * @return A string containing the name of the job.
		 */
		[[nodiscard]] auto get_name(void) const -> std::string;

		/**
		 * @brief Executes the job's work.
		 * @return @c std::optional<std::string>
		 *         - If @c std::nullopt, the job is considered successful.
		 *         - If a non-empty string is returned, it is treated as an error message
		 *           describing why the job failed or what went wrong.
		 *
		 * The default implementation returns @c std::nullopt, indicating success with no
		 * additional logic. Override this in a derived class to perform actual work.
		 */
		[[nodiscard]] virtual auto do_work(void) -> std::optional<std::string>;

		/**
		 * @brief Associates this job with a specific job queue.
		 * @param job_queue A shared pointer to the @c job_queue instance.
		 *
		 * This method stores the provided @c job_queue as a @c std::weak_ptr, meaning
		 * the queue may no longer exist if it is destroyed elsewhere.
		 */
		virtual auto set_job_queue(const std::shared_ptr<job_queue>& job_queue) -> void;

		/**
		 * @brief Retrieves the @c job_queue associated with this job.
		 * @return A @c std::shared_ptr to the associated @c job_queue, or an empty pointer
		 *         if no association exists or if the queue has been destroyed.
		 */
		[[nodiscard]] virtual auto get_job_queue(void) const -> std::shared_ptr<job_queue>;

		/**
		 * @brief Returns a string representation of the job.
		 * @return A string containing diagnostic information about the job (e.g., name).
		 *
		 * This is used internally by formatting functions but can also be used for logging
		 * and debugging. By default, it returns the job's name. Derived classes can provide
		 * additional details if needed.
		 */
		[[nodiscard]] virtual auto to_string(void) const -> std::string;

	protected:
		/** @brief The name of the job, provided during construction. */
		std::string name_;

		/**
		 * @brief A weak pointer to the @c job_queue that manages this job.
		 *
		 * This pointer may expire if the queue is destroyed, so always check validity before use.
		 */
		std::weak_ptr<job_queue> job_queue_;
	};
} // namespace thread_module

// ----------------------------------------------------------------------------
// Formatter specializations for job
// ----------------------------------------------------------------------------

#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for @c thread_module::job.
 *
 * Allows @c job objects to be formatted as strings using the C++20 <format> library
 * (when @c USE_STD_FORMAT is defined).
 *
 * Usage example:
 * @code
 * auto my_job_ptr = std::make_shared<my_job>();
 * std::string output = std::format("Job info: {}", *my_job_ptr);
 * @endcode
 */
template <> struct std::formatter<thread_module::job> : std::formatter<std::string_view>
{
	/**
	 * @brief Formats a @c job object as a string.
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
 * @brief Specialization of std::formatter for wide-character @c thread_module::job.
 *
 * Enables @c job objects to be formatted as wide strings using the C++20 <format> library
 * with wide character support.
 */
template <>
struct std::formatter<thread_module::job, wchar_t> : std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a @c job object as a wide string.
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
 * Allows @c job objects to be formatted using the {fmt} library.
 * Usage example:
 * @code
 * auto my_job_ptr = std::make_shared<my_job>();
 * std::string output = fmt::format("Job info: {}", *my_job_ptr);
 * @endcode
 */
template <> struct fmt::formatter<thread_module::job> : fmt::formatter<std::string_view>
{
	/**
	 * @brief Formats a @c job object as a string using {fmt}.
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