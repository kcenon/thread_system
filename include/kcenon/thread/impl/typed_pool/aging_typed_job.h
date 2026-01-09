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

#include "typed_job.h"
#include "priority_aging_config.h"

#include <functional>
#include <string>

namespace kcenon::thread
{
	/**
	 * @class aging_typed_job_t
	 * @brief A typed job with priority aging support.
	 *
	 * This class extends typed_job_t to include aging information,
	 * allowing the job's effective priority to increase over time
	 * based on how long it has been waiting in the queue.
	 *
	 * @tparam job_type The type used to represent priority levels.
	 *
	 * ### Features
	 * - Tracks enqueue time automatically
	 * - Maintains current priority boost
	 * - Provides effective priority calculation
	 * - Supports configurable max boost
	 *
	 * ### Example Usage
	 * @code{.cpp}
	 * auto job = std::make_unique<aging_typed_job_t<job_types>>(
	 *     job_types::Background,
	 *     []() -> common::VoidResult {
	 *         // Do work
	 *         return common::ok();
	 *     },
	 *     "background_task"
	 * );
	 *
	 * // Later, apply boost
	 * job->apply_boost(1);
	 * auto effective = job->get_aged_priority().effective_priority();
	 * @endcode
	 */
	template <typename job_type>
	class aging_typed_job_t : public typed_job_t<job_type>
	{
	public:
		using base = typed_job_t<job_type>;

		/**
		 * @brief Constructs a new aging typed job.
		 *
		 * @param priority The base priority level for this job.
		 * @param work The work function to execute.
		 * @param name An optional name for the job.
		 */
		aging_typed_job_t(job_type priority,
		                  std::function<common::VoidResult()> work,
		                  const std::string& name = "aging_job");

		/**
		 * @brief Destroys the aging typed job.
		 */
		~aging_typed_job_t() override;

		// Non-copyable
		aging_typed_job_t(const aging_typed_job_t&) = delete;
		auto operator=(const aging_typed_job_t&) -> aging_typed_job_t& = delete;

		// Movable
		aging_typed_job_t(aging_typed_job_t&&) noexcept = default;
		auto operator=(aging_typed_job_t&&) noexcept -> aging_typed_job_t& = default;

		/**
		 * @brief Executes the job's work function.
		 *
		 * @return The result of executing the work function.
		 */
		[[nodiscard]] auto do_work() -> common::VoidResult override;

		/**
		 * @brief Gets the aged priority information.
		 *
		 * @return A const reference to the aged_priority structure.
		 */
		[[nodiscard]] auto get_aged_priority() const -> const aged_priority<job_type>&;

		/**
		 * @brief Gets the aged priority information (mutable).
		 *
		 * @return A reference to the aged_priority structure.
		 */
		[[nodiscard]] auto get_aged_priority() -> aged_priority<job_type>&;

		/**
		 * @brief Applies a priority boost to this job.
		 *
		 * The boost is capped at the maximum boost value.
		 *
		 * @param boost_amount The amount to boost.
		 */
		auto apply_boost(int boost_amount) -> void;

		/**
		 * @brief Resets the priority boost to zero.
		 */
		auto reset_boost() -> void;

		/**
		 * @brief Sets the maximum allowed boost.
		 *
		 * @param max The maximum boost value.
		 */
		auto set_max_boost(int max) -> void;

		/**
		 * @brief Gets the maximum allowed boost.
		 *
		 * @return The maximum boost value.
		 */
		[[nodiscard]] auto get_max_boost() const -> int;

		/**
		 * @brief Checks if this job has reached maximum boost.
		 *
		 * @return true if the current boost equals or exceeds max boost.
		 */
		[[nodiscard]] auto is_max_boosted() const -> bool;

		/**
		 * @brief Gets the effective priority after applying boost.
		 *
		 * @return The effective priority value.
		 */
		[[nodiscard]] auto effective_priority() const -> job_type;

		/**
		 * @brief Gets the time this job has been waiting.
		 *
		 * @return The wait time in milliseconds.
		 */
		[[nodiscard]] auto wait_time() const -> std::chrono::milliseconds;

		/**
		 * @brief Creates job_info for starvation callbacks.
		 *
		 * @return A job_info structure with current state.
		 */
		[[nodiscard]] auto to_job_info() const -> job_info;

	private:
		aged_priority<job_type> aged_priority_;
		int max_boost_{3};
		std::function<common::VoidResult()> work_;
	};

	/**
	 * @typedef aging_typed_job
	 * @brief A convenient alias for aging_typed_job_t using the job_types type.
	 */
	using aging_typed_job = aging_typed_job_t<job_types>;

} // namespace kcenon::thread
