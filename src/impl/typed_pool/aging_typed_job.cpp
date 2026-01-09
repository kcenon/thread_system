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

#include <kcenon/thread/impl/typed_pool/aging_typed_job.h>

namespace kcenon::thread
{
	template <typename job_type>
	aging_typed_job_t<job_type>::aging_typed_job_t(
		job_type priority,
		std::function<common::VoidResult()> work,
		const std::string& name)
		: base(priority, name)
		, aged_priority_{priority, 0, std::chrono::steady_clock::now()}
		, work_(std::move(work))
	{
	}

	template <typename job_type>
	aging_typed_job_t<job_type>::~aging_typed_job_t() = default;

	template <typename job_type>
	auto aging_typed_job_t<job_type>::do_work() -> common::VoidResult
	{
		if (work_)
		{
			return work_();
		}
		return common::ok();
	}

	template <typename job_type>
	auto aging_typed_job_t<job_type>::get_aged_priority() const
		-> const aged_priority<job_type>&
	{
		return aged_priority_;
	}

	template <typename job_type>
	auto aging_typed_job_t<job_type>::get_aged_priority()
		-> aged_priority<job_type>&
	{
		return aged_priority_;
	}

	template <typename job_type>
	auto aging_typed_job_t<job_type>::apply_boost(int boost_amount) -> void
	{
		aged_priority_.apply_boost(boost_amount, max_boost_);
	}

	template <typename job_type>
	auto aging_typed_job_t<job_type>::reset_boost() -> void
	{
		aged_priority_.reset_boost();
	}

	template <typename job_type>
	auto aging_typed_job_t<job_type>::set_max_boost(int max) -> void
	{
		max_boost_ = max;
	}

	template <typename job_type>
	auto aging_typed_job_t<job_type>::get_max_boost() const -> int
	{
		return max_boost_;
	}

	template <typename job_type>
	auto aging_typed_job_t<job_type>::is_max_boosted() const -> bool
	{
		return aged_priority_.is_max_boosted(max_boost_);
	}

	template <typename job_type>
	auto aging_typed_job_t<job_type>::effective_priority() const -> job_type
	{
		return aged_priority_.effective_priority();
	}

	template <typename job_type>
	auto aging_typed_job_t<job_type>::wait_time() const -> std::chrono::milliseconds
	{
		return aged_priority_.wait_time();
	}

	template <typename job_type>
	auto aging_typed_job_t<job_type>::to_job_info() const -> job_info
	{
		return job_info{
			this->get_name(),
			aged_priority_.wait_time(),
			aged_priority_.boost
		};
	}

	// Explicit template instantiation for job_types
	template class aging_typed_job_t<job_types>;

} // namespace kcenon::thread
