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

#include "callback_job.h"

#include "job_queue.h"

using namespace utility_module;

namespace thread_module
{
	callback_job::callback_job(const std::function<std::optional<std::string>(void)>& callback,
							   const std::string& name)
		: job(name), callback_(callback), data_callback_(nullptr)
	{
	}

	callback_job::callback_job(
		const std::function<std::optional<std::string>(const std::vector<uint8_t>&)>& data_callback,
		const std::vector<uint8_t>& data,
		const std::string& name)
		: job(data, name), callback_(nullptr), data_callback_(data_callback)
	{
	}

	callback_job::~callback_job(void) {}

	auto callback_job::do_work(void) -> std::optional<std::string>
	{
		if (callback_ != nullptr)
		{
			try
			{
				return callback_();
			}
			catch (const std::exception& e)
			{
				return std::string(e.what());
			}
			catch (...)
			{
				return "unknown error";
			}
		}

		if (data_callback_ != nullptr)
		{
			try
			{
				return data_callback_(data_);
			}
			catch (const std::exception& e)
			{
				return std::string(e.what());
			}
			catch (...)
			{
				return "unknown error";
			}
		}

		return "cannot execute job without callback";
	}
} // namespace thread_module