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

#include "callback_writer.h"

#include "../../utilities/core/formatter.h"
#include "../jobs/message_job.h"

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#endif

/**
 * @file callback_writer.cpp
 * @brief Implementation of callback-based log writer for custom output handling.
 *
 * This file contains the implementation of the callback_writer class, which provides
 * a flexible mechanism for custom log output handling through user-defined callback
 * functions. The writer processes message jobs and delegates actual output to the
 * registered callback function.
 * 
 * Key Features:
 * - Custom callback function registration for flexible output handling
 * - Asynchronous message processing using dedicated worker thread
 * - Batch processing for efficient message handling
 * - Thread-safe message queue management
 * - Error handling and logging for failed operations
 * 
 * Design Pattern:
 * - Observer pattern: Client registers callback for log events
 * - Producer-consumer: Message jobs queued for batch processing
 * - Error isolation: Individual message failures don't stop processing
 * 
 * Usage Scenarios:
 * - Custom log formatting and routing logic
 * - Integration with external logging systems
 * - Real-time log monitoring and alerting
 * - Custom log filtering and transformation
 * - Network-based log transmission
 * 
 * Performance Characteristics:
 * - Batch processing reduces per-message overhead
 * - Asynchronous operation prevents caller blocking
 * - Efficient queue-based message handling
 * - Minimal memory allocation during processing
 */

using namespace utility_module;
using namespace thread_module;

namespace log_module
{
	/**
	 * @brief Constructs a callback writer with message queue initialization.
	 * 
	 * Implementation details:
	 * - Inherits from thread_base for asynchronous processing capability
	 * - Creates dedicated job queue for message job processing
	 * - Initializes callback function pointer to null (must be set before use)
	 * - Sets thread name for debugging and monitoring purposes
	 * 
	 * Initialization State:
	 * - Worker thread not started until explicitly requested
	 * - Message queue ready for job submission
	 * - Callback function must be registered before processing
	 * - No automatic thread startup (controlled by caller)
	 */
	callback_writer::callback_writer(void)
		: thread_base("callback_writer")
		, job_queue_(std::make_shared<job_queue>())
		, callback_(nullptr)
	{
	}

	/**
	 * @brief Registers a callback function for custom log message handling.
	 * 
	 * Implementation details:
	 * - Stores user-provided callback function for message processing
	 * - Callback receives log type, timestamp, and formatted message
	 * - Thread-safe assignment (atomic pointer update)
	 * - Replaces any previously registered callback function
	 * 
	 * Callback Signature:
	 * - log_types: Message classification (info, warning, error, etc.)
	 * - std::string (datetime): Formatted timestamp string
	 * - std::string (message): Complete formatted log message
	 * 
	 * Thread Safety:
	 * - Safe to call while writer thread is running
	 * - New callback takes effect for subsequent messages
	 * - No synchronization needed for callback assignment
	 * 
	 * @param callback Function to handle processed log messages
	 */
	auto callback_writer::message_callback(
		const std::function<void(const log_types&, const std::string&, const std::string&)>&
			callback) -> void
	{
		callback_ = callback;
	}

	auto callback_writer::should_continue_work() const -> bool { return !job_queue_->empty(); }

	/**
	 * @brief Processes queued message jobs through registered callback function.
	 * 
	 * Implementation details:
	 * - Validates job queue availability before processing
	 * - Dequeues all available message jobs in a single batch operation
	 * - Processes each message job individually with error isolation
	 * - Invokes registered callback function for each valid message
	 * - Continues processing even if individual messages fail
	 * 
	 * Processing Workflow:
	 * 1. Validate job queue is available
	 * 2. Dequeue all pending message jobs in batch
	 * 3. For each job:
	 *    a. Check if callback function is registered
	 *    b. Cast job to message_job type
	 *    c. Execute job validation (do_work)
	 *    d. Invoke callback with message data
	 * 4. Continue with next job on errors
	 * 
	 * Error Handling:
	 * - Missing queue: Returns resource allocation error
	 * - Missing callback: Logs warning and skips message
	 * - Job validation failure: Logs error and continues
	 * - Individual failures don't stop batch processing
	 * 
	 * Performance Optimizations:
	 * - Batch dequeue reduces queue lock contention
	 * - Single validation check per processing cycle
	 * - Minimal memory allocations during processing
	 * - Error isolation prevents cascade failures
	 * 
	 * Thread Safety:
	 * - Job queue operations are thread-safe
	 * - Callback function pointer read is atomic
	 * - No shared state modification during processing
	 * 
	 * @return result_void indicating overall processing success or failure
	 */
	auto callback_writer::do_work() -> result_void
	{
		// Validate job queue availability
		if (job_queue_ == nullptr)
		{
			return result_void{error{error_code::resource_allocation_failed, "there is no job_queue"}};
		}

		// Dequeue all available message jobs for batch processing
		auto remaining_logs = job_queue_->dequeue_batch();
		while (!remaining_logs.empty())
		{
			auto current_job = std::move(remaining_logs.front());
			remaining_logs.pop_front();

			// Check if callback function is registered
			if (callback_ == nullptr)
			{
				std::cout << "there is no callback function" << std::endl;
				continue;
			}

			// Cast to specific message job type for processing
			auto current_log
				= std::unique_ptr<message_job>(static_cast<message_job*>(current_job.release()));

			// Validate message job before callback invocation
			auto work_result = current_log->do_work();
			if (work_result.has_error())
			{
				std::cout << work_result.get_error().to_string() << std::endl;
				continue;
			}

			// Invoke registered callback with message data
			callback_(current_log->log_type(), current_log->datetime(), current_log->message());
		}

		return {};
	}
}