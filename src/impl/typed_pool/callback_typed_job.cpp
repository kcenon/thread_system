// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#include <kcenon/thread/impl/typed_pool/callback_typed_job.h>

namespace kcenon::thread
{
	// Explicit template instantiation for job_types
	template class callback_typed_job_t<job_types>;
} // namespace kcenon::thread
