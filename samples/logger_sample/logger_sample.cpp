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

#include <iostream>

#include "logger.h"
#include "formatter.h"

using namespace log_module;
using namespace utility_module;

bool use_backup_ = false;
uint32_t max_lines_ = 0;
uint16_t wait_interval_ = 100;
uint32_t test_line_count_ = 10000;
log_types file_target_ = log_types::Sequence;
log_types console_target_ = log_types::Sequence;
log_types callback_target_ = log_types::None;

auto initialize_logger() -> std::tuple<bool, std::optional<std::string>>
{
	logger::handle().set_title("logger_sample");
	logger::handle().set_use_backup(use_backup_);
	logger::handle().set_max_lines(max_lines_);
	logger::handle().file_target(file_target_);
	logger::handle().console_target(console_target_);
	logger::handle().callback_target(callback_target_);
	logger::handle().message_callback(
		[](const log_types& type, const std::string& datetime, const std::string& message)
		{ std::cout << formatter::format("[{}][{}] {}\n", datetime, type, message); });
	if (wait_interval_ > 0)
	{
		logger::handle().set_wake_interval(std::chrono::milliseconds(wait_interval_));
	}

	return logger::handle().start();
}

auto main() -> int
{
	auto [started, start_error] = initialize_logger();
	if (start_error.has_value())
	{
		std::cerr << formatter::format("error starting logger: {}\n",
									   start_error.value_or("unknown error"));
		return 0;
	}

	for (auto index = 0; index < test_line_count_; ++index)
	{
		logger::handle().write(log_types::Debug, "안녕, World!: {}", index);
		logger::handle().write(log_types::Debug, "테스트 #{} - Hello, 世界!", index);
		logger::handle().write(log_types::Debug, "警告 {}: こんにちは", index);

		logger::handle().write(log_types::Sequence, L"안녕, World!: {}", index);
		logger::handle().write(log_types::Sequence, L"테스트 #{} - Hello, 世界!", index);
		logger::handle().write(log_types::Sequence, L"警告 {}: こんにちは", index);

		logger::handle().write(log_types::Parameter, "복합 테스트 - 값: {}, 이름: {}", index,
							   "홍길동");
		logger::handle().write(log_types::Parameter, L"복합 테스트 - 값: {}, 이름: {}", index,
							   L"홍길동");

		logger::handle().write(log_types::Information,
							   "여러 줄 테스트:\n  라인 1: {}\n  라인 2: {}\n  라인 3: {}",
							   "안녕하세요", "Hello, World", "こんにちは");
		logger::handle().write(log_types::Information,
							   L"여러 줄 테스트:\n  라인 1: {}\n  라인 2: {}\n  라인 3: {}",
							   L"안녕하세요", L"Hello, World", L"こんにちは");
	}

	logger::handle().stop();
	logger::destroy();

	return 0;
}
