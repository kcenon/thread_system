/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, kcenon
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

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>

namespace kcenon::thread::diagnostics
{
	/**
	 * @enum event_type
	 * @brief Type of job execution event.
	 *
	 * @ingroup diagnostics
	 *
	 * Represents the different events that can occur during
	 * a job's lifecycle.
	 */
	enum class event_type
	{
		enqueued,    ///< Job was added to the queue
		dequeued,    ///< Job was taken from queue by a worker
		started,     ///< Job execution started
		completed,   ///< Job completed successfully
		failed,      ///< Job failed with an error
		cancelled,   ///< Job was cancelled
		retried      ///< Job is being retried after failure
	};

	/**
	 * @brief Converts event_type to human-readable string.
	 * @param type The event type to convert.
	 * @return String representation of the event type.
	 */
	[[nodiscard]] inline auto event_type_to_string(event_type type) -> std::string
	{
		switch (type)
		{
			case event_type::enqueued: return "enqueued";
			case event_type::dequeued: return "dequeued";
			case event_type::started: return "started";
			case event_type::completed: return "completed";
			case event_type::failed: return "failed";
			case event_type::cancelled: return "cancelled";
			case event_type::retried: return "retried";
			default: return "unknown";
		}
	}

	/**
	 * @struct job_execution_event
	 * @brief Event data for job execution tracing.
	 *
	 * @ingroup diagnostics
	 *
	 * Contains detailed information about a job execution event,
	 * suitable for logging, tracing, and monitoring purposes.
	 *
	 * ### Event Flow
	 * ```
	 * enqueued → dequeued → started → completed/failed/cancelled
	 *                                  ↓
	 *                               retried → started → ...
	 * ```
	 *
	 * ### Usage Example
	 * @code
	 * pool->diagnostics().add_event_listener(
	 *     std::make_shared<MyEventLogger>()
	 * );
	 *
	 * // In MyEventLogger::on_event
	 * void on_event(const job_execution_event& event) override {
	 *     LOG_INFO("{} [job:{}] {}",
	 *              event.format_timestamp(),
	 *              event.job_name,
	 *              event_type_to_string(event.type));
	 * }
	 * @endcode
	 */
	struct job_execution_event
	{
		/**
		 * @brief Unique identifier for this event.
		 *
		 * Monotonically increasing within the thread pool lifetime.
		 */
		std::uint64_t event_id{0};

		/**
		 * @brief ID of the job this event relates to.
		 */
		std::uint64_t job_id{0};

		/**
		 * @brief Human-readable name of the job.
		 */
		std::string job_name;

		/**
		 * @brief Type of event that occurred.
		 */
		event_type type{event_type::enqueued};

		/**
		 * @brief Time when the event occurred.
		 */
		std::chrono::steady_clock::time_point timestamp;

		/**
		 * @brief System time when the event occurred.
		 *
		 * Used for logging and correlation with external systems.
		 */
		std::chrono::system_clock::time_point system_timestamp;

		/**
		 * @brief ID of the thread that processed this event.
		 *
		 * May be empty for enqueued events.
		 */
		std::thread::id thread_id;

		/**
		 * @brief Worker ID that processed this job.
		 */
		std::size_t worker_id{0};

		// =========================================================================
		// Timing Information
		// =========================================================================

		/**
		 * @brief Time spent waiting in queue before dequeue.
		 *
		 * Only valid for dequeued, started events and later.
		 */
		std::chrono::nanoseconds wait_time{0};

		/**
		 * @brief Time spent executing the job.
		 *
		 * Only valid for completed, failed, cancelled events.
		 */
		std::chrono::nanoseconds execution_time{0};

		// =========================================================================
		// Error Information
		// =========================================================================

		/**
		 * @brief Error code if the job failed.
		 */
		std::optional<int> error_code;

		/**
		 * @brief Error message if the job failed.
		 */
		std::optional<std::string> error_message;

		// =========================================================================
		// Utility Methods
		// =========================================================================

		/**
		 * @brief Formats the event timestamp as ISO 8601 string.
		 * @return Formatted timestamp string.
		 */
		[[nodiscard]] auto format_timestamp() const -> std::string
		{
			auto time_t = std::chrono::system_clock::to_time_t(system_timestamp);
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			    system_timestamp.time_since_epoch()) % 1000;

			char buffer[32];
			std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S",
			              std::gmtime(&time_t));

			return std::string(buffer) + "." +
			       std::to_string(ms.count()) + "Z";
		}

		/**
		 * @brief Converts wait_time to milliseconds.
		 * @return Wait time in milliseconds.
		 */
		[[nodiscard]] auto wait_time_ms() const -> double
		{
			return std::chrono::duration<double, std::milli>(wait_time).count();
		}

		/**
		 * @brief Converts execution_time to milliseconds.
		 * @return Execution time in milliseconds.
		 */
		[[nodiscard]] auto execution_time_ms() const -> double
		{
			return std::chrono::duration<double, std::milli>(execution_time).count();
		}

		/**
		 * @brief Checks if this is a terminal event (job finished).
		 * @return true if completed, failed, or cancelled.
		 */
		[[nodiscard]] auto is_terminal() const -> bool
		{
			return type == event_type::completed ||
			       type == event_type::failed ||
			       type == event_type::cancelled;
		}

		/**
		 * @brief Checks if this event indicates an error.
		 * @return true if failed or cancelled.
		 */
		[[nodiscard]] auto is_error() const -> bool
		{
			return type == event_type::failed ||
			       type == event_type::cancelled;
		}
	};

	/**
	 * @class execution_event_listener
	 * @brief Interface for receiving job execution events.
	 *
	 * @ingroup diagnostics
	 *
	 * Implement this interface to receive notifications about
	 * job execution events for logging, monitoring, or tracing.
	 *
	 * ### Thread Safety
	 * The on_event() method may be called from multiple worker threads
	 * concurrently. Implementations must be thread-safe.
	 *
	 * ### Performance
	 * Event processing should be fast (< 1μs) to avoid impacting
	 * job execution performance. Consider using async logging or
	 * buffering for expensive operations.
	 *
	 * ### Usage Example
	 * @code
	 * class JsonEventLogger : public execution_event_listener {
	 * public:
	 *     void on_event(const job_execution_event& event) override {
	 *         // Fast path: just queue the event
	 *         event_queue_.push(event.to_json());
	 *     }
	 * private:
	 *     concurrent_queue<std::string> event_queue_;
	 * };
	 * @endcode
	 */
	class execution_event_listener
	{
	public:
		/**
		 * @brief Virtual destructor.
		 */
		virtual ~execution_event_listener() = default;

		/**
		 * @brief Called when a job execution event occurs.
		 * @param event The event details.
		 *
		 * @note This method must be thread-safe.
		 * @note This method should be fast (< 1μs) to avoid
		 *       impacting job execution.
		 */
		virtual void on_event(const job_execution_event& event) = 0;
	};

} // namespace kcenon::thread::diagnostics
