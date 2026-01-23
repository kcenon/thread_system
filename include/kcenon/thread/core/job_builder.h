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

#include "job.h"
#include "retry_policy.h"
#include "cancellation_token.h"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace kcenon::thread
{
	/**
	 * @class job_builder
	 * @brief Fluent builder for creating and configuring jobs with composition.
	 *
	 * The job_builder class provides a clean, fluent interface for creating jobs
	 * with various behaviors composed together. This replaces the need for
	 * specialized job subclasses like cancellable_job, callback_job, etc.
	 *
	 * ### Design Philosophy
	 * Instead of inheritance-based specialization:
	 * @code
	 * // Old way (inheritance)
	 * class my_cancellable_callback_job : public callback_job { ... };
	 * @endcode
	 *
	 * Use composition via the builder:
	 * @code
	 * // New way (composition)
	 * auto job = job_builder()
	 *     .name("my_job")
	 *     .work([](){ return common::ok(); })
	 *     .cancellation(token)
	 *     .on_complete([](auto result){ ... })
	 *     .build();
	 * @endcode
	 *
	 * ### Thread Safety
	 * - The builder itself is not thread-safe during construction
	 * - The resulting job is safe to submit to any queue
	 * - Callbacks are invoked on the worker thread
	 *
	 * ### Usage Examples
	 *
	 * #### Basic Job
	 * @code
	 * auto job = job_builder()
	 *     .name("simple_job")
	 *     .work([]{ std::cout << "Hello\n"; return common::ok(); })
	 *     .build();
	 * @endcode
	 *
	 * #### Job with Retry and Callback
	 * @code
	 * auto job = job_builder()
	 *     .name("network_request")
	 *     .work([]{ return fetch_data(); })
	 *     .retry(retry_policy::exponential_backoff(3))
	 *     .on_error([](const auto& err) {
	 *         log_error("Failed: {}", err.message);
	 *     })
	 *     .build();
	 * @endcode
	 *
	 * #### Custom Job Type with Builder
	 * @code
	 * class MyJob : public job {
	 * public:
	 *     MyJob(int value) : job("my_job"), value_(value) {}
	 *     auto do_work() -> common::VoidResult override { ... }
	 * private:
	 *     int value_;
	 * };
	 *
	 * auto job = job_builder()
	 *     .from<MyJob>(42)  // Creates MyJob with constructor arg 42
	 *     .priority(job_priority::high)
	 *     .timeout(std::chrono::seconds(30))
	 *     .build();
	 * @endcode
	 */
	class job_builder
	{
	public:
		/**
		 * @brief Default constructor.
		 */
		job_builder() = default;

		/**
		 * @brief Sets the job name.
		 * @param name Descriptive name for the job
		 * @return Reference to this builder for chaining
		 */
		auto name(const std::string& name) -> job_builder&
		{
			name_ = name;
			return *this;
		}

		/**
		 * @brief Sets the work function for the job.
		 *
		 * @param work_fn Function that performs the job's work
		 * @return Reference to this builder for chaining
		 *
		 * @note The work function should return common::ok() on success
		 *       or common::error_info on failure.
		 */
		auto work(std::function<common::VoidResult()> work_fn) -> job_builder&
		{
			work_fn_ = std::move(work_fn);
			return *this;
		}

		/**
		 * @brief Sets the work function with data parameter.
		 *
		 * @param data Binary data to pass to the work function
		 * @param work_fn Function that processes the data
		 * @return Reference to this builder for chaining
		 */
		auto work_with_data(
			const std::vector<uint8_t>& data,
			std::function<common::VoidResult(const std::vector<uint8_t>&)> work_fn) -> job_builder&
		{
			data_ = data;
			data_work_fn_ = std::move(work_fn);
			return *this;
		}

		/**
		 * @brief Sets a cancellation token for cooperative cancellation.
		 *
		 * @param token Cancellation token to use
		 * @return Reference to this builder for chaining
		 */
		auto cancellation(const cancellation_token& token) -> job_builder&
		{
			cancellation_token_ = token;
			has_cancellation_ = true;
			return *this;
		}

		/**
		 * @brief Sets a completion callback.
		 *
		 * @param callback Function called when job completes (success or failure)
		 * @return Reference to this builder for chaining
		 */
		auto on_complete(std::function<void(common::VoidResult)> callback) -> job_builder&
		{
			on_complete_ = std::move(callback);
			return *this;
		}

		/**
		 * @brief Sets an error callback.
		 *
		 * @param callback Function called only when job fails
		 * @return Reference to this builder for chaining
		 */
		auto on_error(std::function<void(const common::error_info&)> callback) -> job_builder&
		{
			on_error_ = std::move(callback);
			return *this;
		}

		/**
		 * @brief Sets the job priority.
		 *
		 * @param prio Priority level for scheduling
		 * @return Reference to this builder for chaining
		 */
		auto priority(job_priority prio) -> job_builder&
		{
			priority_ = prio;
			has_priority_ = true;
			return *this;
		}

		/**
		 * @brief Sets the retry policy.
		 *
		 * @param policy Retry behavior configuration
		 * @return Reference to this builder for chaining
		 */
		auto retry(const retry_policy& policy) -> job_builder&
		{
			retry_policy_ = policy;
			has_retry_ = true;
			return *this;
		}

		/**
		 * @brief Sets the execution timeout.
		 *
		 * @param timeout Maximum execution time allowed
		 * @return Reference to this builder for chaining
		 */
		auto timeout(std::chrono::milliseconds timeout) -> job_builder&
		{
			timeout_ = timeout;
			has_timeout_ = true;
			return *this;
		}

		/**
		 * @brief Creates a job from a custom job type with constructor arguments.
		 *
		 * @tparam JobType The job class type (must derive from job)
		 * @tparam Args Constructor argument types
		 * @param args Arguments to forward to the job constructor
		 * @return Reference to this builder for chaining
		 *
		 * @example
		 * @code
		 * class MyJob : public job {
		 * public:
		 *     MyJob(int x, const std::string& s) : job(s), value_(x) {}
		 *     auto do_work() -> common::VoidResult override { ... }
		 * private:
		 *     int value_;
		 * };
		 *
		 * auto job = job_builder()
		 *     .from<MyJob>(42, "custom_job")
		 *     .priority(job_priority::high)
		 *     .build();
		 * @endcode
		 */
		template<typename JobType, typename... Args>
		auto from(Args&&... args) -> job_builder&
		{
			static_assert(std::is_base_of_v<job, JobType>,
				"JobType must be derived from kcenon::thread::job");

			custom_job_factory_ = [args = std::make_tuple(std::forward<Args>(args)...)]() mutable
			{
				return std::apply([](auto&&... a) {
					return std::make_unique<JobType>(std::forward<decltype(a)>(a)...);
				}, std::move(args));
			};
			use_custom_job_ = true;
			return *this;
		}

		/**
		 * @brief Builds and returns the configured job.
		 *
		 * @return Unique pointer to the constructed job
		 *
		 * @note If no work function is set and no custom job type is used,
		 *       the job's do_work() will return an error.
		 */
		[[nodiscard]] auto build() -> std::unique_ptr<job>
		{
			std::unique_ptr<job> result;

			if (use_custom_job_ && custom_job_factory_)
			{
				result = custom_job_factory_();
			}
			else
			{
				result = std::make_unique<built_job>(
					name_.empty() ? "builder_job" : name_,
					data_,
					std::move(work_fn_),
					std::move(data_work_fn_)
				);
			}

			// Apply composition
			if (has_cancellation_)
			{
				result->with_cancellation(cancellation_token_);
			}
			if (on_complete_)
			{
				result->with_on_complete(std::move(on_complete_));
			}
			if (on_error_)
			{
				result->with_on_error(std::move(on_error_));
			}
			if (has_priority_)
			{
				result->with_priority(priority_);
			}
			if (has_retry_)
			{
				result->with_retry(retry_policy_);
			}
			if (has_timeout_)
			{
				result->with_timeout(timeout_);
			}

			return result;
		}

		/**
		 * @brief Builds and returns the configured job as a shared pointer.
		 *
		 * @return Shared pointer to the constructed job
		 */
		[[nodiscard]] auto build_shared() -> std::shared_ptr<job>
		{
			return build();
		}

	private:
		/**
		 * @class built_job
		 * @brief Internal job implementation created by the builder.
		 *
		 * This class is used when no custom job type is specified via from<>().
		 * It wraps the work function provided to the builder.
		 */
		class built_job : public job
		{
		public:
			built_job(
				const std::string& name,
				const std::vector<uint8_t>& data,
				std::function<common::VoidResult()> work_fn,
				std::function<common::VoidResult(const std::vector<uint8_t>&)> data_work_fn)
				: job(data.empty() ? name : name)
				, work_fn_(std::move(work_fn))
				, data_work_fn_(std::move(data_work_fn))
			{
				if (!data.empty())
				{
					data_ = data;
				}
			}

			[[nodiscard]] auto do_work() -> common::VoidResult override
			{
				// Check cancellation before starting
				if (cancellation_token_.is_cancelled())
				{
					auto result = make_error_result(error_code::operation_canceled);
					invoke_callbacks(result);
					return result;
				}

				common::VoidResult result = [this]() -> common::VoidResult {
					if (data_work_fn_ && !data_.empty())
					{
						return data_work_fn_(data_);
					}
					else if (work_fn_)
					{
						return work_fn_();
					}
					else
					{
						return common::error_info{
							static_cast<int>(error_code::not_implemented),
							"No work function provided to job_builder",
							"thread_system"
						};
					}
				}();

				// Invoke callbacks
				invoke_callbacks(result);

				return result;
			}

		private:
			std::function<common::VoidResult()> work_fn_;
			std::function<common::VoidResult(const std::vector<uint8_t>&)> data_work_fn_;
		};

		// Builder state
		std::string name_;
		std::vector<uint8_t> data_;
		std::function<common::VoidResult()> work_fn_;
		std::function<common::VoidResult(const std::vector<uint8_t>&)> data_work_fn_;

		cancellation_token cancellation_token_;
		bool has_cancellation_{false};

		std::function<void(common::VoidResult)> on_complete_;
		std::function<void(const common::error_info&)> on_error_;

		job_priority priority_{job_priority::normal};
		bool has_priority_{false};

		retry_policy retry_policy_;
		bool has_retry_{false};

		std::chrono::milliseconds timeout_{0};
		bool has_timeout_{false};

		std::function<std::unique_ptr<job>()> custom_job_factory_;
		bool use_custom_job_{false};
	};

	/**
	 * @brief Convenience function to create a job builder.
	 *
	 * @return A new job_builder instance
	 *
	 * @example
	 * @code
	 * auto job = make_job()
	 *     .name("quick_job")
	 *     .work([]{ return common::ok(); })
	 *     .build();
	 * @endcode
	 */
	[[nodiscard]] inline auto make_job() -> job_builder
	{
		return job_builder{};
	}

} // namespace kcenon::thread
