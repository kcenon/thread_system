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

#include "priority_job.h"

#include "priority_job_queue.h"

namespace priority_thread_pool_module
{
	template <typename priority_type>
	priority_job_t<priority_type>::priority_job_t(
		const std::function<std::optional<std::string>(void)>& callback,
		priority_type priority,
		const std::string& name)
		: job(callback, name), priority_(priority)
	{
	}

	template <typename priority_type> priority_job_t<priority_type>::~priority_job_t(void) {}

	template <typename priority_type>
	auto priority_job_t<priority_type>::priority() const -> priority_type
	{
		return priority_;
	}

	template <typename priority_type>
	auto priority_job_t<priority_type>::set_job_queue(const std::shared_ptr<job_queue>& job_queue)
		-> void
	{
		job_queue_ = std::dynamic_pointer_cast<priority_job_queue_t<priority_type>>(job_queue);
	}

	template <typename priority_type>
	auto priority_job_t<priority_type>::get_job_queue(void) const -> std::shared_ptr<job_queue>
	{
		return std::dynamic_pointer_cast<job_queue>(job_queue_.lock());
	}
} // namespace priority_thread_pool_module