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

namespace thread_module
{
	job::job(const std::function<std::tuple<bool, std::optional<std::string>>(void)>& callback,
			 const std::string& name)
		: name_(name), callback_(callback)
	{
	}

	job::~job(void) {}

	auto job::get_ptr(void) -> std::shared_ptr<job> { return shared_from_this(); }

	auto job::get_name(void) const -> std::string { return name_; }

	auto job::do_work(void) -> std::tuple<bool, std::optional<std::string>>
	{
		if (callback_ == nullptr)
		{
			return { false, "cannot execute job without callback" };
		}

		try
		{
			return callback_();
		}
		catch (const std::exception& e)
		{
			return { false, std::string(e.what()) };
		}
		catch (...)
		{
			return { false, "unknown error" };
		}
	}

	auto job::set_job_queue(const std::shared_ptr<job_queue>& job_queue) -> void
	{
		job_queue_ = job_queue;
	}

	auto job::get_job_queue(void) const -> std::shared_ptr<job_queue> { return job_queue_.lock(); }
} // namespace thread_module