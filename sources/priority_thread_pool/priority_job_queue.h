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

#include "job_queue.h"
#include "priority_job.h"
#include "convert_string.h"
#include "job_priorities.h"

#include <map>

using namespace thread_module;

namespace priority_thread_pool_module
{
	/**
	 * @class priority_job_queue
	 * @brief A queue for managing priority jobs.
	 *
	 * This template class implements a queue that manages jobs with different priority levels.
	 * It inherits from job_queue and std::enable_shared_from_this to allow creating shared_ptr from
	 * this.
	 *
	 * @tparam priority_type The type used to represent the priority levels.
	 */
	template <typename priority_type = job_priorities> class priority_job_queue_t : public job_queue
	{
	public:
		/**
		 * @brief Constructs a new priority_job_queue object.
		 */
		priority_job_queue_t(void);

		/**
		 * @brief Virtual destructor for the priority_job_queue class.
		 */
		~priority_job_queue_t(void) override;

		/**
		 * @brief Enqueues a job into the queue.
		 * @param value A unique pointer to the job to be enqueued.
		 * @return std::optional<std::string> A tuple containing:
		 *         - bool: Indicates whether the enqueue operation was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		[[nodiscard]] auto enqueue(std::unique_ptr<job>&& value)
			-> std::optional<std::string> override;

		/**
		 * @brief Enqueues a priority job into the queue.
		 * @param value A unique pointer to the priority job to be enqueued.
		 * @return std::optional<std::string> A tuple containing:
		 *         - bool: Indicates whether the enqueue operation was successful (true) or not
		 * (false).
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		[[nodiscard]] auto enqueue(std::unique_ptr<priority_job_t<priority_type>>&& value)
			-> std::optional<std::string>;

		/**
		 * @brief Dequeues a job from the queue.
		 * @return std::tuple<std::optional<std::unique_ptr<job>>, std::optional<std::string>> A
		 * tuple containing:
		 *         - std::optional<std::unique_ptr<job>>: The dequeued job if available, or nullopt
		 * if the queue is empty.
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		[[nodiscard]] auto dequeue(void)
			-> std::tuple<std::optional<std::unique_ptr<job>>, std::optional<std::string>> override;

		/**
		 * @brief Dequeues a job from the queue based on the given priorities.
		 * @param priorities A vector of priority levels to consider when dequeuing.
		 * @return std::tuple<std::optional<std::unique_ptr<priority_job<priority_type>>>,
		 * std::optional<std::string>> A tuple containing:
		 *         - std::optional<std::unique_ptr<priority_job<priority_type>>>: The dequeued job
		 * if available, or nullopt if no job is found.
		 *         - std::optional<std::string>: An optional string message, typically used for
		 * error descriptions.
		 */
		[[nodiscard]] auto dequeue(const std::vector<priority_type>& priorities)
			-> std::tuple<std::optional<std::unique_ptr<priority_job_t<priority_type>>>,
						  std::optional<std::string>>;

		/**
		 * @brief Clears all jobs from the queue.
		 */
		auto clear() -> void override;

		/**
		 * @brief Checks if the queue is empty for the given priorities.
		 * @param priorities A vector of priority levels to check.
		 * @return bool True if the queue is empty for all given priorities, false otherwise.
		 */
		[[nodiscard]] auto empty(const std::vector<priority_type>& priorities) const -> bool;

		/**
		 * @brief Converts the job queue to a string representation.
		 * @return std::string A string representation of the job queue.
		 */
		[[nodiscard]] auto to_string(void) const -> std::string override;

	protected:
		/**
		 * @brief Checks if the queue is empty for the given priorities without locking.
		 * @param priorities A vector of priority levels to check.
		 * @return bool True if the queue is empty for all given priorities, false otherwise.
		 */
		[[nodiscard]] auto empty_check_without_lock(
			const std::vector<priority_type>& priorities) const -> bool;

		/**
		 * @brief Attempts to dequeue a job from a specific priority level.
		 * @param priority The priority level to dequeue from.
		 * @return std::optional<std::unique_ptr<priority_job<priority_type>>> The dequeued job if
		 * available, or nullopt if no job is found.
		 */
		[[nodiscard]] auto try_dequeue_from_priority(const priority_type& priority)
			-> std::optional<std::unique_ptr<priority_job_t<priority_type>>>;

	private:
		/** @brief Map of priority deque */
		std::map<priority_type, std::deque<std::unique_ptr<priority_job_t<priority_type>>>> queues_;

		/** @brief The size of the queue */
		std::map<priority_type, std::atomic_size_t> queue_sizes_;
	};

	using priority_job_queue = priority_job_queue_t<job_priorities>;
} // namespace priority_thread_pool_module

// Formatter specializations for priority_job_queue_t<priority_type>
#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for priority_job_queue_t<priority_type>.
 * Enables formatting of priority_job_queue_t<priority_type> enum values as strings in the standard
 * library format.
 */
template <typename priority_type>
struct std::formatter<priority_thread_pool_module::priority_job_queue_t<priority_type>>
	: std::formatter<std::string_view>
{
	/**
	 * @brief Formats a priority_job_queue_t<priority_type> value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param priority The priority_job_queue_t<priority_type> enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const priority_thread_pool_module::priority_job_queue_t<priority_type>& item,
				FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character priority_job_queue_t<priority_type>.
 * Allows priority_job_queue_t<priority_type> enum values to be formatted as wide strings in the
 * standard library format.
 */
template <typename priority_type>
struct std::formatter<priority_thread_pool_module::priority_job_queue_t<priority_type>, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a priority_job_queue_t<priority_type> value as a wide string.
	 * @tparam FormatContext Type of the format context.
	 * @param priority The priority_job_queue_t<priority_type> enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const priority_thread_pool_module::priority_job_queue_t<priority_type>& item,
				FormatContext& ctx) const
	{
		auto str = item.to_string();
		auto wstr = convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
#else
/**
 * @brief Specialization of fmt::formatter for priority_job_queue_t<priority_type>.
 * Enables formatting of priority_job_queue_t<priority_type> enum values using the fmt library.
 */
template <typename priority_type>
struct fmt::formatter<priority_thread_pool_module::priority_job_queue_t<priority_type>>
	: fmt::formatter<std::string_view>
{
	/**
	 * @brief Formats a priority_job_queue_t<priority_type> value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param priority The priority_job_queue_t<priority_type> enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const priority_thread_pool_module::priority_job_queue_t<priority_type>& item,
				FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};
#endif

#include "priority_job_queue.tpp"