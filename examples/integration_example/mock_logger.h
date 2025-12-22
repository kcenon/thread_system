#pragma once

/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, 🍀☀🌕🌥 🌊
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

#include <kcenon/common/interfaces/logger_interface.h>
#include <kcenon/common/interfaces/global_logger_registry.h>
#include <iostream>
#include <mutex>
#include <iomanip>
#include <sstream>
#include <chrono>

/**
 * @brief Mock logger implementation for demonstration
 *
 * Implements common::interfaces::ILogger for use in examples.
 * In a real application, this would be replaced with logger_system.
 *
 * @note Issue #261: Updated to use common_system's ILogger interface.
 */
class mock_logger : public kcenon::common::interfaces::ILogger {
public:
    using log_level = kcenon::common::interfaces::log_level;
    using log_entry = kcenon::common::interfaces::log_entry;
    using VoidResult = kcenon::common::VoidResult;
    using source_location = kcenon::common::source_location;

    mock_logger() : min_level_(log_level::trace) {}

    VoidResult log(log_level level, const std::string& message) override {
        if (!is_enabled(level)) {
            return VoidResult::ok({});
        }

        std::lock_guard<std::mutex> lock(mutex_);
        auto& stream = (level >= log_level::error) ? std::cerr : std::cout;

        stream << "[" << format_time() << "] "
               << "[" << level_to_string(level) << "] "
               << message << std::endl;

        return VoidResult::ok({});
    }

    VoidResult log(log_level level,
                   std::string_view message,
                   const source_location& loc = source_location::current()) override {
        if (!is_enabled(level)) {
            return VoidResult::ok({});
        }

        std::lock_guard<std::mutex> lock(mutex_);
        auto& stream = (level >= log_level::error) ? std::cerr : std::cout;

        stream << "[" << format_time() << "] "
               << "[" << level_to_string(level) << "] ";

        std::string file = loc.file_name();
        if (!file.empty()) {
            size_t pos = file.find_last_of("/\\");
            std::string filename = (pos != std::string::npos) ? file.substr(pos + 1) : file;
            stream << filename << ":" << loc.line() << " (" << loc.function_name() << ") ";
        }

        stream << message << std::endl;

        return VoidResult::ok({});
    }

    VoidResult log(const log_entry& entry) override {
        if (!is_enabled(entry.level)) {
            return VoidResult::ok({});
        }

        std::lock_guard<std::mutex> lock(mutex_);
        auto& stream = (entry.level >= log_level::error) ? std::cerr : std::cout;

        stream << "[" << format_time() << "] "
               << "[" << level_to_string(entry.level) << "] ";

        if (!entry.file.empty()) {
            size_t pos = entry.file.find_last_of("/\\");
            std::string filename = (pos != std::string::npos) ? entry.file.substr(pos + 1) : entry.file;
            stream << filename << ":" << entry.line << " (" << entry.function << ") ";
        }

        stream << entry.message << std::endl;

        return VoidResult::ok({});
    }

    bool is_enabled(log_level level) const override {
        return static_cast<int>(level) >= static_cast<int>(min_level_);
    }

    VoidResult set_level(log_level level) override {
        min_level_ = level;
        return VoidResult::ok({});
    }

    log_level get_level() const override {
        return min_level_;
    }

    VoidResult flush() override {
        std::cout.flush();
        std::cerr.flush();
        return VoidResult::ok({});
    }

    void start() {
        std::cout << "[MockLogger] Started" << std::endl;
    }

    void stop() {
        flush();
        std::cout << "[MockLogger] Stopped" << std::endl;
    }

private:
    std::string format_time() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
        oss << "." << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }

    std::string level_to_string(log_level level) const {
        return std::string(kcenon::common::interfaces::to_string(level));
    }

private:
    log_level min_level_;
    mutable std::mutex mutex_;
};
