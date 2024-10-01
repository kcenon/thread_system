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

#include <tuple>
#include <memory>
#include <string>
#include <optional>
#include <functional>

namespace thread_module
{
	class job_queue;

	/**
	 * @class job
	 * @brief Represents a job that can be executed by a job queue.
	 *
	 * This class is a base class for all jobs that can be executed by a job queue.
	 * It provides a callback function that is executed when the job is executed.
	 * The callback function must return a tuple with a boolean value indicating
	 * whether the job was executed successfully and an optional string with an
	 * error message in case the job failed.
	 */
	class job : public std::enable_shared_from_this<job>
	{
	public:
		/**
		 * @brief Constructs a new job object.
		 * @param callback The function to be executed when the job is processed.
		 *        It should return a tuple containing a boolean indicating success and an optional
		 * string message.
		 * @param name The name of the job (default is "job").
		 */
		job(const std::function<std::tuple<bool, std::optional<std::string>>(void)>& callback,
			const std::string& name = "job");

		/**
		 * @brief Virtual destructor for the job class.
		 */
		virtual ~job(void);

		/**
		 * @brief Get a shared pointer to this job object.
		 * @return std::shared_ptr<job> A shared pointer to this job.
		 */
		[[nodiscard]] auto get_ptr(void) -> std::shared_ptr<job>;

		/**
		 * @brief Get the name of the job.
		 * @return std::string The name of the job.
		 */
		[[nodiscard]] auto get_name(void) const -> std::string;

		/**
		 * @brief Execute the job's work.
		 * @return std::tuple<bool, std::optional<std::string>> A tuple containing:
		 *         - bool: Indicates whether the job was successful (true) or not (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions or additional information.
		 */
		[[nodiscard]] virtual auto do_work(void) -> std::tuple<bool, std::optional<std::string>>;

		/**
		 * @brief Set the job queue for this job.
		 * @param job_queue A shared pointer to the job queue to be associated with this job.
		 */
		virtual auto set_job_queue(const std::shared_ptr<job_queue>& job_queue) -> void;

		/**
		 * @brief Get the job queue associated with this job.
		 * @return std::shared_ptr<job_queue> A shared pointer to the associated job queue.
		 *         If no job queue is associated, this may return a null shared_ptr.
		 */
		[[nodiscard]] virtual auto get_job_queue(void) const -> std::shared_ptr<job_queue>;

	protected:
		/** @brief The name of the job */
		std::string name_;

		/** @brief Weak pointer to the associated job queue */
		std::weak_ptr<job_queue> job_queue_;

		/** @brief The callback function to be executed when the job is processed */
		std::function<std::tuple<bool, std::optional<std::string>>(void)> callback_;
	};
} // namespace thread_module