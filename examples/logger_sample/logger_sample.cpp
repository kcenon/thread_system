// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#include <iostream>
#include <chrono>
#include <thread>

#include "logger/core/logger.h"
#include <kcenon/thread/utils/formatter.h>

using kcenon::thread::utils::formatter;

bool use_backup_ = false;
uint32_t max_lines_ = 0;
uint16_t wait_interval_ = 100;
uint32_t test_line_count_ = 10000;
log_module::log_types file_target_ = log_module::log_types::Sequence;
log_module::log_types console_target_ = log_module::log_types::Sequence;
log_module::log_types callback_target_ = log_module::log_types::None;

auto initialize_logger() -> std::optional<std::string>
{
	log_module::set_title("logger_sample");
	log_module::set_use_backup(use_backup_);
	log_module::set_max_lines(max_lines_);
	log_module::file_target(file_target_);
	log_module::console_target(console_target_);
	log_module::callback_target(callback_target_);
	log_module::message_callback(
		[](const log_module::log_types& type, const std::string& datetime,
		   const std::string& message)
		{ std::cout << formatter::format("[{}][{}] {}\n", datetime, type, message); });
	if (wait_interval_ > 0)
	{
		log_module::set_wake_interval(std::chrono::milliseconds(wait_interval_));
	}

	return log_module::start();
}

auto main() -> int
{
	auto error_message = initialize_logger();
	if (error_message.has_value())
	{
		std::cerr << formatter::format("error starting logger: {}\n",
									   error_message.value_or("unknown error"));
		return 0;
	}

	for (auto index = 0; index < test_line_count_; ++index)
	{
		log_module::write_debug("안녕, World!: {}", index);
		log_module::write_debug("테스트 #{} - Hello, 世界!", index);
		log_module::write_debug("警告 {}: こんにちは", index);

		log_module::write_sequence(L"안녕, World!: {}", index);
		log_module::write_sequence(L"테스트 #{} - Hello, 世界!", index);
		log_module::write_sequence(L"警告 {}: こんにちは", index);

		log_module::write_parameter("복합 테스트 - 값: {}, 이름: {}", index, "홍길동");
		log_module::write_parameter(L"복합 테스트 - 값: {}, 이름: {}", index, L"홍길동");

		log_module::write_information("여러 줄 테스트:\n  라인 1: {}\n  라인 2: {}\n  라인 3: {}",
									  "안녕하세요", "Hello, World", "こんにちは");
		log_module::write_information(L"여러 줄 테스트:\n  라인 1: {}\n  라인 2: {}\n  라인 3: {}",
									  L"안녕하세요", L"Hello, World", L"こんにちは");
	}

	log_module::stop();

	return 0;
}
