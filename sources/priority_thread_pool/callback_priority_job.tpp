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

#include "callback_priority_job.h"

#include "priority_job_queue.h"

namespace priority_thread_pool_module
{
	template <typename priority_type>
	callback_priority_job_t<priority_type>::callback_priority_job_t(
		const std::function<std::optional<std::string>(void)>& callback,
		priority_type priority,
		const std::string& name)
		: priority_job_t<priority_type>(priority, name), callback_(callback)
	{
	}

	template <typename priority_type>
	callback_priority_job_t<priority_type>::~callback_priority_job_t(void)
	{
	}

	template <typename priority_type>
	auto callback_priority_job_t<priority_type>::do_work(void) -> std::optional<std::string>
	{
		if (callback_ == nullptr)
		{
			return "cannot execute job without callback";
		}

		try
		{
			return callback_();
		}
		catch (const std::exception& e)
		{
			return e.what();
		}
	}
} // namespace priority_thread_pool_module