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

#include "gtest/gtest.h"

#include <kcenon/thread/diagnostics/execution_event.h>

#include <chrono>
#include <thread>

using namespace kcenon::thread::diagnostics;

// ============================================================================
// event_type enum tests
// ============================================================================

TEST(EventTypeTest, EventTypeToStringConversion)
{
	EXPECT_EQ(event_type_to_string(event_type::enqueued), "enqueued");
	EXPECT_EQ(event_type_to_string(event_type::dequeued), "dequeued");
	EXPECT_EQ(event_type_to_string(event_type::started), "started");
	EXPECT_EQ(event_type_to_string(event_type::completed), "completed");
	EXPECT_EQ(event_type_to_string(event_type::failed), "failed");
	EXPECT_EQ(event_type_to_string(event_type::cancelled), "cancelled");
	EXPECT_EQ(event_type_to_string(event_type::retried), "retried");
}

TEST(EventTypeTest, InvalidEventTypeReturnsUnknown)
{
	auto invalid_type = static_cast<event_type>(999);
	EXPECT_EQ(event_type_to_string(invalid_type), "unknown");
}

// ============================================================================
// job_execution_event struct tests
// ============================================================================

class JobExecutionEventTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		event_.event_id = 12345;
		event_.job_id = 67890;
		event_.job_name = "ProcessPayment";
		event_.type = event_type::completed;
		event_.timestamp = std::chrono::steady_clock::now();
		event_.system_timestamp = std::chrono::system_clock::now();
		event_.thread_id = std::this_thread::get_id();
		event_.worker_id = 2;
		event_.wait_time = std::chrono::milliseconds(5);
		event_.execution_time = std::chrono::milliseconds(50);
	}

	job_execution_event event_;
};

TEST_F(JobExecutionEventTest, DefaultConstruction)
{
	job_execution_event default_event;

	EXPECT_EQ(default_event.event_id, 0U);
	EXPECT_EQ(default_event.job_id, 0U);
	EXPECT_TRUE(default_event.job_name.empty());
	EXPECT_EQ(default_event.type, event_type::enqueued);
	EXPECT_EQ(default_event.worker_id, 0U);
	EXPECT_EQ(default_event.wait_time.count(), 0);
	EXPECT_EQ(default_event.execution_time.count(), 0);
	EXPECT_FALSE(default_event.error_code.has_value());
	EXPECT_FALSE(default_event.error_message.has_value());
}

TEST_F(JobExecutionEventTest, FormatTimestampReturnsISO8601)
{
	auto timestamp = event_.format_timestamp();

	// Should match pattern: YYYY-MM-DDTHH:MM:SS.mmmZ
	EXPECT_NE(timestamp.find("T"), std::string::npos);
	EXPECT_NE(timestamp.find("Z"), std::string::npos);
	EXPECT_EQ(timestamp.back(), 'Z');
}

TEST_F(JobExecutionEventTest, WaitTimeMsConversion)
{
	EXPECT_DOUBLE_EQ(event_.wait_time_ms(), 5.0);
}

TEST_F(JobExecutionEventTest, ExecutionTimeMsConversion)
{
	EXPECT_DOUBLE_EQ(event_.execution_time_ms(), 50.0);
}

TEST_F(JobExecutionEventTest, IsTerminalForCompletedEvent)
{
	event_.type = event_type::completed;
	EXPECT_TRUE(event_.is_terminal());
}

TEST_F(JobExecutionEventTest, IsTerminalForFailedEvent)
{
	event_.type = event_type::failed;
	EXPECT_TRUE(event_.is_terminal());
}

TEST_F(JobExecutionEventTest, IsTerminalForCancelledEvent)
{
	event_.type = event_type::cancelled;
	EXPECT_TRUE(event_.is_terminal());
}

TEST_F(JobExecutionEventTest, IsNotTerminalForEnqueuedEvent)
{
	event_.type = event_type::enqueued;
	EXPECT_FALSE(event_.is_terminal());
}

TEST_F(JobExecutionEventTest, IsNotTerminalForDequeuedEvent)
{
	event_.type = event_type::dequeued;
	EXPECT_FALSE(event_.is_terminal());
}

TEST_F(JobExecutionEventTest, IsNotTerminalForStartedEvent)
{
	event_.type = event_type::started;
	EXPECT_FALSE(event_.is_terminal());
}

TEST_F(JobExecutionEventTest, IsNotTerminalForRetriedEvent)
{
	event_.type = event_type::retried;
	EXPECT_FALSE(event_.is_terminal());
}

TEST_F(JobExecutionEventTest, IsErrorForFailedEvent)
{
	event_.type = event_type::failed;
	EXPECT_TRUE(event_.is_error());
}

TEST_F(JobExecutionEventTest, IsErrorForCancelledEvent)
{
	event_.type = event_type::cancelled;
	EXPECT_TRUE(event_.is_error());
}

