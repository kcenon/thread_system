// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#include <kcenon/thread/impl/typed_pool/typed_job.h>

namespace kcenon::thread
{
	template <typename job_type>
	typed_job_t<job_type>::typed_job_t(job_type priority, const std::string& name)
		: job(name)
		, priority_(priority)
	{
	}

	template <typename job_type>
	typed_job_t<job_type>::~typed_job_t()
	{
	}

	// Explicit template instantiation for job_types
	template class typed_job_t<job_types>;

} // namespace kcenon::thread
