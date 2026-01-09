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

#include "dag_job.h"

#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace kcenon::thread
{
	/**
	 * @class dag_job_builder
	 * @brief Fluent builder for creating dag_job instances
	 *
	 * The dag_job_builder provides a convenient way to construct dag_job objects
	 * with dependencies, work functions, and other properties using a fluent API.
	 *
	 * ### Design Pattern
	 * This class implements the Builder pattern with method chaining (fluent interface).
	 * Each setter method returns a reference to the builder, allowing calls to be chained.
	 *
	 * ### Thread Safety
	 * - The builder itself is not thread-safe
	 * - The built dag_job follows dag_job's thread-safety guarantees
	 *
	 * ### Usage Example
	 * @code
	 * auto job = dag_job_builder("process_data")
	 *     .depends_on(fetch_job_id)
	 *     .work([]() -> common::VoidResult {
	 *         process_data();
	 *         return common::ok();
	 *     })
	 *     .on_failure([]() -> common::VoidResult {
	 *         log_failure();
	 *         return common::ok();
	 *     })
	 *     .build();
	 * @endcode
	 */
	class dag_job_builder
	{
	public:
		/**
		 * @brief Constructs a new builder with an optional job name
		 * @param name Human-readable name for the job (default: "dag_job")
		 */
		explicit dag_job_builder(const std::string& name = "dag_job");

		/**
		 * @brief Move constructor
		 */
		dag_job_builder(dag_job_builder&&) noexcept = default;

		/**
		 * @brief Move assignment operator
		 */
		auto operator=(dag_job_builder&&) noexcept -> dag_job_builder& = default;

		/**
		 * @brief Copy constructor (deleted - use move semantics)
		 */
		dag_job_builder(const dag_job_builder&) = delete;

		/**
		 * @brief Copy assignment operator (deleted - use move semantics)
		 */
		auto operator=(const dag_job_builder&) -> dag_job_builder& = delete;

		/**
		 * @brief Destructor
		 */
		~dag_job_builder() = default;

		/**
		 * @brief Sets the work function to execute
		 * @param callable The function to execute
		 * @return Reference to this builder for chaining
		 *
		 * The callable should return common::VoidResult:
		 * - common::ok() on success
		 * - common::VoidResult(error_info) on failure
		 */
		auto work(std::function<common::VoidResult()> callable) -> dag_job_builder&;

		/**
		 * @brief Sets the work function with result
		 * @tparam T The result type
		 * @param callable The function to execute
		 * @return Reference to this builder for chaining
		 *
		 * The result will be stored in the job and can be retrieved
		 * by dependent jobs.
		 */
		template<typename T>
		auto work_with_result(std::function<common::Result<T>()> callable) -> dag_job_builder&
		{
			work_with_result_func_ = [func = std::move(callable)](dag_job& job) -> common::VoidResult {
				auto result = func();
				if (result.is_ok())
				{
					job.set_result(result.value());
					return common::ok();
				}
				return common::VoidResult(result.error());
			};
			return *this;
		}

		/**
		 * @brief Adds a single dependency
		 * @param dependency The job ID to depend on
		 * @return Reference to this builder for chaining
		 */
		auto depends_on(job_id dependency) -> dag_job_builder&;

		/**
		 * @brief Adds multiple dependencies from initializer list
		 * @param dependencies List of job IDs to depend on
		 * @return Reference to this builder for chaining
		 */
		auto depends_on(std::initializer_list<job_id> dependencies) -> dag_job_builder&;

		/**
		 * @brief Adds multiple dependencies from vector
		 * @param dependencies Vector of job IDs to depend on
		 * @return Reference to this builder for chaining
		 */
		auto depends_on(const std::vector<job_id>& dependencies) -> dag_job_builder&;

		/**
		 * @brief Sets the fallback function for failure recovery
		 * @param fallback The function to execute on failure
		 * @return Reference to this builder for chaining
		 *
		 * The fallback function is called when the main work function fails
		 * and the DAG scheduler is configured with dag_failure_policy::fallback.
		 */
		auto on_failure(std::function<common::VoidResult()> fallback) -> dag_job_builder&;

		/**
		 * @brief Builds and returns the configured dag_job
		 * @return A unique_ptr to the built dag_job
		 *
		 * After calling build(), the builder is in a moved-from state and
		 * should not be used again.
		 */
		[[nodiscard]] auto build() -> std::unique_ptr<dag_job>;

	private:
		std::string name_;
		std::function<common::VoidResult()> work_func_;
		std::function<common::VoidResult(dag_job&)> work_with_result_func_;
		std::function<common::VoidResult()> fallback_func_;
		std::vector<job_id> dependencies_;
	};

} // namespace kcenon::thread
