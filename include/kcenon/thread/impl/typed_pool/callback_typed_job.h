// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#pragma once

#include "typed_job.h"

namespace kcenon::thread
{
	/**
	 * @class callback_typed_job_t
	 * @brief A template for creating priority-based jobs that execute a user-defined callback.
	 *
	 * This class inherits from @c typed_job_t and stores a callback function along with a
	 * priority value. When scheduled by a priority-based thread pool or any task scheduler,
	 * the higher-priority jobs generally take precedence.
	 *
	 * @tparam job_type
	 *   The type used to represent the priority level.
	 *   Typically an enum or comparable value to determine job ordering.
	 */
	template <typename job_type>
	class callback_typed_job_t : public typed_job_t<job_type>
	{
	public:
		/**
		 * @brief Constructs a new @c callback_typed_job_t with a callback, priority, and name.
		 *
		 * @param callback
		 *   The function object to be executed when the job is processed. It must return
		 *   a @c common::VoidResult, where an error typically contains additional
		 *   status or error information.
		 * @param priority
		 *   The priority level of the job (e.g., high, normal, low).
		 * @param name
		 *   The name of the job, used primarily for logging or debugging. Defaults to
		 * "typed_job".
		 *
		 * Example usage:
		 * @code
		 * auto jobCallback = []() -> common::VoidResult {
		 *     // Your job logic here
		 *     return common::ok();
		 * };
		 * auto myJob = std::make_shared<callback_typed_job_t<int>>(jobCallback, 10, "MyJob");
		 * @endcode
		 */
		callback_typed_job_t(const std::function<common::VoidResult(void)>& callback,
							job_type priority,
							const std::string& name = "typed_job");

		/**
		 * @brief Virtual destructor for the @c callback_typed_job_t class.
		 */
		~callback_typed_job_t(void) override;

		/**
		 * @brief Executes the stored callback function for this job.
		 *
		 * This method overrides @c typed_job_t::do_work. When invoked by the job executor,
		 * the stored callback will be called and its result will be propagated.
		 *
		 * @return common::VoidResult
		 *   - If the callback returns an error, it typically contains an informational or error
		 * message.
		 *   - If it returns a success value, the execution completed without additional
		 * info.
		 */
		[[nodiscard]] auto do_work(void) -> common::VoidResult override;

	private:
		/**
		 * @brief The user-provided callback function to execute when the job is processed.
		 *
		 * This function should encapsulate the main logic of the job.
		 * It must return a @c common::VoidResult, often representing
		 * any error messages or status feedback.
		 */
		std::function<common::VoidResult(void)> callback_;
	};

	/**
	 * @typedef callback_typed_job
	 * @brief Type alias for a @c callback_typed_job_t that uses @c job_types as its
	 * priority type.
	 */
	using callback_typed_job = callback_typed_job_t<job_types>;

	// Template definitions are provided in-header to support external priority types.
	template <typename job_type>
	callback_typed_job_t<job_type>::callback_typed_job_t(
		const std::function<common::VoidResult(void)>& callback,
		job_type priority,
		const std::string& name)
		: typed_job_t<job_type>(priority, name)
		, callback_(callback)
	{
	}

	template <typename job_type>
	callback_typed_job_t<job_type>::~callback_typed_job_t() = default;

	template <typename job_type>
	auto callback_typed_job_t<job_type>::do_work() -> common::VoidResult
	{
		if (!callback_)
		{
			return common::error_info{static_cast<int>(error_code::invalid_argument), "Callback is null", "thread_system"};
		}

		return callback_();
	}

	// Keep a single explicit instantiation for the default job_types in the .cpp translation unit.
	extern template class callback_typed_job_t<job_types>;
} // namespace kcenon::thread