TEST_F(JobExecutionEventTest, IsNotErrorForCompletedEvent)
{
	event_.type = event_type::completed;
	EXPECT_FALSE(event_.is_error());
}

TEST_F(JobExecutionEventTest, IsNotErrorForEnqueuedEvent)
{
	event_.type = event_type::enqueued;
	EXPECT_FALSE(event_.is_error());
}

TEST_F(JobExecutionEventTest, ToJsonContainsRequiredFields)
{
	auto json = event_.to_json();

	EXPECT_NE(json.find("\"event_id\": 12345"), std::string::npos);
	EXPECT_NE(json.find("\"job_id\": 67890"), std::string::npos);
	EXPECT_NE(json.find("\"job_name\": \"ProcessPayment\""), std::string::npos);
	EXPECT_NE(json.find("\"type\": \"completed\""), std::string::npos);
	EXPECT_NE(json.find("\"timestamp\""), std::string::npos);
	EXPECT_NE(json.find("\"thread_id\""), std::string::npos);
	EXPECT_NE(json.find("\"worker_id\": 2"), std::string::npos);
	EXPECT_NE(json.find("\"wait_time_ms\""), std::string::npos);
	EXPECT_NE(json.find("\"execution_time_ms\""), std::string::npos);
	EXPECT_NE(json.find("\"error_code\": null"), std::string::npos);
	EXPECT_NE(json.find("\"error_message\": null"), std::string::npos);
}

TEST_F(JobExecutionEventTest, ToJsonWithErrorCode)
{
	event_.type = event_type::failed;
	event_.error_code = 500;

	auto json = event_.to_json();

	EXPECT_NE(json.find("\"error_code\": 500"), std::string::npos);
}

TEST_F(JobExecutionEventTest, ToJsonWithErrorMessage)
{
	event_.type = event_type::failed;
	event_.error_message = "Connection refused";

	auto json = event_.to_json();

	EXPECT_NE(json.find("\"error_message\": \"Connection refused\""), std::string::npos);
}

TEST_F(JobExecutionEventTest, ToStringContainsEventInfo)
{
	auto str = event_.to_string();

	EXPECT_NE(str.find("Event#12345"), std::string::npos);
	EXPECT_NE(str.find("job:ProcessPayment#67890"), std::string::npos);
	EXPECT_NE(str.find("type:completed"), std::string::npos);
	EXPECT_NE(str.find("worker:2"), std::string::npos);
	EXPECT_NE(str.find("wait:"), std::string::npos);
	EXPECT_NE(str.find("exec:"), std::string::npos);
}

TEST_F(JobExecutionEventTest, ToStringWithError)
{
	event_.type = event_type::failed;
	event_.error_code = 404;
	event_.error_message = "Resource not found";

	auto str = event_.to_string();

	EXPECT_NE(str.find("error:"), std::string::npos);
	EXPECT_NE(str.find("code=404"), std::string::npos);
	EXPECT_NE(str.find("Resource not found"), std::string::npos);
}

TEST_F(JobExecutionEventTest, ToStringWithOnlyErrorCode)
{
	event_.type = event_type::failed;
	event_.error_code = 503;

	auto str = event_.to_string();

	EXPECT_NE(str.find("error:"), std::string::npos);
	EXPECT_NE(str.find("code=503"), std::string::npos);
}

TEST_F(JobExecutionEventTest, ToStringWithOnlyErrorMessage)
{
	event_.type = event_type::failed;
	event_.error_message = "Timeout exceeded";

	auto str = event_.to_string();

	EXPECT_NE(str.find("error:"), std::string::npos);
	EXPECT_NE(str.find("Timeout exceeded"), std::string::npos);
}

// ============================================================================
// execution_event_listener interface tests
// ============================================================================

class TestEventListener : public execution_event_listener
{
public:
	void on_event(const job_execution_event& event) override
	{
		events.push_back(event);
	}

	std::vector<job_execution_event> events;
};

TEST(ExecutionEventListenerTest, ListenerReceivesEvents)
{
	auto listener = std::make_shared<TestEventListener>();

	job_execution_event event;
	event.event_id = 1;
	event.job_id = 100;
	event.job_name = "TestJob";
	event.type = event_type::completed;

	listener->on_event(event);

	ASSERT_EQ(listener->events.size(), 1U);
	EXPECT_EQ(listener->events[0].event_id, 1U);
	EXPECT_EQ(listener->events[0].job_id, 100U);
	EXPECT_EQ(listener->events[0].job_name, "TestJob");
	EXPECT_EQ(listener->events[0].type, event_type::completed);
}

TEST(ExecutionEventListenerTest, ListenerReceivesMultipleEvents)
{
	auto listener = std::make_shared<TestEventListener>();

	for (int i = 0; i < 5; ++i)
	{
		job_execution_event event;
		event.event_id = static_cast<uint64_t>(i);
		event.job_id = static_cast<uint64_t>(i * 10);
		listener->on_event(event);
	}

	EXPECT_EQ(listener->events.size(), 5U);
}
