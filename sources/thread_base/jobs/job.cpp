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

#include "job.h"

#include "job_queue.h"

using namespace utility_module;

namespace thread_module
{
	job::job(const std::string& name) : name_(name), data_(std::vector<uint8_t>()) {}

	job::job(const std::vector<uint8_t>& data, const std::string& name) : name_(name), data_(data)
	{
	}

	job::~job(void) {}

	auto job::get_name(void) const -> std::string { return name_; }

	auto job::do_work(void) -> result_void { 
    // For backward compatibility, we convert the old string-based error to the new typed error
    std::optional<std::string> old_result = "not implemented";
    
    if (!old_result.has_value()) {
        return result_void{};
    } else {
        return error{error_code::not_implemented, old_result.value()};
    }
}

auto job::set_cancellation_token(const cancellation_token& token) -> void {
    cancellation_token_ = token;
}

auto job::get_cancellation_token() const -> cancellation_token {
    return cancellation_token_;
}

	auto job::set_job_queue(const std::shared_ptr<job_queue>& job_queue) -> void
	{
		job_queue_ = job_queue;
	}

	auto job::get_job_queue(void) const -> std::shared_ptr<job_queue> { return job_queue_.lock(); }

	auto job::to_string(void) const -> std::string { return "job: " + name_; }
} // namespace thread_module