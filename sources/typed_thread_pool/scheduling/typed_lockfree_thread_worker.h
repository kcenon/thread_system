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

#include "../../utilities/core/formatter.h"
#include "../../thread_base/core/thread_base.h"
#include "../../utilities/conversion/convert_string.h"
#include "typed_lockfree_job_queue.h"
#include "../jobs/typed_job.h"

#include <memory>
#include <vector>

using namespace utility_module;
using namespace thread_module;

namespace typed_thread_pool_module
{
	/**
	 * @class typed_lockfree_thread_worker_t
	 * @brief A lock-free worker thread class that processes jobs from a typed lockfree job queue.
	 *
	 * This class extends thread_base by continually retrieving and executing jobs 
	 * from a typed_lockfree_job_queue_t. Each worker can be configured to handle 
	 * specific priority levels, allowing for flexible job distribution among multiple workers.
	 * The lock-free nature provides better scalability and performance under contention.
	 *
	 * @tparam job_type
	 * A type that represents the priority levels. Often an enumeration (e.g., job_types).
	 * By default, it is set to job_types.
	 */
	template <typename job_type = job_types>
	class typed_lockfree_thread_worker_t : public thread_base
	{
	public:
		/**
		 * @brief Constructs a new typed_lockfree_thread_worker_t object.
		 *
		 * @param types
		 * A list of priority levels that this worker is responsible for processing.
		 * If no list is provided, the default is all_types(), meaning
		 * the worker handles all known priority levels.
		 *
		 * @param worker_name
		 * A descriptive name for this worker thread.
		 */
		typed_lockfree_thread_worker_t(std::vector<job_type> types = all_types(),
		                                const std::string& worker_name = "lockfree_worker");

		/**
		 * @brief Virtual destructor for the typed_lockfree_thread_worker_t class.
		 */
		virtual ~typed_lockfree_thread_worker_t(void);

		/**
		 * @brief Assigns a lockfree priority job queue to this worker.
		 *
		 * @param job_queue
		 * A shared pointer to a typed_lockfree_job_queue_t instance from which
		 * this worker will fetch jobs.
		 */
		auto set_job_queue(std::shared_ptr<typed_lockfree_job_queue_t<job_type>> job_queue) -> void;

		/**
		 * @brief Retrieves the list of priority levels this worker handles.
		 *
		 * @return
		 * A std::vector of job_type values indicating the priority
		 * levels the worker will process.
		 */
		[[nodiscard]] auto types(void) const -> std::vector<job_type>;

	protected:
		/**
		 * @brief Determines if there is any pending work for this worker.
		 *
		 * @return
		 * true if there is at least one job waiting that matches the
		 * worker's priority settings. Otherwise, false.
		 */
		[[nodiscard]] auto should_continue_work() const -> bool override;

		/**
		 * @brief Executes pending work by processing one job at a time.
		 *
		 * @return
		 * An optional error if job processing fails.
		 */
		auto do_work() -> result_void override;

	private:
		/**
		 * @brief The priority levels this worker will process.
		 */
		std::vector<job_type> types_;

		/**
		 * @brief The lockfree priority job queue to retrieve and execute jobs from.
		 */
		std::shared_ptr<typed_lockfree_job_queue_t<job_type>> job_queue_;
	};

	/// Convenience typedef for a worker configured with default job_types.
	using typed_lockfree_thread_worker = typed_lockfree_thread_worker_t<job_types>;
} // namespace typed_thread_pool_module

// Formatter specializations for typed_lockfree_thread_worker_t<job_type>
#ifdef USE_STD_FORMAT

template <typename job_type>
struct std::formatter<typed_thread_pool_module::typed_lockfree_thread_worker_t<job_type>>
	: std::formatter<std::string_view>
{
	template <typename FormatContext>
	auto format(const typed_thread_pool_module::typed_lockfree_thread_worker_t<job_type>& item,
				FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};

template <typename job_type>
struct std::formatter<typed_thread_pool_module::typed_lockfree_thread_worker_t<job_type>, wchar_t>
	: std::formatter<std::wstring_view, wchar_t>
{
	template <typename FormatContext>
	auto format(const typed_thread_pool_module::typed_lockfree_thread_worker_t<job_type>& item,
				FormatContext& ctx) const
	{
		auto str = item.to_string();
		auto wstr = convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
#else
template <typename job_type>
struct fmt::formatter<typed_thread_pool_module::typed_lockfree_thread_worker_t<job_type>>
	: fmt::formatter<std::string_view>
{
	template <typename FormatContext>
	auto format(const typed_thread_pool_module::typed_lockfree_thread_worker_t<job_type>& item,
				FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(item.to_string(), ctx);
	}
};
#endif

#include "typed_lockfree_thread_worker.tpp"